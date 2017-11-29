#include "globals.h"

#include "axioms.h"
#include "global_operator.h"
#include "global_state.h"
#include "heuristic.h"

#include "algorithms/int_packer.h"
#include "task_utils/causal_graph.h"
#include "task_utils/successor_generator.h"
#include "tasks/root_task.h"
#include "utils/logging.h"
#include "utils/rng.h"
#include "utils/system.h"
#include "utils/timer.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using utils::ExitCode;

bool test_goal(const GlobalState &state) {
    for (size_t i = 0; i < g_goal.size(); ++i) {
        if (state[g_goal[i].first] != g_goal[i].second) {
            return false;
        }
    }
    return true;
}

int calculate_plan_cost(const vector<OperatorID> &plan, const TaskProxy &task_proxy) {
    // TODO: Refactor: this is only used by save_plan (see below)
    //       and the SearchEngine classes and hence should maybe
    //       be moved into the SearchEngine (along with save_plan).
    OperatorsProxy operators = task_proxy.get_operators();
    int plan_cost = 0;
    for (OperatorID op_id : plan) {
        plan_cost += operators[op_id].get_cost();
    }
    return plan_cost;
}

void save_plan(const vector<OperatorID> &plan,
               const TaskProxy &task_proxy,
               bool generates_multiple_plan_files) {
    // TODO: Refactor: this is only used by the SearchEngine classes
    //       and hence should maybe be moved into the SearchEngine.
    ostringstream filename;
    filename << g_plan_filename;
    int plan_number = g_num_previously_generated_plans + 1;
    if (generates_multiple_plan_files || g_is_part_of_anytime_portfolio) {
        filename << "." << plan_number;
    } else {
        assert(plan_number == 1);
    }
    ofstream outfile(filename.str());
    OperatorsProxy operators = task_proxy.get_operators();
    for (OperatorID op_id : plan) {
        cout << operators[op_id].get_name() << " (" << operators[op_id].get_cost() << ")" << endl;
        outfile << "(" << operators[op_id].get_name() << ")" << endl;
    }
    int plan_cost = calculate_plan_cost(plan, task_proxy);
    outfile << "; cost = " << plan_cost << " ("
            << (is_unit_cost() ? "unit cost" : "general cost") << ")" << endl;
    outfile.close();
    cout << "Plan length: " << plan.size() << " step(s)." << endl;
    cout << "Plan cost: " << plan_cost << endl;
    ++g_num_previously_generated_plans;
}

void check_magic(istream &in, string magic) {
    string word;
    in >> word;
    if (word != magic) {
        cout << "Failed to match magic word '" << magic << "'." << endl;
        cout << "Got '" << word << "'." << endl;
        if (magic == "begin_version") {
            cerr << "Possible cause: you are running the planner "
                 << "on a preprocessor file from " << endl
                 << "an older version." << endl;
        }
        utils::exit_with(ExitCode::INPUT_ERROR);
    }
}

void dump_goal() {
    cout << "Goal Conditions:" << endl;
    for (size_t i = 0; i < g_goal.size(); ++i)
        cout << "  " << g_variable_name[g_goal[i].first] << ": "
             << g_goal[i].second << endl;
}

void read_everything(istream &in) {
    g_root_task = tasks::parse_root_task(in);

    cout << "Building successor generator..." << flush;
    int peak_memory_before = utils::get_peak_memory_in_kb();
    TaskProxy task_proxy(*g_root_task);
    utils::Timer successor_generator_timer;
    g_successor_generator = new successor_generator::SuccessorGenerator(task_proxy);
    successor_generator_timer.stop();
    cout << "done! [t=" << utils::g_timer << "]" << endl;
    int peak_memory_after = utils::get_peak_memory_in_kb();
    int memory_diff = peak_memory_after - peak_memory_before;
    cout << "peak memory difference for root successor generator creation: "
         << memory_diff << " KB" << endl
         << "time for root successor generation creation: "
         << successor_generator_timer << endl;
    cout << "done initializing global data [t=" << utils::g_timer << "]" << endl;
}

void dump_everything() {
    cout << "Use metric? " << g_use_metric << endl;
    cout << "Min Action Cost: " << g_min_action_cost << endl;
    cout << "Max Action Cost: " << g_max_action_cost << endl;
    // TODO: Dump the actual fact names.
    cout << "Variables (" << g_variable_name.size() << "):" << endl;
    for (size_t i = 0; i < g_variable_name.size(); ++i)
        cout << "  " << g_variable_name[i]
             << " (range " << g_variable_domain[i] << ")" << endl;
    State initial_state = TaskProxy(*g_root_task).get_initial_state();
    cout << "Initial State (PDDL):" << endl;
    initial_state.dump_pddl();
    cout << "Initial State (FDR):" << endl;
    initial_state.dump_fdr();
    dump_goal();
    /*
    for(int i = 0; i < g_variable_domain.size(); ++i)
      g_transition_graphs[i]->dump();
    */
}

bool is_unit_cost() {
    return g_min_action_cost == 1 && g_max_action_cost == 1;
}

bool g_use_metric;
int g_min_action_cost = numeric_limits<int>::max();
int g_max_action_cost = 0;
vector<int> g_variable_domain;
int_packer::IntPacker *g_state_packer;
vector<int> g_initial_state_data;
AxiomEvaluator *g_axiom_evaluator;
successor_generator::SuccessorGenerator *g_successor_generator;

string g_plan_filename = "sas_plan";
int g_num_previously_generated_plans = 0;
bool g_is_part_of_anytime_portfolio = false;

utils::Log g_log;


vector<std::vector<std::set<FactPair>>> g_inconsistent_facts;
vector<std::vector<std::string>> g_fact_names;
vector<int> g_axiom_layers;
vector<int> g_default_axiom_values;
vector<std::string> g_variable_name;
vector<std::pair<int, int>> g_goal;
vector<GlobalOperator> g_axioms;
vector<GlobalOperator> g_operators;
shared_ptr<AbstractTask> g_root_task;
