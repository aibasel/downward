#include "max_cliques.h"
#include "utilities.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

class MaxCliqueComputer {
    const vector<vector<int> > &graph;
    vector<vector<int> > &max_cliques;
    vector<int> q_clique; // contains actual calculated maximal clique

    int get_maximizing_vertex(
        const vector<int> &subg, const vector<int> &cand) {
        // assert that subg and cand are sorted
        for (int i = 0; i < subg.size() - 1; ++i) {
            assert(subg[i] < subg[i + 1]);
        }
        for (int i = 0; i < cand.size() - 1; ++i) {
            assert(cand[i] < cand[i + 1]);
        }
        
        // cout << "subg: " << subg << endl;
        // cout << "cand: " << cand << endl;
        int max = 0; 
        // TODO If we use -1 the comparation intersection.size() > max is always false.
        int vertex = subg[0]; // We will take the first vertex if there is no better one.

        for (size_t i = 0; i < subg.size(); ++i) {
            vector<int> intersection;
            intersection.reserve(subg.size());
            // for vertex u in subg get u's adjacent vertices --> graph[subg[i]];
            set_intersection(cand.begin(), cand.end(), graph[subg[i]].begin(), graph[subg[i]].end(), back_inserter(intersection));

            if (intersection.size() > max) {
                max = intersection.size();
                vertex = subg[i];
                //cout << "sucess: there is a maximizing vertex." << endl;
            }
        }
        return vertex;
    }

    void expand(vector<int> &subg, vector<int> &cand) {
        // cout << "subg: " << subg << endl;
        // cout << "cand: " << cand << endl;
        if (subg.empty()) {
            //cout << "clique" << endl;
            max_cliques.push_back(q_clique);
        } else {
            int u = get_maximizing_vertex(subg, cand);

            vector<int> ext_u;
            ext_u.reserve(cand.size());
            set_difference(cand.begin(), cand.end(), graph[u].begin(), graph[u].end(), back_inserter(ext_u));

            while (!ext_u.empty()) {
                int q = ext_u.back();
                ext_u.pop_back();
                //cout << q << ",";
                q_clique.push_back(q);

                // subg_q = subg n gamma(q)
                vector<int> subg_q;
                subg_q.reserve(subg.size());
                set_intersection(subg.begin(), subg.end(), graph[q].begin(), graph[q].end(), back_inserter(subg_q));

                // cand_q = cand n gamma(q)
                vector<int> cand_q;
                cand_q.reserve(cand.size());
                set_intersection(cand.begin(), cand.end(), graph[q].begin(), graph[q].end(), back_inserter(cand_q));
                expand(subg_q, cand_q);

                // remove q from cand --> cand = cand - q
                // the deleted element seems to be always the first element in cand
                cand.erase(lower_bound(cand.begin(), cand.end(), q));

                //cout << "back" << endl;
                q_clique.pop_back();
            }
        }
    }

public:
    MaxCliqueComputer(const vector<vector<int> > &graph_,
           vector<vector<int> > &max_cliques_)
        : graph(graph_), max_cliques(max_cliques_) {
    }

    void compute() {
        vector<int> vertices_1;
        vertices_1.reserve(graph.size());
        for (size_t i = 0; i < graph.size(); ++i) {
            vertices_1.push_back(i);
        }
        vector<int> vertices_2(vertices_1); // copy vector
        q_clique.reserve(graph.size());
        expand(vertices_1, vertices_2);
    }
};


void compute_max_cliques(
    const vector<vector<int> > &graph,
    vector<vector<int> > &max_cliques) {
    MaxCliqueComputer clique_computer(graph, max_cliques);
    clique_computer.compute();
}
