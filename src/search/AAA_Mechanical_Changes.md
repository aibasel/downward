Mechanical changes: <TODO issue1082 remove this file>

- Choose one class with ByOptions-constructor [e.g. HMHeuristic] and announce on discord your choice to block it for others.
- Check all classes that are higher up in the hierarchy [e.g. Heuristic, Evaluator] starting with the root [e.g. Evaluator]:

	- Each of these base classes keep their ByOptions-constructor but get a `// TODO issue1082 remove this` comment (until all its children are updated, then it can be removed).
    - If not there yet, add flat constructors.
      - The flat constructor is not `explicit` if it receives more than one parameter.
      - It expects the arguments in the same order as its base class, new introduced parameter are added to the front.
    The new introduced parameters are the ones that have a `feature.add_option<Foo>(
        "my_bar",
        "help txt",
        "my_default");` in the ComponentFeature class.
      The inherited parameters are the one added to the feature by a `add_xyz_options_to_feature`
      - If the base class has a `add_<base>_options_to_feature`, then add a `get_<baseComponent>_parameters_from_options` function that returns a tuple with the parameters to call the constructor of that component (in the fitting order). [cf. src/search/evaluator.cc]
      - If the constructor uses `opts.get_unparsed_configs`, then adjust the `add_<component>_options_to_feature` to expect a string parameter to forward the fitting default string. You might have to overload the function that eventually calls 'add_<baseComponent>_options_to_feature' with such an additional string parameter. If you do, then mark the old one with `// TODO issue1082 remove` [cf. add_evaluator_options_to_feature].


- For the chosen class and all its children:
  - Replace the ByOptions-constructor with a flat constructor.
If needed, use one of the overloaded `add_<whatever>_options` calls to forward the default description. It should be the same as the feature_key (the string used to call the TypedFeature constructor in the XyzFeature constructor).
  - If not already there, add a `create_component` function in the XyzFeature. The body is simply:
    `return plugins::make_shared_from_arg_tuples<Xyz>(
opts.get<int>("a"),
// ... further new introduced parameters extracted from options
opts.get<double>("z"),
get_<A>_parameters_from_options(opts),
// ... in the oreder as the add_<A-Z>_options_to_feature was called above.
get_<Z>_parameter_from_options(opts)
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
- ❓ not checked if it should be updated

```
search
├── ✅ abstract_task.cc
├── ✅ abstract_task.h
├── ✅ algorithms
├── ✅ axioms.cc
├── ✅ axioms.h
├── ❌ cartesian_abstractions
├── ✅ cmake
├── ✅ CMakeLists.txt
├── ✅ command_line.cc
├── ✅ command_line.h
├── ✅ evaluation_context.cc
├── ✅ evaluation_context.h
├── ✅ evaluation_result.cc
├── ✅ evaluation_result.h
├── ✅ evaluator_cache.cc
├── ✅ evaluator_cache.h
├── ❓ evaluator.cc
├── ❓ evaluator.h
├── ❌ evaluators
├── ❓ heuristic.cc
├── ❓ heuristic.h
├── ✅ heuristics
├── ✅ landmarks
├── ❌ lp
├── ✅ merge_and_shrink
├── ✅ open_list_factory.cc
├── ✅ open_list_factory.h
├── ✅ open_list.h
├── ❌ open_lists
├── ✅ operator_cost.cc
├── ✅ operator_cost.h
├── ❌ operator_counting
├── ✅ operator_id.cc
├── ✅ operator_id.h
├── ✅ parser
├── ✅ pdbs
├── ✅ per_state_array.h
├── ✅ per_state_bitset.cc
├── ✅ per_state_bitset.h
├── ✅ per_state_information.h
├── ✅ per_task_information.h
├── ✅ plan_manager.cc
├── ✅ plan_manager.h
├── ✅ planner.cc
├── ✅ plugins
├── ❌ potentials
├── ❌ pruning
├── ❌ pruning_method.cc
├── ❌ pruning_method.h
├── ❌ search_algorithm.cc
├── ❌ search_algorithm.h
├── ❌ search_algorithms
├── ✅ search_node_info.cc
├── ✅ search_node_info.h
├── ✅ search_progress.cc
├── ✅ search_progress.h
├── ✅ search_space.cc
├── ✅ search_space.h
├── ✅ search_statistics.cc
├── ✅ search_statistics.h
├── ✅ state_id.cc
├── ✅ state_id.h
├── ✅ state_registry.cc
├── ✅ state_registry.h
├── ✅ task_id.h
├── ✅ task_proxy.cc
├── ✅ task_proxy.h
├── ❌ tasks
├── ✅ task_utils
└── ✅ utils


```
