#include "max_dag.h"

#include <cassert>
#include <iostream>
using namespace std;

vector<int> MaxDAG::get_result() {
    if (debug) {
        for (int i = 0; i < weighted_graph.size(); i++) {
            cout << "From " << i << ":";
            for (int j = 0; j < weighted_graph[i].size(); j++)
                cout << " " << weighted_graph[i][j].first
                     << " [weight " << weighted_graph[i][j].second << "]";
            cout << endl;
        }
    }
    vector<int> incoming_weights; // indexed by the graph's nodes
    incoming_weights.resize(weighted_graph.size(), 0);
    for (int node = 0; node < weighted_graph.size(); node++) {
        const vector<pair<int, int> > &weighted_edges = weighted_graph[node];
        for (int i = 0; i < weighted_edges.size(); i++)
            incoming_weights[weighted_edges[i].first] += weighted_edges[i].second;
    }

    // Build minHeap of nodes, compared by number of incoming edges.
    typedef multimap<int, int>::iterator HeapPosition;

    vector<HeapPosition> heap_positions;
    multimap<int, int> heap;
    for (int node = 0; node < weighted_graph.size(); node++) {
        if (debug)
            cout << "node " << node << " has " << incoming_weights[node] << " edges" << endl;
        HeapPosition pos = heap.insert(make_pair(incoming_weights[node], node));
        heap_positions.push_back(pos);
    }
    vector<bool> done;
    done.resize(weighted_graph.size(), false);

    vector<int> result;
    // Recursively delete node with minimal weight of incoming edges.
    while (!heap.empty()) {
        if (debug)
            cout << "minimal element is " << heap.begin()->second << endl;
        int removed = heap.begin()->second;
        done[removed] = true;
        result.push_back(removed);
        heap.erase(heap.begin());
        const vector<pair<int, int> > &succs = weighted_graph[removed];
        for (int i = 0; i < succs.size(); i++) {
            int target = succs[i].first;
            if (!done[target]) {
                int arc_weight = succs[i].second;
                while (arc_weight >= 100000)
                    arc_weight -= 100000;
                //cout << "Looking at arc from " << removed << " to " << target << endl;
                int new_weight = heap_positions[target]->first - arc_weight;
                heap.erase(heap_positions[target]);
                heap_positions[target] = heap.insert(make_pair(new_weight, target));
                if (debug)
                    cout << "node " << target << " has now " << new_weight << " edges " << endl;
            }
        }
    }
    if (debug) {
        cout << "result: " << endl;
        for (int i = 0; i < result.size(); i++)
            cout << result[i] << " - ";
        cout << endl;
    }
    return result;
}

/*
#include <iostream>
using namespace std;

int main() {
#if 0
  int n0[] = {1, -1};
  int n1[] = {-1};
  int n2[] = {3, 4, -1};
  int n3[] = {2, 4, -1};
  int n4[] = {0, 3, -1};
  int *all_nodes[] = {n0, n1, n2, n3, n4, 0};
#endif

  int n0[] = {-1};
  int n1[] = {3, -1};
  int n2[] = {1, 4, 5, -1};
  int n3[] = {2, 8, -1};
  int n4[] = {0, 7, -1};
  int n5[] = {4, -1};
  int n6[] = {5, 8, -1};
  int n7[] = {2, 5, -1};
  int *all_nodes[] = {n0, n1, n2, n3, n4, n5, n6, n7, 0};

  vector<vector<pair<int, int> > > graph;
  for(int i = 0; all_nodes[i] != 0; i++) {
    vector<pair<int, int> > weighted_successors;
    for(int j = 0; all_nodes[i][j] != -1; j++)
      weighted_successors.push_back(make_pair(all_nodes[i][j],all_nodes[i][j]));
    graph.push_back(weighted_successors);
  }

  vector<int> m = MaxDAG(graph).get_result();
  for(int i = 0; i < m.size(); i++)
    cout << m[i] << " - ";
  cout << endl;
}
*/
