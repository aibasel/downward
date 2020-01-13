#include "cartesian_abstraction_generator.h"

#include "explicit_abstraction.h"
#include "types.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../cegar/abstraction.h"
#include "../cegar/abstract_state.h"
#include "../cegar/cegar.h"
#include "../cegar/cost_saturation.h"
#include "../cegar/refinement_hierarchy.h"
#include "../cegar/split_selector.h"
#include "../cegar/subtask_generators.h"
#include "../cegar/transition_system.h"
#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/rng_options.h"

using namespace std;

namespace cost_saturation {
class CartesianAbstractionFunction : public AbstractionFunction {
    unique_ptr<cegar::RefinementHierarchy> refinement_hierarchy;

public:
    explicit CartesianAbstractionFunction(
        unique_ptr<cegar::RefinementHierarchy> refinement_hierarchy)
        : refinement_hierarchy(move(refinement_hierarchy)) {
    }

    virtual int get_abstract_state_id(const State &concrete_state) const override {
        return refinement_hierarchy->get_abstract_state_id(concrete_state);
    }
};


CartesianAbstractionGenerator::CartesianAbstractionGenerator(
    const options::Options &opts)
    : subtask_generators(
          opts.get_list<shared_ptr<cegar::SubtaskGenerator>>("subtasks")),
      max_states(opts.get<int>("max_states")),
      max_transitions(opts.get<int>("max_transitions")),
      max_time(opts.get<double>("max_time")),
      extra_memory_padding_mb(opts.get<int>("memory_padding")),
      prune_unreachable_transitions(opts.get<bool>("prune_unreachable_transitions")),
      rng(utils::parse_rng_from_options(opts)),
      debug(opts.get<bool>("debug")),
      num_states(0),
      num_transitions(0) {
}

static vector<bool> get_looping_operators(const cegar::TransitionSystem &ts) {
    int num_operators = ts.get_num_operators();
    vector<bool> operator_induces_self_loop(num_operators, false);
    for (const auto &loops : ts.get_loops()) {
        for (int op_id : loops) {
            operator_induces_self_loop[op_id] = true;
        }
    }
    return operator_induces_self_loop;
}


static pair<bool, unique_ptr<Abstraction>> convert_abstraction(
    cegar::Abstraction &cartesian_abstraction,
    const vector<int> &operator_costs,
    bool prune_unreachable_transitions) {
    // Compute g and h values.
    const cegar::TransitionSystem &ts =
        cartesian_abstraction.get_transition_system();
    int initial_state_id = cartesian_abstraction.get_initial_state().get_id();
    vector<int> g_values = cegar::compute_distances(
        ts.get_outgoing_transitions(), operator_costs, {initial_state_id});
    vector<int> h_values = cegar::compute_distances(
        ts.get_incoming_transitions(), operator_costs, cartesian_abstraction.get_goals());

    // Retrieve non-looping transitions.
    vector<vector<Successor>> backward_graph(cartesian_abstraction.get_num_states());
    for (int state_id = 0; state_id < cartesian_abstraction.get_num_states(); ++state_id) {
        // Prune transitions from unsolvable or unreachable states.
        if (h_values[state_id] == INF ||
            (prune_unreachable_transitions && g_values[state_id] == INF)) {
            continue;
        }
        for (const cegar::Transition &transition : ts.get_outgoing_transitions()[state_id]) {
            // Prune transitions to unsolvable states (we know target is reachable).
            if (h_values[transition.target_id] == INF) {
                continue;
            }
            backward_graph[transition.target_id].emplace_back(transition.op_id, state_id);
        }
    }
    for (vector<Successor> &succesors : backward_graph) {
        succesors.shrink_to_fit();
    }

    vector<bool> looping_operators = get_looping_operators(ts);
    vector<int> goal_states(
        cartesian_abstraction.get_goals().begin(),
        cartesian_abstraction.get_goals().end());

    bool unsolvable = h_values[initial_state_id] == INF;
    return {
               unsolvable,
               utils::make_unique_ptr<ExplicitAbstraction>(
                   utils::make_unique_ptr<CartesianAbstractionFunction>(
                       cartesian_abstraction.extract_refinement_hierarchy()),
                   move(backward_graph),
                   move(looping_operators),
                   move(goal_states))
    };
}

unique_ptr<cegar::Abstraction> CartesianAbstractionGenerator::build_abstraction_for_subtask(
    const shared_ptr<AbstractTask> &subtask,
    int remaining_subtasks,
    const utils::CountdownTimer &timer) {
    cegar::CEGAR cegar(
        subtask,
        max(1, (max_states - num_states) / remaining_subtasks),
        max(1, (max_transitions - num_transitions) / remaining_subtasks),
        timer.get_remaining_time() / remaining_subtasks,
        cegar::PickSplit::MAX_REFINED,
        *rng,
        debug);
    cout << endl;
    return cegar.extract_abstraction();
}

void CartesianAbstractionGenerator::build_abstractions_for_subtasks(
    const vector<shared_ptr<AbstractTask>> &subtasks,
    const utils::CountdownTimer &timer,
    Abstractions &abstractions) {
    int remaining_subtasks = subtasks.size();
    for (const shared_ptr<AbstractTask> &subtask : subtasks) {
        unique_ptr<cegar::Abstraction> cartesian_abstraction =
            build_abstraction_for_subtask(subtask, remaining_subtasks, timer);

        /* If we already have an abstraction A and run out of memory while
           building another abstraction B, we discard B to avoid running out of
           memory during the conversion. */
        if (!abstractions.empty() && !utils::extra_memory_padding_is_reserved()) {
            break;
        }

        num_states += cartesian_abstraction->get_num_states();
        num_transitions += cartesian_abstraction->get_transition_system().get_num_non_loops();

        vector<int> operator_costs = task_properties::get_operator_costs(TaskProxy(*subtask));
        auto result = convert_abstraction(
            *cartesian_abstraction, operator_costs, prune_unreachable_transitions);
        bool unsolvable = result.first;
        abstractions.push_back(move(result.second));

        if (num_states >= max_states || num_transitions >= max_transitions ||
            !utils::extra_memory_padding_is_reserved() || unsolvable) {
            break;
        }

        --remaining_subtasks;
    }
}

Abstractions CartesianAbstractionGenerator::generate_abstractions(
    const shared_ptr<AbstractTask> &task) {
    utils::CountdownTimer timer(max_time);
    num_states = 0;
    num_transitions = 0;
    utils::Log log;
    log << "Build Cartesian abstractions" << endl;

    // The CEGAR code expects that some extra memory is reserved.
    utils::reserve_extra_memory_padding(extra_memory_padding_mb);

    Abstractions abstractions;
    for (const auto &subtask_generator : subtask_generators) {
        cegar::SharedTasks subtasks = subtask_generator->get_subtasks(task);
        build_abstractions_for_subtasks(subtasks, timer, abstractions);
        if (num_states >= max_states ||
            num_transitions >= max_transitions ||
            timer.is_expired() ||
            !utils::extra_memory_padding_is_reserved()) {
            break;
        }
    }

    if (utils::extra_memory_padding_is_reserved()) {
        utils::release_extra_memory_padding();
    }

    log << "Cartesian abstractions: " << abstractions.size() << endl;
    log << "Time for building Cartesian abstractions: "
        << timer.get_elapsed_time() << endl;
    log << "Total number of Cartesian states: " << num_states << endl;
    log << "Total number of transitions in Cartesian abstractions: "
        << num_transitions << endl;
    return abstractions;
}

static shared_ptr<AbstractionGenerator> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Cartesian abstraction generator",
        "");

    parser.add_list_option<shared_ptr<cegar::SubtaskGenerator>>(
        "subtasks",
        "subtask generators",
        "[landmarks(order=random), goals(order=random)]");
    parser.add_option<int>(
        "max_states",
        "maximum sum of abstract states over all abstractions",
        "infinity",
        Bounds("0", "infinity"));
    parser.add_option<int>(
        "max_transitions",
        "maximum sum of state-changing transitions (excluding self-loops) over "
        "all abstractions",
        "1000000",
        Bounds("0", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time for computing abstractions",
        "infinity",
        Bounds("0.0", "infinity"));
    parser.add_option<int>(
        "memory_padding",
        "amount of extra memory in MB to reserve for recovering from "
        "out-of-memory situations gracefully. When the memory runs out, we "
        "stop refining and start the search. Due to memory fragmentation, "
        "the memory used for building the abstraction (states, transitions, "
        "etc.) often can't be reused for things that require big continuous "
        "blocks of memory. It is for this reason that we require a rather "
        "large amount of memory padding by default.",
        "500",
        Bounds("0", "infinity"));
    parser.add_option<bool>(
        "prune_unreachable_transitions",
        "remove transitions from and to unreachable Cartesian states",
        "false");
    parser.add_option<bool>(
        "debug",
        "print debugging info",
        "false");
    utils::add_rng_options(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<CartesianAbstractionGenerator>(opts);
}

static Plugin<AbstractionGenerator> _plugin("cartesian", _parse);
}
