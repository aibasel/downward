#ifndef _STEP_EXPLORE_H
#define _STEP_EXPLORE_H

#include <vector>
#include <list>
using namespace std;

class ExploreStep {
  Domain &domain;   // only needed for logging facts
  int objectCount;

  list<pair<Predicate *, int> > queue;
  BitArray processedFacts;
  BitArray enqueuedFacts;
  BitArray constantFacts;
  int instantiationCount;

  void initPredicateData(vector<Predicate *> &predicates, vector<Action *> &actions);
  void initActionData(vector<Action *> &actions);
  void fireEmptyPreconditions(vector<Action *> &actions);
  void exploreFact(Predicate &pred, int factId);
  bool prepareArgLists(SymbolicFact &pre, vector<vector<int> *> &argLists, int fact);
  void instantiate(Action &a, vector<vector<int> *> &argLists);
  bool satisfied(Action &a, const vector<vector<int>::iterator> &parameters, int argBound);
  void handleEffects(Action &a, const vector<vector<int>::iterator> &parameters);
 public:
  ExploreStep(vector<Predicate *> &predicates, vector<Action *> &actions,
	      int objCount, int factCount, vector<int> &init, Domain &dom);
  const BitArray &getFluentFacts() const   {return processedFacts;}
  const BitArray &getConstantFacts() const {return constantFacts;}
  int getInstantiationCount() {return instantiationCount;}
};

#endif






