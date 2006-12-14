#include "data.h"
#include "parser.h"
#include "tools.h"

SymbolicFact::SymbolicFact(Action *a, map<string, int> &parTable,
                           Domain &d, LispEntity &le)
      : action(a) {
  vector<LispEntity> &vec = le.getList();
  if(vec.size() == 0)
    ::error("invalid symbolic fact: no predicate given");
  predicate = &d.lookupPredicate(vec[0].getString());

  arguments.reserve(vec.size() - 1);
  for(int i = 1; i < vec.size(); i++) {
    if(!parTable.count(vec[i].getString()))
      ::error("invalid symbolic fact: illegal parameter"
              + vec[i].getString());
    arguments.push_back(parTable[vec[i].getString()]);
  }
}

SymbolicFact::~SymbolicFact() {
}

string SymbolicFact::toString() {
  string back = predicate->getName();
  for(int i = 0; i < arguments.size(); i++)
    back = back + " ?" + (char)('a' + arguments[i]);
  return back;
}

bool SymbolicFact::existsMatchingPrecondition() {
  vector<SymbolicFact *> &pre = action->getPreconditions();
  for(int i = 0; i < pre.size(); i++)
    if(predicate == pre[i]->predicate && arguments == pre[i]->arguments)
      return true;
  return false;
}

vector<vector<int> > SymbolicFact::getMatchingParOrders(const vector<int> &srcArgList) {
  vector<vector<int> > back;

  if(arguments.size() != srcArgList.size() && arguments.size() != srcArgList.size() - 1)
    return back;   // wrong argument count: no possible match

  int maxArgNo = action->getParameterCount();
  
  typedef vector<vector<int> > Match;
  Match sourceArgs;
  Match matchArgs;
  sourceArgs.resize(maxArgNo);
  matchArgs.resize(maxArgNo);

  for(int i = 1; i < srcArgList.size(); i++)   // skip value field
    sourceArgs[srcArgList[i]].push_back(i);
  for(int i = 0; i < arguments.size(); i++)
    matchArgs[arguments[i]].push_back(i);

  for(int i = 0; i < maxArgNo; i++)
    if(matchArgs[i].size() < sourceArgs[i].size())
      return back;    // no match
    else if(matchArgs[i].size() > sourceArgs[i].size())  // this is our value field
      sourceArgs[i].push_back(0);
  
  vector<Match> matches;
  matches.push_back(Match());
  for(int i = 0; i < maxArgNo; i++) {
    vector<vector<int> > perms = ::getPerms(matchArgs[i]);
    vector<Match> tempMatches = matches;
    matches.clear();
    matches.reserve(tempMatches.size() * perms.size());
    for(int j = 0; j < tempMatches.size(); j++)
      for(int k = 0; k < perms.size(); k++) {
	      Match m = tempMatches[j];
        m.push_back(perms[k]);
        matches.push_back(m);
      }
  }
  
  back.reserve(matches.size());
  vector<int> parOrder;
  parOrder.resize(srcArgList.size());
  for(int i = 0; i < matches.size(); i++) {
    parOrder[0] = -1;         // only necessary for null state predicates
    for(int j = 0; j < maxArgNo; j++)
      for(int k = 0; k < sourceArgs[j].size(); k++)
        parOrder[sourceArgs[j][k]] = matches[i][j][k];
    back.push_back(parOrder);
  }
  return back;
}							

