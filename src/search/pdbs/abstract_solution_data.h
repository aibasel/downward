#ifndef PDBS_ABSTRACT_SOLUTION_DATA_H
#define PDBS_ABSTRACT_SOLUTION_DATA_H

#include "pattern_database.h"

#include "../tasks/pdb_abstracted_task.h"

#include <set>
#include <utility>

namespace successor_generator {
class  SuccessorGenerator;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace pdbs {
struct SearchNode;

class AbstractSolutionData {
    std::shared_ptr<PatternDatabase> pdb;
    tasks::PDBAbstractedTask abstracted_task;
    TaskProxy abs_task_proxy;
    std::set<int> blacklist;
    std::vector<std::vector<OperatorID>> wildcard_plan;
    bool wildcard_plans;
    bool is_solvable;
    bool solved;

    std::vector<State> extract_state_sequence(
        const SearchNode &goal_node) const;
    std::vector<State> bfs_for_improving_state(
        const successor_generator::SuccessorGenerator &succ_gen,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        const int f_star,
        std::shared_ptr<SearchNode> &current_node) const;
    std::vector<State> steepest_ascent_enforced_hillclimbing(
        const successor_generator::SuccessorGenerator &succ_gen,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        utils::Verbosity verbosity) const;
    void turn_state_sequence_into_plan(
        const successor_generator::SuccessorGenerator &succ_gen,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        utils::Verbosity verbosity,
        const std::vector<State> &state_sequence);
public:
    AbstractSolutionData(
        const std::shared_ptr<AbstractTask> &concrete_task,
        const Pattern &pattern,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        bool wildcard_plans,
        utils::Verbosity verbosity);

    const Pattern &get_pattern() const {
        return pdb->get_pattern();
    }

    const std::shared_ptr<PatternDatabase> &get_pdb() {
        return pdb;
    }

    bool solution_exists() const {
        return is_solvable;
    }

    void mark_as_solved() {
        solved = true;
    }

    bool is_solved() {
        return solved;
    }

    const std::vector<std::vector<OperatorID>> &get_plan() const {
        return wildcard_plan;
    }

    OperatorID get_concrete_op_id_for_abs_op_id(
        OperatorID abs_op_id, const std::shared_ptr<AbstractTask> &parent_task) const;

    int compute_plan_cost() const;

    void print_plan() const;
};
}


#endif
