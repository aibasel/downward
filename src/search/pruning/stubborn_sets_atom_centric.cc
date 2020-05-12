#include "stubborn_sets_atom_centric.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/memory.h"

using namespace std;

namespace stubborn_sets_atom_centric {
StubbornSetsAtomCentric::StubbornSetsAtomCentric(const options::Options &opts)
    : StubbornSets(opts),
      use_sibling_shortcut(opts.get<bool>("use_sibling_shortcut")),
      atom_selection_strategy(opts.get<AtomSelectionStrategy>("atom_selection_strategy")) {
}

void StubbornSetsAtomCentric::initialize(const shared_ptr<AbstractTask> &task) {
    StubbornSets::initialize(task);
    utils::g_log << "pruning method: atom-centric stubborn sets" << endl;

    TaskProxy task_proxy(*task);

    int num_variables = task_proxy.get_variables().size();
    marked_producers.reserve(num_variables);
    marked_consumers.reserve(num_variables);
    for (VariableProxy var : task_proxy.get_variables()) {
        marked_producers.emplace_back(var.get_domain_size(), false);
        marked_consumers.emplace_back(var.get_domain_size(), false);
    }

    if (use_sibling_shortcut) {
        marked_producer_variables.resize(num_variables, MARKED_VALUES_NONE);
        marked_consumer_variables.resize(num_variables, MARKED_VALUES_NONE);
    }

    compute_consumers(task_proxy);
}

void StubbornSetsAtomCentric::compute_consumers(const TaskProxy &task_proxy) {
    consumers.reserve(task_proxy.get_variables().size());
    for (VariableProxy var : task_proxy.get_variables()) {
        consumers.emplace_back(var.get_domain_size());
    }

    for (OperatorProxy op : task_proxy.get_operators()) {
        int op_id = op.get_id();
        for (FactProxy fact_proxy : op.get_preconditions()) {
            FactPair fact = fact_proxy.get_pair();
            consumers[fact.var][fact.value].push_back(op_id);
        }
    }

    for (auto &outer : consumers) {
        for (auto &inner : outer) {
            inner.shrink_to_fit();
        }
    }
}

bool StubbornSetsAtomCentric::operator_is_applicable(int op, const State &state) const {
    return find_unsatisfied_precondition(op, state) == FactPair::no_fact;
}

void StubbornSetsAtomCentric::enqueue_producers(const FactPair &fact) {
    if (!marked_producers[fact.var][fact.value]) {
        marked_producers[fact.var][fact.value] = true;
        producer_queue.push_back(fact);
    }
}

void StubbornSetsAtomCentric::enqueue_consumers(const FactPair &fact) {
    if (!marked_consumers[fact.var][fact.value]) {
        marked_consumers[fact.var][fact.value] = true;
        consumer_queue.push_back(fact);
    }
}

void StubbornSetsAtomCentric::enqueue_sibling_producers(const FactPair &fact) {
    /*
      If we don't use the sibling shortcut handling, we ignore any
      variable-based marking info and always enqueue all sibling facts of the
      given fact v=d.
    */
    int dummy_mark = MARKED_VALUES_NONE;
    int &mark = use_sibling_shortcut ? marked_producer_variables[fact.var] : dummy_mark;
    if (mark == MARKED_VALUES_NONE) {
        /*
          If we don't have marking info for variable v, enqueue all sibling
          producers of v=d and remember that we marked all siblings.
        */
        int domain_size = consumers[fact.var].size();
        for (int value = 0; value < domain_size; ++value) {
            if (value != fact.value) {
                enqueue_producers(FactPair(fact.var, value));
            }
        }
        mark = fact.value;
    } else if (mark != MARKED_VALUES_ALL && mark != fact.value) {
        /*
          Exactly one fact v=d' has not been enqueued. It is therefore the only
          sibling of v=d that we need to enqueue.
        */
        enqueue_producers(FactPair(fact.var, mark));
        mark = MARKED_VALUES_ALL;
    }
}

void StubbornSetsAtomCentric::enqueue_sibling_consumers(const FactPair &fact) {
    // For documentation, see enqueue_sibling_producers().
    int dummy_mark = MARKED_VALUES_NONE;
    int &mark = use_sibling_shortcut ? marked_consumer_variables[fact.var] : dummy_mark;
    if (mark == MARKED_VALUES_NONE) {
        int domain_size = consumers[fact.var].size();
        for (int value = 0; value < domain_size; ++value) {
            if (value != fact.value) {
                enqueue_consumers(FactPair(fact.var, value));
            }
        }
        mark = fact.value;
    } else if (mark != MARKED_VALUES_ALL && mark != fact.value) {
        enqueue_consumers(FactPair(fact.var, mark));
        mark = MARKED_VALUES_ALL;
    }
}

FactPair StubbornSetsAtomCentric::select_fact(
    const vector<FactPair> &facts, const State &state) const {
    FactPair fact = FactPair::no_fact;
    if (atom_selection_strategy == AtomSelectionStrategy::FAST_DOWNWARD) {
        fact = stubborn_sets::find_unsatisfied_condition(facts, state);
    } else if (atom_selection_strategy == AtomSelectionStrategy::QUICK_SKIP) {
        /*
          If there is an unsatisfied fact whose producers are already marked,
          choose it. Otherwise, choose the first unsatisfied fact.
        */
        for (const FactPair &condition : facts) {
            if (state[condition.var].get_value() != condition.value) {
                if (marked_producers[condition.var][condition.value]) {
                    fact = condition;
                    break;
                } else if (fact == FactPair::no_fact) {
                    fact = condition;
                }
            }
        }
    } else if (atom_selection_strategy == AtomSelectionStrategy::STATIC_SMALL) {
        int min_count = numeric_limits<int>::max();
        for (const FactPair &condition : facts) {
            if (state[condition.var].get_value() != condition.value) {
                int count = achievers[condition.var][condition.value].size();
                if (count < min_count) {
                    fact = condition;
                    min_count = count;
                }
            }
        }
    } else if (atom_selection_strategy == AtomSelectionStrategy::DYNAMIC_SMALL) {
        int min_count = numeric_limits<int>::max();
        for (const FactPair &condition : facts) {
            if (state[condition.var].get_value() != condition.value) {
                const vector<int> &ops = achievers[condition.var][condition.value];
                int count = count_if(
                    ops.begin(), ops.end(), [&](int op) {return !stubborn[op];});
                if (count < min_count) {
                    fact = condition;
                    min_count = count;
                }
            }
        }
    } else {
        ABORT("Unknown atom selection strategy");
    }
    assert(fact != FactPair::no_fact);
    return fact;
}

void StubbornSetsAtomCentric::enqueue_nes(int op, const State &state) {
    FactPair fact = select_fact(sorted_op_preconditions[op], state);
    enqueue_producers(fact);
}

void StubbornSetsAtomCentric::enqueue_interferers(int op) {
    for (const FactPair &fact : sorted_op_preconditions[op]) {
        // Enqueue operators that disable op.
        enqueue_sibling_producers(fact);
    }
    for (const FactPair &fact : sorted_op_effects[op]) {
        // Enqueue operators that conflict with op.
        enqueue_sibling_producers(fact);

        // Enqueue operators that op disables.
        enqueue_sibling_consumers(fact);
    }
}

void StubbornSetsAtomCentric::initialize_stubborn_set(const State &state) {
    assert(producer_queue.empty());
    assert(consumer_queue.empty());
    // Reset data structures from previous call.
    for (auto &facts : marked_producers) {
        facts.assign(facts.size(), false);
    }
    for (auto &facts : marked_consumers) {
        facts.assign(facts.size(), false);
    }
    if (use_sibling_shortcut) {
        int num_variables = state.size();
        marked_producer_variables.assign(num_variables, MARKED_VALUES_NONE);
        marked_consumer_variables.assign(num_variables, MARKED_VALUES_NONE);
    }

    FactPair unsatisfied_goal = select_fact(sorted_goals, state);
    assert(unsatisfied_goal != FactPair::no_fact);
    enqueue_producers(unsatisfied_goal);

    while (!producer_queue.empty() || !consumer_queue.empty()) {
        if (!producer_queue.empty()) {
            FactPair fact = producer_queue.back();
            producer_queue.pop_back();
            for (int op : achievers[fact.var][fact.value]) {
                handle_stubborn_operator(state, op);
            }
        } else {
            FactPair fact = consumer_queue.back();
            consumer_queue.pop_back();
            for (int op : consumers[fact.var][fact.value]) {
                handle_stubborn_operator(state, op);
            }
        }
    }
}

void StubbornSetsAtomCentric::handle_stubborn_operator(const State &state, int op) {
    if (!stubborn[op]) {
        stubborn[op] = true;
        if (operator_is_applicable(op, state)) {
            enqueue_interferers(op);
        } else {
            enqueue_nes(op, state);
        }
    }
}


static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Atom-centric stubborn sets",
        "Stubborn sets are a state pruning method which computes a subset "
        "of applicable actions in each state such that completeness and "
        "optimality of the overall search is preserved. Previous stubborn set "
        "implementations mainly track information about actions. In contrast, "
        "this implementation focuses on atomic propositions (atoms), which "
        "often speeds up the computation on IPC benchmarks. For details, see" +
        utils::format_conference_reference(
            {"Gabriele Roeger", "Malte Helmert", "Jendrik Seipp", "Silvan Sievers"},
            "An Atom-Centric Perspective on Stubborn Sets",
            "https://ai.dmi.unibas.ch/papers/roeger-et-al-socs2020.pdf",
            "Proceedings of the 13th Annual Symposium on Combinatorial Search "
            "(SoCS 2020)",
            // TODO: add page numbers.
            "",
            "AAAI Press",
            "2020"));
    parser.add_option<bool>(
        "use_sibling_shortcut",
        "use variable-based marking in addition to atom-based marking",
        "true");
    vector<string> strategies;
    vector<string> strategies_docs;
    strategies.push_back("fast_downward");
    strategies_docs.push_back(
        "select the atom (v, d) with the variable v that comes first in the Fast "
        "Downward variable ordering (which is based on the causal graph)");
    strategies.push_back("quick_skip");
    strategies_docs.push_back(
        "if possible, select an unsatisfied atom whose producers are already marked");
    strategies.push_back("static_small");
    strategies_docs.push_back("select the atom achieved by the fewest number of actions");
    strategies.push_back("dynamic_small");
    strategies_docs.push_back(
        "select the atom achieved by the fewest number of actions that are not "
        "yet part of the stubborn set");
    parser.add_enum_option<AtomSelectionStrategy>(
        "atom_selection_strategy",
        strategies,
        "Strategy for selecting unsatisfied atoms from action preconditions or "
        "the goal atoms. All strategies use the fast_downward strategy for "
        "breaking ties.",
        "quick_skip",
        strategies_docs);
    stubborn_sets::add_pruning_options(parser);

    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsAtomCentric>(opts);
}

static Plugin<PruningMethod> _plugin("atom_centric_stubborn_sets", _parse);
}
