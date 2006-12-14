#ifndef OPERATOR_H
#define OPERATOR_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "state.h"

class Variable;

struct Prevail {
  int var;
  int prev;
  Prevail(istream &in);
  Prevail(int v, int p) : var(v), prev(p) {}

  bool is_applicable(const State &state) const {
    assert(var >= 0 && var < g_variable_name.size());
    assert(prev >= 0 && prev < g_variable_domain[var]);
    return state[var] == prev;
  }

  void dump() const;
};

struct PrePost {
  int var;
  int pre, post;
  vector<Prevail> cond;
  PrePost() {} // Needed for axiom file-reading constructor, unfortunately.
  PrePost(istream &in);
  PrePost(int v, int pr, int po, const vector<Prevail> &co)
    : var(v), pre(pr), post(po), cond(co) {}

  bool is_applicable(const State &state) const {
    assert(var >= 0 && var < g_variable_name.size());
    assert(pre == -1 || pre >= 0 && pre < g_variable_domain[var]);
    return pre == -1 || state[var] == pre;
  }

  bool does_fire(const State &state) const {
    for(int i = 0; i < cond.size(); i++)
      if(!cond[i].is_applicable(state))
	return false;
    return true;
  }

  void dump() const;
};

class Operator {
  bool is_an_axiom;
  vector<Prevail> prevail;      // var, val
  vector<PrePost> pre_post;     // var, old-val, new-val, effect conditions
  string name;
public:
  Operator(istream &in, bool is_axiom);
  void dump() const;
  string get_name() const {return name;}

  bool is_axiom() const {return is_an_axiom;}

  const vector<Prevail> &get_prevail() const {return prevail;}
  const vector<PrePost> &get_pre_post() const {return pre_post;}

  bool is_applicable(const State &state) const {
    for(int i = 0; i < prevail.size(); i++)
      if(!prevail[i].is_applicable(state))
	return false;
    for(int i = 0; i < pre_post.size(); i++)
      if(!pre_post[i].is_applicable(state))
	return false;
    return true;
  }
};

#endif
