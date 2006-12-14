#include "data.h"

#include "tools.h"
#include "options.h"
#include "outputGenerator.h"
#include "step.coding.h"

Domain *g_domain = 0; // HACK!

Domain::Domain(string domFile, string probFile) {
  g_domain = this; // HACK!
  try {
    Timer t;

    cout << "domain file:  " << domFile << endl
	 << "problem file: " << probFile << endl;

    if(!options.allSilent())
      cout << endl << "Preprocessing..." << endl;
    parsingStep(domFile, probFile);
    constantStep();
    vector<MergedPredicate> merged = mergeStep();
    exploreStep();
    SASEncoding code(codingStep(merged));
    cout << "Total preprocessing time: " << t.stop() << endl;

    if(!options.allSilent())
      cout << endl << "Writing output..." << endl;
    OutputGenerator generator(*this, code);
    cout << "Total running time: " << t.total() << endl << endl;
  } catch(StringException &e) {
    cerr << "Error: " << e.toString() << endl;
  }
}

Domain::~Domain() {
  for(int i = 0; i < predicates.size(); i++)
    delete predicates[i];
  for(int i = 0; i < actions.size(); i++)
    delete actions[i];
}

const Object &Domain::lookupObject(string str) const {
  if(objectTable.count(str) == 0)
    ::error("unknown object: " + str);
  return objects[objectTable.find(str)->second];
}

Predicate &Domain::lookupPredicate(string str) {
  if(predicateTable.count(str) == 0)
    ::error("unknown predicate: " + str);
  return *predicates[predicateTable[str]];
}

const Predicate &Domain::lookupPredicate(string str) const {
  if(predicateTable.count(str) == 0)
    ::error("unknown predicate: " + str);
  return *predicates[predicateTable.find(str)->second];
}

double Domain::getOperatorCount() const {
  double back = 0.0;
  for(int i = 0; i < actions.size(); i++)
    back += actions[i]->getMaxOperators(objects.size());
  return back;
}

string Domain::toString() {
  string back =
    "domain:         " + problemName + "\n"
    "problem:        " + domainName + "\n"
    "object count:   " + ::toString(objects.size()) + "\n"
    "fact count:     " + ::toString(factCount) + "\n"
    "operator count: " + ::toString(getOperatorCount()) + "\n"
    "predicates:\n";
  for(int i = 0; i < predicates.size(); i++)
    back += "[" + ::toString(i) + "] " + predicates[i]->toString() + "\n";
  back += "\nactions:\n";
  for(int i = 0; i < actions.size(); i++)
    back += actions[i]->toString() + "\n";
  back += "objects:\n";
  for(int i = 0; i < objects.size(); i++)
    back += "[" + ::toString(i) + "] " + objects[i].getName() + "\n";
  back += "\ninitial state:\n";
  for(int i = 0; i < initFacts.size(); i++)
    back += "   " + Fact(initFacts[i]).toString(*this) + "\n";
  back += "\ngoal state:\n";
  for(int i = 0; i < goalFacts.size(); i++)
    back += "   " + Fact(goalFacts[i]).toString(*this) + "\n";
  return back;
}

// Problems:
// * There must be at least one object for this to work.
// * Fact identifiers aren't checked against overflow.
