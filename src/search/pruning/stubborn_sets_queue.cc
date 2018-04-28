#include "stubborn_sets_queue.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"
#include "../utils/memory.h"

using namespace std;

namespace stubborn_sets_queue {
StubbornSetsQueue::StubbornSetsQueue(const options::Options &opts)
    : StubbornSets(opts) {
}

void StubbornSetsQueue::initialize(const shared_ptr<AbstractTask> &task) {
    StubbornSets::initialize(task);
    cout << "pruning method: stubborn sets queue" << endl;

    TaskProxy task_proxy(*task);

    for (VariableProxy var : task_proxy.get_variables()) {
        marked_producers.emplace_back(var.get_domain_size(), false);
        marked_consumers.emplace_back(var.get_domain_size(), false);
    }

    compute_consumers(task_proxy);
}

void StubbornSetsQueue::compute_consumers(const TaskProxy &task_proxy) {
    consumers = utils::map_vector<vector<vector<int>>>(
        task_proxy.get_variables(), [](const VariableProxy &var) {
            return vector<vector<int>>(var.get_domain_size());
        });

    for (const OperatorProxy op : task_proxy.get_operators()) {
        int op_id = op.get_id();
        for (const FactProxy fact_proxy : op.get_preconditions()) {
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
    int domain_size = consumers[fact.var].size();
    for (int value = 0; value < domain_size; ++value) {
        if (value != fact.value) {
            enqueue_producers(FactPair(fact.var, value));
        }
    }
}

void StubbornSetsQueue::enqueue_sibling_consumers(const FactPair &fact) {
    int domain_size = consumers[fact.var].size();
    for (int value = 0; value < domain_size; ++value) {
        if (value != fact.value) {
            enqueue_consumers(FactPair(fact.var, value));
        }
    }
}

void StubbornSetsQueue::enqueue_nes(int op, const State &state) {
    FactPair fact = FactPair::no_fact;
    for (const FactPair &condition : sorted_op_preconditions[op]) {
        if (state[condition.var].get_value() != condition.value) {
            if (marked_producers[condition.var][condition.value]) {
                return;
            } else if (fact == FactPair::no_fact) {
                fact = condition;
            }
        }
    }

    assert(fact != FactPair::no_fact);
    assert(!marked_producers[fact.var][fact.value]);
    marked_producers[fact.var][fact.value] = true;
    producer_queue.push_back(fact);
}

void StubbornSetsQueue::enqueue_interferers(int op) {
    for (const FactPair &fact : sorted_op_preconditions[op]) {
        enqueue_sibling_producers(fact);
    }
    for (const FactPair &fact : sorted_op_effects[op]) {
        enqueue_sibling_producers(fact);
        enqueue_sibling_consumers(fact);
    }
}

void StubbornSetsQueue::initialize_stubborn_set(const State &state) {
    assert(producer_queue.empty());
    assert(consumer_queue.empty());
    for (auto &facts : marked_producers) {
        fill(facts.begin(), facts.end(), false);
    }
    for (auto &facts : marked_consumers) {
        fill(facts.begin(), facts.end(), false);
    }

    FactPair unsatisfied_goal = find_unsatisfied_goal(state);
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
        "Stubborn sets represent a state pruning method which computes a subset "
        "of applicable operators in each state such that completeness and "
        "optimality of the overall search is preserved. For details, see the "
        "following papers: "
        + utils::format_paper_reference(
            {"Yusra Alkhazraji", "Martin Wehrle", "Robert Mattmueller", "Malte Helmert"},
            "A Stubborn Set Algorithm for Optimal Planning",
            "http://ai.cs.unibas.ch/papers/alkhazraji-et-al-ecai2012.pdf",
            "Proceedings of the 20th European Conference on Artificial Intelligence "
            "(ECAI 2012)",
            "891-892",
            "IOS Press 2012")
        + utils::format_paper_reference(
            {"Martin Wehrle", "Malte Helmert"},
            "Efficient Stubborn Sets: Generalized Algorithms and Selection Strategies",
            "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS14/paper/view/7922/8042",
            "Proceedings of the 24th International Conference on Automated Planning "
            " and Scheduling (ICAPS 2014)",
            "323-331",
            "AAAI Press, 2014"));

    stubborn_sets::add_pruning_options(parser);

    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsQueue>(opts);
}

static PluginShared<PruningMethod> _plugin("stubborn_sets_queue", _parse);
}
