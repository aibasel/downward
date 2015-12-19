#include "additive_cartesian_heuristic.h"

#include "abstraction.h"
#include "cartesian_heuristic.h"
#include "decompositions.h"
#include "utils.h"

#include "../evaluation_context.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_tools.h"

#include "../tasks/modified_operator_costs_task.h"

#include "../utils/logging.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace CEGAR {
static const int memory_padding_in_mb = 75;

AdditiveCartesianHeuristic::AdditiveCartesianHeuristic(const Options &opts)
    : Heuristic(opts),
      options(opts),
      decompositions(opts.get_list<shared_ptr<Decomposition>>("decompositions")),
      max_states(options.get<int>("max_states")),
      timer(new Utils::CountdownTimer(options.get<double>("max_time"))),
      num_abstractions(0),
      num_states(0) {
    DEBUG = opts.get<bool>("debug");

    verify_no_axioms(task_proxy);
    verify_no_conditional_effects(task_proxy);

    for (OperatorProxy op : task_proxy.get_operators())
        remaining_costs.push_back(op.get_cost());
}

void AdditiveCartesianHeuristic::reduce_remaining_costs(
    const vector<int> &needed_costs) {
    assert(remaining_costs.size() == needed_costs.size());
    for (size_t i = 0; i < remaining_costs.size(); ++i) {
        assert(needed_costs[i] <= remaining_costs[i]);
        remaining_costs[i] -= needed_costs[i];
        assert(remaining_costs[i] >= 0);
    }
}

shared_ptr<AbstractTask> AdditiveCartesianHeuristic::get_remaining_costs_task(
    shared_ptr<AbstractTask> &parent) const {
    vector<int> costs = remaining_costs;
    return make_shared<ExtraTasks::ModifiedOperatorCostsTask>(parent, move(costs));
}

bool AdditiveCartesianHeuristic::may_build_another_abstraction() {
    return num_states < max_states &&
           !timer->is_expired() &&
           Utils::extra_memory_padding_is_reserved() &&
           compute_heuristic(g_initial_state()) != DEAD_END;
}

void AdditiveCartesianHeuristic::build_abstractions(
    const Decomposition &decomposition) {
    SharedTasks subtasks = decomposition.get_subtasks(task);

    int rem_subtasks = subtasks.size();
    for (shared_ptr<AbstractTask> subtask : subtasks) {
        cout << endl;

        subtask = get_remaining_costs_task(subtask);

        Options abs_opts(options);
        abs_opts.set<shared_ptr<AbstractTask>>("transform", subtask);
        abs_opts.set<int>("max_states", (max_states - num_states) / rem_subtasks);
        abs_opts.set<double>("max_time", timer->get_remaining_time() / rem_subtasks);
        /*
          For landmark tasks we have to map all states in which the landmark
          might have been achieved to arbitrary abstract goal states. For the
          other decompositions our method won't find unreachable facts, but
          calling it unconditionally for subtasks with one goal doesn't hurt
          and simplifies the implementation.
        */
        abs_opts.set<bool>("separate_unreachable_facts",
                           TaskProxy(*subtask).get_goals().size() == 1);
        Abstraction abstraction(abs_opts);

        ++num_abstractions;
        num_states += abstraction.get_num_states();
        vector<int> needed_costs = abstraction.get_needed_costs();
        reduce_remaining_costs(needed_costs);
        int init_h = abstraction.get_h_value_of_initial_state();

        if (init_h > 0) {
            Options opts;
            opts.set<int>("cost_type", 0);
            opts.set<shared_ptr<AbstractTask>>("transform", subtask);
            opts.set<bool>("cache_estimates", cache_h_values);
            heuristics.push_back(
                Utils::make_unique_ptr<CartesianHeuristic>(
                    opts, abstraction.get_split_tree()));
        }
        if (!may_build_another_abstraction())
            break;

        --rem_subtasks;
    }
}

void AdditiveCartesianHeuristic::initialize() {
    g_log << "Initializing additive Cartesian heuristic..." << endl;
    Utils::reserve_extra_memory_padding(memory_padding_in_mb);
    for (shared_ptr<Decomposition> decomposition : decompositions) {
        build_abstractions(*decomposition);
        cout << endl;
        if (!may_build_another_abstraction())
            break;
    }
    if (Utils::extra_memory_padding_is_reserved())
        Utils::release_extra_memory_padding();
    print_statistics();
    cout << endl;
}

void AdditiveCartesianHeuristic::print_statistics() const {
    g_log << "Done initializing additive Cartesian heuristic" << endl;
    cout << "Cartesian abstractions built: " << num_abstractions << endl;
    cout << "Cartesian heuristics stored: " << heuristics.size() << endl;
    cout << "Cartesian states: " << num_states << endl;
}

int AdditiveCartesianHeuristic::compute_heuristic(const GlobalState &global_state) {
    EvaluationContext eval_context(global_state);
    int sum_h = 0;
    for (auto &heuristic : heuristics) {
        if (eval_context.is_heuristic_infinite(heuristic.get()))
            return DEAD_END;
        sum_h += eval_context.get_heuristic_value(heuristic.get());
    }
    assert(sum_h >= 0);
    return sum_h;
}

static string t2t_escape(const string &s) {
    return "\"\"" + s + "\"\"";
}

static string format_paper_reference(
    string authors, string title, string url, string conference,
    string pages, string publisher) {
    stringstream ss;
    ss << "\n\n"
       << " * " << t2t_escape(authors) << ".<<BR>>\n"
       << " [" << t2t_escape(title) << " " << t2t_escape(url) << "].<<BR>>\n"
       << " In //" << t2t_escape(conference) << "//,"
       << " pp. " << t2t_escape(pages) << ". "
       << t2t_escape(publisher) << ".\n\n\n";
    return ss.str();
}

static Heuristic *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Additive CEGAR heuristic",
        "See the paper introducing Counterexample-guided Abstraction "
        "Refinement (CEGAR) for classical planning:" +
        format_paper_reference(
            "Jendrik Seipp and Malte Helmert",
            "Counterexample-guided Cartesian Abstraction Refinement",
            "http://ai.cs.unibas.ch/papers/seipp-helmert-icaps2013.pdf",
            "Proceedings of the 23rd International Conference on Automated "
            "Planning and Scheduling (ICAPS 2013)",
            "347-351",
            "AAAI Press 2013") +
        "and the paper showing how to make the abstractions additive:" +
        format_paper_reference(
            "Jendrik Seipp and Malte Helmert",
            "Diverse and Additive Cartesian Abstraction Heuristics",
            "http://ai.cs.unibas.ch/papers/seipp-helmert-icaps2014.pdf",
            "Proceedings of the 24th International Conference on "
            "Automated Planning and Scheduling (ICAPS 2014)",
            "289-297",
            "AAAI Press 2014"));
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_list_option<shared_ptr<Decomposition>>(
        "decompositions",
        "task decompositions",
        "[decomposition_by_landmarks(),decomposition_by_goals()]");
    parser.add_option<int>(
        "max_states",
        "maximum sum of abstract states over all abstractions",
        "infinity",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for building abstractions",
        "900",
        Bounds("0.0", "infinity"));
    vector<string> pick_strategies;
    pick_strategies.push_back("RANDOM");
    pick_strategies.push_back("MIN_UNWANTED");
    pick_strategies.push_back("MAX_UNWANTED");
    pick_strategies.push_back("MIN_REFINED");
    pick_strategies.push_back("MAX_REFINED");
    pick_strategies.push_back("MIN_HADD");
    pick_strategies.push_back("MAX_HADD");
    parser.add_enum_option(
        "pick", pick_strategies, "split-selection strategy", "MAX_REFINED");
    parser.add_option<bool>(
        "use_general_costs", "allow negative costs in cost partitioning", "true");
    parser.add_option<bool>(
        "debug", "print debugging output", "false");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return new AdditiveCartesianHeuristic(opts);
}

static Plugin<Heuristic> _plugin("cegar", _parse);
}
