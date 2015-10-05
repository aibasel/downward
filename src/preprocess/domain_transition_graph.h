#ifndef DOMAIN_TRANSITION_GRAPH_H
#define DOMAIN_TRANSITION_GRAPH_H

#include <iosfwd>
#include <vector>
using namespace std;

#include "operator.h"

class Axiom;
class Variable;

class DomainTransitionGraph {
public:
    typedef vector<pair<const Variable *, int>> Condition;
private:
    struct Transition {
        Transition(int theTarget, int theOp) : target(theTarget), op(theOp) {}
        bool operator==(const Transition &other) const {
            return target == other.target &&
                   op == other.op &&
                   condition == other.condition;
        }
        bool operator<(const Transition &other) const;
        int target;
        int op;
        int cost;
        Condition condition;
    };
    typedef vector<Transition> Vertex;
    vector<Vertex> vertices;
    int level;
public:
    DomainTransitionGraph(const Variable &var);
    void addTransition(int from, int to, const Operator &op, int op_index,
                       const Operator::PrePost &pre_pos);
    void addAxTransition(int from, int to, const Axiom &ax, int ax_index);
    void finalize();
    void dump() const;
    void generate_cpp_input(ofstream &outfile) const;
    bool is_strongly_connected() const;
};

extern void build_DTGs(const vector<Variable *> &varOrder,
                       const vector<Operator> &operators,
                       const vector<Axiom> &axioms,
                       vector<DomainTransitionGraph> &transition_graphs);
extern bool are_DTGs_strongly_connected(const vector<DomainTransitionGraph> &transition_graphs);
//extern vector<DomainTransitionGraph> &transition_graphs;

#endif
