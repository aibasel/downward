#ifndef SUCCESSOR_GENERATOR_H
#define SUCCESSOR_GENERATOR_H

#include <vector>
#include <fstream>
#include <map>
using namespace std;

class GeneratorBase;
class Operator;
class Variable;

class SuccessorGenerator {
  GeneratorBase *root;

  typedef vector<pair<Variable *, int> > Condition;
  GeneratorBase *construct_recursive(const vector<Variable *> &varOrder,
				     int switchVarNo,
				     vector<int> ops,
				     vector<Condition> &conds);
  SuccessorGenerator(const SuccessorGenerator &copy);
  // private copy constructor to forbid copying;
  // typical idiom for classes with non-trivial destructors
  map<const Operator *, int> op_to_index;
public:
  SuccessorGenerator();
  SuccessorGenerator(const vector<Variable *> &variables,
		     const vector<Operator> &operators);
  ~SuccessorGenerator();
  void dump() const;
  void generate_cpp_input(ofstream &outfile) const;
};

#endif
