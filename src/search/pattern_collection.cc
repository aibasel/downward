#include "pattern_collection.h"
#include "globals.h"
#include "plugin.h"
#include "state.h"
#include "operator.h"
#include "timer.h"
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <limits>

using namespace std;

PatternCollection::PatternCollection(const vector<vector<int> > &pat_coll) : pattern_collection(pat_coll), number_patterns(pat_coll.size()) {
    precompute_additive_vars();
    compute_max_cliques();

    // build all pattern databases
    Timer timer;
    for (int i = 0; i < pattern_collection.size(); ++i) {
        PDBAbstraction pdb(pattern_collection[i]);
        pattern_databases.push_back(pdb);
    }
    cout << pattern_collection.size() << " pdbs constructed." << endl;
    cout << "Construction time for all pdbs: " << timer << endl;
}

PatternCollection::~PatternCollection() {
}

//bool PatternCollection::are_additive(int pattern1, int pattern2) const {
  //  return are_additive[pattern1][pattern2];
//}

void PatternCollection::precompute_additive_vars() {
    int num_vars = g_variable_domain.size();
    are_additive.resize(num_vars, vector<bool>(num_vars, true));
    for (size_t k = 0; k < g_operators.size(); ++k) {
        const Operator &o = g_operators[k];
        const vector<PrePost> effects = o.get_pre_post();
        for (size_t e1 = 0; e1 < effects.size(); ++e1) {
            for (size_t e2 = 0; e2 < effects.size(); ++e2) {
                are_additive[effects[e1].var][effects[e2].var] = false;
            }
        }
    }
}

void PatternCollection::compute_max_cliques() {
    build_cgraph();
    cout << "built cgraph." << endl;
    vector<int> vertices_1;
    vertices_1.reserve(number_patterns);
    for (size_t i = 0; i < number_patterns; ++i) {
        vertices_1.push_back(i);
    }
    vector<int> vertices_2(vertices_1); // copy vector
    vector<int> q_clique; // contains actual calculated maximal clique
    q_clique.reserve(number_patterns);
    max_cliques_expand(vertices_1, vertices_2, q_clique);
    dump();

    cgraph.clear(); // or replace cgraph with empty vector
}

void PatternCollection::build_cgraph() {
    // initialize compatibility graph
    cgraph.resize(number_patterns);

    for (size_t i = 0; i < number_patterns; ++i) {
        for (size_t j = i + 1; j < number_patterns; ++j) {
            if (are_additive[i][j]) {
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
    int max_val = -1;
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        const vector<int> &clique = max_cliques[i];
        int h_val = 0;
        for (size_t j = 0; j < clique.size(); ++j) {
            int h = pattern_databases[clique[j]].get_heuristic_value(state);
            if (h == numeric_limits<int>::max()) {
                h_val = -1;
                break;
            }
            h_val += h;
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
    for (size_t i = 0; i < cgraph.size(); ++i) {
        cout << i << " adjacent to [ ";
        for (size_t j = 0; j < cgraph[i].size(); ++j) {
            cout << cgraph[i][j] << " ";
        }
        cout << "]" << endl;
    }
    // print maximal cliques
    assert(max_cliques.size() > 0);
    cout << max_cliques.size() << " maximal clique(s)" << endl;
    cout << "Maximal cliques are { ";
    for (size_t i = 0; i < max_cliques.size(); ++i) {
        cout << "[ ";
        for (size_t j = 0; j < max_cliques[i].size(); ++j) {
            cout << max_cliques[i][j] << " ";
        }
        cout << "] ";
    }
    cout << "}" << endl;
}
