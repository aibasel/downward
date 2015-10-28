#ifndef MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H

#include <forward_list>
#include <memory>
#include <vector>

class Distances;
class HeuristicRepresentation;
class Labels;
class State;
class TaskProxy;
class Timer;
class TransitionSystem;


class FactoredTransitionSystem {
    static const int PRUNED_STATE = -1;
    std::shared_ptr<Labels> labels;
    // Entries with nullptr have been merged.
    /*
      TODO: after splitting transition system into several parts, we may
      want to change all transition system pointers into unique_ptr.
    */
    std::vector<TransitionSystem *> transition_systems;
    std::vector<std::unique_ptr<HeuristicRepresentation>> heuristic_representations;
    std::vector<std::unique_ptr<Distances>> distances;
    int final_index;
    bool solvable;
    // TODO: add something like "current_index"? for shrink classes e.g.

    void compute_distances_and_prune(int index);
    void discard_states(int index, const std::vector<bool> &to_be_pruned_states);

    bool is_index_valid(int index) const;
    bool is_component_valid(int index) const;
    bool is_finalized() const {
        return final_index != -1;
    }
public:
    FactoredTransitionSystem(
        std::shared_ptr<Labels> labels,
        std::vector<TransitionSystem *> &&transition_systems,
        std::vector<std::unique_ptr<HeuristicRepresentation>> &&heuristic_representations,
        std::vector<std::unique_ptr<Distances>> &&distances);
    FactoredTransitionSystem(FactoredTransitionSystem &&other);
    ~FactoredTransitionSystem();

    // No copying or assignment.
    FactoredTransitionSystem(const FactoredTransitionSystem &) = delete;
    FactoredTransitionSystem &operator=(
        const FactoredTransitionSystem &) = delete;

    TransitionSystem &get_ts(int index) {
        return *transition_systems[index];
    }

    // temporary method while we're transitioning from the old code;
    // TODO: remove
    const std::vector<TransitionSystem *> &get_vector() {
        return transition_systems;
    }

    void label_reduction(std::pair<int, int> merge_indices);
    bool apply_abstraction(int index, const std::vector<std::forward_list<int>> &collapsed_groups);
    int merge(const TaskProxy &task_proxy, int index1, int index2);
    void finalize(int index = -1);
    bool is_solvable() const {
        return solvable;
    }
    int get_size(int index) const;
    int get_cost(const State &state) const;

    void statistics(int index, const Timer &timer) const;
    void dump(int index) const;

    /*
      TODO: We probably want to get rid of the methods below that just
      forward to distances, by giving the users of these methods
      access to the the distances object instead.

      This might also help address a possible performance problem we
      might have at the moment, now that these methods are no longer
      inlined here. (To be able to inline them, we would need to
      include distances.h here, which we would rather not.)
    */
    int get_max_f(int index) const; // used by shrink strategies
    int get_max_g(int index) const; // unused
    int get_max_h(int index) const; // used by shrink strategies
    int get_init_distance(int index, int state) const; // used by shrink_fh
    int get_goal_distance(int index, int state) const; // used by shrink strategies and merge_dfp
    int get_num_labels() const; // used by merge_dfp
};

#endif
