#ifndef LEARNING_SELECTIVE_MAX_HEURISTIC_H
#define LEARNING_SELECTIVE_MAX_HEURISTIC_H

#include "../heuristic.h"
#include "../globals.h"
#include "../search_space.h"
#include "../timer.h"
#include "classifier.h"
#include "feature_extractor.h"
#include "state_space_sample.h"
#include <math.h>

class SelectiveMaxHeuristic : public Heuristic {
private:
    bool dead_end;
protected:
    vector<Heuristic *> heuristics;
    int *hvalue;
    bool *computed;
    int num_always_calc;

    //statistics
    int num_evals;
    int correct0_classifications;
    int incorrect0_classifications;
    int correct1_classifications;
    int incorrect1_classifications;

    int training_set_size;
    int num_learn_from_no_confidence;
    double *total_computation_time;
    double *avg_time;
    ExactTimer total_classification_time;
    ExactTimer total_training_time;

    int *num_evaluated;
    int *num_winner;
    int *num_only_winner;
    double branching_factor;
    int num_pairs;
    bool test_mode;
    bool perfect_mode;
    bool zero_threshold;



    //int current_min_training_set;

    // parameters
    int min_training_set;
    double conf_threshold;
    bool uniform_sampling;
    bool random_selection;
    double alpha;
    bool retime_heuristics;

    int num_heuristics;

    // classification
    state_space_sample_t state_space_sample_type;

    classifier_t classifier_type;
    Classifier **classifiers;

    FeatureExtractor *feature_extractor;
    FeatureExtractorFactory feature_extractor_types;

    int *expensive;
    int *cheap;
    double *threshold;
    double *dist;
    double *heuristic_conf;
    int num_classes;


    virtual void initialize();
    virtual int compute_heuristic(const State &state);

    void learn(const State &state);
    void classify(const State &state);
    int eval_heuristic(const State &state, int index, bool count);
    int calc_max();

    //int diff2tag(int diff) {return diff + max_diff;}
    //int tag2diff(int tag) {return tag - max_diff;}

    void train();
    void reset_statistics();
public:
    SelectiveMaxHeuristic(const Options &opts);
    virtual ~SelectiveMaxHeuristic();

    inline void add_heuristic(Heuristic *h) {
        heuristics.push_back(h);
    }

    inline void set_num_always_calc(int num) {
        num_always_calc = num;
    }

    inline void set_uniform_sampling(bool uniform) {
        uniform_sampling = uniform;
    }

    inline void set_confidence(double conf) {
        conf_threshold = conf;
    }

    inline void set_training_set_size(int size) {
        min_training_set = size;
    }

    inline void set_random_selection(bool r) {
        random_selection = r;
    }

    inline void set_zero_threshold(bool z) {
        zero_threshold = z;
    }

    inline void set_alpha(double alph) {
        alpha = alph;
    }

    inline void set_classifier(classifier_t ct) {
        classifier_type = ct;
    }

    inline void set_state_space_sample(state_space_sample_t st) {
        state_space_sample_type = st;
    }

    inline FeatureExtractorFactory &get_feature_extractors() {
        return feature_extractor_types;
    }

    inline void set_retime_heuristics(bool retime) {
        retime_heuristics = retime;
    }

    virtual void print_statistics() const;
    virtual bool reach_state(const State &parent_state, const Operator &op,
                             const State &state);
};

#endif
