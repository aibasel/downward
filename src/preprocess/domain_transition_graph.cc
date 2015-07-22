// TODO: document further

/* unary operators are constructed in "build_DTGs" by calling
 * "addTransition" with the appropriate arguments: for every
 * pre-postcondition pair of an operator, an edge in the
 * DTG is built between the corresponding values in the dtg of the
 * variable, annotated with all prevail conditions and preconditions and
 * effect conditions of that postcondition.
 */

#include "domain_transition_graph.h"
#include "operator.h"
#include "axiom.h"
#include "variable.h"
#include "scc.h"

#include <algorithm>
#include <cassert>
#include <iostream>
using namespace std;

DomainTransitionGraph::DomainTransitionGraph(const Variable &var) {
    vertices.resize(var.get_range());
    level = var.get_level();
    assert(level != -1);
}

void DomainTransitionGraph::addTransition(int from, int to, const Operator &op,
                                          int op_index, const Operator::PrePost &pre_post) {
    assert(pre_post.var->get_level() == level && pre_post.post == to);
    Transition trans(to, op_index);
    trans.cost = op.get_cost();
    Condition &cond = trans.condition;

    // Collect operator preconditions.
    const vector<Operator::Prevail> &prevail = op.get_prevail();
    for (const Operator::Prevail &prev : prevail)
        cond.push_back(make_pair(prev.var, prev.prev));
    for (const Operator::PrePost &op_pre_post : op.get_pre_post()) {
        if (op_pre_post.pre != -1) {
            if (op_pre_post.var->get_level() == level)
                assert(op_pre_post.pre == from);
            else
                cond.push_back(make_pair(op_pre_post.var, op_pre_post.pre));
        }
    }
    // Collect effect conditions for this effect.
    for (const Operator::EffCond eff_cond : pre_post.effect_conds) {
        if (eff_cond.var->get_level() == level) {
            if (eff_cond.cond != from) {
                // This is a conditional effect that cannot trigger a transition
                // from "from" to "to" because it requires var to have a value
                // different from "from".
                return;
            }
        } else {
            trans.condition.push_back(make_pair(eff_cond.var, eff_cond.cond));
        }
    }

    vertices[from].push_back(trans);
}

void DomainTransitionGraph::addAxTransition(int from, int to, const Axiom &ax,
                                            int ax_index) {
    Transition trans(to, ax_index);
    Condition &cond = trans.condition;
    for (const Axiom::Condition &ax_cond : ax.get_conditions())
        if (true) // [cycles]
            // if(prevail[i].var->get_level() < level) // [no cycles]
            cond.push_back(make_pair(ax_cond.var, ax_cond.cond));
    vertices[from].push_back(trans);
}

bool DomainTransitionGraph::Transition::operator<(const Transition &other) const {
    if (target != other.target)
        return target < other.target;
    else if (condition.size() != other.condition.size())
        return condition.size() < other.condition.size();
    else
        return cost < other.cost;
}

void DomainTransitionGraph::finalize() {
    for (Vertex &transitions : vertices) {
        // For all sources, sort transitions according to targets and condition length
        sort(transitions.begin(), transitions.end());
        transitions.erase(unique(transitions.begin(), transitions.end()),
                          transitions.end());
        // For all transitions, sort conditions (acc. to pointer addresses)
        for (Transition &trans : transitions) {
            Condition &cond = trans.condition;
            sort(cond.begin(), cond.end());
        }
        // Look for dominated transitions
        vector<Transition> undominated_trans;
        vector<bool> is_dominated;
        int num_transitions = transitions.size();
        is_dominated.resize(num_transitions, false);
        for (int j = 0; j < num_transitions; j++) {
            if (!is_dominated[j]) {
                Transition &trans = transitions[j];
                undominated_trans.push_back(trans);
                Condition &cond = trans.condition;
                int comp = j + 1; // compare transition no. j to no. comp
                // comp is dominated if it has same target and same and more conditions
                while (comp < num_transitions) {
                    if (is_dominated[comp]) {
                        comp++;
                        continue;
                    }
                    Transition &other_trans = transitions[comp];
                    if (other_trans.cost < trans.cost) {
                        comp++;
                        continue; // lower cost transition can't be dominated
                    }
                    assert(other_trans.target >= trans.target);
                    if (other_trans.target != trans.target)
                        break;  // transition and all after it have different targets
                    else { //domination possible
                        assert(other_trans.condition.size() >= cond.size());
                        if (cond.size() == 0) {
                            is_dominated[comp] = true; // comp is dominated
                            comp++;
                        } else {
                            bool same_conditions = true;
                            for (const auto &c1 : cond) {
                                bool comp_dominated = false;
                                for (const auto &c2 : other_trans.condition) {
                                    if (c2.first > c1.first) {
                                        break; // comp doesn't have this condition, not dominated
                                    }
                                    if (c2.first == c1.first &&
                                        c2.second == c1.second) {
                                        comp_dominated = true;
                                        break; // comp has this condition, look for next cond
                                    }
                                }
                                if (!comp_dominated) {
                                    same_conditions = false;
                                    break;
                                }
                            }
                            is_dominated[comp] = same_conditions;
                            comp++;
                        }
                    }
                }
            }
        }
        transitions.swap(undominated_trans);
    }
}

void build_DTGs(const vector<Variable *> &var_order,
                const vector<Operator> &operators,
                const vector<Axiom> &axioms,
                vector<DomainTransitionGraph> &transition_graphs) {
    for (Variable *var : var_order) {
        transition_graphs.push_back(DomainTransitionGraph(*var));
    }

    int num_operators = operators.size();
    for (int i = 0; i < num_operators; ++i) {
        const Operator &op = operators[i];
        for (const auto &eff : op.get_pre_post()) {
            const Variable *var = eff.var;
            int var_level = var->get_level();
            if (var_level != -1) {
                int pre = eff.pre;
                int post = eff.post;
                if (pre != -1) {
                    transition_graphs[var_level].addTransition(pre, post, op, i, eff);
                } else {
                    for (int pre = 0; pre < var->get_range(); pre++)
                        if (pre != post)
                            transition_graphs[var_level].addTransition(pre, post, op, i, eff);
                }
            }
            //else
            //cout <<"leave out var "<< var->get_name()<<" (unimportant) " << endl;
        }
    }
    int num_axioms = axioms.size();
    for (int i = 0; i < num_axioms; i++) {
        const Axiom &ax = axioms[i];
        Variable *var = ax.get_effect_var();
        int var_level = var->get_level();
        assert(var_level != -1);
        int old_val = ax.get_old_val();
        int new_val = ax.get_effect_val();
        transition_graphs[var_level].addAxTransition(old_val, new_val, ax, i);
    }

    for (auto &transition_graph : transition_graphs)
        transition_graph.finalize();
}
bool are_DTGs_strongly_connected(const vector<DomainTransitionGraph> &transition_graphs) {
    bool connected = true;
    // no need to test last variable's dtg (highest level variable)
    int num_dtgs = transition_graphs.size();
    for (int i = 0; i < num_dtgs - 1; i++)
        if (!transition_graphs[i].is_strongly_connected())
            connected = false;
    return connected;
}
bool DomainTransitionGraph::is_strongly_connected() const {
    vector<vector<int> > easy_graph;
    int num_vertices = vertices.size();
    for (int i = 0; i < num_vertices; i++) {
        vector<int> empty_vector;
        easy_graph.push_back(empty_vector);
        for (const Transition &trans : vertices[i]) {
            easy_graph[i].push_back(trans.target);
        }
    }
    vector<vector<int> > sccs = SCC(easy_graph).get_result();
    //  cout << "easy graph sccs for var " << level << endl;
//   for(int i = 0; i < sccs.size(); i++) {
//     for(int j = 0; j < sccs[i].size(); j++)
//       cout << " " << sccs[i][j];
//     cout << endl;
//   }
    bool connected = false;
    if (sccs.size() == 1) {
        connected = true;
        //cout <<"is strongly connected" << endl;
    }
    //else cout << "not strongly connected" << endl;
    return connected;
}
void DomainTransitionGraph::dump() const {
    cout << "Level: " << level << endl;
    int num_vertices = vertices.size();
    for (int i = 0; i < num_vertices; i++) {
        cout << "  From value " << i << ":" << endl;
        for (const Transition &trans : vertices[i]) {
            cout << "    " << "To value " << trans.target << endl;
            for (const auto &cond : trans.condition)
                cout << "      if " << cond.first->get_name()
                     << " = " << cond.second << endl;
        }
    }
}

void DomainTransitionGraph::generate_cpp_input(ofstream &outfile) const {
    //outfile << vertices.size() << endl; // the variable's range
    for (const Vertex &vertex : vertices) {
        outfile << vertex.size() << endl; // number of transitions from this value
        for (const Transition &trans : vertex) {
            outfile << trans.target << endl; // target of transition
            outfile << trans.op << endl; // operator doing the transition
            // calculate number of important prevail conditions
            int number = 0;
            for (const auto &cond : trans.condition)
                if (cond.first->get_level() != -1)
                    number++;
            outfile << number << endl;
            for (const auto &cond : trans.condition)
                if (cond.first->get_level() != -1)
                    outfile << cond.first->get_level() <<
                    " " << cond.second << endl;  // condition: var, val
        }
    }
}
