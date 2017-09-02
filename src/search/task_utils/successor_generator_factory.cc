#include "successor_generator_factory.h"

#include "successor_generator_internals.h"

#include "../task_proxy.h"

#include "../utils/collections.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace successor_generator {
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
    return utils::make_unique_ptr<GeneratorLeafList>(OperatorList());
}

GeneratorPtr SuccessorGeneratorFactory::construct_immediate(
    OperatorList operators) const {
    assert(!operators.empty());
    if (operators.size() == 1)
        return utils::make_unique_ptr<GeneratorLeafSingle>(operators.front());
    else
        return utils::make_unique_ptr<GeneratorLeafList>(move(operators));
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
    // TODO: Remove duplication of these using declarations.
    using OperatorList = std::list<OperatorID>;

    const SuccessorGeneratorFactory &factory;
    const GroupOperatorsBy group_by;
    const OperatorList operators;
    OperatorList::const_iterator pos;

    int get_current_group_key() const {
        assert(!done());
        int op_index = pos->get_index();
        const auto &cond_iter = factory.next_condition_by_op[op_index];
        if (cond_iter == factory.conditions[op_index].end()) {
            return -1;
        } else if (group_by == GroupOperatorsBy::VAR) {
            return cond_iter->var;
        } else {
            assert(group_by == GroupOperatorsBy::VALUE);
            return cond_iter->value;
        }
    }
public:
    explicit OperatorGrouper(
        const SuccessorGeneratorFactory &factory,
        GroupOperatorsBy group_by,
        OperatorList operators)
        : factory(factory),
          group_by(group_by),
          operators(move(operators)),
          pos(this->operators.begin()) {
    }

    bool done() const {
        return pos == operators.end();
    }

    pair<int, OperatorList> next() {
        assert(!done());
        int key = get_current_group_key();
        OperatorList op_group;
        do {
            op_group.push_back(*pos);
            ++pos;
        } while (!done() && get_current_group_key() == key);
        return make_pair(key, move(op_group));
    }
};


GeneratorPtr SuccessorGeneratorFactory::construct_recursive(
    OperatorList operator_queue) {
    VariablesProxy variables = task_proxy.get_variables();

    vector<GeneratorPtr> nodes;

    /*
      TODO: This could be declared more locally (where we have the resize call).
      We currently have it here to reuse the object for efficiency.
    */
    ValuesAndGenerators current_values_and_generators;

    OperatorGrouper grouper_by_var(*this, GroupOperatorsBy::VAR, operator_queue);
    // TODO: Replace done()/next() interface with something one can iterate over?
    while (!grouper_by_var.done()) {
        auto var_group = grouper_by_var.next();
        int cond_var = var_group.first;
        OperatorList var_operators = move(var_group.second);

        if (cond_var == -1) {
            // Handle a group of immediately applicable operators.
            nodes.push_back(construct_immediate(move(var_operators)));
        } else {
            // Handle a group of operators who share the first precondition variable.

            int var_domain = variables[cond_var].get_domain_size();
            current_values_and_generators.resize(var_domain);

            OperatorGrouper grouper_by_value(*this, GroupOperatorsBy::VALUE, var_operators);
            while (!grouper_by_value.done()) {
                auto value_group = grouper_by_value.next();
                int cond_value = value_group.first;
                OperatorList value_operators = move(value_group.second);

                // Advance the condition iterators. TODO: Can optimize this away later?
                for (OperatorID op : value_operators)
                    ++next_condition_by_op[op.get_index()];

                current_values_and_generators[cond_value] = construct_recursive(
                    move(value_operators));
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

GeneratorPtr SuccessorGeneratorFactory::create() {
    conditions.clear();
    next_condition_by_op.clear();

    OperatorsProxy operators = task_proxy.get_operators();
    // We need the iterators to conditions to be stable:
    conditions.reserve(operators.size());
    OperatorList all_operators;
    for (OperatorProxy op : operators) {
        Condition cond;
        cond.reserve(op.get_preconditions().size());
        for (FactProxy pre : op.get_preconditions()) {
            cond.push_back(pre.get_pair());
        }
        // Conditions must be ordered by variable id.
        sort(cond.begin(), cond.end());
        all_operators.push_back(OperatorID(op.get_id()));
        conditions.push_back(cond);
        next_condition_by_op.push_back(conditions.back().begin());
    }
    all_operators.sort([&](OperatorID op1, OperatorID op2) {
            return conditions[op1.get_index()] < conditions[op2.get_index()];
        });

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

    GeneratorPtr root = construct_recursive(move(all_operators));
    if (!root) {
        /* Task has no operators. Create empty successor generator
           so we don't have to check root for nullptr everywhere. */
        root = construct_empty();
    }
    return move(root);
}
}
