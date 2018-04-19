#include "globals.h"

#include "axioms.h"

#include "algorithms/int_packer.h"
#include "task_utils/successor_generator.h"
#include "task_utils/task_properties.h"
#include "tasks/root_task.h"
#include "utils/logging.h"

#include <iostream>
#include <limits>

using namespace std;
using utils::ExitCode;

void dump_goal(const TaskProxy &task_proxy) {
    cout << "Goal conditions:" << endl;
    for (FactProxy goal : task_proxy.get_goals()) {
        cout << "  " << goal.get_variable().get_name() << ": "
             << goal.get_value() << endl;
    }
}

void read_everything(istream &in) {
    cout << "reading input... [t=" << utils::g_timer << "]" << endl;
    tasks::read_root_task(in);
    cout << "done reading input! [t=" << utils::g_timer << "]" << endl;

    TaskProxy task_proxy(*tasks::g_root_task);
    g_axiom_evaluator = new AxiomEvaluator(task_proxy);

    cout << "packing state variables..." << flush;
    VariablesProxy variables = task_proxy.get_variables();
    vector<int> variable_ranges;
    variable_ranges.reserve(variables.size());
    for (VariableProxy var : variables) {
        variable_ranges.push_back(var.get_domain_size());
    }
    g_state_packer = new int_packer::IntPacker(variable_ranges);
    cout << "done! [t=" << utils::g_timer << "]" << endl;

    int num_facts = 0;
    for (VariableProxy var : variables)
        num_facts += var.get_domain_size();

    cout << "Variables: " << variables.size() << endl;
    cout << "FactPairs: " << num_facts << endl;
    cout << "Bytes per state: "
         << g_state_packer->get_num_bins() * sizeof(int_packer::IntPacker::Bin)
         << endl;

    cout << "Building successor generator..." << flush;
    int peak_memory_before = utils::get_peak_memory_in_kb();
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
    TaskProxy task_proxy(*tasks::g_root_task);
    OperatorsProxy operators = task_proxy.get_operators();
    int min_action_cost = numeric_limits<int>::max();
    int max_action_cost = 0;
    for (OperatorProxy op : operators) {
        min_action_cost = min(min_action_cost, op.get_cost());
        max_action_cost = max(max_action_cost, op.get_cost());
    }
    cout << "Min Action Cost: " << min_action_cost << endl;
    cout << "Max Action Cost: " << max_action_cost << endl;

    VariablesProxy variables = task_proxy.get_variables();
    cout << "Variables (" << variables.size() << "):" << endl;
    for (VariableProxy var : variables) {
        cout << "  " << var.get_name()
             << " (range " << var.get_domain_size() << ")" << endl;
        for (int val = 0; val < var.get_domain_size(); ++val) {
            cout << "    " << val << ": " << var.get_fact(val).get_name() << endl;
        }
    }
    State initial_state = task_proxy.get_initial_state();
    cout << "Initial State (PDDL):" << endl;
    initial_state.dump_pddl();
    cout << "Initial State (FDR):" << endl;
    initial_state.dump_fdr();
    dump_goal(task_proxy);
}

bool is_unit_cost() {
    assert(tasks::g_root_task);
    static bool is_unit_cost = task_properties::is_unit_cost(TaskProxy(*tasks::g_root_task));
    return is_unit_cost;
}

int_packer::IntPacker *g_state_packer;
vector<int> g_initial_state_data;
AxiomEvaluator *g_axiom_evaluator;
successor_generator::SuccessorGenerator *g_successor_generator;

utils::Log g_log;
