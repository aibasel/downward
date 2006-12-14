#include "data.h"
#include "parser.h"
#include "tools.h"

#include <vector>
using namespace std;

Action::Action(Domain &d, LispEntity &le) {
  if(le.isString()
     || le.getList().size() == 0
     || le.getList()[0].isList()
     || le.getList()[0].getString() != ":action")
    ::error("action specification expected");

  vector<LispEntity> &vec = le.getList();

  if(vec.size() == 1 || vec[1].isList())
    ::error("invalid action: no name given");
  name = vec[1].getString();

  map<string, int> parameters;
  parameterCount = 0;
  int i = 2;
  if(i + 1 < vec.size() && vec[i].getString() == ":parameters") {
    vector<LispEntity> le = vec[i + 1].getList();
    for(int j = 0; j < le.size(); j++) {
      if(parameters.count(le[j].getString()))
        ::error("doubly defined parameter: " + le[j].getString());
      parameters[le[j].getString()] = j;
    }
    i += 2;
  }
  parameterCount = parameters.size();

  if(i + 1 < vec.size() && vec[i].getString() == ":precondition") {
    vector<LispEntity> le = vec[i + 1].getAndList();
    pre.reserve(le.size());
    for(int j = 0; j < le.size(); j++)
      pre.push_back(new SymbolicFact(this, parameters, d, le[j]));
    i += 2;
  }

  if(i + 1 < vec.size() && vec[i].getString() == ":effect") {
    vector<LispEntity> le = vec[i + 1].getAndList();
    for(int j = 0; j < le.size(); j++) {
      vector<LispEntity> l = le[j].getList();
      if(l.size() == 2 && l[0].getString() == "not")
        del.push_back(new SymbolicFact(this, parameters, d, l[1]));
      else
        add.push_back(new SymbolicFact(this, parameters, d, le[j]));
    }
    i += 2;
  }
  if(i < vec.size())
    ::error("invalid action parameter: " + vec[i].getString());
}

Action::~Action() {
  for(int i = 0; i < pre.size(); i++)
    delete pre[i];
  for(int i = 0; i < add.size(); i++)
    delete add[i];
  for(int i = 0; i < del.size(); i++)
    delete del[i];
}

string Action::toString() {
  string back = name + " (" + ::toString(parameterCount) + ")\n";
  for(int i = 0; i < pre.size(); i++)
    back += "pre: " + pre[i]->toString() + "\n";
  for(int i = 0; i < add.size(); i++)
    back += "add: " + add[i]->toString() + "\n";
  for(int i = 0; i < del.size(); i++)
    back += "del: " + del[i]->toString() + "\n";
  return back;
}

double Action::getMaxOperators(int objNo) const {
  return ::pow(objNo, parameterCount);
}

InstantiatedAction::InstantiatedAction(string action,
				       const vector<vector<int>::iterator> pars,
				       Domain &dom) {
  name = action;
  for(int i = 0; i < pars.size(); i++) {
    name += ' ';
    name += dom.getObject(*(pars[i])).getName();
  }
}

string InstantiatedAction::toString() const {
  return name;
}

string InstantiatedAction::getName() const {
  return name;
}

bool InstantiatedAction::operator<(const InstantiatedAction &other) const {
  return name < other.name;
}

bool InstantiatedAction::operator==(const InstantiatedAction &other) const {
  return name == other.name;
}
