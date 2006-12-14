#include "outputGenerator.h"

#include "data.h"
#include "options.h"
#include "SAS.h"
#include "step.coding.h"

#include <algorithm>
#include <fstream>
#include <vector>
using namespace std;

OutputGenerator::OutputGenerator(Domain &theDomain, const SASEncoding &theCode)
  : domain(theDomain), code(theCode) {
  // HACK: Side effect: Sorts instantiations (doesn't belong here).

  if(!options.silent(Options::TRANSITION))
    cout << "Creating SAS output... " << flush;

  SASState init(domain.getInitFacts(), code);
  SASGoal goal(domain.getGoalFacts(), code);
  SASTask task(init, goal);

  int count = domain.getActionCount();
  for(int i = 0; i < count; i++) {
    vector<InstantiatedAction> &inst = domain.getAction(i).getInstantiations();
    if(!inst.empty()) {
      sort(inst.begin(), inst.end());
      inst.erase(unique(inst.begin(), inst.end()), inst.end());
      for(int j = 0; j < inst.size(); j++)
	task.addOperator(SASOperator(inst[j], code));
    }
  }

  ofstream file("output.sas");
  task.dump(file);
  file.close();
  
  if(!options.silent(Options::TRANSITION))
    cout << "done!" << endl;
}

