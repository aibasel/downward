#include "data.h"
#include "parser.h"
#include "tools.h"

Fact::Fact(const Domain &d, LispEntity &le) {
  vector<LispEntity> &vec = le.getList();
  if(vec.size() == 0)
    ::error("invalid fact: no predicate given");
  const Predicate &pred = d.lookupPredicate(vec[0].getString());
  
  int objCount = d.getObjectCount();
  id = 0;
  for(int i = 1; i < vec.size(); i++) {
    id *= objCount;
    id += d.lookupObject(vec[i].getString()).toInt();
  }
  id += pred.getFactLowerBound();
}

const Predicate &Fact::getPredicate(const Domain &d) const {
  int i = 1, predCount = d.getPredicateCount();
  while(i < predCount && id >= d.getPredicate(i).getFactLowerBound())
    i++;
  return d.getPredicate(i - 1);
}

string Fact::toString(const Domain &d) const {
  const Predicate &pred = getPredicate(d);
  int val = id - pred.getFactLowerBound();
  int objCount = d.getObjectCount();
  string back;
  for(int i = 0; i < pred.getParameterCount(); i++) {
    back = d.getObject(val % objCount).getName() + " " + back;
    val /= objCount;
  }
  return pred.getName() + " " + back;	
}
