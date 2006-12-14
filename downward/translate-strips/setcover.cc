#include "setcover.h"

#include <map>
#include <set>
#include <vector>
using namespace std;

struct Candidate;

typedef multimap<int, Candidate *> OpenList;

struct Candidate {
  set<int> elements;
  int index;
  OpenList::iterator pos;
  Candidate() {}
  Candidate(const set<int> &elem, int i) : elements(elem), index(i) {}
};

vector<int> set_cover(const vector<vector<int> > &input) {
  /*
    "input" encodes a system of sets of integers, S = {S_1, ... S_n}.
    The returned vector contains a list of unique indices i1, ..., ik such
    that T = {S_i1, ..., S_ik} is a minimal subset of S satisfying
    S_i1 \cup ... \cup S_ik = S_1 \cup ... \cup S_n.
    The list is returned in the order of creation, i.e. sets which contribute
    much to the cover appear near the start.

    This uses a greedy algorithm: At every stage, add a set containing a
    maximal number of elements not contained in the covering chosen so far.
   */


  OpenList openList;
  vector<Candidate> candidate;
  multimap<int, Candidate *> itemToCandidates;
  for(int i = 0; i < input.size(); i++)
    candidate.push_back(Candidate(set<int>(input[i].begin(), input[i].end()), i));
  
  // Pointers into the candidate vector won't change from here onwards.
  for(int i = 0; i < candidate.size(); i++) {
    candidate[i].pos = openList.insert(make_pair(-input[i].size(), &candidate[i]));
    for(int j = 0; j < input[i].size(); j++)
      itemToCandidates.insert(make_pair(input[i][j], &candidate[i]));
  }

  vector<int> result;
  while(!openList.empty()) {
    OpenList::iterator top = openList.begin();
    if(top->first == 0)
      break;
    Candidate *selectedCand = top->second;
    openList.erase(top);
    result.push_back(selectedCand->index);
    set<int>::iterator curr, end = selectedCand->elements.end();
    for(curr = selectedCand->elements.begin(); curr != end; ++curr) {
      int item = *curr;
      typedef multimap<int, Candidate *>::const_iterator CandIt;
      pair<CandIt, CandIt> candRange = itemToCandidates.equal_range(item);
      CandIt currCand, lastCand = candRange.second;
      for(currCand = candRange.first; currCand != lastCand; ++currCand) {
	Candidate *cand = currCand->second;
	if(cand != selectedCand) {
	  cand->elements.erase(item);
	  openList.erase(cand->pos);
	  cand->pos = openList.insert(make_pair(-cand->elements.size(), cand));
	}
      }
    }
  }
  
  return result;
}

/*
#include <iostream>
using namespace std;

int main() {
  int test1[] = {1, 5, -1};
  int test2[] = {2, 4, 6, 7, -1};
  int test3[] = {1, 2, 3, -1};
  int test4[] = {4, 5, 6, -1};
  int test5[] = {2, 5, 6, -1};
  int *test[] = {test1, test2, test3, test4, test5, 0};
  
  vector<vector<int> > input;
  for(int i = 0; test[i] != 0; i++) {
    vector<int> vec;
    for(int j = 0; test[i][j] != -1; j++)
      vec.push_back(test[i][j]);
    input.push_back(vec);
  }

  vector<int> result = set_cover(input);
  for(int i = 0; i < result.size(); i++) {
    vector<int> &vec = input[result[i]];
    for(int j = 0; j < vec.size(); j++)
      cout << " " << vec[j];
    cout << endl;
  }
}
*/
