#ifndef SAS_H
#define SAS_H

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class InstantiatedAction;
class SASEncoding;

class SASState {
  vector<int> values;
public:
  SASState(const vector<int> &facts, const SASEncoding &code);
  const vector<int> &getValues() const;
  void dump(ostream &os) const;
};

class SASGoal {
public:
  struct Goal {
    int var, val;
    Goal(int theVar, int theVal) : var(theVar), val(theVal) {}
  };
private:
  vector<Goal> goals;
public:
  SASGoal(const vector<int> &facts, const SASEncoding &code);
  const vector<Goal> &getGoals() const;
  void dump(ostream &os) const;
};

class SASOperator {
public:
  struct Prevail {
    Prevail(int theVar, int theVal)
      : var(theVar), val(theVal) {}
    int var, val;
  };
  struct Transition {
    Transition(int theVar, int thePre, int thePost)
      : var(theVar), pre(thePre), post(thePost) {}
    int var, pre, post;
  };
private:
  enum {DONT_CARE = -1};
  string name;
  vector<Prevail> prevail;
  vector<Transition> trans;
public:
  SASOperator(const InstantiatedAction &inst, const SASEncoding &code);
  const vector<Prevail> &getPrevail() const;
  const vector<Transition> &getTransitions() const;
  void dump(ostream &os) const;
};

class SASTask {
  vector<int> ranges;
  SASState init;
  SASGoal goal;
  vector<SASOperator> operators;
  void updateRange(int var, int val);
public:
  SASTask(const SASState &init, const SASGoal &goal);
  void addOperator(const SASOperator &op);
  void dump(ostream &os) const;
};

#endif
