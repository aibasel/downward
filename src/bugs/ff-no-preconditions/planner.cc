#include "enforced_hill_climbing.h"
#include "best_first_search.h"
#include "iterative_search.h"
#include "deadend.h"
#include "globals.h"
// #include "multi_bfs.h"
#include "operator.h"
#include "scheduler.h"

#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include <sys/times.h>

void save_plan(const vector<const Operator *> &plan);

int main(int argc, const char **argv) {
  struct tms start, search_start, search_end;
  times(&start);
  bool poly_time_method = false;

  bool use_climber = false;

  for(int i = 1; i < argc; i++) {
    for(const char *c = argv[i]; *c != 0; c++) {
      if(*c == 'd') {
	g_downward_helpful_actions = true;
      } else if(*c == 'f') {
	g_ff_helpful_actions = true;
      } else if(*c == 'x') {
	g_mixed_helpful_actions = true;
      } else if(*c == 'm') {
	g_multi_heuristic = true;
      } else if(*c == 'c') {
	use_climber = true;
      } else if(*c >= 'a' && *c <= 'z') {
	cerr << "Unknown option: " << *c << endl;
	return 1;
      }
    }
  }

  cin >> poly_time_method;
  if(poly_time_method) {
    cout << "Poly-time method not implemented in this branch." << endl;
    cout << "Starting normal solver." << endl;
    // cout << "Aborting." << endl;
    // return 1;
  }

  read_everything(cin);
  // dump_everything();

  int expanded, generated;

  Scheduler scheduler;
  int count = 0;
  if(!use_climber || g_downward_helpful_actions) {
    scheduler.add_search_engine(new BestFirstSearchEngine);
    count++;
  }
  if(use_climber) {
    scheduler.add_search_engine(new IterativeSearchEngine(2000));
    count++;
  }
  if(count == 2) {
    scheduler.add_time_limit(1);
    scheduler.add_time_limit(15);
    scheduler.add_time_limit(60);
    scheduler.add_time_limit(600);
    scheduler.add_time_limit(2000);
  } else {
    scheduler.add_time_limit(2000);
  }
  times(&search_start);
  scheduler.search();
  times(&search_end);
  if(scheduler.found_solution())
    save_plan(scheduler.get_plan());
  expanded = scheduler.get_expanded_states();
  generated = scheduler.get_generated_states();

  /*
    BestFirstSearchEngine *engine = new BestFirstSearchEngine;
    times(&search_start);
    engine->search();
    times(&search_end);
    if(engine->found_solution())
      save_plan(engine->get_plan());
    expanded = engine->get_expanded_states();
    generated = engine->get_generated_states();
  */

  cout << "Cache hits: " << g_cache_hits << endl;
  cout << "Cache misses: " << g_cache_misses << endl;
  cout << "Expanded " << expanded << " state(s)." << endl;
  cout << "Generated " << generated << " state(s)." << endl;
  int search_ms = (search_end.tms_utime - search_start.tms_utime) * 10;
  cout << "Search time: " << search_ms / 1000.0 << " seconds" << endl;
  int total_ms = (search_end.tms_utime - start.tms_utime) * 10;
  cout << "Total time: " << total_ms / 1000.0 << " seconds" << endl;

  /*
  struct tms reach_start, reach_end;
  times(&reach_start);
  cout << "Starting reachability test..." << endl;
  bool goal_reachable = DeadEndTest().is_reachable();
  cout << "Goal reachable: " << goal_reachable << endl;
  times(&reach_end);
  int search_ms = (reach_end.tms_utime - reach_start.tms_utime) * 10;
  cout << "Time for test: " << search_ms / 1000.0 << " seconds" << endl;
  */

  return scheduler.found_solution() ? 0 : 1;
}

void save_plan(const vector<const Operator *> &plan) {
  ofstream outfile;
  outfile.open("sas_plan", ios::out);
  for(int i = 0; i < plan.size(); i++) {
    cout << plan[i]->get_name() << endl;
    outfile << "(" << plan[i]->get_name() << ")" << endl;
  }
  outfile.close();
  cout << "Plan length: " << plan.size() << " step(s)." << endl;
}
