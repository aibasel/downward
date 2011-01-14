#include "selective_max_heuristic.h"
#include "../successor_generator.h"
#include "maximum_heuristic.h"
#include "naive_bayes_classifier.h"
#include "state_vars_feature_extractor.h"
#include "composite_feature_extractor.h"
#include "AODE.h"
#include "probe_state_space_sample.h"
#include "PDB_state_space_sample.h"
#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>


static ScalarEvaluatorPlugin selective_max_heuristic_plugin(
    "selmax", SelectiveMaxHeuristic::create);


SelectiveMaxHeuristic::SelectiveMaxHeuristic(const HeuristicOptions &options)
    : Heuristic(options), num_always_calc(0) {
    // default parameter
    min_training_set = 100;
    conf_threshold = 0.6;
    uniform_sampling = false;
    random_selection = false;
    alpha = 1.0;
    classifier_type = NB;
    state_space_sample_type = Probe;
    retime_heuristics = false;

    // statistics
    num_evals = 0;
    correct0_classifications = 0;
    incorrect0_classifications = 0;
    correct1_classifications = 0;
    incorrect1_classifications = 0;
    training_set_size = 0;
    num_learn_from_no_confidence = 0;
    // timing
    total_classification_time.reset();
    total_classification_time.stop();
    total_training_time.reset();
    total_training_time.stop();


    //test_mode = false;
    //perfect_mode = g_test_mode;

    test_mode = false;    //g_test_mode;
    perfect_mode = false;
    zero_threshold = false;
}

SelectiveMaxHeuristic::~SelectiveMaxHeuristic() {
    for (int i = 0; i < num_pairs; i++) {
        delete classifiers[i];
    }
    delete hvalue;
    delete num_evaluated;
    delete num_winner;
    delete num_only_winner;
    delete classifiers;
    delete expensive;
    delete cheap;
    delete computed;
    delete total_computation_time;
    delete avg_time;
    delete heuristic_conf;
    delete dist;
    delete threshold;
}


void SelectiveMaxHeuristic::initialize() {
    // initialize statistics
    cout << "Initializing Selective Max" << endl;

    num_heuristics = heuristics.size();

    hvalue = new int[num_heuristics];
    computed = new bool[num_heuristics];

    num_evaluated = new int[num_heuristics];
    num_winner = new int[num_heuristics];
    num_only_winner = new int[num_heuristics];
    total_computation_time = new double[num_heuristics];
    heuristic_conf = new double[num_heuristics];
    avg_time = new double[num_heuristics];

    //cout << "Stage 1 Done" << endl;

    for (int i = 0; i < num_heuristics; i++) {
        num_evaluated[i] = 0;
        num_winner[i] = 0;
        num_only_winner[i] = 0;
        total_computation_time[i] = 0;
        avg_time[i] = 0.0;
    }

    //cout << "Stage 2 Done" << endl;

    if (random_selection)
        return;


    // initialize classifiers
    num_classes = 2;
    num_pairs = (num_heuristics * (num_heuristics - 1)) / 2;
    classifiers = new Classifier *[num_pairs];
    expensive = new int[num_pairs];
    cheap = new int[num_pairs];
    threshold = new double[num_pairs];

    feature_extractor = feature_extractor_types.create();
    cout << "Number of features: " << feature_extractor->get_num_features() << endl;

    for (int i = 0; i < num_pairs; i++) {
        switch (classifier_type) {
        case NB:
            classifiers[i] = new NBClassifier();
            break;
        case AODE:
            classifiers[i] = new AODEClassifier();
            break;
        default:
            cerr << "Unknown classifier type" << endl;
            exit(353);
        }
        classifiers[i]->set_feature_extractor(feature_extractor);
        classifiers[i]->buildClassifier(num_classes);
        threshold[i] = 0;
    }
    dist = new double[num_classes];

    // initialize active learning statistics
    //current_min_training_set = min_training_set;
    cout << "Done Initializing Selective Max" << endl;
    train();
}

void SelectiveMaxHeuristic::reset_statistics() {
    for (int i = 0; i < num_heuristics; i++) {
        num_evaluated[i] = 0;
        num_winner[i] = 0;
        num_only_winner[i] = 0;
        total_computation_time[i] = 0;
    }
    num_evals = 0;
    correct0_classifications = 0;
    incorrect0_classifications = 0;
    correct1_classifications = 0;
    incorrect1_classifications = 0;

    num_learn_from_no_confidence = 0;
}

void SelectiveMaxHeuristic::train() {
    cout << "Beginning Training" << endl;
    total_training_time.reset();

    MaxHeuristic max(g_default_heuristic_options);
    for (int i = 0; i < num_heuristics; i++) {
        max.add_heuristic(heuristics[i]);
    }

    // initialize all heuristics
    int goal_depth_estimate = 0;
    max.evaluate(*g_initial_state);
    int h0 = max.get_heuristic();
    goal_depth_estimate = 2 * h0;

    if (min_training_set == 0) {
        min_training_set = 10 * h0;
    }

    //cout << "Min Training Set Size: " << min_training_set << endl;


    cout << "Starting Training Example Collection" << endl;
    StateSpaceSample *sample;

    switch (state_space_sample_type) {
    case Probe:
        sample = new ProbeStateSpaceSample(goal_depth_estimate,
                                           2 * min_training_set / goal_depth_estimate,
                                           min_training_set);
        for (int i = 0; i < heuristics.size(); i++)
            sample->add_heuristic(heuristics[i]);
        break;
    /*
    case ProbAStar:
            sample = new ProbAStarSample(&max, min_training_set);
            break;
    */
    case PDB:
        sample = new PDBStateSpaceSample(goal_depth_estimate,
                                         min_training_set,
                                         min_training_set);
        for (int i = 0; i < heuristics.size(); i++)
            sample->add_heuristic(heuristics[i]);
        break;
    default:
        cerr << "Unknown state space sample" << endl;
        exit(931);
    }

    sample->collect();

    sample_t &training_set = sample->get_samp();
    training_set_size = training_set.size();
    branching_factor = sample->get_branching_factor();

    cout << "Training Example Collection Finished" << endl;
    cout << "Branching Factor: " << branching_factor << endl;
    cout << "Training Set Size: " << training_set_size << endl;

    // Get Heuristic Evaluation Times and Values

    //cout << "Collecting Timing Data" << endl;
    ExactTimer retimer;
    for (int i = 0; i < num_heuristics; i++) {
        total_computation_time[i] = sample->get_computation_time(i) + 1;
        if (retime_heuristics) {
            total_computation_time[i] = 0;
            double before = retimer();
            sample_t::const_iterator it;
            for (it = training_set.begin(); it != training_set.end(); it++) {
                const State s = (*it).first;
                heuristics[i]->evaluate(s);
            }
            double after = retimer();
            total_computation_time[i] = after - before + 1;
        }
        cout << "Time " << i << " - " << total_computation_time[i] << endl;
    }
    //cout << "Timing Data Collected" << endl;

    cout << "Beginning Training Classifier" << endl;
    // Calculate Thresholds
    int classifier_index = 0;
    cout << "Thresholds" << endl;
    for (int i = 0; i < num_heuristics; i++) {
        for (int j = i + 1; j < num_heuristics; j++) {
            cout << "Training pair: " << i << ", " << j << endl;
            if (total_computation_time[i] > total_computation_time[j]) {
                expensive[classifier_index] = i;
                cheap[classifier_index] = j;
            } else {
                expensive[classifier_index] = j;
                cheap[classifier_index] = i;
            }

            double thr = alpha *
                         log((double)total_computation_time[expensive[classifier_index]] /
                             (double)total_computation_time[cheap[classifier_index]]) /
                         log(branching_factor);

            if (zero_threshold) {
                thr = 0;
            }

            threshold[classifier_index] = thr;

            cout << expensive[classifier_index] /*heuristics[expensive[classifier_index]]->get_name()*/ <<
            " <=> " << expensive[classifier_index] /*heuristics[expensive[classifier_index]]->get_name()*/ << " - " <<
            cheap[classifier_index] /*heuristics[cheap[classifier_index]]->get_name()*/ << " > " << thr << endl;
            //training_set.reset_iterator();

            sample_t::const_iterator it;
            //while (training_set.has_more_states()) {
            for (it = training_set.begin(); it != training_set.end(); it++) {
                //const State s = training_set.get_next_state();
                const State s = (*it).first;
                int value_expensive = training_set[s][expensive[classifier_index]];
                int value_cheap = training_set[s][cheap[classifier_index]];

                int tag = 0;
                int diff = value_expensive - value_cheap;
                if (diff > threshold[classifier_index]) {
                    tag = 1;
                }
                classifiers[classifier_index]->addExample(&s, tag);
            }
            classifier_index++;
        }
    }

    cout << "Finished Training Classifier" << endl;

    //training_set.resize(0);
    delete sample;

    total_training_time.stop();

    // reset statistics and heuristics after training
    reset_statistics();
    for (int i = 0; i < num_heuristics; i++) {
        heuristics[i]->reset();
    }

    //cout << "Freed Memory" << endl;
}

int SelectiveMaxHeuristic::eval_heuristic(const State &state, int index, bool count) {
    if (count)
        num_evaluated[index]++;
    computed[index] = true;

    heuristics[index]->evaluate(state);

    if (heuristics[index]->is_dead_end()) {
        if (heuristics[index]->dead_ends_are_reliable()) {
            hvalue[index] = numeric_limits<int>::max();
            dead_end = true;
        }
    } else {
        hvalue[index] = heuristics[index]->get_heuristic();
    }
    return hvalue[index];
}

int SelectiveMaxHeuristic::compute_heuristic(const State &state) {
    num_evals++;
    dead_end = false;

    // initialize vector
    for (int i = 0; i < num_heuristics; i++) {
        hvalue[i] = 0;
        computed[i] = false;
    }

    // compute the heuristics that are always computed
    for (int i = 0; i < num_always_calc; i++) {
        eval_heuristic(state, i, true);
    }

    if (random_selection) {
        int h = num_always_calc + (rand() % (num_heuristics - num_always_calc));
        eval_heuristic(state, h, true);
        return calc_max();
    }

    if (perfect_mode) {
        for (int i = num_always_calc; i < num_heuristics; i++) {
            eval_heuristic(state, i, false);
        }

        for (int i = 0; i < num_pairs; i++) {
            int c = cheap[i];
            int e = expensive[i];
            double thr = threshold[i];

            if ((hvalue[e] - hvalue[c]) > thr) {
                num_evaluated[e]++;
                return hvalue[e];
            } else {
                num_evaluated[c]++;
                return hvalue[c];
            }
        }
    }

    classify(state);

    /*
    // choose whether to learn or follow the classifiers advice
    current_min_training_set = max(min_training_set, (int) (num_evals * min_training_ratio));
    if (num_evals < current_min_training_set) {
            learn(state);
    }
    else {
            classify(state);
    }
    */

    int ret = calc_max();

    if (test_mode) {
        // evaluate all heuristics
        for (int i = 0; i < heuristics.size(); i++) {
            if (!computed[i]) {
                eval_heuristic(state, i, false);
            }
        }
        for (int classifier_index = 0; classifier_index < num_pairs; classifier_index++) {
            int diff = hvalue[expensive[classifier_index]] - hvalue[cheap[classifier_index]];
            classifiers[classifier_index]->distributionForInstance(&state, dist);
            //cout << hvalue[i] << " " << hvalue[j] << " " << diff << " " << dist[1] << endl;
            if (dist[1] > 0.5) {
                if (diff > threshold[classifier_index]) {
                    correct1_classifications++;
                } else {
                    incorrect1_classifications++;
                }
            } else {
                if (diff > threshold[classifier_index]) {
                    incorrect0_classifications++;
                } else {
                    correct0_classifications++;
                }
            }
        }
    }

    return ret;
}


void SelectiveMaxHeuristic::learn(const State &state) {
    total_training_time.resume();
    //calculate all (remaining) heuristics
    for (int i = num_always_calc; i < heuristics.size(); i++) {
        eval_heuristic(state, i, true);
    }

    //go over all pairs of heuristics to learn the difference
    int classifier_index = 0;
    for (int i = 0; i < num_heuristics; i++) {
        for (int j = i + 1; j < num_heuristics; j++) {
            // compute the difference (cap at +/- max_diff)
            int diff = hvalue[expensive[classifier_index]] - hvalue[cheap[classifier_index]];
            int tag = 0;
            if (diff > threshold[classifier_index]) {
                tag = 1;
            }

            training_set_size++;
            classifiers[classifier_index]->addExample(&state, tag);
            classifier_index++;
        }
    }
    total_training_time.stop();
}

void SelectiveMaxHeuristic::classify(const State &state) {
    total_classification_time.resume();
    double max_confidence = 0.0;
    int best_heuristics = -1;

    for (int i = 0; i < num_heuristics; i++) {
        heuristic_conf[i] = 0.0;
    }

    for (int classifier_index = 0; classifier_index < num_pairs; classifier_index++) {
        classifiers[classifier_index]->distributionForInstance(&state, dist);
        // get confidence for h_i better than h_j and for h_j better than h_i
        double confidence_expensive = dist[1];
        double confidence_cheap = dist[0];

        heuristic_conf[expensive[classifier_index]] += confidence_expensive;
        heuristic_conf[cheap[classifier_index]] += confidence_cheap;
    }


    for (int i = 0; i < num_heuristics; i++) {
        //cout << i << " " << heuristic_conf[i]  << endl;
        if (heuristic_conf[i] > max_confidence) {
            max_confidence = heuristic_conf[i];
            best_heuristics = i;
        }
    }

    total_classification_time.stop();

    if (max_confidence > (conf_threshold * (num_heuristics - 1))) {
        // if there is a decision over the confidence threshold, use it
        eval_heuristic(state, best_heuristics, true);
    } else {
        // otherwise, learn from this state
        num_learn_from_no_confidence++;
        //cout << "no confidence: " << max_confidence << endl;
        learn(state);
    }
}


int SelectiveMaxHeuristic::calc_max() {
    int max = 0;
    // return max estimate
    if (dead_end) {
        max = DEAD_END;
    } else {
        // calculate the max
        int winner_id = -1;
        int winner_count = 0;
        for (int i = 0; i < num_heuristics; i++) {
            //cout << hvalue[i] << " ";
            if (hvalue[i] > max) {
                max = hvalue[i];
                winner_id = i;
                winner_count = 1;
            } else if (hvalue[i] == max) {
                winner_count++;
            }
        }
        //cout << endl;

        //update statistics
        if (winner_count == 1) {
            num_winner[winner_id]++;
            num_only_winner[winner_id]++;
        } else {
            for (int i = 0; i < num_heuristics; i++) {
                if (hvalue[i] == max) {
                    num_winner[i]++;
                }
            }
        }
    }

    return max;
}

bool SelectiveMaxHeuristic::reach_state(const State &parent_state, const Operator &op,
                                        const State &state) {
    int ret = false;
    int val;
    for (int i = 0; i < num_heuristics; i++) {
        val = heuristics[i]->reach_state(parent_state, op, state);
        ret = ret || val;
    }
    return ret;
}

void SelectiveMaxHeuristic::print_statistics() const {
    cout << "Selective Max Statistics" << endl;
    cout << "Num evals: " << num_evals << endl;
    cout << "Training set size: " << training_set_size << endl;
    cout << "Total Training Time: " << total_training_time << endl;
    cout << "Total Classification Time: " << total_classification_time << endl;
    if (test_mode) {
        cout << "Correct 0 Classifications: " << correct0_classifications << endl;
        cout << "Incorrect 0 Classifications: " << incorrect0_classifications << endl;
        cout << "Correct 1 Classifications: " << correct1_classifications << endl;
        cout << "Incorrect 1 Classifications: " << incorrect1_classifications << endl;

        double acc =
            (double)(correct0_classifications + correct1_classifications) /
            (double)(correct0_classifications + correct1_classifications + incorrect0_classifications + incorrect1_classifications);
        cout << "Total accuracy: " << acc * 100 << "%" << endl;
    }

    cout << "Learn From Low Confidence: " << num_learn_from_no_confidence << endl;

    double eval_time = 0;
    cout << "heuristic,  evaluated, winner,   only winner, total time, average time" << endl;
    for (int i = 0; i < heuristics.size(); i++) {
        cout << i /*heuristics[i]->get_name()*/ << " , " <<
        num_evaluated[i] << " , " <<
        num_winner[i] << " , " <<
        num_only_winner[i] << " , " <<
        total_computation_time[i] << " , " <<
        ((double)total_computation_time[i] / (double)num_evaluated[i]) << endl;
        eval_time = eval_time + (num_evaluated[i] * avg_time[i]);
    }

    cout << "Total evaluation time: " << eval_time << endl;
}

ScalarEvaluator *SelectiveMaxHeuristic::create(
    const std::vector<string> &config, int start, int &end, bool dry_run) {
    if (config[start + 1] != "(") {
        throw ParseError(start + 1);
    }

    vector<Heuristic *> heuristics_;
    OptionParser::instance()->parse_heuristic_list(config, start + 2,
                                                   end, false, heuristics_,
                                                   dry_run);
    if (heuristics_.empty()) {
        throw ParseError(end);
    }
    end++;

    // default parameter
    double alpha_ = 1.0;
    int classifier_type_ = NB;
    double conf_threshold_ = 0.6;
    int min_training_set_ = 100;
    int num_always_calc_ = 0;
    bool random_selection_ = false;
    bool retime_heuristics_ = false;
    int state_space_sample_type_ = Probe;
    bool uniform_sampling_ = false;
    bool zero_threshold_ = false;
    HeuristicOptions common_options;


    if (config[end] != ")") {
        end++;
        NamedOptionParser option_parser;

        common_options.add_option_to_parser(option_parser);

        option_parser.add_double_option("alpha", &alpha_, "alpha");
        option_parser.add_int_option("classifier", &classifier_type_,
                                     "classifier type (0: NB, 1: AODE)");
        option_parser.add_double_option("conf_threshold", &conf_threshold_,
                                        "confidence threshold");
        option_parser.add_int_option("training_set", &min_training_set_,
                                     "minimum size of training set");
        option_parser.add_int_option("eval_always", &num_always_calc_,
                                     "number of heuristics that should always be evaluated");
        option_parser.add_bool_option("random_sel", &random_selection_,
                                      "random selection");
        option_parser.add_bool_option("retime", &retime_heuristics_,
                                      "retime heuristics");
        option_parser.add_int_option("sample", &state_space_sample_type_,
                                     "state space sample type (0: Probe, 1: ProbAStar, 2: PDB)");
        option_parser.add_bool_option("uniform", &uniform_sampling_,
                                      "uniform sampling");
        option_parser.add_bool_option("zero_threshold", &zero_threshold_,
                                      "set threshold constant 0");
        option_parser.parse_options(config, end, end, dry_run);
        end++;
    }
    if (config[end] != ")") {
        throw ParseError(end);
    }

    SelectiveMaxHeuristic *heur = 0;
    if (!dry_run) {
        heur = new SelectiveMaxHeuristic(common_options);
        for (unsigned int i = 0; i < heuristics_.size(); i++) {
            heur->add_heuristic(heuristics_[i]);
        }
        heur->set_alpha(alpha_);
        heur->set_classifier((classifier_t)classifier_type_);
        heur->set_confidence(conf_threshold_);
        heur->set_num_always_calc(num_always_calc_);
        heur->set_training_set_size(min_training_set_);
        heur->set_random_selection(random_selection_);
        heur->set_retime_heuristics(retime_heuristics_);
        heur->set_state_space_sample(
            (state_space_sample_t)state_space_sample_type_);
        heur->set_uniform_sampling(uniform_sampling_);
        heur->set_zero_threshold(zero_threshold_);
    }
    return heur;
}
