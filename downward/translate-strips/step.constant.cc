#include "data.h"
#include "tools.h"
#include "options.h"

#include <iostream>
using namespace std;

void Domain::constantStep() {
  Timer t;
  if(!options.silent(Options::CONSTANT))
    cout << "Detecting constant predicates..." << flush;
  if(options.verbose(Options::CONSTANT))
    cout << endl;

  for(int i = 0; i < actions.size(); i++) {
    Action &a = *actions[i];
    vector<SymbolicFact *>::iterator pos;
    for(pos = a.getAddEffects().begin(); pos != a.getAddEffects().end(); ++pos)
      (*pos)->getPredicate().markSettable();
    for(pos = a.getDelEffects().begin(); pos != a.getDelEffects().end(); ++pos)
      (*pos)->getPredicate().markClearable();
  }

  int count = 0;
  for(int i = 0; i < predicates.size(); i++)
    if(predicates[i]->isConstant()) {
      count++;
      if(options.verbose(Options::CONSTANT))
	cout << "  " << predicates[i]->getName() << endl;
    }
  if(options.verbose(Options::CONSTANT))
    cout << "  Elapsed time: " << t.total() << endl << endl;
  else if(!options.silent(Options::CONSTANT))
    cout << " time: " << t.total() << "; "
	 << count << " found" << endl;
}


