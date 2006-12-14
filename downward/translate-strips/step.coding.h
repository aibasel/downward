#ifndef STEP_CODING_H
#define STEP_CODING_H

#include <map>
#include <vector>
using namespace std;

class SASEncoding {
  struct Assignment {
    Assignment(int theVar, int theVal) : var(theVar), val(theVal) {}
    Assignment() {} // required for map<int, Assignment>
    int var, val;
  };
  vector<int> rangesWithoutNone;
  map<int, Assignment> factToAssignment;
public:
  SASEncoding(const vector<int> &fluentFacts, const vector<vector<int> > &rawGroups);
  int getVariableCount() const;
  int lookupVariable(int fact) const;
  pair<int, int> lookupVariableValue(int fact) const;
  int getNoneValue(int var) const;
  void dump(const Domain &dom) const;
};

#endif
