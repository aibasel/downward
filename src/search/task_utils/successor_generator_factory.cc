class SuccessorGeneratorFactory {
    using Condition = vector<FactPair>;
    using OperatorList = list<OperatorID>;
    // TODO: Later switch to the following pair-based representation?
    // using ValuesAndOperators = vector<pair<int, OperatorList>>;
    using ValuesAndOperators = vector<OperatorList>;

    const TaskProxy &task_proxy;
    vector<Condition> conditions;
    vector<Condition::const_iterator> next_condition_by_op;

    unique_ptr<GeneratorBase> construct_chain(vector<unique_ptr<GeneratorBase>> &&nodes) {
        if (nodes.empty()) {
            return nullptr;
        } else {
            unique_ptr<GeneratorBase> result = move(nodes.at(0));
            for (size_t i = 1; i < nodes.size(); ++i)
                result = utils::make_unique_ptr<GeneratorFork>(move(result), move(nodes.at(i)));
            return result;
        }
    }

    unique_ptr<GeneratorBase> construct_immediate(list<OperatorID> &&operators) {
        /*
          TODO: This is identical to the old construct_leaf. Don't
          keep both methods, and think some more about the names of
          the methods and the different successor generator versions.
        */
        assert(!operators.empty());
        if (operators.size() == 1)
            return utils::make_unique_ptr<GeneratorLeafSingle>(operators.front());
        else
            return utils::make_unique_ptr<GeneratorLeafList>(move(operators));
    }

    unique_ptr<GeneratorBase> construct_branch_new(
        int variable,
        ValuesAndOperators values_and_operators) {
        return construct_branch(variable, move(values_and_operators),
                                OperatorList(), OperatorList());
    }

    unique_ptr<GeneratorBase> construct_leaf(list<OperatorID> &&operators) {
        assert(!operators.empty());
        if (operators.size() == 1) {
            return utils::make_unique_ptr<GeneratorLeafSingle>(operators.front());
        } else {
            return utils::make_unique_ptr<GeneratorLeafList>(move(operators));
        }
    }

    unique_ptr<GeneratorBase> construct_switch(
        int switch_var_id,
        vector<list<OperatorID>> &&operators_for_value) {
        int num_values = operators_for_value.size();
        vector<unique_ptr<GeneratorBase>> generator_for_value;
        generator_for_value.reserve(num_values);
        int num_non_zero = 0;
        for (list<OperatorID> &ops : operators_for_value) {
            unique_ptr<GeneratorBase> value_generator = construct_recursive(
                switch_var_id + 1, move(ops));
            if (value_generator) {
                ++num_non_zero;
            }
            generator_for_value.push_back(move(value_generator));
        }

        if (num_non_zero == 1) {
            for (int value = 0; value < num_values; ++value) {
                if (generator_for_value[value]) {
                    return utils::make_unique_ptr<GeneratorSwitchSingle>(
                        switch_var_id, value, move(generator_for_value[value]));
                }
            }
            assert(false);
        }
        int vector_size = estimate_vector_size<unique_ptr<GeneratorBase>>(num_values);
        int hash_size = estimate_unordered_map_size<int, unique_ptr<GeneratorBase>>(num_non_zero);
        if (hash_size < vector_size) {
            unordered_map<int, unique_ptr<GeneratorBase>> map;
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

    unique_ptr<GeneratorBase> construct_branch(
        int switch_var_id,
        vector<list<OperatorID>> &&operators_for_value,
        list<OperatorID> &&default_operators,
        list<OperatorID> &&applicable_operators) {
        unique_ptr<GeneratorBase> switch_generator = construct_switch(
            switch_var_id, move(operators_for_value));
        assert(switch_generator);

        unique_ptr<GeneratorBase> non_immediate_generator = nullptr;
        if (default_operators.empty()) {
            non_immediate_generator = move(switch_generator);
        } else {
            unique_ptr<GeneratorBase> default_generator = construct_recursive(
                switch_var_id + 1, move(default_operators));
            non_immediate_generator = utils::make_unique_ptr<GeneratorFork>(
                move(switch_generator), move(default_generator));
        }

        if (applicable_operators.empty()) {
            return non_immediate_generator;
        } else {
            return utils::make_unique_ptr<GeneratorImmediate>(
                move(applicable_operators), move(non_immediate_generator));
        }
    }

    unique_ptr<GeneratorBase> construct_recursive(
        int /*switch_var_id*/, list<OperatorID> &&operator_queue) {
        /*
          TODO: the new implementation doesn't need switch_var_id any
          more, so get rid of it.
        */

        /*
          TODO: Does the new implementation need this test? I think we
          usually won't call this function with an empty operator
          queue, except perhaps at the root, and hence we probably
          don't need to speed this case up.
        */
        if (operator_queue.empty())
            return nullptr;

        VariablesProxy variables = task_proxy.get_variables();

        vector<unique_ptr<GeneratorBase>> nodes;

        int last_var = -1;
        int last_value = -1;
        OperatorList immediate_operators;
        OperatorList current_operators;
        ValuesAndOperators current_values_and_operators;
        for (OperatorID op : operator_queue) {
            int op_index = op.get_index();

            Condition::const_iterator &cond_iter = next_condition_by_op[op_index];
            if (cond_iter == conditions[op_index].end()) {
                /*
                  TODO: Can we handle this by setting cond_var = -1 and cond_value = -1
                  and then handle both cases uniformly?
                */
                // No more conditions for this operator.
                assert(last_var == -1 && last_value == -1);
                immediate_operators.push_back(op);
            } else {
                // Obtain condition and consume it.
                assert(utils::in_bounds(
                           cond_iter - conditions[op_index].begin(),
                           conditions[op_index]));
                int cond_var = cond_iter->var;
                int cond_value = cond_iter->value;
                ++cond_iter;
                assert(cond_iter == conditions[op_index].end() ||
                       cond_iter->var > cond_var);
                if (cond_var == last_var) {
                    if (cond_value == last_value) {
                        current_operators.push_back(op);
                    } else {
                        assert(cond_value > last_value);
                        /*
                          TODO: With other data structures, this would
                          be the time to finalize the previous operator list
                          if it needs finalization.
                        */
                        assert(!current_operators.empty());
                        current_values_and_operators[last_value] = move(current_operators);
                        last_value = cond_value;
                        // TODO: Reduce code duplication.
                        current_operators.push_back(op);
                    }
                } else {
                    assert(cond_var > last_var);

                    if (!current_operators.empty()) {
                        // It can only be empty if last_var == -1.
                        current_values_and_operators[last_value] = move(current_operators);
                    }

                    if (last_var == -1) {
                        if (!immediate_operators.empty())
                            nodes.push_back(construct_immediate(
                                                move(immediate_operators)));
                    } else {
                        nodes.push_back(
                            construct_branch_new(
                                last_var, move(current_values_and_operators)));
                    }

                    last_var = cond_var;
                    last_value = cond_value;

                    assert(current_values_and_operators.empty());

                    int var_domain = variables[cond_var].get_domain_size();
                    current_values_and_operators.resize(var_domain);

                    // TODO: Reduce code duplication.
                    current_operators.push_back(op);
                }
            }
        }

        // TODO: Deal with code duplication, perhaps by using a sentinel past the last var_no?
        if (!current_operators.empty()) {
            // It can only be empty if last_var == -1.
            current_values_and_operators[last_value] = move(current_operators);
        }

        if (last_var == -1) {
            if (!immediate_operators.empty())
                nodes.push_back(construct_immediate(
                                    move(immediate_operators)));
        } else {
            nodes.push_back(
                construct_branch_new(
                    last_var, move(current_values_and_operators)));
        }

        return construct_chain(move(nodes));
    }
public:
    explicit SuccessorGeneratorFactory(const TaskProxy &task_proxy)
        : task_proxy(task_proxy) {
    }

    unique_ptr<GeneratorBase> create() {
        conditions.clear();
        next_condition_by_op.clear();

        OperatorsProxy operators = task_proxy.get_operators();
        // We need the iterators to conditions to be stable:
        conditions.reserve(operators.size());
        list<OperatorID> all_operators;
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
        */

        unique_ptr<GeneratorBase> root = construct_recursive(0, move(all_operators));
        if (!root) {
            /* Task is trivially unsolvable. Create dummy leaf,
               so we don't have to check root for nullptr everywhere. */
            list<OperatorID> no_applicable_operators;
            root = utils::make_unique_ptr<GeneratorLeafList>(
                move(no_applicable_operators));
        }
        return move(root);
    }
};
