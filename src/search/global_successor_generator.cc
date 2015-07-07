#include "global_successor_generator.h"

#include "global_operator.h"
#include "global_state.h"
#include "globals.h"
#include "utilities.h"

#include <cstdlib>
#include <iostream>
#include <vector>
using namespace std;

class GlobalSuccessorGeneratorSwitch : public GlobalSuccessorGenerator {
    int switch_var;
    GlobalSuccessorGenerator *immediate_ops;
    vector<GlobalSuccessorGenerator *> generator_for_value;
    GlobalSuccessorGenerator *default_generator;
public:
    GlobalSuccessorGeneratorSwitch(istream &in);
    virtual void generate_applicable_ops(const GlobalState &curr,
                                         vector<const GlobalOperator *> &ops);
    virtual void _dump(string indent);
};

class GlobalSuccessorGeneratorGenerate : public GlobalSuccessorGenerator {
    vector<const GlobalOperator *> op;
public:
    GlobalSuccessorGeneratorGenerate(istream &in);
    virtual void generate_applicable_ops(const GlobalState &curr,
                                         vector<const GlobalOperator *> &ops);
    virtual void _dump(string indent);
};

GlobalSuccessorGeneratorSwitch::GlobalSuccessorGeneratorSwitch(istream &in) {
    in >> switch_var;
    immediate_ops = read_successor_generator(in);
    for (int i = 0; i < g_variable_domain[switch_var]; ++i)
        generator_for_value.push_back(read_successor_generator(in));
    default_generator = read_successor_generator(in);
}

void GlobalSuccessorGeneratorSwitch::generate_applicable_ops(
    const GlobalState &curr, vector<const GlobalOperator *> &ops) {
    immediate_ops->generate_applicable_ops(curr, ops);
    generator_for_value[curr[switch_var]]->generate_applicable_ops(curr, ops);
    default_generator->generate_applicable_ops(curr, ops);
}

void GlobalSuccessorGeneratorSwitch::_dump(string indent) {
    cout << indent << "switch on " << g_variable_name[switch_var] << endl;
    cout << indent << "immediately:" << endl;
    immediate_ops->_dump(indent + "  ");
    for (int i = 0; i < g_variable_domain[switch_var]; ++i) {
        cout << indent << "case " << i << ":" << endl;
        generator_for_value[i]->_dump(indent + "  ");
    }
    cout << indent << "always:" << endl;
    default_generator->_dump(indent + "  ");
}

void GlobalSuccessorGeneratorGenerate::generate_applicable_ops(
    const GlobalState &, vector<const GlobalOperator *> &ops) {
    ops.insert(ops.end(), op.begin(), op.end());
}

GlobalSuccessorGeneratorGenerate::GlobalSuccessorGeneratorGenerate(istream &in) {
    int count;
    in >> count;
    for (int i = 0; i < count; ++i) {
        int op_index;
        in >> op_index;
        op.push_back(&g_operators[op_index]);
    }
}

void GlobalSuccessorGeneratorGenerate::_dump(string indent) {
    for (size_t i = 0; i < op.size(); ++i) {
        cout << indent;
        op[i]->dump();
    }
}

GlobalSuccessorGenerator *read_successor_generator(istream &in) {
    string type;
    in >> type;
    if (type == "switch") {
        return new GlobalSuccessorGeneratorSwitch(in);
    } else if (type == "check") {
        return new GlobalSuccessorGeneratorGenerate(in);
    }
    cout << "Illegal successor generator statement!" << endl;
    cout << "Expected 'switch' or 'check', got '" << type << "'." << endl;
    exit_with(EXIT_INPUT_ERROR);
}
