#include "data.h"
#include "options.h"
#include "tools.h"

#include <algorithm>    // sort, max_element
#include <iostream>
using namespace std;

void Domain::exploreStep() {
  Timer t;
  if(!options.silent(Options::EXPLORE))
    cout << "Exploring fact space..." << flush;
  if(options.verbose(Options::EXPLORE))
    cout << endl;
  ExploreStep expl(predicates, actions, objects.size(), factCount,
                   initFacts, *this);
  
  expl.getConstantFacts().toIntVector(trueFacts);
  expl.getFluentFacts().toIntVector(fluentFacts);

  if(options.verbose(Options::EXPLORE)) {
    cout << "  true facts:" << endl;
    for(int i = 0; i < trueFacts.size(); i++)
      cout << "    " << Fact(trueFacts[i]).toString(*this) << endl;
    cout << "  fluent facts:" << endl;
    for(int i = 0; i < fluentFacts.size(); i++)
      cout << "    " << Fact(fluentFacts[i]).toString(*this) << endl;
    cout << "  Elapsed time: " << t.total() << endl << "  "
	 << expl.getInstantiationCount() << " operators instantiated"
	 << endl << endl;
  } else if(!options.silent(Options::EXPLORE)) {
    cout << " time: " << t.total() << "; "
	 << trueFacts.size() << " true facts; "
	 << fluentFacts.size() << " fluents; "
	 << expl.getInstantiationCount() << " operators" << endl;
  }
}

void ExploreStep::initPredicateData(vector<Predicate *> &predicates,
                                    vector<Action *> &actions) {
  BitArray proj(objectCount);
  for(int i = 0; i < predicates.size(); i++)
    predicates[i]->projections.resize(predicates[i]->getParameterCount(), proj);
  for(int i = 0; i < actions.size(); i++) {
    vector<SymbolicFact *> &pre = actions[i]->getPreconditions();
    for(int j = 0; j < pre.size(); j++)
      pre[j]->getPredicate().preconditions.push_back(pre[j]);
  }
}

void ExploreStep::initActionData(vector<Action *> &actions) {
  for(int i = 0; i < actions.size(); i++) {
    Action &a = *actions[i];
    int parNo = a.getParameterCount();
    a.preByMaxPar.resize(parNo + 1);
    a.validArguments.resize(parNo);
    a.preconditionCount.resize(parNo);
    vector<int> preCount;
    preCount.resize(parNo, 0);
    vector<SymbolicFact *> &pre = a.getPreconditions();
    for(int j = 0; j < pre.size(); j++) {
      const vector<int> &arg = pre[j]->getArguments();
      if(arg.size() != 1) {   // also register if count is zero!
        int maxArgNo = arg.empty() ? -1 : *max_element(arg.begin(), arg.end());
        a.preByMaxPar[maxArgNo + 1].push_back(pre[j]);
      }
      for(int j = 0; j < arg.size(); j++)
        preCount[arg[j]]++;
    }
    for(int j = 0; j < parNo; j++)
      a.preconditionCount[j].resize(objectCount, preCount[j]);
  }
}

ExploreStep::ExploreStep(vector<Predicate *> &predicates,
      vector<Action *> &actions, int objCount, int factCount,
      vector<int> &init, Domain &dom)
  : domain(dom), objectCount(objCount), instantiationCount(0) {
  // setup predicate and action data
  initPredicateData(predicates, actions);
  initActionData(actions);
  
  // setup fact queue
  sort(init.begin(), init.end());
  vector<Predicate *>::iterator aktPred = predicates.begin(), end = predicates.end();
  for(int i = 0; i < init.size(); i++) {
    while(aktPred + 1 != end && init[i] >= (*(aktPred + 1))->getFactLowerBound())
      ++aktPred;
    queue.push_back(make_pair(*aktPred, init[i]));
  }

  // setup fact markers
  constantFacts = processedFacts = BitArray(factCount);
  for(int i = 0; i < init.size(); i++)
    constantFacts.set(init[i]);
  enqueuedFacts = constantFacts;

  // instantiate actions without preconditions
  fireEmptyPreconditions(actions);

  // process queue
  while(!queue.empty()) {
    Predicate &pred = *queue.front().first;
    int factId = queue.front().second;
    if(options.debug(Options::EXPLORE))
      cout << "  queue: " << queue.size() << " elements; "
	"processed fact " << Fact(factId).toString(domain) << endl;
    processedFacts.set(factId);
    exploreFact(pred, factId);
    queue.pop_front();
  }
  if(options.debug(Options::EXPLORE))
    cout << endl;

  processedFacts ^= constantFacts;    // clear constant facts from fluents
}

void ExploreStep::fireEmptyPreconditions(vector<Action *> &actions) {
  vector<int> allObjects = ::fromTo(0, objectCount);
  vector<vector<int> *> argLists;
  
  for(int i = 0; i < actions.size(); i++)
    if(actions[i]->getPreconditions().empty()) {
      argLists.resize(actions[i]->getParameterCount(), &allObjects);
      instantiate(*actions[i], argLists);
    }
}

void ExploreStep::exploreFact(Predicate &pred, int factId) {
  int id = factId - pred.getFactLowerBound();
  vector<vector<int> *> argLists; // more efficient than having it in the inner loop
  for(int i = pred.getParameterCount() - 1; i >= 0; i--) {
    int obj = id % objectCount;
    id /= objectCount;
    bool oldProj = pred.getProjection(i, obj);
    pred.setProjection(i, obj);

    vector<SymbolicFact *> &vec = pred.getPreconditions();
    vector<SymbolicFact *>::iterator pos, end = vec.end();
    for(pos = vec.begin(); pos != end; ++pos) {
      SymbolicFact &pre = **pos;
      const vector<int> &arg = pre.getArguments();
      if(!oldProj && pre.getAction().decreasePreconditionCountdown(arg[i], obj)
	 || oldProj && pre.getAction().isValidArgument(arg[i], obj)) {
	if(prepareArgLists(pre, argLists, factId)) {
	  instantiate(pre.getAction(), argLists);
	  for(int i = 0; i < arg.size(); i++)
	    delete argLists[arg[i]];
	}
      }
    }
  }
  if(pred.getParameterCount() == 0) // degenerate case
    for(int i = 0; i < pred.getPreconditions().size(); i++)
      if(prepareArgLists(*pred.getPreconditions()[i], argLists, factId))
	instantiate(pred.getPreconditions()[i]->getAction(), argLists);
}

bool ExploreStep::prepareArgLists(SymbolicFact &pre, vector<vector<int> *> &argLists, int fact) {
  const vector<int> &args = pre.getArguments();
  Action &a = pre.getAction();
  int actionArg = a.getParameterCount();

  argLists.clear();
  argLists.reserve(actionArg);
  for(int i = 0; i < actionArg; i++) {
    vector<int> &validArgs = a.getValidArguments(i);
    if(validArgs.empty())
      return false;
    argLists.push_back(&validArgs);
  }
  
  fact -= pre.getPredicate().getFactLowerBound();
  for(int i = args.size() - 1; i >= 0; i--) {
    int pos = fact % objectCount;
    fact /= objectCount;
    if(a.isValidArgument(args[i], pos)) {
      argLists[args[i]] = new vector<int>(1, pos);
    } else { // no valid instantiations, clean up
      while(++i < args.size())
        delete argLists[args[i]];
      return false;
    }
  }
  return true;
}

void ExploreStep::instantiate(Action &a, vector<vector<int> *> &argLists) {
  typedef vector<int>::iterator Iterator;
  vector<Iterator> pos(argLists.size());
  int argNo = -1, argCount = a.getParameterCount();
  for(;;) {
    while(satisfied(a, pos, ++argNo)) {
      if(argNo == argCount) { // everything satisfied, instantiate
        handleEffects(a, pos);
        break;
      } else {
        pos[argNo] = argLists[argNo]->begin();
      }
    }
    do { // advance iterator
      if(--argNo < 0)
        return;
      ++(pos[argNo]);
    } while(pos[argNo] == argLists[argNo]->end());
  }
}

bool ExploreStep::satisfied(Action &a,
    const vector<vector<int>::iterator> &parameters, int argBound) {
  const vector<SymbolicFact *> &pre = a.getPreconditionsByMaxPar(argBound);
  vector<SymbolicFact *>::const_iterator pos, end = pre.end();
  for(pos = pre.begin(); pos != end; ++pos)
    if(!processedFacts.get((*pos)->instantiateFact(parameters, objectCount)))
      return false;
  return true;
}

void ExploreStep::handleEffects(Action &a,
          const vector<vector<int>::iterator> &parameters) {
  InstantiatedAction inst(a.getName(), parameters, domain);
  inst.pre.reserve(a.getPreconditions().size());
  inst.add.reserve(a.getAddEffects().size());
  inst.del.reserve(a.getAddEffects().size());

  vector<SymbolicFact *>::iterator pos, end;
  for(pos = a.getPreconditions().begin(), end = a.getPreconditions().end(); pos != end; ++pos)
    if(!(*pos)->getPredicate().isConstant())
      inst.pre.push_back((*pos)->instantiateFact(parameters, objectCount));
  for(pos = a.getAddEffects().begin(), end = a.getAddEffects().end(); pos != end; ++pos) {
    int factId = (*pos)->instantiateFact(parameters, objectCount);
    inst.add.push_back(factId);
    if(!enqueuedFacts.get(factId)) {
      enqueuedFacts.set(factId);
      queue.push_back(make_pair(&(*pos)->getPredicate(), factId));
    }
  }  
  for(pos = a.getDelEffects().begin(), end = a.getDelEffects().end(); pos != end; ++pos) {
    int factId = (*pos)->instantiateFact(parameters, objectCount);
    inst.del.push_back(factId);
    constantFacts.clear(factId);
  }
  
  // (Malte 2002) The following is a bit of a hack; hopefully it doesn't
  // have too much of an impact on overall performance (this used to be
  // one of the critical loops). Instantiated actions are "normalized"
  // here by sorting their precondition, add and delete lists. This
  // serves two purposes:
  // (1) It allows checking for conflicting add and delete lists more
  //     quickly (below).
  // (2) It allows applying the "unique" algorithm on instantiations
  //     (just in case the "duplicated instantiation bug" survived).
  sort(inst.pre.begin(), inst.pre.end());
  sort(inst.add.begin(), inst.add.end());
  sort(inst.del.begin(), inst.del.end());

  int i = 0, j = 0;
  while(i < inst.add.size() && j < inst.del.size()) {
    if(inst.add[i] < inst.del[j])
      i++;
    else if(inst.add[i] > inst.del[j])
      j++;
    else
      return; // invalid operator
  }

  // record this operator in the action log
  a.logInstantiation(inst);
  instantiationCount++;
}

