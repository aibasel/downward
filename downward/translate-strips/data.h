#ifndef _DATA_H
#define _DATA_H

#include <string>
#include <vector>
#include <map>
using namespace std;

class Object;
class Predicate;
class Fact;
class SymbolicFact;
class Action;
class Domain;

class PartPredicate;
class MergedPredicate;

class LispEntity;

#include "bitarray.h"
#include "step.explore.h"
#include "step.coding.h"

class Object {
  int id;
  string name;
public:
  Object(int objId, LispEntity &e);
  int toInt() const {return id;}
  string getName() const {return name;}
};

class Predicate {
  Predicate(const Predicate &copy); // prevent copying
  string name;
  int baseId;
  int parameterCount;
  bool settable, clearable;
public:
  Predicate(LispEntity &le, int factLowerBound);
  int getFactLowerBound() const {return baseId;}
  int getFactUpperBound(int objectCount) const;
  int getParameterCount() const {return parameterCount;}
  string getName() const       {return name;}
  string toString() const;

  void markSettable()  {settable = true;}
  void markClearable() {clearable = true;}
  bool isConstant()    {return !settable && !clearable;}

private:                    // special methods and members for ExploreStep
  friend class ExploreStep;
  // actually, we only need to befriend
  // ExploreStep::initPredicateData, but cannot do this in more
  // recent g++ versions because that method is private.
  // See "Must friend declaration names be accessible?"
  // (http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_closed.html#209).
  vector<BitArray> projections;
  vector<SymbolicFact *> preconditions;
public:
  unsigned long getProjection(int arg, int obj) {return projections[arg].get(obj);} // treat as bool
  void setProjection(int arg, int obj) {projections[arg].set(obj);}
  vector<SymbolicFact *> &getPreconditions() {return preconditions;}
};

class Fact {
  int id;
  const Predicate &getPredicate(const Domain &d) const;       // very slow
public:
  Fact(const Domain &d, LispEntity &le);
  explicit Fact(int factId) : id(factId) {}
  int toInt() const                      {return id;}
  string toString(const Domain &d) const;
};

class SymbolicFact {
  SymbolicFact(const SymbolicFact &copy); // prevent copying
  Action *action;
  Predicate *predicate;
  vector<int> arguments;
public:
  SymbolicFact(Action *a, map<string, int> &parTable,
	       Domain &d, LispEntity &le);
  ~SymbolicFact();
  string toString();
  Action &getAction()                   {return *action;}
  Predicate &getPredicate()             {return *predicate;}
  const vector<int> &getArguments()     {return arguments;}

  bool existsMatchingPrecondition();
  vector<vector<int> > getMatchingParOrders(const vector<int> &srcArgList);

  // for ExploreStep
  int instantiateFact(const vector<vector<int>::iterator> &parameters,
                      int objectCount) {
    int code = 0;
    vector<int>::iterator pos, end = arguments.end();
    for(pos = arguments.begin(); pos != end; ++pos) {
      code *= objectCount;
      code += *parameters[*pos];
    }
    return code + predicate->getFactLowerBound();
  }
};

class InstantiatedAction {
  string name;
public:
  InstantiatedAction(string action,
		     const vector<vector<int>::iterator> pars,
		     Domain &dom);
  vector<int> pre, add, del;
  string getName() const;
  bool operator<(const InstantiatedAction &other) const; // sorting
  bool operator==(const InstantiatedAction &other) const; // unique
  string toString() const;
};

class Action {
  Action(const Action &copy); // prevent copying
  string name;
  int parameterCount;
  vector<SymbolicFact *> pre, add, del;
public:
  Action(Domain &d, LispEntity &le);
  ~Action();

  string getName()                            {return name;}
  string toString();
  vector<SymbolicFact *> &getPreconditions()  {return pre;}
  vector<SymbolicFact *> &getAddEffects()     {return add;}
  vector<SymbolicFact *> &getDelEffects()     {return del;}
  int getParameterCount()                   {return parameterCount;}
private:                   
  // special methods and members for ExploreStep::initActionData
  friend class ExploreStep;
  vector<vector<SymbolicFact *> > preByMaxPar;
          // non-unary preconditions with a given maximum argument number
  vector<vector<int> > preconditionCount;
  vector<vector<int> > validArguments;
public:
  vector<SymbolicFact *> &getPreconditionsByMaxPar(int maxPar) {return preByMaxPar[maxPar];}
  bool decreasePreconditionCountdown(int parNo, int objNo) {
    if(--(preconditionCount[parNo][objNo]) == 0) {
      validArguments[parNo].push_back(objNo);
      return true;
    }
    return false;
  }
  vector<int> &getValidArguments(int parNo) {return validArguments[parNo];}
  bool isValidArgument(int parNo, int objNo) {return preconditionCount[parNo][objNo] == 0;}

public:
  double getMaxOperators(int objNo) const;
  void logInstantiation(const InstantiatedAction &inst) {
    instantiations.push_back(inst);
  }
  vector<InstantiatedAction> &getInstantiations() {return instantiations;}
private:
  vector<InstantiatedAction> instantiations;
};

class Domain {
  Domain(const Domain &copy); // prevent copying
  // the vectors contain pointers rather than objects since they cannot
  // be copied (and thus put into the vector) without major difficulties,
  // since this would invalidate cross-references (e.g. SymbolicFact::action).

  string domainName, problemName;
  vector<Object> objects;
  vector<Predicate *> predicates;
  vector<Action *> actions;
  vector<int> initFacts, goalFacts;
  map<string, int> objectTable;
  map<string, int> predicateTable;
  int factCount;    // total number of possible fact instantiations
  vector<int> trueFacts, fluentFacts;   // set by exploreStep()

  double getOperatorCount() const;

  void parsingStep(string domFile, string probFile);
  void constantStep();
  vector<MergedPredicate> mergeStep();
  void mergeCheckMergedPredicate(vector<MergedPredicate> &results, MergedPredicate &set);
  bool mergeCheckAddEffect(MergedPredicate &set, vector<vector<int> > &vec, SymbolicFact &eff);
  void mergeCheckDelEffect(MergedPredicate &set, vector<vector<int> > &vec, SymbolicFact &eff);
  void exploreStep();
  SASEncoding codingStep(const vector<MergedPredicate> &pred);
public:
  Domain(string domFile, string probFile);
  ~Domain();
  
  int getObjectCount() const        {return objects.size();}
  Object &getObject(int idx)        {return objects[idx];}
  const Object &getObject(int idx) const {return objects[idx];}
  const Object &lookupObject(string str) const;       // throws "not found" exception

  int getPredicateCount() const     {return predicates.size();}
  const Predicate &getPredicate(int idx) const {return *predicates[idx];}
  Predicate &getPredicate(int idx)  {return *predicates[idx];}
  const Predicate &lookupPredicate(string str) const; // throws "not found" exception
  Predicate &lookupPredicate(string str); // throws "not found" exception

  int getActionCount() const        {return actions.size();}
  Action &getAction(int idx)        {return *actions[idx];}
  
  const vector<int> &getInitFacts() {return initFacts;}
  const vector<int> &getGoalFacts() {return goalFacts;}

  string toString();
};

class PartPredicate {
  Predicate *predicate;
  bool nullState;
  vector<int> parOrder;
  PartPredicate(Predicate &pr, bool nullSt, const vector<int> &ord)
    : predicate(&pr), nullState(nullSt), parOrder(ord) {}
public:
  friend class MergedPredicate;
  bool operator<(const PartPredicate &comp) const;  // for sorting MergedPredicate
  bool operator==(const PartPredicate &comp) const; // for finding duplicate MergedPredicates
  vector<int> getOrderedArgList(SymbolicFact &eff);
  void appendInstantiations(vector<int> &rawArgs, vector<int> &group, int objectCount) const;
                                                    // for encoding step
  string toString();
};

class MergedPredicate {
  int parCount;
  vector<PartPredicate> parts;
public:
  MergedPredicate(Predicate &initPred, int parNo);
  void pushPredicate(Predicate &p, vector<int> &order);
  void popPredicate();
  PartPredicate *findPredicate(Predicate &p);
  void makeCanonical();
  bool operator==(const MergedPredicate &comp) const; // for finding duplicate VarSets
  vector<vector<int> > getFactGroups(int objectCount) const;  // for encoding step
  string toString();
};

extern Domain *g_domain; // HACK! for debugging dumps

#endif
