#include "successor_generator_factory.h"

#include "successor_generator_internals.h"

#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <list>

using namespace std;

namespace successor_generator {

// TODO: Decide where to declare this, and declare it only once.
using Condition = vector<FactPair>;
using OperatorInfos = vector<OperatorInfo>;


struct OperatorRange {
    int begin;
    int end;

    OperatorRange(int begin, int end)
        : begin(begin), end(end) {
    }

    bool empty() const {
        return begin == end;
    }
};


class OperatorInfo {
    /*
      op_index and precondition are "conceptually const", but we
      need to support assignment/swapping to support sorting
      vector<OperatorInfo>.
    */
    int op_index;
    Condition precondition;
    Condition::const_iterator next_condition;
public:
    OperatorInfo(int op_index, Condition precondition)
        : op_index(op_index),
          precondition(move(precondition)),
          next_condition(this->precondition.begin()) {
    }

    bool operator<(const OperatorInfo &other) const {
        return precondition < other.precondition;
    }

    void advance_condition() {
        assert(next_condition != precondition.end());
        ++next_condition;
    }

    // If there are no further conditions, this returns -1.
    int get_current_var() const {
        if (next_condition == precondition.end()) {
            return -1;
        } else {
            return next_condition->var;
        }
    }

    // It is illegal to call this if there are no further conditions.
    int get_current_value() const {
        assert(next_condition != precondition.end());
        return next_condition->value;
    }

    OperatorID get_op() const {
        return OperatorID(op_index);
    }
};


SuccessorGeneratorFactory::~SuccessorGeneratorFactory() = default;

GeneratorPtr SuccessorGeneratorFactory::construct_chain(
    vector<GeneratorPtr> &nodes) const {
    if (nodes.empty()) {
        return nullptr;
    } else {
        GeneratorPtr result = move(nodes.at(0));
        for (size_t i = 1; i < nodes.size(); ++i)
            result = utils::make_unique_ptr<GeneratorFork>(
                move(result), move(nodes.at(i)));
        return result;
    }
}

GeneratorPtr SuccessorGeneratorFactory::construct_empty() const {
    return utils::make_unique_ptr<GeneratorLeafList>(list<OperatorID>());
}

GeneratorPtr SuccessorGeneratorFactory::construct_immediate(
    OperatorRange range) const {
    assert(!range.empty());
    list<OperatorID> operators;
    while (range.begin != range.end) {
        operators.emplace_back(operator_infos[range.begin].get_op());
        ++range.begin;
    }

    if (operators.size() == 1) {
        return utils::make_unique_ptr<GeneratorLeafSingle>(operators.front());
    } else {
        return utils::make_unique_ptr<GeneratorLeafList>(move(operators));
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

/*
  TODO: The two partitioning classes are quite similar. We could
  change "var" and "variable" to "value" everywhere in the first one
  and would get an acceptable implementation of the second. It would
  contain unnecessary code for the case value == -1, which cannot
  arise, but that's not a big deal.

  Hence, should we templatize this? Needs benchmarking.
*/

enum class GroupOperatorsBy {
    VAR,
    VALUE
};

class OperatorGrouper {
    const OperatorInfos &operator_infos;
    const GroupOperatorsBy group_by;
    OperatorRange range;

    const OperatorInfo &get_current_op_info() const {
        assert(!range.empty());
        return operator_infos[range.begin];
    }

    int get_current_group_key() const {
        const OperatorInfo &op_info = get_current_op_info();
        if (group_by == GroupOperatorsBy::VAR) {
            return op_info.get_current_var();
        } else {
            assert(group_by == GroupOperatorsBy::VALUE);
            return op_info.get_current_value();
        }
    }
public:
    explicit OperatorGrouper(
        const OperatorInfos &operator_infos,
        GroupOperatorsBy group_by,
        OperatorRange range)
        : operator_infos(operator_infos),
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


GeneratorPtr SuccessorGeneratorFactory::construct_recursive(
    OperatorRange range) {
    VariablesProxy variables = task_proxy.get_variables();

    vector<GeneratorPtr> nodes;

    /*
      TODO: This could be declared more locally (where we have the resize call).
      We currently have it here to reuse the object for efficiency.
    */
    ValuesAndGenerators current_values_and_generators;

    OperatorGrouper grouper_by_var(operator_infos, GroupOperatorsBy::VAR, range);
    // TODO: Replace done()/next() interface with something one can iterate over?
    while (!grouper_by_var.done()) {
        auto var_group = grouper_by_var.next();
        int cond_var = var_group.first;
        OperatorRange var_range = var_group.second;

        if (cond_var == -1) {
            // Handle a group of immediately applicable operators.
            nodes.push_back(construct_immediate(var_range));
        } else {
            // Handle a group of operators who share the first precondition variable.

            int var_domain = variables[cond_var].get_domain_size();
            current_values_and_generators.resize(var_domain);

            OperatorGrouper grouper_by_value(operator_infos, GroupOperatorsBy::VALUE, var_range);
            while (!grouper_by_value.done()) {
                auto value_group = grouper_by_value.next();
                int cond_value = value_group.first;
                OperatorRange value_range = value_group.second;

                // Advance the condition iterators. TODO: Optimize this away later?
                for (int i = value_range.begin; i < value_range.end; ++i)
                    operator_infos[i].advance_condition();

                current_values_and_generators[cond_value] = construct_recursive(value_range);
            }

            nodes.push_back(construct_switch(
                                cond_var, move(current_values_and_generators)));
        }
    }
    return construct_chain(nodes);
}

SuccessorGeneratorFactory::SuccessorGeneratorFactory(
    const TaskProxy &task_proxy)
    : task_proxy(task_proxy) {
}

static Condition build_sorted_precondition(const OperatorProxy &op) {
    Condition cond;
    cond.reserve(op.get_preconditions().size());
    for (FactProxy pre : op.get_preconditions())
        cond.emplace_back(pre.get_pair());
    // Conditions must be ordered by variable id.
    sort(cond.begin(), cond.end());
    return cond;
}

GeneratorPtr SuccessorGeneratorFactory::create() {
    operator_infos.clear();

    OperatorsProxy operators = task_proxy.get_operators();
    // We need the iterators to conditions to be stable:
    operator_infos.reserve(operators.size());
    for (OperatorProxy op : operators) {
        operator_infos.emplace_back(
            op.get_id(), build_sorted_precondition(op));
    }
    /* Use stable_sort rather than sort for reproducibility.
       This amounts to breaking ties by operator ID. */
    stable_sort(operator_infos.begin(), operator_infos.end());

    /*
      TODO: conditions and next_condition_by_op are organized by
      the actual operator number, but for better cache locality,
      it would probably make sense to organize them in the order
      we process them instead, i.e., by the order imposed by
      "all_operators".

      This is perhaps most easily done by using our own internal
      operator representation, and perhaps we should use one that
      supports popping conditions from the front, so that we don't
      need next_condition_by_op any more. (But perhaps it's better
      to stick with vectors and an index for efficiency reasons.
      Can hide this inside the implementation of the internal
      operator representation by giving it a pop_front method that
      may be implemented lazily by advancing an
      iterator/incrementing a counter.)

      We need to move far less data around if we exploit that

      A) all operator lists we create are contiguous subsequences
      of all_operators, so it would be sufficient to store
      indices into all_operators (which should then be a
      vector) rather than create lots of explicit lists. (We
      can create explicit lists as we build the immediate
      generators, but before that, we don't really need to.)

      B) all iterators in next_condition_by_op that are used in
      one call to construct_recursive() are advanced by the
      same amount, so it would be sufficient for
      construct_recursive to store this one number rather than
      maintaining a separate iterator for each operator.
    */

    OperatorRange full_range(0, operator_infos.size());
    GeneratorPtr root = construct_recursive(full_range);
    if (!root) {
        /* Task has no operators. Create empty successor generator
           so we don't have to check root for nullptr everywhere. */
        root = construct_empty();
    }
    return move(root);
}
}
