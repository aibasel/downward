#include "successor_generator_factory.h"

#include "successor_generator_internals.h"

#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>

using namespace std;

/*
  The key ideas of the construction algorithm are as follows.

  Initially, we sort the preconditions of the operators
  lexicographically.

  We then group the operators by the *first variable* to be tested,
  forming a group for each variable and possibly a special group for
  operators with no precondition. (This special group forms the base
  case of the recursion, leading to a leaf node in the successor
  generator.) Each group forms a contiguous subrange of the overall
  operator sequence.

  We then further group each subsequence (except for the special one)
  by the *value* that the given variable is tested against, again
  obtaining contiguous subranges of the original operator sequence.

  For each of these subranges, we "pop" the first condition and then
  recursively apply the same construction algorithm to compute a child
  successor generator for this subset of operators.

  Successor generators for different values of the same variable are
  then combined into a switch node, and the generated switch nodes for
  different variables are combined into a fork node.

  The important property of lexicographic sorting that we exploit here
  is that if the original sequence is sorted, then all subsequences we
  must consider recursively are also sorted. Crucially, this remains
  true when we pop the first condition, because this popping always
  happens within a subsequence where all operators have the *same*
  first condition.

  To make the implementation more efficient, we do not physically pop
  conditions but only keep track of how many conditions have been
  dealt with so far, which is simply the recursion depth of the
  "construct_recursive" function.

  Because we only consider contiguous subranges of the operator
  sequence and never need to modify any of the data describing the
  operators, we can simply keep track of the current operator sequence
  by a begin and end index into the overall operator sequence.
*/

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
    int switch_var_id, ValuesAndGenerators values_and_generators) const {
    VariablesProxy variables = task_proxy.get_variables();
    int var_domain = variables[switch_var_id].get_domain_size();
    int num_children = values_and_generators.size();

    assert(num_children > 0);

    if (num_children == 1) {
        int value = values_and_generators[0].first;
        GeneratorPtr generator = move(values_and_generators[0].second);
        return utils::make_unique_ptr<GeneratorSwitchSingle>(
            switch_var_id, value, move(generator));
    }

    int vector_bytes = utils::estimate_vector_bytes<GeneratorPtr>(var_domain);
    int hash_bytes = utils::estimate_unordered_map_bytes<int, GeneratorPtr>(num_children);
    if (hash_bytes < vector_bytes) {
        unordered_map<int, GeneratorPtr> generator_by_value;
        for (auto &item : values_and_generators)
            generator_by_value[item.first] = move(item.second);
        return utils::make_unique_ptr<GeneratorSwitchHash>(
            switch_var_id, move(generator_by_value));
    } else {
        vector<GeneratorPtr> generator_by_value(var_domain);
        for (auto &item : values_and_generators)
            generator_by_value[item.first] = move(item.second);
        return utils::make_unique_ptr<GeneratorSwitchVector>(
            switch_var_id, move(generator_by_value));
    }
}

GeneratorPtr SuccessorGeneratorFactory::construct_recursive(
    int depth, OperatorRange range) const {
    vector<GeneratorPtr> nodes;
    OperatorGrouper grouper_by_var(
        operator_infos, depth, GroupOperatorsBy::VAR, range);
    while (!grouper_by_var.done()) {
        auto var_group = grouper_by_var.next();
        int var = var_group.first;
        OperatorRange var_range = var_group.second;

        if (var == -1) {
            // Handle a group of immediately applicable operators.
            nodes.push_back(construct_leaf(var_range));
        } else {
            // Handle a group of operators sharing the first precondition variable.
            ValuesAndGenerators values_and_generators;
            OperatorGrouper grouper_by_value(
                operator_infos, depth, GroupOperatorsBy::VALUE, var_range);
            while (!grouper_by_value.done()) {
                auto value_group = grouper_by_value.next();
                int value = value_group.first;
                OperatorRange value_range = value_group.second;

                values_and_generators.emplace_back(
                    value, construct_recursive(depth + 1, value_range));
            }

            nodes.push_back(construct_switch(
                                var, move(values_and_generators)));
        }
    }
    return construct_fork(move(nodes));
}

static vector<FactPair> build_sorted_precondition(const OperatorProxy &op) {
    vector<FactPair> precond;
    precond.reserve(op.get_preconditions().size());
    for (FactProxy pre : op.get_preconditions())
        precond.emplace_back(pre.get_pair());
    // Preconditions must be sorted by variable.
    sort(precond.begin(), precond.end());
    return precond;
}

GeneratorPtr SuccessorGeneratorFactory::create() {
    OperatorsProxy operators = task_proxy.get_operators();
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
    operator_infos.clear();
    return root;
}
}
