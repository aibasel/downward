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

    std::unique_ptr<LandmarkStatusManager> lm_status_manager;
    std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;

    void initialize(const plugins::Options &opts);
    void compute_landmark_graph(const plugins::Options &opts);

    /*
      Unlike most landmark-related code, this function takes the
      task-transformation of the state, not the original one (i.e., not
      *ancestor_state*). This is because updating the landmark status manager
      happens in *compute_heuristic(...)* before *get_heuristic_value(...)*
      is called. Here, we only compute a heuristic value based on the
      information in the landmark status manager, which does not require the
      state at this point. The only reason we need this argument is to guarantee
      goal-awareness of the LM-count heuristic which does not hold under the
      current function used for progressing the landmark statuses. Checking
      whether a state is a goal state requires the task-transformed state.
    */
    virtual int get_heuristic_value(const State &state) = 0;

    void generate_preferred_operators(
        const State &state, const BitsetView &reached);
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    explicit LandmarkHeuristic(const plugins::Options &opts);

    virtual void get_path_dependent_evaluators(
        std::set<Evaluator *> &evals) override {
        evals.insert(this);
    }

    static void add_options_to_feature(plugins::Feature &feature);

    virtual void notify_initial_state(const State &initial_state) override;
    virtual void notify_state_transition(const State &parent_state,
                                         OperatorID op_id,
                                         const State &state) override;
};
}

#endif
