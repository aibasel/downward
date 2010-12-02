#include "pattern_collection.h"
#include "globals.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include "timer.h"
#include <cassert>
#include <cstdlib>
#include <algorithm>

using namespace std;

PatternCollection::PatternCollection(const vector<vector<int> > &pat_coll) : pattern_collection(pat_coll), number_patterns(pat_coll.size()) {
    build_cgraph();
    cout << "built cgraph." << endl;
    vector<int> vertices_1;
    vertices_1.reserve(number_patterns);
    vector<int> vertices_2;
    vertices_2.reserve(number_patterns);
    for (size_t i = 0; i < number_patterns; ++i) {
        vertices_1.push_back(i);
        vertices_2.push_back(i);
    }
    vector<int> q_clique; // contains actual calculated maximal clique
    q_clique.reserve(number_patterns);
    max_cliques_expand(vertices_1, vertices_2, q_clique);
    dump();

    // maybe unnecessary, but we don't need cgraph anymore
    cgraph.clear();

    // build all pattern databases
    Timer timer;
    timer();
    for (int i = 0; i < pattern_collection.size(); ++i) {
        PDBAbstraction pdb = PDBAbstraction(pattern_collection[i]);
        pattern_databases.push_back(pdb);
    }
    timer.stop();
    cout << pattern_collection.size() << " pdbs constructed." << endl;
    cout << "Construction time for all pdbs: " << timer << endl;
}

PatternCollection::~PatternCollection() {
}

bool PatternCollection::are_additive(int pattern1, int pattern2) const {
    vector<int> p1 = pattern_collection[pattern1];
    vector<int> p2 = pattern_collection[pattern2];

    for (size_t i = 0; i < p1.size(); i++) {
        for (size_t j = 0; j < p2.size(); j++) {
            // now check which operators affect pattern1
            for (size_t k = 0; k < g_operators.size(); k++) {
                bool p1_affected = false;
                const Operator &o = g_operators[k];
                const vector<PrePost> effects = o.get_pre_post();
                for (size_t l = 0; l < effects.size(); l++) {
                    // every effect affect one specific variable
                    int var = effects[l].var;
                    // we have to check if this variable is in our pattern
                    for (size_t m = 0; m < p1.size(); m++) {
                        if (var == p1[m]) {
                            // this operator affects pattern 1
                            p1_affected = true;
                            //cout << "operator:" << endl;
                            //o.dump();
                            //cout << "affects pattern:" << endl;
                            //for (size_t n = 0; n < p1.size(); n++) {
                                //cout << "variable: " << p1[n] << " (real name: " << g_variable_name[p1[n]] << ")" << endl;
                            //}
                            break;
                        }
                    }
                    if (p1_affected) {
                        break;
                    }
                }
                if (p1_affected) {
                    // check if the same operator also affects pattern 2
                    for (size_t l = 0; l < effects.size(); l++) {
                        int var = effects[l].var;
                        for (size_t m = 0; m < p2.size(); m++) {
                            if (var == p2[m]) {
                                // this operator affects also pattern 2
                                //cout << "and also affects pattern:" << endl;
                                //for (size_t n = 0; n < p2.size(); n++) {
                                    //cout << "variable: " << p2[n] << " (real name: " << g_variable_name[p2[n]] << ")" << endl;
                                //}
                                //cout << endl;
                                return false;
                            }
                        }
                    }
                    //cout << "but doesn't affect pattern:" << endl;
                    //for (size_t n = 0; n < p2.size(); n++) {
                        //cout << "variable: " << p2[n] << " (real name: " << g_variable_name[p2[n]] << ")" << endl;
                    //}
                    //cout << endl;
                }
            }
        }
    }
    return true;
}

void PatternCollection::build_cgraph() {
    // initialize compatibility graph
    cgraph = vector<vector<int> >();
    cgraph.resize(number_patterns);

    for (size_t i = 0; i < number_patterns; i++) {
        for (size_t j = i+1; j < number_patterns; j++) {
            if (are_additive(i,j)) {
                // if the two patterns are additive there is an edge in the compatibility graph
                cout << "pattern (index) " << i << " additive with " << "pattern (index) " << j << endl;
                cgraph[i].push_back(j);
                cgraph[j].push_back(i);
            }
        }
    }
}

int PatternCollection::get_maxi_vertex(const vector<int> &subg, const vector<int> &cand) const {
    // assert that subg and cand are sorted
    size_t max = 0;
    int vertex = subg[0];

    for (size_t i = 1; i < subg.size(); ++i) {
        vector<int> intersection;
        intersection.reserve(subg.size());
        // for vertex u in subg get u's adjacent vertices --> cgraph[subg[i]];
        set_intersection(cand.begin(), cand.end(), cgraph[subg[i]].begin(), cgraph[subg[i]].end(), back_inserter(intersection));

        if (intersection.size() > max) {
            max = intersection.size();
            vertex = subg[i];
        }
    }
    return vertex;
}

void PatternCollection::max_cliques_expand(vector<int> &subg, vector<int> &cand, vector<int> &q_clique) {
    if (subg.empty()) {
        //cout << "clique" << endl;
        max_cliques.push_back(q_clique);
    } else {
        int u = get_maxi_vertex(subg, cand);

        vector<int> ext_u;
        ext_u.reserve(cand.size());
        set_difference(cand.begin(), cand.end(), cgraph[u].begin(), cgraph[u].end(), back_inserter(ext_u));

        for (size_t i = 0; i < ext_u.size(); ++i) { // while cand - gamma(u) is not empty
            int q = ext_u[i]; // q is a vertex in cand - gamma(u)
            //cout << q << ",";
            q_clique.push_back(q);

            // subg_q = subg n gamma(q)
            vector<int> subg_q;
            subg_q.reserve(subg.size());
            set_intersection(subg.begin(), subg.end(), cgraph[q].begin(), cgraph[q].end(), back_inserter(subg_q));

            // cand_q = cand n gamma(q)
            vector<int> cand_q;
            cand_q.reserve(cand.size());
            set_intersection(cand.begin() + i, cand.end(), cgraph[q].begin(), cgraph[q].end(), back_inserter(cand_q));

            max_cliques_expand(subg_q, cand_q, q_clique);

            // remove q from cand --> cand = cand - q
            // is done --> with index i

            //cout << "back"  << endl;
            q_clique.pop_back();
        }
    }
}

int PatternCollection::get_heuristic_value(const State &state) const {
    int max_val = 0;
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        vector<int> clique = max_cliques[i];
        int h_val = 0;
        for (size_t j = 0; j < clique.size(); ++j) {
            h_val += pattern_databases[clique[j]].get_heuristic_value(state);
        }
        if (h_val > max_val) {
            max_val = h_val;
        }
    }
    return max_val;
}

void PatternCollection::dump() const {
    // print compatibility graph
    cout << "Compatibility graph" << endl;
    for (size_t i = 0; i < cgraph.size(); i++) {
        cout << i << " adjacent to [ ";
        for (size_t j = 0; j < cgraph[i].size(); j++) {
            cout << cgraph[i][j] << " ";
        }
        cout << "]" << endl;
    }
    // print maximal cliques
    assert(max_cliques.size() > 0);
    cout << max_cliques.size() << " maximal clique(s)" << endl;
    cout << "Maximal cliques are { ";
    for (size_t i = 0; i < max_cliques.size(); i++) {
        cout << "[ ";
        for (size_t j = 0; j < max_cliques[i].size(); j++) {
            cout << max_cliques[i][j] << " ";
        }
        cout << "] ";
    }
    cout << "}" << endl;
}
