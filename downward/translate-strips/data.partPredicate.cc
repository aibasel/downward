#include "data.h"
#include "tools.h"

bool PartPredicate::operator==(const PartPredicate &comp) const {
  return predicate == comp.predicate && nullState == comp.nullState
    && parOrder == comp.parOrder;
}

bool PartPredicate::operator<(const PartPredicate &comp) const {
  if(predicate != comp.predicate)
    return predicate < comp.predicate;
  if(nullState != comp.nullState)
    return nullState == false;
  return parOrder < comp.parOrder;
}

string PartPredicate::toString() {
  string back = predicate->getName();
  if(nullState)
    back += " (null state)";
  for(int i = 0; i < parOrder.size(); i++)
    back += " " + ::toString(parOrder[i]);
  return back;
}

vector<int> PartPredicate::getOrderedArgList(SymbolicFact &eff) {
  // Back permuted by parOrder equals eff.parameters and
  // the length of back equals the number of parameters of the
  // merged predicate; inserts -1 at the front for null states.
  // This allows comparing parameter lists of PartPredicates with
  // different parameter orders.

  int parCount = parOrder.size();
  vector<int> args;
  args.resize(parCount);
  if(nullState)   // special tag for value field of null-state predicates
    args[0] = -1;
  for(int i = (nullState ? 1 : 0); i < parCount; i++)
      args[i] = eff.getArguments()[parOrder[i]];
  return args;
}

void PartPredicate::appendInstantiations(vector<int> &rawArgs, vector<int> &group,
					 int objectCount) const {
  int code = predicate->getFactLowerBound();
  int maxPower = parOrder.size() - (nullState ? 2 : 1);
  for(int i = 1; i < parOrder.size(); i++)
    code += rawArgs[i - 1] * ::pow(objectCount, maxPower - parOrder[i]);

  if(nullState) {
    group.push_back(code);
  } else {
    int multiplier = ::pow(objectCount, maxPower - parOrder[0]);
    for(int i = 0; i < objectCount; i++) {
      group.push_back(code);
      code += multiplier;
    }
  }
}

