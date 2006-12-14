#include "data.h"
#include "parser.h"
#include "tools.h"

Predicate::Predicate(LispEntity &le, int factLowerBound) {
  settable = clearable = false;
  vector<LispEntity> &vec = le.getList();
  if(vec.size() == 0)
    ::error("invalid predicate: no name given");
  name = vec[0].getString();
  parameterCount = vec.size() - 1;
  for(int i = 1; i < vec.size(); i++)
    if(vec[i].getString()[0] != '?')
      ::error("invalid predicate: illegal parameter"
	          + vec[i].getString());

  baseId = factLowerBound;
}

int Predicate::getFactUpperBound(int objectCount) const {
  return baseId + ::pow(objectCount, parameterCount);
}

string Predicate::toString() const {
  string back = name + " (" + ::toString(parameterCount) + " parameter";
  if(parameterCount != 1)
    back += 's';
  back += ", base id: " + ::toString(baseId);
  if(settable)
    back += ", settable";
  if(clearable)
    back += ", clearable";
  return back + ")";
}
