Mechanical changes: <TODO issue1082 remove this file>

- Choose one class with ByOptions-constructor [e.g. HMHeuristic] and announce on discord your choice to block it for others.
- Check all classes that are higher up in the hierarchy [e.g. Heuristic, Evaluator] starting with the root [e.g. Evaluator]:

	- Each of these base classes keep their ByOptions-constructor but get a `// TODO issue1082 remove this` comment (until all its children are updated, then it can be removed).
    - If not there yet, add flat constructors.
      - The flat constructor is not `explicit` if it receives more than one parameter.
      - It expects the arguments in the same order as its base class, new introduced parameter are added to the front.
    The new introduced parameters are the ones extracted from the options object by the ByOptions-constructor (not by the base class constructor).
      - If the base class has parameters for its constructor, then add a `get_<baseComponent>_parameters_from_options` function that returns a shared pointer to a tuple with the parameters to call the constructor of that component (in the fitting order). [cf. src/search/heuristic.cc]
      - If the constructor uses `opts.get_unparsed_configs`, then adjust the 'add_<component>_options_to_feature' to expect a string parameter to forward the fitting default string. You might have to overload the function that eventually calls 'add_<baseComponent>_options_to_feature' with such an additional string parameter. If you do, then mark the old one with `// TODO issue1082 remove` [cf. Heuristic::add_options_to_feature].


- For the chosen class and all its children:
  - Replace the ByOptions-constructor with a flat constructor.
If needed, use one of the overloaded `add_<whatever>_options` calls to forward the default description. It should be the same as the feature_key (the string used to call the TypedFeature constructor in the XyzFeature constructor).
  - If not already there, add a `create_component` function in the XyzFeature. The body is simply:
    `return plugins::make_shared_from_args_tuple_and_args<Xyz>(
get_<baseComponent>_parameters_from_options(opts),
opts.get<int>("a"),
// ... further new introduced parameters extracted from options
opts.get<double>("z")
);`
[cf. src/search/heuristics/hm_heuristic.cc]
  - **ELSE** there is probably something acting on a copy of the options object.
  use that opts_copy to call `plugins::make_shared_from_args_tuple_and_args<Xyz>` like above but with `opts_copy`.
[cf. merge_and_shrink/merge_scoring_function_miasm.cc]


Confirm with `./builds/release/bin/downward --help --txt2tags <feature_key>` that the order and parameter names are the same as they would appear in the wiki as in the constructor. If they are not, then change the order of the 'add_options' calls and/or the parameters in the flat constructor.
DriveBy cleanup: Do we need the destructor? and is there a reason why we cannot use = default? Our convention is to leave it out if it is using its default.


Change constuctor calls in other files.
If possible remove the plugins include as plugins::Options is not needed anymore.
[cf. src/search/cartesian_abstractions/utils.cc]

Mark in the list below what you updated: 

- ❌ not updated yet
- ✅ updated (or no need to update)
- no mark = not checked if it should be updated

```
search
├── abstract_task.cc
├── abstract_task.h
├── algorithms
├── axioms.cc
├── axioms.h
├── cartesian_abstractions
├── cmake
├── CMakeLists.txt
├── command_line.cc
├── command_line.h
├── evaluation_context.cc
├── evaluation_context.h
├── evaluation_result.cc
├── evaluation_result.h
├── evaluator_cache.cc
├── evaluator_cache.h
├── evaluator.cc
├── evaluator.h
├── evaluators
├── heuristic.cc
├── heuristic.h
├── heuristics
│   ├── additive_heuristic.cc✅
│   ├── additive_heuristic.h✅
│   ├── array_pool.h
│   ├── blind_search_heuristic.cc
│   ├── blind_search_heuristic.h
│   ├── cea_heuristic.cc
│   ├── cea_heuristic.h
│   ├── cg_cache.cc
│   ├── cg_cache.h
│   ├── cg_heuristic.cc
│   ├── cg_heuristic.h
│   ├── domain_transition_graph.cc
│   ├── domain_transition_graph.h
│   ├── ff_heuristic.cc✅
│   ├── ff_heuristic.h✅
│   ├── goal_count_heuristic.cc
│   ├── goal_count_heuristic.h
│   ├── hm_heuristic.cc✅
│   ├── hm_heuristic.h✅
│   ├── lm_cut_heuristic.cc
│   ├── lm_cut_heuristic.h
│   ├── lm_cut_landmarks.cc
│   ├── lm_cut_landmarks.h
│   ├── max_heuristic.cc✅
│   ├── max_heuristic.h✅
│   ├── relaxation_heuristic.cc✅
│   └── relaxation_heuristic.h✅
├── landmarks
├── lp
├── merge_and_shrink✅
│   ├── distances.cc✅
│   ├── distances.h✅
│   ├── factored_transition_system.cc✅
│   ├── factored_transition_system.h✅
│   ├── fts_factory.cc✅
│   ├── fts_factory.h✅
│   ├── label_reduction.cc✅
│   ├── label_reduction.h✅
│   ├── labels.cc✅
│   ├── labels.h✅
│   ├── merge_and_shrink_algorithm.cc✅
│   ├── merge_and_shrink_algorithm.h✅
│   ├── merge_and_shrink_heuristic.cc✅
│   ├── merge_and_shrink_heuristic.h✅
│   ├── merge_and_shrink_representation.cc✅
│   ├── merge_and_shrink_representation.h✅
│   ├── merge_scoring_function.cc✅
│   ├── merge_scoring_function_dfp.cc✅ 
│   ├── merge_scoring_function_dfp.h✅ 
│   ├── merge_scoring_function_goal_relevance.cc✅ 
│   ├── merge_scoring_function_goal_relevance.h✅ 
│   ├── merge_scoring_function.h✅
│   ├── merge_scoring_function_miasm.cc✅ 
│   ├── merge_scoring_function_miasm.h✅ 
│   ├── merge_scoring_function_miasm_utils.cc✅
│   ├── merge_scoring_function_miasm_utils.h✅
│   ├── merge_scoring_function_single_random.cc✅ 
│   ├── merge_scoring_function_single_random.h✅ 
│   ├── merge_scoring_function_total_order.cc✅ 
│   ├── merge_scoring_function_total_order.h✅ 
│   ├── merge_selector.cc✅
│   ├── merge_selector.h✅
│   ├── merge_selector_score_based_filtering.cc✅
│   ├── merge_selector_score_based_filtering.h✅
│   ├── merge_strategy.cc✅
│   ├── merge_strategy_factory.cc✅
│   ├── merge_strategy_factory.h✅
│   ├── merge_strategy_factory_precomputed.cc✅
│   ├── merge_strategy_factory_precomputed.h✅
│   ├── merge_strategy_factory_sccs.cc✅
│   ├── merge_strategy_factory_sccs.h✅
│   ├── merge_strategy_factory_stateless.cc✅
│   ├── merge_strategy_factory_stateless.h✅
│   ├── merge_strategy.h✅
│   ├── merge_strategy_precomputed.cc✅
│   ├── merge_strategy_precomputed.h✅
│   ├── merge_strategy_sccs.cc✅
│   ├── merge_strategy_sccs.h✅
│   ├── merge_strategy_stateless.cc✅
│   ├── merge_strategy_stateless.h✅
│   ├── merge_tree.cc✅
│   ├── merge_tree_factory.cc✅
│   ├── merge_tree_factory.h✅
│   ├── merge_tree_factory_linear.cc✅
│   ├── merge_tree_factory_linear.h✅
│   ├── merge_tree.h✅
│   ├── shrink_bisimulation.cc✅
│   ├── shrink_bisimulation.h✅
│   ├── shrink_bucket_based.cc✅
│   ├── shrink_bucket_based.h✅
│   ├── shrink_fh.cc✅
│   ├── shrink_fh.h✅
│   ├── shrink_random.cc✅
│   ├── shrink_random.h✅
│   ├── shrink_strategy.cc✅
│   ├── shrink_strategy.h✅
│   ├── transition_system.cc✅
│   ├── transition_system.h✅
│   ├── types.cc✅
│   ├── types.h✅
│   ├── utils.cc✅
│   └── utils.h✅
├── open_list_factory.cc
├── open_list_factory.h
├── open_list.h
├── open_lists
├── operator_cost.cc
├── operator_cost.h
├── operator_counting
├── operator_id.cc
├── operator_id.h
├── parser
├── pdbs✅
├── per_state_array.h
├── per_state_bitset.cc
├── per_state_bitset.h
├── per_state_information.h
├── per_task_information.h
├── plan_manager.cc
├── plan_manager.h
├── planner.cc
├── plugins
├── potentials
├── pruning
├── pruning_method.cc
├── pruning_method.h
├── search_algorithm.cc
├── search_algorithm.h
├── search_algorithms
├── search_node_info.cc
├── search_node_info.h
├── search_progress.cc
├── search_progress.h
├── search_space.cc
├── search_space.h
├── search_statistics.cc
├── search_statistics.h
├── state_id.cc
├── state_id.h
├── state_registry.cc
├── state_registry.h
├── task_id.h
├── task_proxy.cc
├── task_proxy.h
├── tasks
├── task_utils
└── utils

  
```