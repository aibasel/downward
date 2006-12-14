#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include <iostream>

class Operator;
class State;

class SuccessorGenerator {
public:
  virtual void generate_applicable_ops(const State &curr,
				       vector<const Operator *> &ops) = 0;
  void dump() {_dump("  ");}
  virtual void _dump(string indent) = 0;
};

SuccessorGenerator *read_successor_generator(istream &in);

#endif
