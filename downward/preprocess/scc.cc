#include "scc.h"
#include <algorithm>
#include <vector>
using namespace std;

vector<vector<int> > SCC::get_result() {
  int node_count = graph.size();
  dfs_numbers.resize(node_count, -1);
  dfs_minima.resize(node_count, -1);
  stack_indices.resize(node_count, -1);
  stack.reserve(node_count);
  current_dfs_number = 0;

  for(int i = 0; i < node_count; i++)
    if(dfs_numbers[i] == -1)
      dfs(i);

  reverse(sccs.begin(), sccs.end());
  return sccs;
}

void SCC::dfs(int vertex) {
  int vertex_dfs_number = current_dfs_number++;
  dfs_numbers[vertex] = dfs_minima[vertex] = vertex_dfs_number;
  stack_indices[vertex] = stack.size();
  stack.push_back(vertex);

  const vector<int> &successors = graph[vertex];
  for(int i = 0; i < successors.size(); i++) {
    int succ = successors[i];
    int succ_dfs_number = dfs_numbers[succ];
    if(succ_dfs_number == -1) {
      dfs(succ);
      dfs_minima[vertex] = min(dfs_minima[vertex], dfs_minima[succ]);
    } else if(succ_dfs_number < vertex_dfs_number && stack_indices[succ] != -1) {
      dfs_minima[vertex] = min(dfs_minima[vertex], succ_dfs_number);
    }
  }

  if(dfs_minima[vertex] == vertex_dfs_number) {
    int stack_index = stack_indices[vertex];
    vector<int> scc;
    for(int i = stack_index; i < stack.size(); i++) {
      scc.push_back(stack[i]);
      stack_indices[stack[i]] = -1;
    }
    stack.erase(stack.begin() + stack_index, stack.end());
    sccs.push_back(scc);
  }
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
  int n2[] = {4, -1};
  int n3[] = {8, -1};
  int n4[] = {0, 7, -1};
  int n5[] = {4, -1};
  int n6[] = {5, 8, -1};
  int n7[] = {2, 6, -1};
  int n8[] = {1, -1};
  int *all_nodes[] = {n0, n1, n2, n3, n4, n5, n6, n7, n8, 0};

  vector<vector<int> > graph;
  for(int i = 0; all_nodes[i] != 0; i++) {
    vector<int> successors;
    for(int j = 0; all_nodes[i][j] != -1; j++)
      successors.push_back(all_nodes[i][j]);
    graph.push_back(successors);
  }

  vector<vector<int> > sccs = SCC(graph).get_result();
  for(int i = 0; i < sccs.size(); i++) {
    for(int j = 0; j < sccs[i].size(); j++)
      cout << " " << sccs[i][j];
    cout << endl;
  }
}
*/
