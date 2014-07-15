set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wno-sign-compare -Wno-deprecated -ansi -pedantic -Werror")

include_directories(ext)


set(downward_SRC planner.cc
                axioms.cc
                causal_graph.cc
                combining_evaluator.cc
                domain_transition_graph.cc
                eager_search.cc
                enforced_hill_climbing_search.cc
                exact_timer.cc
                g_evaluator.cc
                globals.cc
                heuristic.cc
                int_packer.cc
                ipc_max_heuristic.cc
                iterated_search.cc
                lazy_search.cc
                legacy_causal_graph.cc
                max_evaluator.cc
                operator.cc
                operator_cost.cc
                option_parser.cc
                option_parser_util.cc
                segmented_vector.cc
                per_state_information.cc
                pref_evaluator.cc
                relaxation_heuristic.cc
                rng.cc
                search_engine.cc
                search_node_info.cc
                search_progress.cc
                search_space.cc
                state.cc
                state_id.cc
                state_registry.cc
                successor_generator.cc
                sum_evaluator.cc
                timer.cc
                utilities.cc
                weighted_evaluator.cc

                open_lists/alternation_open_list.cc
                open_lists/open_list_buckets.cc
                open_lists/pareto_open_list.cc
                open_lists/standard_scalar_open_list.cc
                open_lists/tiebreaking_open_list.cc
                open_lists/typed_open_list.cc

                additive_heuristic.cc
                blind_search_heuristic.cc
                cea_heuristic.cc
                cg_heuristic.cc cg_cache.cc
                ff_heuristic.cc
                goal_count_heuristic.cc
                hm_heuristic.cc
                lm_cut_heuristic.cc
                max_heuristic.cc

                merge_and_shrink/abstraction.cc
                merge_and_shrink/equivalence_relation.cc
                merge_and_shrink/label.cc
                merge_and_shrink/labels.cc
                merge_and_shrink/label_reducer.cc
                merge_and_shrink/merge_and_shrink_heuristic.cc
                merge_and_shrink/merge_dfp.cc
                merge_and_shrink/merge_linear.cc
                merge_and_shrink/merge_strategy.cc
                merge_and_shrink/shrink_bisimulation.cc
                merge_and_shrink/shrink_bucket_based.cc
                merge_and_shrink/shrink_fh.cc
                merge_and_shrink/shrink_random.cc
                merge_and_shrink/shrink_strategy.cc
                merge_and_shrink/variable_order_finder.cc

                landmarks/exploration.cc
                landmarks/h_m_landmarks.cc
                landmarks/lama_ff_synergy.cc
                landmarks/landmark_cost_assignment.cc
                landmarks/landmark_count_heuristic.cc
                landmarks/landmark_status_manager.cc
                landmarks/landmark_graph_merged.cc
                landmarks/landmark_graph.cc
                landmarks/landmark_factory.cc
                landmarks/landmark_factory_rpg_exhaust.cc
                landmarks/landmark_factory_rpg_sasp.cc
                landmarks/landmark_factory_zhu_givan.cc
                landmarks/util.cc

                pdbs/canonical_pdbs_heuristic.cc
                pdbs/dominance_pruner.cc
                pdbs/match_tree.cc
                pdbs/max_cliques.cc
                pdbs/pattern_generation_edelkamp.cc
                pdbs/pattern_generation_haslum.cc
                pdbs/pdb_heuristic.cc
                pdbs/util.cc
                pdbs/zero_one_pdbs_heuristic.cc
           )


            
            
            
add_executable(downward ${downward_SRC})

install(TARGETS downward DESTINATION bin/search/)
