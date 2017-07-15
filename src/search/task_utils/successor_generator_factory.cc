#include "successor_generator_factory.h"

#include "successor_generator_internals.h"

#include "../task_proxy.h"

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

    int vector_size = estimate_vector_size<GeneratorPtr>(num_values);
    int hash_size = estimate_unordered_map_size<int, GeneratorPtr>(num_non_null);
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
    OperatorList operator_queue) {
    VariablesProxy variables = task_proxy.get_variables();

    vector<GeneratorPtr> nodes;

    int last_var = -1;
    int last_value = -1;
    OperatorList current_operators;
    ValuesAndGenerators current_values_and_generators;
    for (OperatorID op : operator_queue) {
        int op_index = op.get_index();

        Condition::const_iterator &cond_iter = next_condition_by_op[op_index];

        int cond_var;
        int cond_value;

        /*
          Consume next condition of op (if any) and set cond_var/cond_value.
          Set both to -1 if there are no further conditions.
        */
        if (cond_iter == conditions[op_index].end()) {
            assert(last_var == -1 && last_value == -1);
            cond_var = -1;
            cond_value = -1;
        } else {
            // Obtain condition and consume it.
            assert(utils::in_bounds(
                       cond_iter - conditions[op_index].begin(),
                       conditions[op_index]));
            cond_var = cond_iter->var;
            cond_value = cond_iter->value;
            assert(make_pair(cond_var, cond_value) >=
                   make_pair(last_var, last_value));
            ++cond_iter;
            assert(cond_iter == conditions[op_index].end() ||
                   cond_iter->var > cond_var);
        }

        if (cond_var != last_var || cond_value != last_value) {
            if (last_var != -1) {
                /*
                  We just finished the operator range for last_var/last_value.
                  Recursively generate a successor generator for the range.
                */
                assert(!current_operators.empty());
                current_values_and_generators[last_value] = construct_recursive(
                    move(current_operators));
                assert(current_operators.empty());
            }

            if (cond_var != last_var) {
                if (last_var == -1) {
                    /*
                      We just finished the operator range with the immediate operators.
                      Create a successor generator for them.
                    */
                    assert(cond_var != last_var && cond_value != last_value);
                    if (!current_operators.empty()) {
                        nodes.push_back(construct_immediate(
                                            move(current_operators)));
                    }
                    assert(current_operators.empty());
                } else {
                    /*
                      We just finished the whole operator range for last_var.
                      Create a switch generator for the variable.
                    */
                    nodes.push_back(
                        construct_switch(
                            last_var, move(current_values_and_generators)));
                    assert(current_values_and_generators.empty());
                }

                int var_domain = variables[cond_var].get_domain_size();
                current_values_and_generators.resize(var_domain);
                last_var = cond_var;
            }
            last_value = cond_value;
        }
        current_operators.push_back(op);
    }

    /*
      TODO: The following is code duplication. Deal with this,
      perhaps by using a sentinel past the last var_no?
    */
    if (last_var == -1) {
        if (!current_operators.empty()) {
            nodes.push_back(construct_immediate(
                                move(current_operators)));
        }
    } else {
        current_values_and_generators[last_value] = construct_recursive(
            move(current_operators));
        assert(current_operators.empty());
        nodes.push_back(
            construct_switch(
                last_var, move(current_values_and_generators)));
        assert(current_values_and_generators.empty());
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
        /* Task has no operators. Create dummy leaf,
           so we don't have to check root for nullptr everywhere. */
        OperatorList no_operators;
        root = construct_immediate(no_operators);
    }
    return move(root);
}
}
