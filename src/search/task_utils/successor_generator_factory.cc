using Condition = vector<FactPair>;

class SuccessorGeneratorFactory {
    const TaskProxy &task_proxy;
    vector<Condition> conditions;
    vector<Condition::const_iterator> next_condition_by_op;

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
        int switch_var_id, list<OperatorID> &&operator_queue) {
        if (operator_queue.empty())
            return nullptr;

        VariablesProxy variables = task_proxy.get_variables();
        int num_variables = variables.size();

        while (true) {
            // Test if no further switch is necessary (or possible).
            if (switch_var_id == num_variables) {
                return construct_leaf(move(operator_queue));
            }

            VariableProxy switch_var = variables[switch_var_id];
            int number_of_children = switch_var.get_domain_size();

            vector<list<OperatorID>> operators_for_val(number_of_children);
            list<OperatorID> default_operators;
            list<OperatorID> applicable_operators;

            bool all_ops_are_immediate = true;
            bool var_is_interesting = false;

            /*
              TODO: Look into more efficient algorithms that don't iterate over all
              operators in the subtree at each layer of recursion. It also seems
              that we currently consider variables "interesting" as soon as there
              are immediately applicable operators, even if no preconditions on the
              variable exist. This could be improved.
            */
            while (!operator_queue.empty()) {
                OperatorID op_id = operator_queue.front();
                operator_queue.pop_front();
                int op_index = op_id.get_index();
                assert(utils::in_bounds(op_index, next_condition_by_op));
                Condition::const_iterator &cond_iter = next_condition_by_op[op_index];
                if (cond_iter == conditions[op_index].end()) {
                    var_is_interesting = true;
                    applicable_operators.push_back(op_id);
                } else {
                    assert(utils::in_bounds(
                               cond_iter - conditions[op_index].begin(),
                               conditions[op_index]));
                    all_ops_are_immediate = false;
                    FactPair fact = *cond_iter;
                    if (fact.var == switch_var.get_id()) {
                        var_is_interesting = true;
                        while (cond_iter != conditions[op_index].end() &&
                               cond_iter->var == switch_var.get_id()) {
                            ++cond_iter;
                        }
                        operators_for_val[fact.value].push_back(op_id);
                    } else {
                        default_operators.push_back(op_id);
                    }
                }
            }

            if (all_ops_are_immediate) {
                return construct_leaf(move(applicable_operators));
            } else if (var_is_interesting) {
                return construct_branch(
                    switch_var_id,
                    move(operators_for_val),
                    move(default_operators),
                    move(applicable_operators));
            } else {
                // this switch var can be left out because no operator depends on it
                ++switch_var_id;
                default_operators.swap(operator_queue);
            }
        }
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
