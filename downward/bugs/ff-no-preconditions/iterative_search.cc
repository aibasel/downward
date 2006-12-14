#include "best_first_search.h"
#include "iterative_search.h"
#include "globals.h"
#include "operator.h"
#include "successor_generator.h"

#include <cassert>
#include <climits>
#include <list>
#include <set>
#include <vector>
using namespace std;

static vector<vector<int> > g_causal_graph_goal_distances;
static int g_causal_graph_goal_diameter;

void register_action(vector<set<int> > &successors, const Operator &op) {
  const vector<PrePost> &pre_post = op.get_pre_post();
  const vector<Prevail> &prevail = op.get_prevail();
  for(int i = 0; i < pre_post.size(); i++) {
    int var = pre_post[i].var;
    // Prevail conditions.
    for(int j = 0; j < prevail.size(); j++)
      successors[var].insert(prevail[j].var);
    // Effect conditions of conditional effects.
    const vector<Prevail> &eff_condition = pre_post[i].cond;
    for(int j = 0; j < eff_condition.size(); j++)
      successors[var].insert(eff_condition[j].var);
    // Preconditions for other effects.
    for(int j = 0; j < pre_post.size(); j++)
      if(pre_post[j].var != var && pre_post[j].pre != -1)
	successors[var].insert(pre_post[j].var);
  }
}

void compute_causal_graph(vector<vector<int> > &graph) {
  // Compute graph with arc from var. u to var. v iff there
  // exists an operator with precondition on u and effect on v
  // or axiom with condition on u and effect on v.
  // (Different to the ordinary causal graph in that co-occurring
  // effects are not considered.)

  // Graph is represented as vector of vector of successors.

  int var_count = g_variable_domain.size();
  vector<set<int> > successors(var_count);
  for(int i = 0; i < g_operators.size(); i++)
    register_action(successors, g_operators[i]);
  for(int i = 0; i < g_axioms.size(); i++)
    register_action(successors, g_axioms[i]);

  assert(graph.empty());
  for(int i = 0; i < var_count; i++)
    graph.push_back(vector<int>(successors[i].begin(), successors[i].end()));

}

void compute_distances(const vector<vector<int> > &graph, int source_node,
		       vector<int> &distances) {
  // Dijkstra algorithm: Compute distances in directed unweighted graph <graph>
  // from <source_node> to all other nodes and store them in <distances>.
  // Infinite path costs are stored as 
  assert(distances.empty() && source_node >= 0 && source_node < graph.size());
  int var_count = g_variable_domain.size();
  const int unreachable = var_count;
  distances.resize(var_count, unreachable);
  vector<bool> is_vertex_reached(var_count, false);
  vector<vector<int> > buckets(unreachable);
  buckets[0].push_back(source_node);
  for(int current_distance = 0; current_distance < unreachable; current_distance++) {
    vector<int> &current_bucket = buckets[current_distance];
    while(!current_bucket.empty()) {
      int reached_vertex = current_bucket.back();
      current_bucket.pop_back();
      if(!is_vertex_reached[reached_vertex]) {
	is_vertex_reached[reached_vertex] = true;
	distances[reached_vertex] = current_distance;
	if(current_distance + 1 < unreachable)
	  buckets[current_distance + 1].insert(buckets[current_distance + 1].end(),
					       graph[reached_vertex].begin(),
					       graph[reached_vertex].end());
      }
    }
  }
}

void compute_causal_graph_goal_distances() {
  if(!g_causal_graph_goal_distances.empty())
    return;
  vector<vector<int> > causal_graph;
  compute_causal_graph(causal_graph);
  g_causal_graph_goal_distances.resize(g_goal.size());
  g_causal_graph_goal_diameter = 0;
  for(int i = 0; i < g_goal.size(); i++) {
    vector<int> &distances = g_causal_graph_goal_distances[i];
    compute_distances(causal_graph, g_goal[i].first, distances);
    const int unreachable = distances.size();
    for(int j = 0; j < distances.size(); j++)
      if(distances[j] != unreachable)
	g_causal_graph_goal_diameter = max(g_causal_graph_goal_diameter, distances[j]);
  }
}

int get_action_cost(const Operator *action, int goal_index) {
  const vector<PrePost> &pre_post = action->get_pre_post();
  const vector<int> &goal_distances = g_causal_graph_goal_distances[goal_index];
  assert(!pre_post.empty());
  int cost = goal_distances[pre_post[0].var];
  for(int i = 1; i < pre_post.size(); i++)
    cost = min(cost, goal_distances[pre_post[i].var]);
  return cost;
}


// SearchStatistics implementation

SearchStatistics::SearchStatistics() {
  may_undo_goal = false;
  generated_states = 0;
  expanded_states = 0;
  max_cost_limit = 0;
  max_layer = 0;
  closed_states_limit = INT_MAX;
  closed_states = 0;
}

void SearchStatistics::start_goal(int goal_count) {
  cout << "Adding goal " << goal_count << "/" << g_goal.size() << "..." << flush;
  closed_states = 0;
}


void SearchStatistics::finish_goal() {
  cout << endl << "Solved! [" << expanded_states << " state(s)]" << endl;
  max_cost_limit = 0;
  max_layer = 0;
}

void SearchStatistics::fail() {
  cout << endl << "Failed! [" << expanded_states << " state(s)]" << endl;
  max_cost_limit = 0;
  max_layer = 0;
}

void SearchStatistics::update_layer(bool may_undo, int cost_limit, int layer) {
  if(may_undo && !may_undo_goal) {
    may_undo_goal = true;
    max_cost_limit = -1;
  }

  if(cost_limit > max_cost_limit) {
    max_cost_limit = cost_limit;
    max_layer = -1;
    cout << endl;
    cout << "  Limit " << max_cost_limit;
    if(may_undo_goal)
      cout << "*";
    cout << "...";
  }
    
  if(cost_limit == max_cost_limit && layer > max_layer) {
    max_layer = layer;
    cout << " [" << max_layer << "]" << flush;
  }
}

void SearchStatistics::set_closed_limit(int limit) {
  closed_states_limit = limit;
}

bool SearchStatistics::limit_exceeded() const {
  return closed_states > closed_states_limit;
}

// UniformCostSearcher implementation

UniformCostSearcher::UniformCostSearcher(SearchStatistics *stat,
					 const State *init_state,
					 const vector<bool> &solved_goals,
					 int new_goal)
  : statistics(stat), initial_state(init_state), current_state(*init_state) {
  assert(solved_goals.size() == g_goal.size());
  assert(new_goal >= 0 && new_goal < solved_goals.size());
  assert(!solved_goals[new_goal]);
  
  goals.push_back(g_goal[new_goal]);
  for(int i = 0; i < solved_goals.size(); i++)
    if(solved_goals[i])
      goals.push_back(g_goal[i]);

  new_goal_id = new_goal;

  may_undo_goal = false;
  cost_limit = 0;
  path_cost = 0;
  predecessor = 0;
  current_operator = 0;
}

int UniformCostSearcher::search_step() {
  // Invariants:
  // - current_state is the next state for which we want to compute the heuristic.
  // - predecessor is a permanent pointer to the predecessor of that state.
  // - current_operator is the operator which leads to current_state from predecessor.

  if(!closed.contains(current_state)) {
    statistics->update_layer(may_undo_goal, cost_limit, path_cost);

    const State *parent_ptr = closed.insert(current_state, predecessor,
					    current_operator);
    statistics->expanded_states++;
    statistics->closed_states++;

    bool solved_old_goals = true;
    for(int i = 1; i < goals.size(); i++) {
      if(current_state[goals[i].first] != goals[i].second) {
	solved_old_goals = false;
	break;
      }
    }

    if(solved_old_goals || may_undo_goal) {
      if(solved_old_goals && current_state[goals[0].first] == goals[0].second)
	return SOLVED;

      // TRY: Prune if any of the goals 1..MAX_GOAL are not satisfied?

      g_successor_generator->generate_applicable_ops(current_state, actions);
      
      for(int i = 0; i < actions.size(); i++) {
	int action_cost = get_action_cost(actions[i], new_goal_id);
	// TRY: Don't prune, only use weighting.
	if(action_cost <= cost_limit)
	  open.insert(path_cost + action_cost, parent_ptr, actions[i]);
      }
      statistics->generated_states += actions.size();
      actions.clear();
    }
  }
  
  // Fetch new state.
  if(open.empty()) {
    if(cost_limit == g_causal_graph_goal_diameter) {
      if(!may_undo_goal) {
	may_undo_goal = true;
	cost_limit = -1;
      } else {
	// No point in going further?
	return FAILED;
      }
    }
    cost_limit++;
    statistics->closed_states -= closed.size();
    closed.clear();
    path_cost = 0;
    predecessor = 0;
    current_operator = 0;
    current_state = *initial_state;
    return IN_PROGRESS;
  } else {
    path_cost = open.min();
    pair<const State *, const Operator *> next_pair = open.remove_min();
    predecessor = next_pair.first;
    current_operator = next_pair.second;
    current_state = State(*predecessor, *current_operator);
    return IN_PROGRESS;
  }
}

void UniformCostSearcher::extract_plan(vector<const Operator *> &result) {
  closed.get_path_from_initial_state(current_state, result);
}

// IterativeSearchEngine implementation

IterativeSearchEngine::IterativeSearchEngine(int mem_limit)
  : current_state(*g_initial_state) {
  solution_found = false;

  int state_size = sizeof(int) * g_variable_domain.size();
  assert(mem_limit < 4192); // avoid overflow
  unsigned int memory_in_bytes = mem_limit * 1048576U;
  statistics.set_closed_limit(memory_in_bytes / state_size);
}

IterativeSearchEngine::~IterativeSearchEngine() {
  // TODO: Reclaim memory.
}

void IterativeSearchEngine::initialize() {
  compute_causal_graph_goal_distances();

  cout << "Conducting iterated breadth-first search." << endl;
  solved_goals.resize(g_goal.size(), false);
  num_goals_solved = 0;
  initialize_searchers();
}

void IterativeSearchEngine::initialize_searchers() {
  searchers.clear();
  for(int i = 0; i < g_goal.size(); i++)
    if(!solved_goals[i])
      searchers.push_back(UniformCostSearcher(&statistics, &current_state,
					      solved_goals, i));
  statistics.start_goal(num_goals_solved + 1);
}

bool IterativeSearchEngine::found_solution() const {
  return solution_found;
}

const vector<const Operator *> &IterativeSearchEngine::get_plan() const {
  assert(solution_found);
  return plan;
}

int IterativeSearchEngine::get_expanded_states() const {
  return statistics.expanded_states;
}

int IterativeSearchEngine::get_generated_states() const {
  return statistics.generated_states;
}

int IterativeSearchEngine::step() {
  list<UniformCostSearcher>::iterator current_searcher = searchers.begin();
  while(current_searcher != searchers.end()) {
    int result = current_searcher->search_step();
    if(result == UniformCostSearcher::IN_PROGRESS) {
      ++current_searcher;
    } else if(result == UniformCostSearcher::FAILED) {
      list<UniformCostSearcher>::iterator failed_searcher = current_searcher;
      ++current_searcher;
      searchers.erase(failed_searcher);
      if(searchers.empty()) {
	statistics.fail();
	return FAILED;
      }
    } else {
      assert(result == UniformCostSearcher::SOLVED);
      statistics.finish_goal();
      solved_goals[current_searcher->get_new_goal()] = true;
      current_state = current_searcher->get_current_state();
      current_searcher->extract_plan(plan);
      num_goals_solved++;
      if(num_goals_solved == g_goal.size()) {
	solution_found = true;
	cout << "Solution found!" << endl;
	return SOLVED;
      } else {
	initialize_searchers();
	return IN_PROGRESS;
      }
    }
  }
  if(statistics.limit_exceeded()) {
    cout << endl << "Exceeded memory limit!" << endl;
    return FAILED;
  }
  return IN_PROGRESS;
}

