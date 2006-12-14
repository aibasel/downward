#include "data.h"
#include "tools.h"

#include <algorithm>
using namespace std;

MergedPredicate::MergedPredicate(Predicate &initPred, int parNo) {
  parCount = initPred.getParameterCount();
  vector<int> order;
  order.push_back(parNo);
  for(int i = 0; i < parCount; i++)
    if(i != parNo)
      order.push_back(i);
  parts.push_back(PartPredicate(initPred, false, order));
}

void MergedPredicate::makeCanonical() {
  sort(parts.begin(), parts.end());
}

void MergedPredicate::pushPredicate(Predicate &p, vector<int> &order) {
  parts.push_back(PartPredicate(p, p.getParameterCount() != parCount, order));
}

void MergedPredicate::popPredicate() {
  parts.pop_back();
}

bool MergedPredicate::operator==(const MergedPredicate &comp) const {
  return parCount == comp.parCount && parts == comp.parts;
}

PartPredicate *MergedPredicate::findPredicate(Predicate &p) {
  for(int i = 0; i < parts.size(); i++)
    if(parts[i].predicate == &p)
      return &parts[i];
  return 0;
}

string MergedPredicate::toString() {
  string back = "Dumping merged predicate (parCount = "
    + ::toString(parCount) + ")\n";
  for(int i = 0; i < parts.size(); i++)
    back += parts[i].toString() + "\n";
  return back;
}

vector<vector<int> > MergedPredicate::getFactGroups(int objectCount) const {
  vector<vector<int> > back;
  vector<int> group;
  group.reserve(objectCount * parts.size());
  for(vector<int> rawArgs = ::firstTuple(parCount - 1);
      !::lastTuple(rawArgs, objectCount); ::nextTuple(rawArgs, objectCount)) {
    group.clear();
    for(int i = 0; i < parts.size(); i++)
      parts[i].appendInstantiations(rawArgs, group, objectCount);
    back.push_back(group);
  }
  return back;
}

