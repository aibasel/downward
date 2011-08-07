#ifndef LANDMARKS_LAMA_FF_SYNERGY_H
#define LANDMARKS_LAMA_FF_SYNERGY_H

#include "../heuristic.h"
#include "exploration.h"
#include "landmark_count_heuristic.h"

class LamaFFSynergy {
    class HeuristicProxy : public Heuristic {
protected:
        LamaFFSynergy *synergy;
        bool is_first_proxy;

        virtual void initialize();
        virtual int get_heuristic_value() = 0;
        virtual void get_preferred_operators(std::vector<const Operator *> &result) = 0;
public:
        HeuristicProxy(LamaFFSynergy *synergy_);
        virtual ~HeuristicProxy() {}

        virtual int compute_heuristic(const State &state) {
            if (is_first_proxy)
                synergy->compute_heuristics(state);
            return get_heuristic_value();
        }
    };

    class FFHeuristicProxy : public HeuristicProxy {
        virtual int get_heuristic_value() {
            return synergy->ff_heuristic_value;
        }
        virtual void get_preferred_operators(std::vector<const Operator *> &result) {
            synergy->get_ff_preferred_operators(result);
        }
public:
        FFHeuristicProxy(LamaFFSynergy *synergy_) : HeuristicProxy(synergy_) {}
    };

    class LamaHeuristicProxy : public HeuristicProxy {
        virtual int get_heuristic_value() {
            return synergy->lama_heuristic_value;
        }
        virtual void get_preferred_operators(std::vector<const Operator *> &result) {
            synergy->get_lama_preferred_operators(result);
        }
public:
        LamaHeuristicProxy(LamaFFSynergy *synergy_) : HeuristicProxy(synergy_) {}
        virtual bool reach_state(const State &parent_state, const Operator &op,
                                 const State &state) {
            return synergy->lama_reach_state(parent_state, op, state);
        }
    };

    friend class HeuristicProxy;
    friend class LamaHeuristicProxy;
    friend class FFHeuristicProxy;

    LamaHeuristicProxy lama_heuristic_proxy;
    FFHeuristicProxy ff_heuristic_proxy;
    LandmarkCountHeuristic *lama_heuristic;
    Exploration *exploration;
    bool lm_pref;
    bool lm_admissible;
    bool lm_optimal;
    bool use_action_landmarks;
    int lm_type;
    std::vector<const Operator *> lama_preferred_operators;
    std::vector<const Operator *> ff_preferred_operators;
    bool initialized;

    void initialize() {
        // Value change only serves to determine first proxy.
        initialized = true;
    }

    bool lama_reach_state(const State &parent_state, const Operator &op,
                          const State &state);

    void compute_heuristics(const State &);
    void get_lama_preferred_operators(std::vector<const Operator *> &result);
    void get_ff_preferred_operators(std::vector<const Operator *> &result);
public:
    LamaFFSynergy(const Options &opts);
    ~LamaFFSynergy() {}

    int lama_heuristic_value;
    int ff_heuristic_value;

    Heuristic *get_ff_heuristic_proxy() {
        return &ff_heuristic_proxy;
    }

    Heuristic *get_lama_heuristic_proxy() {
        return &lama_heuristic_proxy;
    }
};

#endif
