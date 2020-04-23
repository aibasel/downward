#include "stubborn_sets_queue.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"
#include "../utils/memory.h"

using namespace std;

namespace stubborn_sets_queue {
StubbornSetsQueue::StubbornSetsQueue(const options::Options &opts)
    : StubbornSets(opts),
      mark_variables(opts.get<bool>("mark_variables")),
      variable_ordering(static_cast<VariableOrdering>(opts.get_enum("variable_ordering"))) {
}

void StubbornSetsQueue::initialize(const shared_ptr<AbstractTask> &task) {
    StubbornSets::initialize(task);
    cout << "pruning method: stubborn sets queue" << endl;

    TaskProxy task_proxy(*task);

    for (VariableProxy var : task_proxy.get_variables()) {
        marked_producers.emplace_back(var.get_domain_size(), false);
        marked_consumers.emplace_back(var.get_domain_size(), false);
    }
    if (mark_variables) {
        int num_variables = task_proxy.get_variables().size();
        marked_producer_variables.resize(num_variables, MARKED_VALUES_NONE);
        marked_consumer_variables.resize(num_variables, MARKED_VALUES_NONE);
    }

    compute_consumers(task_proxy);
}

void StubbornSetsQueue::compute_consumers(const TaskProxy &task_proxy) {
    consumers = utils::map_vector<vector<vector<int>>>(
        task_proxy.get_variables(), [](const VariableProxy &var) {
            return vector<vector<int>>(var.get_domain_size());
        });

    for (OperatorProxy op : task_proxy.get_operators()) {
        int op_id = op.get_id();
        for (FactProxy fact_proxy : op.get_preconditions()) {
            FactPair fact = fact_proxy.get_pair();
            consumers[fact.var][fact.value].push_back(op_id);
        }
    }
}

bool StubbornSetsQueue::operator_is_applicable(int op, const State &state) {
    return find_unsatisfied_precondition(op, state) == FactPair::no_fact;
}

void StubbornSetsQueue::enqueue_producers(const FactPair &fact) {
    if (!marked_producers[fact.var][fact.value]) {
        marked_producers[fact.var][fact.value] = true;
        producer_queue.push_back(fact);
    }
}

void StubbornSetsQueue::enqueue_consumers(const FactPair &fact) {
    if (!marked_consumers[fact.var][fact.value]) {
        marked_consumers[fact.var][fact.value] = true;
        consumer_queue.push_back(fact);
    }
}

void StubbornSetsQueue::enqueue_sibling_producers(const FactPair &fact) {
    int dummy_mark = MARKED_VALUES_NONE;
    int &mark = mark_variables ? marked_producer_variables[fact.var] : dummy_mark;
    if (mark == MARKED_VALUES_NONE) {
        int domain_size = consumers[fact.var].size();
        for (int value = 0; value < domain_size; ++value) {
            if (value != fact.value) {
                enqueue_producers(FactPair(fact.var, value));
            }
        }
        mark = fact.value;
    } else if (mark != MARKED_VALUES_ALL && mark != fact.value) {
        enqueue_producers(FactPair(fact.var, mark));
        mark = MARKED_VALUES_ALL;
    }
}

void StubbornSetsQueue::enqueue_sibling_consumers(const FactPair &fact) {
    int dummy_mark = MARKED_VALUES_NONE;
    int &mark = mark_variables ? marked_consumer_variables[fact.var] : dummy_mark;
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

FactPair StubbornSetsQueue::select_fact(
    const vector<FactPair> &facts, const State &state) const {
    FactPair fact = FactPair::no_fact;
    if (variable_ordering == VariableOrdering::FAST_DOWNWARD) {
        fact = stubborn_sets::find_unsatisfied_condition(facts, state);
    } else if (variable_ordering == VariableOrdering::MINIMIZE_SS) {
        /*
          If the precondition contains an unsatisfied fact whose producers are
          already marked, choose it. In this case, there's nothing else to do.
          Otherwise, choose the first unsatisfied precondition and enqueue its
          producers.
        */
        for (const FactPair &condition : facts) {
            if (state[condition.var].get_value() != condition.value) {
                if (marked_producers[condition.var][condition.value]) {
                    return FactPair::no_fact;
                } else if (fact == FactPair::no_fact) {
                    fact = condition;
                }
            }
        }
        assert(!marked_producers[fact.var][fact.value]);
    } else if (variable_ordering == VariableOrdering::STATIC_SMALL) {
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
    } else if (variable_ordering == VariableOrdering::DYNAMIC_SMALL) {
        int min_count = numeric_limits<int>::max();
        for (const FactPair &condition : facts) {
            if (state[condition.var].get_value() != condition.value) {
                const vector<int> &ops = achievers[condition.var][condition.value];
                int count = count_if(ops.begin(), ops.end(), [this](int op) {return !stubborn[op];});
                if (count < min_count) {
                    fact = condition;
                    min_count = count;
                }
            }
        }
    } else {
        ABORT("Unknown variable ordering");
    }
    assert(fact != FactPair::no_fact);
    return fact;
}

void StubbornSetsQueue::enqueue_nes(int op, const State &state) {
    FactPair fact = select_fact(sorted_op_preconditions[op], state);
    if (fact != FactPair::no_fact) {
        enqueue_producers(fact);
    }
}

/*
  Enqueue an operator o2 iff op interferes with o2.

  o1 interferes strongly with o2 iff o1 disables o2, or o2 disables o1, or o1 and o2 conflict.
  o1 interferes weakly with o2 iff o1 disables o2, or o1 and o2 conflict.
*/
void StubbornSetsQueue::enqueue_interferers(int op) {
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

void StubbornSetsQueue::initialize_stubborn_set(const State &state) {
    assert(producer_queue.empty());
    assert(consumer_queue.empty());
    // Reset datastructures from previous call.
    for (auto &facts : marked_producers) {
        fill(facts.begin(), facts.end(), false);
    }
    for (auto &facts : marked_consumers) {
        fill(facts.begin(), facts.end(), false);
    }
    if (mark_variables) {
        fill(marked_producer_variables.begin(), marked_producer_variables.end(),
             MARKED_VALUES_NONE);
        fill(marked_consumer_variables.begin(), marked_consumer_variables.end(),
             MARKED_VALUES_NONE);
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

void StubbornSetsQueue::handle_stubborn_operator(const State &state, int op) {
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
        "Stubborn sets queue",
        "Stubborn sets are a state pruning method which computes a subset "
        "of applicable operators in each state such that completeness and "
        "optimality of the overall search is preserved. Stubborn sets were "
        "adapted to classical planning in the following paper: " +
        utils::format_conference_reference(
            {"Yusra Alkhazraji", "Martin Wehrle", "Robert Mattmueller", "Malte Helmert"},
            "A Stubborn Set Algorithm for Optimal Planning",
            "https://ai.dmi.unibas.ch/papers/alkhazraji-et-al-ecai2012.pdf",
            "Proceedings of the 20th European Conference on Artificial Intelligence "
            "(ECAI 2012)",
            "891-892",
            "IOS Press",
            "2012") +
        "Previous stubborn set implementations mainly track information about operators. "
        "In contrast, this implementation focuses on atomic propositions, which often "
        "speeds up the computation on IPC benchmarks. For details, see" +
        utils::format_conference_reference(
            {"Gabriele Röger", "Malte Helmert", "Jendrik Seipp", "Silvan Sievers"},
            "An Atom-Centric Perspective on Stubborn Sets",
            "https://ai.dmi.unibas.ch/papers/roeger-et-al-socs2020.pdf",
            "Proceedings of the 13th Annual Symposium on Combinatorial Search "
            "(SoCS 2020)",
            "",
            "AAAI Press",
            "2020"));
    parser.add_option<bool>(
        "mark_variables",
        "use variable-based marking in addition to fact-based marking",
        "false");
    vector<string> orderings;
    vector<string> ordering_docs;
    orderings.push_back("fast_downward");
    ordering_docs.push_back("");
    orderings.push_back("minimize_ss");
    ordering_docs.push_back("select NES to opportunistically minimize stubborn set");
    orderings.push_back("static_small");
    ordering_docs.push_back("");
    orderings.push_back("dynamic_small");
    ordering_docs.push_back("");
    parser.add_enum_option(
        "variable_ordering",
        orderings,
        "variable ordering",
        "fast_downward",
        ordering_docs);
    stubborn_sets::add_pruning_options(parser);

    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsQueue>(opts);
}

static Plugin<PruningMethod> _plugin("stubborn_sets_queue", _parse);
}
