#include "potential_optimizer.h"

#include "potential_heuristic.h"

#include "../countdown_timer.h"
#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../state_registry.h"
#include "../successor_generator.h"
#include "../timer.h"

#include <cmath>
#include <numeric>
#include <unordered_map>

using namespace std;


namespace potentials {

PotentialOptimizer::PotentialOptimizer(const Options &options)
    : lp_solver(new LPSolver(LPSolverType(options.get_enum("lpsolver")))),
      optimization_function(OptimizationFunction(options.get_enum("optimization_function"))),
      sampling_heuristic(options.get<Heuristic *>("sampling_heuristic")),
      sampling_steps_factor(options.get<double>("sampling_steps_factor")),
      num_samples(options.get<int>("num_samples")),
      max_potential(options.get<double>("max_potential")),
      max_sampling_time(options.get<double>("max_sampling_time")),
      max_filtering_time(options.get<double>("max_filtering_time")),
      debug(options.get<bool>("debug")),
      num_cols(-1),
      num_samples_covered(0) {
    verify_no_axioms_no_conditional_effects();
    Timer initialization_timer;
    fact_potentials.resize(g_variable_domain.size());
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        fact_potentials[var].resize(g_variable_domain[var]);
    }
    construct_lp();
    if (optimization_function == INITIAL_STATE) {
        optimize_potential_for_state(g_initial_state());
    } else if (optimization_function == ALL_STATES_AVG) {
        optimize_potential_for_all_states();
    } else if (optimization_function == SAMPLES_AVG) {
        vector<GlobalState> samples;
        optimize_potential_for_state(g_initial_state());
        if (has_optimal_solution()) {
            StateRegistry sample_registry;
            Options opts;
            opts.set<int>("cost_type", 0);
            PotentialHeuristic sampling_heuristic(opts, get_fact_potentials());
            sample_states(sample_registry, samples, num_samples, sampling_heuristic);
        }
        if (max_potential == numeric_limits<double>::infinity()) {
            vector<GlobalState> dead_end_free_samples;
            filter_dead_ends(samples, dead_end_free_samples);
            optimize_potential_for_samples(dead_end_free_samples);
        } else {
            optimize_potential_for_samples(samples);
        }
    }
    cout << "Initialization of PotentialOptimizer: " << initialization_timer << endl;
}

bool PotentialOptimizer::has_optimal_solution() const {
    return lp_solver->has_optimal_solution();
}

bool PotentialOptimizer::optimize_potential_for_state(const GlobalState &state) {
    set_objective_for_state(state);
    return solve_lp();
}

bool PotentialOptimizer::optimize_potential_for_all_states() {
    int num_vars = g_variable_domain.size();
    vector<double> coefficients(num_cols, 0);
    for (int var = 0; var < num_vars; ++var) {
        for (int value = 0; value < g_variable_domain[var]; ++value) {
            coefficients[lp_var_ids[var][value]] = 1.0 / g_variable_domain[var];
        }
    }
    for (int i = 0; i < num_cols; ++i) {
        lp_solver->set_objective_coefficient(i, coefficients[i]);
    }
    bool feasible = solve_lp();
    if (!feasible)
        ABORT("Potentials must be bounded in the LP for all states.");
    return feasible;
}

bool PotentialOptimizer::optimize_potential_for_samples(const vector<GlobalState> &samples) {
    set_objective_for_states(samples);
    bool feasible = solve_lp();
    if (!feasible)
        ABORT("LP for samples infeasible. Please filter dead ends or limit the potentials.");
    return feasible;
}

void PotentialOptimizer::filter_dead_ends(const vector<GlobalState> &samples,
                                          vector<GlobalState> &non_dead_end_states) {
    CountdownTimer filtering_timer(max_filtering_time);
    non_dead_end_states.clear();
    for (size_t i = 0; i < samples.size(); ++i) {
        if (filtering_timer.is_expired()) {
            cout << "Ran out of time filtering dead ends." << endl;
            break;
        }
        if (optimize_potential_for_state(samples[i]))
            non_dead_end_states.push_back(samples[i]);
    }
    cout << "Time for filtering dead ends: " << filtering_timer << endl;
    cout << "Non dead-end samples: " << non_dead_end_states.size() << endl;
}

void PotentialOptimizer::set_objective_for_state(const GlobalState &state) {
    assert(lp_solver);
    for (int i = 0; i < num_cols; ++i) {
        lp_solver->set_objective_coefficient(i, 0);
    }
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        int value = state[var];
        lp_solver->set_objective_coefficient(lp_var_ids[var][value], 1);
    }
}

void PotentialOptimizer::set_objective_for_states(const vector<GlobalState> &states) {
    vector<int> coefficients(num_cols, 0);
    int num_vars = g_variable_domain.size();
    for (size_t i = 0; i < states.size(); ++i) {
        const GlobalState &state = states[i];
        for (int var = 0; var < num_vars; ++var) {
            int value = state[var];
            coefficients[lp_var_ids[var][value]] += 1;
        }
    }
    for (int i = 0; i < num_cols; ++i) {
        lp_solver->set_objective_coefficient(i, coefficients[i]);
    }
}

void PotentialOptimizer::construct_lp() {
    double upper_bound = (max_potential == numeric_limits<double>::infinity() ?
                          lp_solver->get_infinity() : max_potential);

    vector<LPVariable> lp_variables;
    int num_lp_vars = 0;
    vector<vector<int> > lp_vars;
    lp_var_ids.resize(g_variable_domain.size());
    const int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        lp_var_ids[var].resize(g_variable_domain[var] + 1);
        for (int val = 0; val < g_variable_domain[var] + 1; ++val) {
            // Use dummy coefficient for now. Set the real coefficients later.
            lp_variables.emplace_back(-lp_solver->get_infinity(), upper_bound, 1.0);
            lp_var_ids[var][val] = num_lp_vars++;
        }
    }
    num_cols = num_lp_vars;

    vector<LPConstraint> lp_constraints;
    lp_constraints.reserve(g_operators.size());
    for (size_t i = 0; i < g_operators.size(); ++i) {
        // Create Constraint:
        // Sum_{V in Vars(eff(o))} (P_{V=pre(o)[V]} - P_{V=eff(o)[V]}) <= cost(o)
        const GlobalOperator &op = g_operators[i];
        const vector<GlobalCondition> &preconditions = op.get_preconditions();
        unordered_map<int, int> var_to_precondition;
        for (size_t j = 0; j < preconditions.size(); ++j) {
            var_to_precondition[preconditions[j].var] = preconditions[j].val;
        }
        const vector<GlobalEffect> &effects = op.get_effects();
        LPConstraint constraint(-lp_solver->get_infinity(), op.get_cost());
        vector<pair<int, int> > coefficients;
        for (size_t j = 0; j < effects.size(); ++j) {
            int var = effects[j].var;
            int pre = -1;
            auto it = var_to_precondition.find(var);
            if (it == var_to_precondition.end()) {
                pre = g_variable_domain[var];
            } else {
                pre = it->second;
            }
            int post = effects[j].val;
            int pre_lp = lp_var_ids[var][pre];
            int post_lp = lp_var_ids[var][post];
            assert(pre_lp != post_lp);
            coefficients.emplace_back(pre_lp, 1);
            coefficients.emplace_back(post_lp, -1);
        }
        sort(coefficients.begin(), coefficients.end());
        for (const auto &coeff : coefficients)
            constraint.insert(coeff.first, coeff.second);
        lp_constraints.push_back(constraint);
    }

    /* Create full goal state. Use dummy value |dom(V)| for variables V
       undefined in the goal. */
    vector<int> goal(g_variable_domain.size(), -1);
    for (size_t i = 0; i < g_goal.size(); ++i) {
        goal[g_goal[i].first] = g_goal[i].second;
    }
    for (int var = 0; var < num_vars; ++var) {
        int undef_val = g_variable_domain[var];
        if (goal[var] == -1)
            goal[var] = undef_val;
    }

    for (int var = 0; var < num_vars; ++var) {
        // Create Constraint (we use variable bounds to express this constraint):
        // P_{V=goal[V]} = 0
        // TODO: Do not set lower bound?
        LPVariable &lp_var = lp_variables[lp_var_ids[var][goal[var]]];
        lp_var.lower_bound = 0;
        lp_var.upper_bound = 0;

        int undef_val = g_variable_domain[var];
        int undef_val_lp = lp_var_ids[var][undef_val];
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            int val_lp = lp_var_ids[var][val];
            // Create Constraint:
            // P_{V=v} <= P_{V=u}
            // Note that we could eliminate variables P_{V=u} if V is undefined in the goal.
            LPConstraint constraint(-lp_solver->get_infinity(), 0);
            constraint.insert(val_lp, 1);
            constraint.insert(undef_val_lp, -1);
            lp_constraints.push_back(constraint);
        }
    }
    lp_solver->load_problem(LPObjectiveSense::MAXIMIZE, lp_variables, lp_constraints);
}

bool PotentialOptimizer::solve_lp() {
    lp_solver->solve();
    if (lp_solver->has_optimal_solution()) {
        extract_lp_solution();
    }
    return lp_solver->has_optimal_solution();
}

void PotentialOptimizer::dump_potentials() const {
    cout << "Fact potentials:" << endl;
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            cout << g_fact_names[var][val] << ": " << fact_potentials[var][val] << endl;
        }
        cout << endl;
    }
}

void PotentialOptimizer::extract_lp_solution() {
    assert(lp_solver->has_optimal_solution());
    const vector<double> solution = lp_solver->extract_solution();
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            fact_potentials[var][val] = solution[lp_var_ids[var][val]];
        }
    }
}

// TODO: Avoid code duplication with iPDB.
void PotentialOptimizer::sample_states(StateRegistry &sample_registry,
                                       vector<GlobalState> &samples,
                                       int num_samples,
                                       Heuristic &heuristic) {
    CountdownTimer sampling_timer(max_sampling_time);

    double average_operator_cost = 0;
    for (size_t i = 0; i < g_operators.size(); ++i)
        average_operator_cost += g_operators[i].get_cost();
    average_operator_cost /= g_operators.size();

    const GlobalState &initial_state = sample_registry.get_initial_state();
    EvaluationContext eval_context(initial_state);
    if (eval_context.is_heuristic_infinite(&heuristic)) {
        cout << "Initial state is a dead end, abort sampling." << endl;
        return;
    }
    int h = eval_context.get_heuristic_value(&heuristic);
    cout << "Sampling heuristic solution cost estimate: " << h << endl;

    int n;
    if (h == 0) {
        n = 10;
    } else {
        // Convert heuristic value into an approximate number of actions
        // (does nothing on unit-cost problems).
        // average_operator_cost cannot equal 0, as in this case, all operators
        // must have costs of 0 and in this case the if-clause triggers.
        int solution_steps_estimate = int((h / average_operator_cost) + 0.5);
        n = 2 * sampling_steps_factor * solution_steps_estimate;
    }
    double p = 0.5;
    // The expected walk length is np = sampling_steps_factor * estimated number of solution steps.
    // (We multiply by 2 because the heuristic is underestimating.)
    cout << "Expected walk length: " << n * p << endl;

    samples.reserve(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        if (sampling_timer.is_expired()) {
            cout << "Ran out of time sampling." << endl;
            break;
        }

        // calculate length of random walk accoring to a binomial distribution
        int length = 0;
        for (int j = 0; j < n; ++j) {
            double random = g_rng(); // [0..1)
            if (random < p)
                ++length;
        }

        // random walk of length length
        GlobalState current_state(initial_state);
        for (int j = 0; j < length; ++j) {
            vector<const GlobalOperator *> applicable_ops;
            g_successor_generator->generate_applicable_ops(current_state, applicable_ops);
            // if there are no applicable operators --> do not walk further
            if (applicable_ops.empty()) {
                break;
            } else {
                const GlobalOperator *random_op = *g_rng.choose(applicable_ops);
                assert(random_op->is_applicable(current_state));
                current_state = sample_registry.get_successor_state(current_state, *random_op);
                // if current state is a dead end, then restart with initial state
                EvaluationContext eval_context(current_state);
                if (eval_context.is_heuristic_infinite(&heuristic))
                    current_state = initial_state;
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
    cout << "Time for sampling: " << sampling_timer << endl;
    cout << "Samples: " << samples.size() << endl;
}

void PotentialOptimizer::release_memory() {
    if (lp_solver){
        delete lp_solver;
        lp_solver = nullptr;
    }
    vector<vector<int> >().swap(lp_var_ids);
}

const std::vector<std::vector<double> > &PotentialOptimizer::get_fact_potentials() const {
    return fact_potentials;
}

void add_common_potential_options_to_parser(OptionParser &parser) {
    vector<string> optimization_function;
    vector<string> optimization_function_doc;
    optimization_function.push_back("INITIAL_STATE");
    optimization_function_doc.push_back(
        "optimize heuristic for initial state");
    optimization_function.push_back("EACH_STATE");
    optimization_function_doc.push_back(
        "optimize heuristic for each state separately");
    optimization_function.push_back("ALL_STATES_AVG");
    optimization_function_doc.push_back(
        "optimize heuristic for all states");
    optimization_function.push_back("SAMPLES_AVG");
    optimization_function_doc.push_back(
        "optimize heuristic for a set of sample states");
    parser.add_enum_option("optimization_function",
                           optimization_function,
                           "Optimization function.",
                           "SAMPLES_AVG",
                           optimization_function_doc);
    parser.add_option<Heuristic *>(
        "sampling_heuristic",
        "heuristic for sampling",
        "",
        OptionFlags(false));
    parser.add_option<double>(
        "sampling_steps_factor",
        "multiply the number of estimated solution steps by this factor and "
        "center the random walk ends around it",
        "2.0");
    parser.add_option<int>(
        "num_samples",
        "Number of states to sample if optimization_function=samples",
        "1000");
    parser.add_option<double>(
        "max_potential",
        "Bound potentials by this number",
        "infinity");
    parser.add_option<double>(
        "max_sampling_time",
        "time limit in seconds for sampling states",
        "infinity");
    parser.add_option<double>(
        "max_filtering_time",
        "time limit in seconds for filtering dead ends",
        "infinity");
    parser.add_option<bool>(
        "debug",
        "Print debug messages",
        "false");
}

static Heuristic *_parse(OptionParser &parser) {
    add_lp_solver_option_to_parser(parser);
    add_common_potential_options_to_parser(parser);
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    if (!opts.contains("sampling_heuristic")) {
        opts.set<Heuristic *>("sampling_heuristic", 0);
    }
    PotentialOptimizer optimizer(opts);
    return new PotentialHeuristic(opts, optimizer.get_fact_potentials());
}

static Plugin<Heuristic> _plugin("potential", _parse);
}
