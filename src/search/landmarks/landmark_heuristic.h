#ifndef LANDMARKS_LANDMARK_HEURISTIC_H
#define LANDMARKS_LANDMARK_HEURISTIC_H

# include "../heuristic.h"

class BitsetView;

namespace successor_generator {
class SuccessorGenerator;
}

namespace landmarks {
class LandmarkFactory;
class LandmarkGraph;
class LandmarkNode;
class LandmarkStatusManager;

class LandmarkHeuristic : public Heuristic {
protected:
    std::shared_ptr<LandmarkGraph> lm_graph;
    const bool use_preferred_operators;
    const bool conditional_effects_supported;

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;

    virtual int get_heuristic_value(const State &state) = 0;

    bool landmark_is_interesting(
        const State &state, const BitsetView &reached,
        LandmarkNode &lm_node, bool all_lms_reached) const;
    void generate_preferred_operators(
        const State &state, const BitsetView &reached);
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkHeuristic(const plugins::Options &opts);

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    static void add_options_to_parser(plugins::OptionParser &parser);

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};
}

#endif
