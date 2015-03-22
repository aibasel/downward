#ifndef LANDMARKS_LAMA_FF_SYNERGY_H
#define LANDMARKS_LAMA_FF_SYNERGY_H

#include "../heuristic.h"
#include "../utilities.h"

// TODO: Get rid of these includes after moving all the code into the
// cc file.
#include "../evaluation_context.h"
#include "../evaluation_result.h"

#include "exploration.h"
#include "landmark_count_heuristic.h"

#include <memory>

class LamaFFSynergy {
    class LamaMasterHeuristic : public Heuristic {
        LamaFFSynergy *synergy;

    protected:
        virtual int compute_heuristic(const GlobalState &/*state*/) {
            ABORT("This method should never be called.");
        }

    public:
        explicit LamaMasterHeuristic(LamaFFSynergy *synergy)
            : Heuristic(Heuristic::default_options()),
              synergy(synergy) {
        }

        virtual ~LamaMasterHeuristic() override = default;

        virtual EvaluationResult compute_result(
            EvaluationContext &eval_context) override {
            synergy->compute_heuristics(eval_context);
            return synergy->lama_result;
        }

        virtual bool reach_state(
            const GlobalState &parent_state, const GlobalOperator &op,
            const GlobalState &state) override {
            return synergy->lama_reach_state(parent_state, op, state);
        }
    };

    class FFSlaveHeuristic : public Heuristic {
        LamaFFSynergy *synergy;
        LamaMasterHeuristic *master;

    protected:
        virtual int compute_heuristic(const GlobalState &/*state*/) {
            ABORT("This method should never be called.");
        }

    public:
        explicit FFSlaveHeuristic(LamaFFSynergy *synergy,
                                  LamaMasterHeuristic *master)
            : Heuristic(Heuristic::default_options()),
              synergy(synergy),
              master(master) {
        }

        virtual EvaluationResult compute_result(
            EvaluationContext &eval_context) override {
            /* Asking for the master's heuristic value triggers both
               heuristic computations if they have not been computed
               yet. If they have been computed yet, then both
               heuristic values are already cached, and this is just a
               quick lookup. In either case, the result is
               subsequently available in the synergy object. */
            eval_context.get_heuristic_value_or_infinity(master);
            return synergy->ff_result;
        }

        virtual ~FFSlaveHeuristic() override = default;
    };

    friend class LamaMasterHeuristic;
    friend class FFSlaveHeuristic;

    std::unique_ptr<LamaMasterHeuristic> lama_master_heuristic;
    std::unique_ptr<FFSlaveHeuristic> ff_slave_heuristic;
    const bool lm_pref;
    const bool lm_admissible;
    const bool lm_optimal;
    const bool use_action_landmarks;

    LandmarkCountHeuristic *lama_heuristic;
    Exploration *exploration;

    EvaluationResult lama_result;
    EvaluationResult ff_result;

    bool lama_reach_state(
        const GlobalState &parent_state, const GlobalOperator &op,
        const GlobalState &state);

    void compute_heuristics(EvaluationContext &eval_context);

public:
    explicit LamaFFSynergy(const Options &opts);
    ~LamaFFSynergy() = default;

    Heuristic *get_lama_heuristic_proxy() {
        return lama_master_heuristic.get();
    }

    Heuristic *get_ff_heuristic_proxy() {
        return ff_slave_heuristic.get();
    }
};

#endif
