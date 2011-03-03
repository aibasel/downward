#include "planning_utilities.h"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

int get_maximizing_vertex(const vector<int> &subg, const vector<int> &cand, const vector<vector<int> > &cgraph) {
    // assert that subg and cand are sorted
    //dump(subg, cand);
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

void expand(vector<int> &subg, vector<int> &cand, vector<int> &q_clique, const vector<vector<int> > &cgraph,
            vector<vector<int> > &max_cliques) {
    //dump(subg, cand);
    if (subg.empty()) {
        //cout << "clique" << endl;
        max_cliques.push_back(q_clique);
    } else {
        int u = get_maximizing_vertex(subg, cand, cgraph);

        vector<int> ext_u;
        ext_u.reserve(cand.size());
        set_difference(cand.begin(), cand.end(), cgraph[u].begin(), cgraph[u].end(), back_inserter(ext_u));

        while (!ext_u.empty()) {
            int q = ext_u.back();
            ext_u.pop_back();
            //cout << q << ",";
            q_clique.push_back(q);

            // subg_q = subg n gamma(q)
            vector<int> subg_q;
            subg_q.reserve(subg.size());
            set_intersection(subg.begin(), subg.end(), cgraph[q].begin(), cgraph[q].end(), back_inserter(subg_q));

            // cand_q = cand n gamma(q)
            vector<int> cand_q;
            cand_q.reserve(cand.size());
            set_intersection(cand.begin(), cand.end(), cgraph[q].begin(), cgraph[q].end(), back_inserter(cand_q));
            expand(subg_q, cand_q, q_clique, cgraph, max_cliques);

            // remove q from cand --> cand = cand - q
            // why? we removed it from ext_u and q is never in subg_q or cand_q--> is it worse to remove it (we don't know
            // the position) or to leave it and always have a bigger set_intersection vector

            //cout << "back" << endl;
            q_clique.pop_back();
        }
    }
}

void compute_max_cliques(const vector<vector<int> > &cgraph, vector<vector<int> > &max_cliques) {
    vector<int> vertices_1;
    vertices_1.reserve(cgraph.size());
    for (size_t i = 0; i < cgraph.size(); ++i) {
        vertices_1.push_back(i);
    }
    vector<int> vertices_2(vertices_1); // copy vector
    vector<int> q_clique; // contains actual calculated maximal clique
    q_clique.reserve(cgraph.size());
    expand(vertices_1, vertices_2, q_clique, cgraph, max_cliques);
}

void dump(const vector<int> &subg, const vector<int> &cand) {
    cout << "subg" << endl;
    for (size_t i = 0; i < subg.size(); ++i) {
        cout << subg[i] << ", ";
    }
    cout << endl;
    cout << "cand" << endl;
    for (size_t i = 0; i < cand.size(); ++i) {
        cout << cand[i] << ", ";
    }
    cout << endl;
}
