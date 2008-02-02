#ifndef MAX_DAG_H
#define MAX_DAG_H

// TODO: Document

#include <map>
#include <vector>
using namespace std;

class MaxDAG {
  const vector<vector<pair<int, int> > > &weighted_graph;
  bool debug;
public:
  MaxDAG(const vector<vector<pair<int, int> > > &graph) : weighted_graph(graph), 
    debug(false) {}
  vector<int> get_result();
};
#endif
