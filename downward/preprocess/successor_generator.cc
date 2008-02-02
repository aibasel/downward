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
  vector<int> immediate_ops_indeces; //TODO: GeneratorLeaf draus machen 
  vector<GeneratorBase *> generator_for_value;
  GeneratorBase *default_generator;
public:
  ~GeneratorSwitch();
  GeneratorSwitch(Variable *switch_variable,
		  vector<int> operators,
		  const vector<GeneratorBase *> &gen_for_val,
		  GeneratorBase *default_gen);
  virtual void dump(string indent) const;
  virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorLeaf : public GeneratorBase {
  vector<int> applicable_ops_indeces;
public:
  GeneratorLeaf(vector<int> operators);
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
				 vector<int> operators,
				 const vector<GeneratorBase *> &gen_for_val,
				 GeneratorBase *default_gen)
  : switch_var(switch_variable), immediate_ops_indeces(operators),
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
  for(int i = 0; i < immediate_ops_indeces.size(); i++) {
    cout << indent << immediate_ops_indeces[i] << endl;
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
  outfile << "check " << immediate_ops_indeces.size() << endl;
  for(int i = 0; i < immediate_ops_indeces.size(); i++) {
    outfile << immediate_ops_indeces[i] << endl;
  }
  for(int i = 0; i < switch_var->get_range(); i++) {
    //cout << "case "<<switch_var->get_name()<<" (Level " <<switch_var->get_level() <<
    //  ") has value " << i << ":" << endl;
    generator_for_value[i]->generate_cpp_input(outfile);
  }
  //cout << "always:" << endl; 
  default_generator->generate_cpp_input(outfile);
  
}
GeneratorLeaf::GeneratorLeaf(vector<int> ops)
  : applicable_ops_indeces(ops) {
}

void GeneratorLeaf::dump(string indent) const {
  for(int i = 0; i < applicable_ops_indeces.size(); i++) {
    cout << indent << applicable_ops_indeces[i] << endl;
  }
}

void GeneratorLeaf::generate_cpp_input(ofstream &outfile) const {
  outfile << "check " << applicable_ops_indeces.size() << endl;
  for(int i = 0; i < applicable_ops_indeces.size(); i++) {
    outfile << applicable_ops_indeces[i] << endl;
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
  vector<int> all_operator_indeces;
  vector<Condition> all_conditions;
  
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
    sort(cond.begin(), cond.end(), greater<pair<Variable *, int> >());
    all_operator_indeces.push_back(i);
    all_conditions.push_back(cond);
  }

  vector<Variable *> varOrder = variables;
  sort(varOrder.begin(), varOrder.end());

  root = construct_recursive(varOrder, 0, all_operator_indeces, all_conditions);
}

GeneratorBase *SuccessorGenerator::construct_recursive(const vector<Variable *> &varOrder,
						       int switch_var_no,
						       vector<int> op_indeces,
						       vector<Condition> &conds) {
  if(op_indeces.empty())
    return new GeneratorEmpty;
  if(switch_var_no == varOrder.size()) // no further switch necessary
    return new GeneratorLeaf(op_indeces);

  Variable *switch_var = varOrder[switch_var_no];
  int number_of_children = switch_var->get_range();

  vector<vector<int> > ops_for_val_indeces;
  vector<vector<Condition> > conds_for_val;
  ops_for_val_indeces.resize(number_of_children);
  conds_for_val.resize(number_of_children);
  
  vector<int> default_ops_indeces;
  vector<Condition> default_conds;
  vector<int> applicable_ops_indeces;
  for(int i = 0; i < op_indeces.size(); i++){
    int op_index = op_indeces[i];
    Condition &cond = conds[i];
    if(cond.empty()) {
      applicable_ops_indeces.push_back(op_index);
    } else {
      Variable *var = cond.back().first;
      int val = cond.back().second;
      if(var == switch_var) {
	cond.pop_back();
	ops_for_val_indeces[val].push_back(op_index);
	conds_for_val[val].push_back(cond);
      } else {
	default_ops_indeces.push_back(op_index);
	default_conds.push_back(cond);
      }
    }
  }

  if(op_indeces.size() == applicable_ops_indeces.size()) { // no further switch necessary
    return new GeneratorLeaf(applicable_ops_indeces);
  } else if(op_indeces.size() == default_ops_indeces.size()) { 
    // this switch var can be left out because no operator depends on it
    return construct_recursive(varOrder, switch_var_no + 1, default_ops_indeces, 
			       default_conds);
  } else {
    GeneratorBase *default_sg = construct_recursive(varOrder, switch_var_no + 1,
						    default_ops_indeces, default_conds);
    vector<GeneratorBase *> gen_for_val;
    for(int j = 0; j < number_of_children; j++) {
      gen_for_val.push_back(construct_recursive(varOrder, switch_var_no + 1,
						ops_for_val_indeces[j], 
						conds_for_val[j]));
    }
    return new GeneratorSwitch(switch_var, applicable_ops_indeces, gen_for_val, default_sg);
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
