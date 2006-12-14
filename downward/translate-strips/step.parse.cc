#include "data.h"
#include "tools.h"
#include "options.h"
#include "parser.h"

void check(bool condition, string msg) {
  if(!condition)
    ::error(msg);
}

void Domain::parsingStep(string domFile, string probFile) {
  Timer t;
  if(!options.silent(Options::PARSING))
    cout << "Parsing..." << flush;
  if(options.verbose(Options::PARSING))
    cout << endl;

  LispParser parser;
  if(options.debug(Options::PARSING))
    cout << "  Reading domain file..." << endl;
  vector<LispEntity> domain = parser.parseFile(domFile).getList();
  if(options.debug(Options::PARSING))
    cout << "  Reading problem file..." << endl;
  vector<LispEntity> problem = parser.parseFile(probFile).getList();
  
  // parse domain file
  if(options.debug(Options::PARSING))
    cout << endl << "  Checking domain file header..." << endl;
  check(domain.size() >= 2
	&& domain[0].isString()
	&& domain[0].getString() == "define",
	"invalid domain file: define clause missing or incomplete");
  check(domain[1].isList()
	&& domain[1].getList().size() == 2
	&& domain[1].getList()[0].isString()
	&& domain[1].getList()[1].isString()
	&& domain[1].getList()[0].getString() == "domain",
	"domain name specification missing or invalid");
  domainName = domain[1].getList()[1].getString();
  vector<LispEntity>::iterator pos = domain.begin() + 2;
  if(pos != domain.end()
     && pos->isList()
     && pos->getList().size() != 0
     && pos->getList()[0].isString()
     && pos->getList()[0].getString() == ":requirements")
    ++pos; // skip requirements
  if(options.debug(Options::PARSING))
    cout << "  Reading predicates..." << endl;
  check(pos != domain.end()
	&& pos->isList()
	&& pos->getList().size() > 0
	&& pos->getList()[0].isString()
	&& pos->getList()[0].getString() == ":predicates",
	"predicate specification missing or invalid");
  vector<LispEntity> predList(pos->getList().begin() + 1, pos->getList().end());
  ++pos;
  
  if(options.debug(Options::PARSING))
    cout << "  Reading actions..." << endl;
  vector<LispEntity> actionList(pos, domain.end());
  
  // parse problem file
  if(options.debug(Options::PARSING))
    cout << "  Checking problem file header..." << endl;
  check(problem.size() >= 1
	&& problem[0].isString()
	&& problem[0].getString() == "define",
	"invalid problem file: define clause missing or incomplete");
  check(problem.size() >= 2
	&& problem[1].isList()
	&& problem[1].getList().size() == 2
	&& problem[1].getList()[0].isString()
	&& problem[1].getList()[1].isString()
	&& problem[1].getList()[0].getString() == "problem",
	"problem name specification missing or invalid");
  problemName = problem[1].getList()[1].getString();
  check(problem.size() >= 3
	&& problem[2].isList()
	&& problem[2].getList().size() == 2
	&& problem[2].getList()[0].isString()
	&& problem[2].getList()[1].isString()
	&& problem[2].getList()[0].getString() == ":domain"
	&& problem[2].getList()[1].getString() == domainName,
	"domain specification in problem file is missing or does not match");
  
  if(options.debug(Options::PARSING))
    cout << "  Reading objects..." << endl;
  check(problem.size() >= 4
	&& problem[3].isList()
	&& problem[3].getList().size() > 0
	&& problem[3].getList()[0].isString()
	&& problem[3].getList()[0].getString() == ":objects",
	"object specification missing or invalid");
  vector<LispEntity> objList(problem[3].getList().begin() + 1,
			     problem[3].getList().end());

  if(options.debug(Options::PARSING))
    cout << "  Reading initial state..." << endl;
  check(problem.size() >= 5
	&& problem[4].isList()
	&& problem[4].getList().size() >= 1
	&& problem[4].getList()[0].isString()
	&& problem[4].getList()[0].getString() == ":init",
	"initial state specification missing or invalid");
  vector<LispEntity> initList(problem[4].getList().begin() + 1,
			      problem[4].getList().end());
  
  if(options.debug(Options::PARSING))
    cout << "  Reading goal state..." << endl;
  check(problem.size() >= 6
	&& problem[5].isList()
	&& problem[5].getList().size() >= 2
	&& problem[5].getList()[0].isString()
	&& problem[5].getList()[0].getString() == ":goal",
	"goal state specification missing or invalid");
  vector<LispEntity> goalList = problem[5].getList()[1].getAndList();
  check(problem.size() == 6,
	"problem file does not end with goal fact list");
  
  if(options.debug(Options::PARSING))
    cout << "  Parsing objects..." << endl;
  for(int i = 0; i < objList.size(); i++) {
    Object o(i, objList[i]);
    check(objectTable.count(o.getName()) == 0,
	  "doubly defined object " + o.getName());
    objectTable[o.getName()] = i;
    objects.push_back(o);
  }
  
  if(options.debug(Options::PARSING))
    cout << "  Parsing predicates..." << endl;
  factCount = 0;
  for(int i = 0; i < predList.size(); i++) {
    Predicate *p = new Predicate(predList[i], factCount);
    check(predicateTable.count(p->getName()) == 0,
	  "doubly defined predicate " + p->getName());
    predicateTable[p->getName()] = i;
    predicates.push_back(p);
    factCount = p->getFactUpperBound(objects.size());
  }
  
  if(options.debug(Options::PARSING))
    cout << "  Parsing actions..." << endl;
  for(int i = 0; i < actionList.size(); i++)
    actions.push_back(new Action(*this, actionList[i]));
  
  if(options.debug(Options::PARSING))
    cout << "  Parsing initial state..." << endl;
  for(int i = 0; i < initList.size(); i++)
    initFacts.push_back(Fact(*this, initList[i]).toInt());
  
  if(options.debug(Options::PARSING))
    cout << "  Parsing goal state..." << endl << endl;
  for(int i = 0; i < goalList.size(); i++)
  goalFacts.push_back(Fact(*this, goalList[i]).toInt());
  
  if(options.verbose(Options::PARSING)) {
    cout << toString() << endl;
    cout << "  Elapsed time: " << t.total() << endl << endl;
  } else if(!options.silent(Options::PARSING)) {
    cout << " time: " << t.total() << "; " << factCount << " facts, "
	 << getOperatorCount() << " operators"  << endl;
  }
}







