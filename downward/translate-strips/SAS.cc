#include "SAS.h"

#include "data.h"        // InstantiatedAction
#include "step.coding.h" // SASEncoding

#include <cassert>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

SASState::SASState(const vector<int> &facts, const SASEncoding &code) {
  const int NOT_INITIALIZED = -1;
  values.resize(code.getVariableCount(), NOT_INITIALIZED);
  for(int i = 0; i < facts.size(); i++) {
    pair<int, int> var_val = code.lookupVariableValue(facts[i]);
    if(var_val.first != -1) {
      assert(values[var_val.first] == NOT_INITIALIZED);
      values[var_val.first] = var_val.second;
    }
  }
  for(int i = 0; i < values.size(); i++)
    if(values[i] == NOT_INITIALIZED)
      values[i] = code.getNoneValue(i);
}

const vector<int> &SASState::getValues() const {
  return values;
}

void SASState::dump(ostream &os) const {
  os << "begin_state" << endl;
  for(int i = 0; i < values.size(); i++)
    os << values[i] << endl;
  os << "end_state" << endl;
}

SASGoal::SASGoal(const vector<int> &facts, const SASEncoding &code) {
  map<int, int> tempState;
  for(int i = 0; i < facts.size(); i++) {
    pair<int, int> var_val = code.lookupVariableValue(facts[i]);
    if(var_val.first != -1) {
      assert(!tempState.count(var_val.first));
      tempState[var_val.first] = var_val.second;
    }
  }
  for(map<int, int>::iterator ptr = tempState.begin(), end = tempState.end();
      ptr != end; ++ptr)
    goals.push_back(Goal(ptr->first, ptr->second));
}

const vector<SASGoal::Goal> &SASGoal::getGoals() const {
  return goals;
}

void SASGoal::dump(ostream &os) const {
  os << "begin_goal" << endl
     << goals.size() << endl;
  for(int i = 0; i < goals.size(); i++)
    os << goals[i].var << " " << goals[i].val << endl;
  os << "end_goal" << endl;
}

SASOperator::SASOperator(const InstantiatedAction &inst, const SASEncoding &code) {
  name = inst.getName();
  
  map<int, int> varToPre, varToPost;
  for(int i = 0; i < inst.pre.size(); i++) {
    pair<int, int> var_val = code.lookupVariableValue(inst.pre[i]);
    if(var_val.first != -1)
      varToPre[var_val.first] = var_val.second;
  }
  // Important: Translate delete effects before add effects because
  // the latter typically override the assignments of the former.
  for(int i = 0; i < inst.del.size(); i++) {
    int var = code.lookupVariable(inst.del[i]);
    if(var != -1)
      varToPost[var] = code.getNoneValue(var);
  }
  for(int i = 0; i < inst.add.size(); i++) {
    pair<int, int> var_val = code.lookupVariableValue(inst.add[i]);
    if(var_val.first != -1)
      varToPost[var_val.first] = var_val.second;
  }
  
  map<int, int>::iterator curr_pre = varToPre.begin(), curr_post = varToPost.begin();
  map<int, int>::iterator last_pre = varToPre.end(), last_post = varToPost.end();
  while(curr_pre != last_pre || curr_post != last_post) {
    int diff;
    if(curr_pre == last_pre) {
      diff = +1;
    } else if(curr_post == last_post) {
      diff = -1;
    } else {
      diff = curr_pre->first - curr_post->first;
    }
    if(diff < 0) {
      prevail.push_back(Prevail(curr_pre->first, curr_pre->second));
      ++curr_pre;
    } else {
      int var = curr_post->first, post = curr_post->second;
      if(diff > 0) {
	trans.push_back(Transition(var, DONT_CARE, post));
	++curr_post;
      } else {
	trans.push_back(Transition(var, curr_pre->second, post));
	++curr_pre;
	++curr_post;
      }
    }
  }
}

const vector<SASOperator::Prevail> &SASOperator::getPrevail() const {
  return prevail;
}

const vector<SASOperator::Transition> &SASOperator::getTransitions() const {
  return trans;
}

void SASOperator::dump(ostream &os) const {
  os << "begin_operator" << endl
     << name << endl
     << prevail.size() << endl;
  for(int i = 0; i < prevail.size(); i++)
    os << prevail[i].var << " " << prevail[i].val << endl;
  os << trans.size() << endl;
  for(int i = 0; i < trans.size(); i++)
    os << "0 " << trans[i].var << " " << trans[i].pre << " " << trans[i].post << endl;
  os << "end_operator" << endl;
}

void SASTask::updateRange(int var, int val) {
  if(ranges[var] < val + 1)
    ranges[var] = val + 1;
}

SASTask::SASTask(const SASState &theInit, const SASGoal &theGoal)
  :init(theInit), goal(theGoal) {
  const vector<int> &initVals = theInit.getValues();
  for(int i = 0; i < initVals.size(); i++)
    ranges.push_back(initVals[i] + 1);
  const vector<SASGoal::Goal> &goals = theGoal.getGoals();
  for(int i = 0; i < goals.size(); i++)
    updateRange(goals[i].var, goals[i].val);
}

void SASTask::addOperator(const SASOperator &op) {
  operators.push_back(op);
  const vector<SASOperator::Prevail> &prev = op.getPrevail();
  for(int i = 0; i < prev.size(); i++)
    updateRange(prev[i].var, prev[i].val);
  const vector<SASOperator::Transition> &trans = op.getTransitions();
  for(int i = 0; i < trans.size(); i++)
    updateRange(trans[i].var, max(trans[i].pre, trans[i].post));
}

void SASTask::dump(ostream &os) const {
  os << "begin_variables" << endl
     << ranges.size() << endl;
  for(int i = 0; i < ranges.size(); i++)
    os << "var" << i << " " << ranges[i] << " -1" << endl;
  os << "end_variables" << endl;
  init.dump(os);
  goal.dump(os);
  os << operators.size() << endl;
  for(int i = 0; i < operators.size(); i++)
    operators[i].dump(os);
  os << "0" << endl;
}
