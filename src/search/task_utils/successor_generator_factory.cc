#include "successor_generator_factory.h"

#include "successor_generator_internals.h"

#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace successor_generator {
struct OperatorRange {
    int begin;
    int end;

    OperatorRange(int begin, int end)
        : begin(begin), end(end) {
    }

    bool empty() const {
        return begin == end;
    }

    int span() const {
        return end - begin;
    }
};


class OperatorInfo {
    /*
      The attributes are not const because we must support
      assignment/swapping to sort vector<OperatorInfo>.
    */
    OperatorID op;
    vector<FactPair> precondition;
public:
    OperatorInfo(OperatorID op, vector<FactPair> precondition)
        : op(op),
          precondition(move(precondition)) {
    }

    bool operator<(const OperatorInfo &other) const {
        return precondition < other.precondition;
    }

    OperatorID get_op() const {
        return op;
    }

    // Returns -1 as a past-the-end sentinel.
    int get_var(int depth) const {
        if (depth == static_cast<int>(precondition.size())) {
            return -1;
        } else {
            return precondition[depth].var;
        }
    }

    int get_value(int depth) const {
        return precondition[depth].value;
    }
};


enum class GroupOperatorsBy {
    VAR,
    VALUE
};


class OperatorGrouper {
    const vector<OperatorInfo> &operator_infos;
    const int depth;
    const GroupOperatorsBy group_by;
    OperatorRange range;

    const OperatorInfo &get_current_op_info() const {
        assert(!range.empty());
        return operator_infos[range.begin];
    }

    int get_current_group_key() const {
        const OperatorInfo &op_info = get_current_op_info();
        if (group_by == GroupOperatorsBy::VAR) {
            return op_info.get_var(depth);
        } else {
            assert(group_by == GroupOperatorsBy::VALUE);
            return op_info.get_value(depth);
        }
    }
public:
    explicit OperatorGrouper(
        const vector<OperatorInfo> &operator_infos,
        int depth,
        GroupOperatorsBy group_by,
        OperatorRange range)
        : operator_infos(operator_infos),
          depth(depth),
          group_by(group_by),
          range(range) {
    }

    bool done() const {
        return range.empty();
    }

    pair<int, OperatorRange> next() {
        assert(!range.empty());
        int key = get_current_group_key();
        int group_begin = range.begin;
        do {
            ++range.begin;
        } while (!range.empty() && get_current_group_key() == key);
        OperatorRange group_range(group_begin, range.begin);
        return make_pair(key, group_range);
    }
};


SuccessorGeneratorFactory::SuccessorGeneratorFactory(
    const TaskProxy &task_proxy)
    : task_proxy(task_proxy) {
}

SuccessorGeneratorFactory::~SuccessorGeneratorFactory() = default;

GeneratorPtr SuccessorGeneratorFactory::construct_fork(
    vector<GeneratorPtr> nodes) const {
    int size = nodes.size();
    if (size == 1) {
        return move(nodes.at(0));
    } else if (size == 2) {
        return utils::make_unique_ptr<GeneratorForkBinary>(
            move(nodes.at(0)), move(nodes.at(1)));
    } else {
        /* This general case includes the case size == 0, which can
           (only) happen for the root for tasks with no operators. */
        return utils::make_unique_ptr<GeneratorForkMulti>(move(nodes));
    }
}

GeneratorPtr SuccessorGeneratorFactory::construct_leaf(
    OperatorRange range) const {
    assert(!range.empty());
    vector<OperatorID> operators;
    operators.reserve(range.span());
    while (range.begin != range.end) {
        operators.emplace_back(operator_infos[range.begin].get_op());
        ++range.begin;
    }

    if (operators.size() == 1) {
        return utils::make_unique_ptr<GeneratorLeafSingle>(operators.front());
    } else {
        return utils::make_unique_ptr<GeneratorLeafVector>(move(operators));
    }
}

GeneratorPtr SuccessorGeneratorFactory::construct_switch(
    int switch_var_id, ValuesAndGenerators generator_for_value) const {
    int num_values = generator_for_value.size();
    int num_null = count(generator_for_value.begin(),
                         generator_for_value.end(), nullptr);
    int num_non_null = num_values - num_null;
    assert(num_non_null > 0);

    if (num_non_null == 1) {
        for (int value = 0; value < num_values; ++value) {
            if (generator_for_value[value]) {
                return utils::make_unique_ptr<GeneratorSwitchSingle>(
                    switch_var_id, value, move(generator_for_value[value]));
            }
        }
        assert(false);
    }

    int vector_size = utils::estimate_vector_size<GeneratorPtr>(num_values);
    int hash_size = utils::estimate_unordered_map_size<int, GeneratorPtr>(num_non_null);
    if (hash_size < vector_size) {
        unordered_map<int, GeneratorPtr> map;
        for (int value = 0; value < num_values; ++value) {
            if (generator_for_value[value]) {
                map[value] = move(generator_for_value[value]);
            }
        }
        return utils::make_unique_ptr<GeneratorSwitchHash>(
            switch_var_id, move(map));
    } else {
        return utils::make_unique_ptr<GeneratorSwitchVector>(
            switch_var_id, move(generator_for_value));
    }
}


GeneratorPtr SuccessorGeneratorFactory::construct_recursive(
    int depth, OperatorRange range) {
    VariablesProxy variables = task_proxy.get_variables();

    vector<GeneratorPtr> nodes;

    /*
      TODO: This could be declared more locally (where we have the resize call).
      We currently have it here to reuse the object for efficiency.
    */
    ValuesAndGenerators current_values_and_generators;

    OperatorGrouper grouper_by_var(
        operator_infos, depth, GroupOperatorsBy::VAR, range);
    while (!grouper_by_var.done()) {
        auto var_group = grouper_by_var.next();
        int cond_var = var_group.first;
        OperatorRange var_range = var_group.second;

        if (cond_var == -1) {
            // Handle a group of immediately applicable operators.
            nodes.push_back(construct_leaf(var_range));
        } else {
            // Handle a group of operators sharing the first precondition variable.

            int var_domain = variables[cond_var].get_domain_size();
            current_values_and_generators.resize(var_domain);

            OperatorGrouper grouper_by_value(
                operator_infos, depth, GroupOperatorsBy::VALUE, var_range);
            while (!grouper_by_value.done()) {
                auto value_group = grouper_by_value.next();
                int cond_value = value_group.first;
                OperatorRange value_range = value_group.second;

                current_values_and_generators[cond_value] = construct_recursive(
                    depth + 1, value_range);
            }

            nodes.push_back(construct_switch(
                                cond_var, move(current_values_and_generators)));
        }
    }
    return construct_fork(move(nodes));
}

static vector<FactPair> build_sorted_precondition(const OperatorProxy &op) {
    vector<FactPair> cond;
    cond.reserve(op.get_preconditions().size());
    for (FactProxy pre : op.get_preconditions())
        cond.emplace_back(pre.get_pair());
    // Conditions must be ordered by variable id.
    sort(cond.begin(), cond.end());
    return cond;
}

GeneratorPtr SuccessorGeneratorFactory::create() {
    OperatorsProxy operators = task_proxy.get_operators();
    operator_infos.clear();
    operator_infos.reserve(operators.size());
    for (OperatorProxy op : operators) {
        operator_infos.emplace_back(
            OperatorID(op.get_id()), build_sorted_precondition(op));
    }
    /* Use stable_sort rather than sort for reproducibility.
       This amounts to breaking ties by operator ID. */
    stable_sort(operator_infos.begin(), operator_infos.end());

    OperatorRange full_range(0, operator_infos.size());
    GeneratorPtr root = construct_recursive(0, full_range);
    return move(root);
}
}
