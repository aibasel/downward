#include "data.h"
#include "options.h"
#include "setcover.h"
#include "tools.h"

#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using namespace std;

SASEncoding Domain::codingStep(const vector<MergedPredicate> &pred) {
  Timer t;
  if(!options.silent(Options::CODING))
    cout << "Generating encoding..." << flush;

  set<int> initSet(initFacts.begin(), initFacts.end());
  set<int> fluentSet(fluentFacts.begin(), fluentFacts.end());

  vector<vector<int> > rawGroups;
  for(int i = 0; i < pred.size(); i++) {
    vector<vector<int> > candidateGroups = pred[i].getFactGroups(objects.size());
    for(int i = 0; i < candidateGroups.size(); i++) {
      vector<int> &group = candidateGroups[i];
      vector<int> fluentGroup;
      int count = 0;
      for(int j = 0; j < group.size(); j++) {
	if(initSet.count(group[j])) {
	  count++;
	  if(count == 2)
	    break;
	}
	if(fluentSet.count(group[j]))
	  fluentGroup.push_back(group[j]);
      }
      if(count == 1)
	rawGroups.push_back(fluentGroup);
    }
  }
  
  SASEncoding code(fluentFacts, rawGroups);

  if(options.verbose(Options::CODING)) {
    cout << endl;
    code.dump(*this);
    cout << "  Elapsed time: " << t.total() << endl
	 << "  Encoding size: " << code.getVariableCount()
	 << " groups" << endl << endl;
  } else if(!options.silent(Options::CODING))
    cout << " time: " << t.total() << "; encoding size: "
	 << code.getVariableCount() << " groups" << endl;

  return code;
}

SASEncoding::SASEncoding(const vector<int> &fluentFacts,
			 const vector<vector<int> > &rawGroups) {
  vector<int> selectedGroups = ::set_cover(rawGroups);

  set<int> unassignedFluents(fluentFacts.begin(), fluentFacts.end());
  for(int i = 0; i < selectedGroups.size(); i++) {
    const vector<int> &rawGroup = rawGroups[selectedGroups[i]];
    for(int j = 0; j < rawGroup.size(); j++)
      unassignedFluents.erase(rawGroup[j]);
  }

  for(int i = 0; i < selectedGroups.size(); i++) {
    const vector<int> &rawGroup = rawGroups[selectedGroups[i]];
    rangesWithoutNone.push_back(0);
    for(int j = 0; j < rawGroup.size(); j++) {
      int fact = rawGroup[j];
      if(!factToAssignment.count(fact)) {
	factToAssignment[fact] = Assignment(i, rangesWithoutNone.back());
	rangesWithoutNone.back()++;
      }
    }
  }

  for(set<int>::iterator curr = unassignedFluents.begin();
      curr != unassignedFluents.end(); ++curr) {
    int fact = *curr;
    factToAssignment[fact] = Assignment(rangesWithoutNone.size(), 0);
    rangesWithoutNone.push_back(1);
  }
}

int SASEncoding::getVariableCount() const {
  return rangesWithoutNone.size();
}

int SASEncoding::lookupVariable(int fact) const {
  map<int, Assignment>::const_iterator it = factToAssignment.find(fact);
  if(it == factToAssignment.end()) // non-fluent fact
    return -1;
  return it->second.var;
}

pair<int, int> SASEncoding::lookupVariableValue(int fact) const {
  map<int, Assignment>::const_iterator it = factToAssignment.find(fact);
  if(it == factToAssignment.end()) // non-fluent fact
    return make_pair(-1, -1);
  return make_pair(it->second.var, it->second.val);
}

int SASEncoding::getNoneValue(int groupNo) const {
  return rangesWithoutNone[groupNo];
}

void SASEncoding::dump(const Domain &dom) const {
  vector<vector<int> > groups;
  for(int i = 0; i < rangesWithoutNone.size(); i++) {
    vector<int> init;
    init.resize(rangesWithoutNone[i], -1);
    groups.push_back(init);
  }

  for(map<int, Assignment>::const_iterator it = factToAssignment.begin();
      it != factToAssignment.end(); ++it) {
    int fact = it->first;
    Assignment ass = it->second;
    assert(groups[ass.var][ass.val] == -1);
    groups[ass.var][ass.val] = fact;
  }

  for(int i = 0; i < groups.size(); i++) {
    cout << "[Group " << i << "]" << endl;
    for(int j = 0; j < groups[i].size(); j++) {
      assert(groups[i][j] != -1);
      cout << Fact(groups[i][j]).toString(dom) << endl;
    }
    cout << endl;
  }
}
