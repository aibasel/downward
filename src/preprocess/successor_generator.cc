#include "operator.h"
#include "successor_generator.h"
#include "variable.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
using namespace std;

/* NOTE on possible optimizations:

   * Sharing "GeneratorEmpty" instances might help quite a bit with
     reducing memory usage and possibly even speed, because there are
     bound to be many instance of those. However, it complicates
     deleting the successor generator, and memory doesn't seem to be
     an issue. Speed appears to be fine now, too. So this is probably
     an unnecessary complication.

   * Using slist instead of list led to a further 10% speedup on the
     largest Logistics instance, logistics-98/prob28.pddl. It would of
     course also reduce memory usage. However, it would make the code
     g++-specific, so it's probably not worth it.

*/

class GeneratorBase {
public:
    virtual ~GeneratorBase() {}
    virtual void dump(string indent) const = 0;
    virtual void generate_cpp_input(ofstream &outfile) const = 0;
};

class GeneratorSwitch : public GeneratorBase {
    Variable *switch_var;
    list<int> immediate_ops_indices;
    vector<GeneratorBase *> generator_for_value;
    GeneratorBase *default_generator;
public:
    ~GeneratorSwitch();
    GeneratorSwitch(Variable *switch_variable,
                    list<int> &operators,
                    const vector<GeneratorBase *> &gen_for_val,
                    GeneratorBase *default_gen);
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorLeaf : public GeneratorBase {
    list<int> applicable_ops_indices;
public:
    GeneratorLeaf(list<int> &operators);
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

class GeneratorEmpty : public GeneratorBase {
public:
    virtual void dump(string indent) const;
    virtual void generate_cpp_input(ofstream &outfile) const;
};

GeneratorSwitch::GeneratorSwitch(Variable *switch_variable,
                                 list<int> &operators,
                                 const vector<GeneratorBase *> &gen_for_val,
                                 GeneratorBase *default_gen)
    : switch_var(switch_variable),
      generator_for_value(gen_for_val),
      default_generator(default_gen) {
    immediate_ops_indices.swap(operators);
}

GeneratorSwitch::~GeneratorSwitch() {
    for (GeneratorBase *generator : generator_for_value)
        delete generator;
    delete default_generator;
}

void GeneratorSwitch::dump(string indent) const {
    cout << indent << "switch on " << switch_var->get_name() << endl;
    cout << indent << "immediately:" << endl;
    for (int op_id : immediate_ops_indices)
        cout << indent << op_id << endl;
    for (int i = 0; i < switch_var->get_range(); i++) {
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
    for (int op_id : immediate_ops_indices)
        outfile << op_id << endl;
    for (int i = 0; i < switch_var->get_range(); i++) {
        //cout << "case "<<switch_var->get_name()<<" (Level " <<switch_var->get_level() <<
        //  ") has value " << i << ":" << endl;
        generator_for_value[i]->generate_cpp_input(outfile);
    }
    //cout << "always:" << endl;
    default_generator->generate_cpp_input(outfile);
}

GeneratorLeaf::GeneratorLeaf(list<int> &ops) {
    applicable_ops_indices.swap(ops);
}

void GeneratorLeaf::dump(string indent) const {
    for (int op_id : applicable_ops_indices)
        cout << indent << op_id << endl;
}

void GeneratorLeaf::generate_cpp_input(ofstream &outfile) const {
    outfile << "check " << applicable_ops_indices.size() << endl;
    for (int op_id : applicable_ops_indices)
        outfile << op_id << endl;
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
    int num_operators = operators.size();
    conditions.reserve(num_operators);
    list<int> all_operator_indices;
    for (int i = 0; i < num_operators; i++) {
        const Operator *op = &operators[i];
        Condition cond;
        for (Operator::Prevail prev : op->get_prevail()) {
            cond.push_back(make_pair(prev.var, prev.prev));
        }
        for (Operator::PrePost pre_post : op->get_pre_post()) {
            if (pre_post.pre != -1)
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
                                                       list<int> &op_indices) {
    if (op_indices.empty())
        return new GeneratorEmpty;
    int num_vars = varOrder.size();

    while (true) {
        // Test if no further switch is necessary (or possible).
        if (switch_var_no == num_vars)
            return new GeneratorLeaf(op_indices);

        Variable *switch_var = varOrder[switch_var_no];
        int number_of_children = switch_var->get_range();

        vector<list<int> > ops_for_val_indices(number_of_children);
        list<int> default_ops_indices;
        list<int> applicable_ops_indices;

        bool all_ops_are_immediate = true;
        bool var_is_interesting = false;

        while (!op_indices.empty()) {
            int op_index = op_indices.front();
            op_indices.pop_front();
            assert(op_index >= 0 && op_index < (int)next_condition_by_op.size());
            Condition::const_iterator &cond_iter = next_condition_by_op[op_index];
            assert(cond_iter - conditions[op_index].begin() >= 0);
            assert(cond_iter - conditions[op_index].begin() <= (int)conditions[op_index].size());
            if (cond_iter == conditions[op_index].end()) {
                var_is_interesting = true;
                applicable_ops_indices.push_back(op_index);
            } else {
                all_ops_are_immediate = false;
                Variable *var = cond_iter->first;
                int val = cond_iter->second;
                if (var == switch_var) {
                    var_is_interesting = true;
                    while (cond_iter != conditions[op_index].end() &&
                           cond_iter->first == switch_var) {
                        ++cond_iter;
                    }
                    ops_for_val_indices[val].push_back(op_index);
                } else {
                    default_ops_indices.push_back(op_index);
                }
            }
        }

        if (all_ops_are_immediate) {
            return new GeneratorLeaf(applicable_ops_indices);
        } else if (var_is_interesting) {
            vector<GeneratorBase *> gen_for_val;
            for (int j = 0; j < number_of_children; j++) {
                gen_for_val.push_back(construct_recursive(switch_var_no + 1,
                                                          ops_for_val_indices[j]));
            }
            GeneratorBase *default_sg = construct_recursive(switch_var_no + 1,
                                                            default_ops_indices);
            return new GeneratorSwitch(switch_var, applicable_ops_indices, gen_for_val, default_sg);
        } else {
            // this switch var can be left out because no operator depends on it
            ++switch_var_no;
            default_ops_indices.swap(op_indices);
        }
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
