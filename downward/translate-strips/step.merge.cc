#include "data.h"
#include "tools.h"
#include "options.h"

#include <iostream>
#include <algorithm>           // equal
using namespace std;

vector<MergedPredicate> Domain::mergeStep() {
  Timer t;
  if(!options.silent(Options::MERGE))
    cout << "Merging predicates..." << flush;
  if(options.verbose(Options::MERGE))
    cout << endl;
  
  vector<MergedPredicate> results;
  for(int pred = 0; pred < predicates.size(); pred++) {
    Predicate &p = *predicates[pred];
    if(!p.isConstant())
      for(int par = 0; par < p.getParameterCount(); par++) {
	if(options.debug(Options::MERGE))
	  cout << "  checking " << p.getName() << " [" << par << "]..." << endl;
	MergedPredicate m(*predicates[pred], par);
	mergeCheckMergedPredicate(results, m);
      }
  }
  
  if(options.verbose(Options::MERGE)) {
    cout << endl;
    for(int i = 0; i < results.size(); i++)
      cout << results[i].toString();
    cout << endl;
  }

  if(options.verbose(Options::MERGE))
    cout << "  Elapsed time: " << t.total() << endl << endl;
  else if(!options.silent(Options::MERGE))
    cout << " time: " << t.total() << "; "
	 << results.size() << " blocks found" << endl;

  return results;
}

void Domain::mergeCheckMergedPredicate(vector<MergedPredicate> &results, MergedPredicate &set) {
  for(int i = 0; i < actions.size(); i++) {
    vector<SymbolicFact *> &add = actions[i]->getAddEffects();
    vector<SymbolicFact *> &del = actions[i]->getDelEffects();
    
    // 1. check add effects (build parLists)
    vector<vector<int> > parLists;
    for(int j = 0; j < add.size(); j++)
      if(!mergeCheckAddEffect(set, parLists, *add[j]))
        return;
    
    // 2. check del effects (shrink parLists)
    for(int j = 0; j < del.size(); j++)
      mergeCheckDelEffect(set, parLists, *del[j]);
    
    // 3. try to merge with other predicates, if necessary
    if(!parLists.empty()) {
      vector<int> arg = parLists.front();
      for(int j = 0; j < del.size(); j++) {
	if(!set.findPredicate(del[j]->getPredicate())) { // predicate not yet in the set
	  vector<vector<int> > matches = del[j]->getMatchingParOrders(arg);
	  for(int k = 0; k < matches.size(); k++) {
	    set.pushPredicate(del[j]->getPredicate(), matches[k]);
	    mergeCheckMergedPredicate(results, set);
	    set.popPredicate();
	  }
	}
      }
      return;
    }
  } // if we arrive here, we have found our candidates
  set.makeCanonical();
  if(find(results.begin(), results.end(), set) == results.end()) { // this is a new result
    results.push_back(set);
  }
}

bool Domain::mergeCheckAddEffect(MergedPredicate &set, vector<vector<int> > &vec, SymbolicFact &eff) {
  PartPredicate *part = set.findPredicate(eff.getPredicate());
  if(part) {
    vector<int> args = part->getOrderedArgList(eff);
    for(int i = 0; i < vec.size(); i++)
      if(equal(vec[i].begin() + 1, vec[i].end(), args.begin() + 1)
	 && vec[i].front() != args.front())
	return false; // Var := x, Var := y, x != y: unbalanced!
    vec.push_back(args);
  }
  return true;
}

void Domain::mergeCheckDelEffect(MergedPredicate &set, vector<vector<int> > &vec, SymbolicFact &eff) {
  PartPredicate *part = set.findPredicate(eff.getPredicate());
  if(part) {
    vector<int> args = part->getOrderedArgList(eff);
    for(int i = 0; i < vec.size(); i++)
	      if(equal(vec[i].begin() + 1, vec[i].end(), args.begin() + 1)
        && eff.existsMatchingPrecondition())
	        vec.erase(vec.begin() + i);
  }
}

