#ifndef LANDMARKS_LANDMARK_FACTORY_ZHU_GIVAN_H
#define LANDMARKS_LANDMARK_FACTORY_ZHU_GIVAN_H

#include "landmark_factory.h"
#include "landmark_graph.h"
#include "landmark_types.h"
#include "../globals.h"

using namespace __gnu_cxx;

class LandmarkFactoryZhuGivan : public LandmarkFactory {
private:

    class plan_graph_node {
public:
        lm_set labels;
        inline bool reached() const {
            // NOTE: nodes are always labeled with itself,
            // if they have been reached
            return !labels.empty();
        }
    };

    typedef vector<vector<plan_graph_node> > proposition_layer;

    // triggers[i][j] is a list of operators that could reach/change
    // labels on some proposition, after proposition (i,j) has changed
    vector<vector<vector<int> > > triggers;

    void compute_triggers();

    // Note: must include operators that only have conditional effects
    vector<int> operators_without_preconditions;

    bool operator_applicable(const Operator &, const proposition_layer &) const;

    bool operator_cond_effect_fires(const vector<Prevail> &cond,
                                    const proposition_layer &layer) const;

    // Apply operator and propagate labels to next layer. Returns set of
    // propositions that:
    // (a) have just been reached OR (b) had their labels changed in next
    // proposition layer
    lm_set apply_operator_and_propagate_labels(const Operator &,
                                               const proposition_layer &current, proposition_layer &next) const;

    // Calculate the union of precondition labels of op, using the
    // labels from current
    lm_set union_of_precondition_labels(const Operator &op,
                                        const proposition_layer &current) const;

    // Calculate the union of precondition labels of a conditional effect,
    // using the labels from current
    lm_set union_of_condition_labels(const vector<Prevail> &cond,
                                     const proposition_layer &current) const;

    // Relaxed exploration, returns the last proposition layer
    // (the fixpoint) with labels
    proposition_layer build_relaxed_plan_graph_with_labels() const;

    // Extract landmarks from last proposition layer and add them to the
    // landmarks graph
    void extract_landmarks(const proposition_layer &last_prop_layer);

    // test if layer satisfies goal
    bool satisfies_goal_conditions(const proposition_layer &) const;

    void generate_landmarks();

public:
    LandmarkFactoryZhuGivan(const Options &opts);
    virtual ~LandmarkFactoryZhuGivan() {}
};

#endif
