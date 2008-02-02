#include "operator.h"
#include "successor_generator.h"
#include "variable.h"

#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
using namespace std;

class GeneratorBase {
public:
  virtual ~GeneratorBase() {}
  virtual void dump(string indent) const = 0;
  virtual void generate_cpp_input(ofstream &outfile) const = 0;
};

class GeneratorSwitch : public GeneratorBase {
  Variable *switch_var;
  vector<int> immediate_ops_indices; //TODO: GeneratorLeaf draus machen 
  vector<GeneratorBase *> generator_for_value;
  GeneratorBase *default_generator;
public:
  ~GeneratorSwitch();
  GeneratorSwitch(Variable *switch_variable,
		  const vector<int> &operators,
		  const vector<GeneratorBase *> &gen_for_val,
		  GeneratorBase *default_gen);
  virtual void dump(string indent) const;
  virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorLeaf : public GeneratorBase {
  vector<int> applicable_ops_indices;
public:
  GeneratorLeaf(const vector<int> &operators);
  virtual void dump(string indent) const;
  virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorEmpty : public GeneratorBase {
public:
  virtual void dump(string indent) const;
  virtual void generate_cpp_input(ofstream &outfile) const;
};

// NOTE:
// Als Optimierung könnte man ein Singleton aus GeneratorEmpty machen.
// Die Generatoren würden sich also dasselbe Empty-Objekt teilen.
// Das führt dann aber zu Komplikationen beim Freigeben der Kinder
// (mit delete), da der leere Generator natürlich nur einmal
// freigegeben werden darf (bzw. gar nicht, falls er nicht auf dem
// Heap alloziert wird).

GeneratorSwitch::GeneratorSwitch(Variable *switch_variable, 
				 //const vector<const Operator *> &operators,
				 const vector<int> &operators,
				 const vector<GeneratorBase *> &gen_for_val,
				 GeneratorBase *default_gen)
  : switch_var(switch_variable), immediate_ops_indices(operators),
    generator_for_value(gen_for_val), default_generator(default_gen) {
}

GeneratorSwitch::~GeneratorSwitch() {
  for(int i = 0; i < generator_for_value.size(); i++)
    delete generator_for_value[i];
  delete default_generator;
}

void GeneratorSwitch::dump(string indent) const {
  cout << indent << "switch on " << switch_var->get_name() << endl;
  cout << indent << "immediately:" << endl;
  for(int i = 0; i < immediate_ops_indices.size(); i++) {
    cout << indent << immediate_ops_indices[i] << endl;
  }
  for(int i = 0; i < switch_var->get_range(); i++) {
    cout << indent << "case " << i << ":" << endl;
    generator_for_value[i]->dump(indent + "  ");
  }
  cout << indent << "always:" << endl;
  default_generator->dump(indent + "  ");
}
void GeneratorSwitch::generate_cpp_input(ofstream &outfile) const {
  
  int level = switch_var->get_level();
  assert(level != -1);
  outfile << "switch " << level << endl;
  outfile << "check " << immediate_ops_indices.size() << endl;
  for(int i = 0; i < immediate_ops_indices.size(); i++) {
    outfile << immediate_ops_indices[i] << endl;
  }
  for(int i = 0; i < switch_var->get_range(); i++) {
    //cout << "case "<<switch_var->get_name()<<" (Level " <<switch_var->get_level() <<
    //  ") has value " << i << ":" << endl;
    generator_for_value[i]->generate_cpp_input(outfile);
  }
  //cout << "always:" << endl; 
  default_generator->generate_cpp_input(outfile);
  
}
GeneratorLeaf::GeneratorLeaf(const vector<int> &ops)
  : applicable_ops_indices(ops) {
}

void GeneratorLeaf::dump(string indent) const {
  for(int i = 0; i < applicable_ops_indices.size(); i++) {
    cout << indent << applicable_ops_indices[i] << endl;
  }
}

void GeneratorLeaf::generate_cpp_input(ofstream &outfile) const {
  outfile << "check " << applicable_ops_indices.size() << endl;
  for(int i = 0; i < applicable_ops_indices.size(); i++) {
    outfile << applicable_ops_indices[i] << endl;
  }
}

void GeneratorEmpty::dump(string indent) const {
  cout << indent << "<empty>" << endl;
}

void GeneratorEmpty::generate_cpp_input(ofstream &outfile) const {
  outfile << "check 0" << endl;
}

SuccessorGenerator::SuccessorGenerator(const vector<Variable *> &variables,
				       const vector<Operator> &operators) {
  // We need the iterators to conditions to be stable:
  conditions.reserve(operators.size());
  vector<int> all_operator_indices;
  for(int i = 0; i < operators.size(); i++) {
    const Operator *op = &operators[i];
    Condition cond;
    for(int j = 0; j < op->get_prevail().size(); j++) {
      Operator::Prevail prev = op->get_prevail()[j];
      cond.push_back(make_pair(prev.var, prev.prev));
    }
    for(int j = 0; j < op->get_pre_post().size(); j++) {
      Operator::PrePost pre_post = op->get_pre_post()[j];
      if(pre_post.pre != -1)
	cond.push_back(make_pair(pre_post.var, pre_post.pre));
    }
    sort(cond.begin(), cond.end());
    all_operator_indices.push_back(i);
    conditions.push_back(cond);
    next_condition_by_op.push_back(conditions.back().begin());
  }
  
  varOrder = variables;
  sort(varOrder.begin(), varOrder.end());

  root = construct_recursive(0, all_operator_indices);
}

GeneratorBase *SuccessorGenerator::construct_recursive(int switch_var_no,
						       const vector<int> &op_indices) {
  if(op_indices.empty())
    return new GeneratorEmpty;
  if(switch_var_no == varOrder.size()) // no further switch necessary
    return new GeneratorLeaf(op_indices);

  Variable *switch_var = varOrder[switch_var_no];
  int number_of_children = switch_var->get_range();

  vector<vector<int> > ops_for_val_indices(number_of_children);
  vector<int> default_ops_indices;
  vector<int> applicable_ops_indices;
  for(int i = 0; i < op_indices.size(); i++){
    int op_index = op_indices[i];
    assert(op_index >= 0 && op_index < next_condition_by_op.size());
    Condition::const_iterator &cond_iter = next_condition_by_op[op_index];
    assert(cond_iter - conditions[op_index].begin() >= 0);
    assert(cond_iter - conditions[op_index].begin() <= conditions[op_index].size());
    if(cond_iter == conditions[op_index].end()) {
      applicable_ops_indices.push_back(op_index);
    } else {
      Variable *var = cond_iter->first;
      int val = cond_iter->second;
      if(var == switch_var) {
	++cond_iter;
	ops_for_val_indices[val].push_back(op_index);
      } else {
	default_ops_indices.push_back(op_index);
      }
    }
  }

  if(op_indices.size() == applicable_ops_indices.size()) { // no further switch necessary
    return new GeneratorLeaf(applicable_ops_indices);
  } else if(op_indices.size() == default_ops_indices.size()) { 
    // this switch var can be left out because no operator depends on it
    return construct_recursive(switch_var_no + 1, default_ops_indices);
  } else {
    vector<GeneratorBase *> gen_for_val;
    for(int j = 0; j < number_of_children; j++) {
      gen_for_val.push_back(construct_recursive(switch_var_no + 1,
						ops_for_val_indices[j]));
    }
    GeneratorBase *default_sg = construct_recursive(switch_var_no + 1,
						    default_ops_indices);
    return new GeneratorSwitch(switch_var, applicable_ops_indices, gen_for_val, default_sg);
  }
}

SuccessorGenerator::SuccessorGenerator() {
  root = 0;
}

SuccessorGenerator::~SuccessorGenerator() {
  delete root;
}

void SuccessorGenerator::dump() const {
  cout << "Successor Generator:" << endl;
  root->dump("  ");
}
void SuccessorGenerator::generate_cpp_input(ofstream &outfile) const {
  root->generate_cpp_input(outfile);
}
