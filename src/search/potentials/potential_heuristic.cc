#include "potential_heuristic.h"

#include "../countdown_timer.h"
#include "../global_operator.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../rng.h"
#include "../successor_generator.h"
#include "../timer.h"

#include <cmath>
#include <numeric>
#include <unordered_map>

#ifdef USE_LP
#include "CoinPackedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include <sys/times.h>
#endif

using namespace std;


namespace potentials {

PotentialHeuristic::PotentialHeuristic(const Options &options)
    : Heuristic(options),
#ifdef USE_LP
    lp_solver(create_lp_solver(LPSolverType(options.get_enum("lpsolver")))),
#endif
    optimization_function(OptimizationFunction(options.get_enum("optimization_function"))),
    sampling_heuristic(options.get<Heuristic *>("sampling_heuristic")),
    sampling_steps_factor(options.get<double>("sampling_steps_factor")),
    num_samples(options.get<int>("num_samples")),
    max_potential(options.get<double>("max_potential")),
    max_sampling_time(options.get<double>("max_sampling_time")),
    max_filtering_time(options.get<double>("max_filtering_time")),
    debug(options.get<bool>("debug")),
    feasible_lp_solution(true),
    num_cols(-1),
    use_resolve(false),
    num_samples_covered(0) {
}

PotentialHeuristic::~PotentialHeuristic() {
#ifdef USE_LP
    if (lp_solver)
        delete lp_solver;
#endif
}

void PotentialHeuristic::initialize() {
    verify_no_axioms_no_conditional_effects();
    Timer initialization_timer;
    fact_potential.resize(g_variable_domain.size());
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        fact_potential[var].resize(g_variable_domain[var]);
    }
    construct_lp();
    if (optimization_function == INITIAL_STATE) {
        feasible_lp_solution = optimize_potential_for_state(g_initial_state());
    } else if (optimization_function == ALL_STATES_AVG) {
        feasible_lp_solution = optimize_potential_for_all_states();
    } else if (optimization_function == SAMPLES_AVG) {
        StateRegistry sample_registry;
        vector<GlobalState> samples;
        sample_states(sample_registry, samples, num_samples, sampling_heuristic);
        if (max_potential == numeric_limits<double>::infinity()) {
            vector<GlobalState> dead_end_free_samples;
            filter_dead_ends(samples, dead_end_free_samples);
            feasible_lp_solution = optimize_potential_for_samples(dead_end_free_samples);
        } else {
            feasible_lp_solution = optimize_potential_for_samples(samples);
        }
    }
    cout << "Initialization of PotentialHeuristic: " << initialization_timer << endl;
}

int PotentialHeuristic::compute_heuristic(const GlobalState &state) {
    if (optimization_function == EACH_STATE)
        feasible_lp_solution = optimize_potential_for_state(state);
    if (!feasible_lp_solution) {
        return DEAD_END;
    }
    double heuristic_value = 0.0;
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        heuristic_value += fact_potential[var][state[var]];
    }
    // TODO avoid code duplication with landmark count heuristic
    double epsilon = 0.01;
    return max(0.0, ceil(heuristic_value - epsilon));
}

bool PotentialHeuristic::optimize_potential_for_state(const GlobalState &state) {
#ifdef USE_LP
    set_objective_for_state(state);
    return solve_lp();
#else
    unused_parameter(state);
    cerr << "You must build the planner with the #USE_LP symbol defined" << endl;
    exit_with(EXIT_CRITICAL_ERROR);
#endif
}

bool PotentialHeuristic::optimize_potential_for_all_states() {
#ifdef USE_LP
    int num_vars = g_variable_domain.size();
    vector<double> objective(num_cols, 0);
    vector<int> indices(num_cols);
    iota(indices.begin(), indices.end(), 0);
    for (int var = 0; var < num_vars; ++var) {
        for (int value = 0; value < g_variable_domain[var]; ++value) {
            objective[fact_lp_vars[var][value]] = 1.0 / g_variable_domain[var];
        }
    }
    lp_solver->setObjCoeffSet(&indices[0], &indices[num_cols], &objective[0]);
    bool feasible = solve_lp();
    if (!feasible)
        ABORT("Potentials must be bounded in the LP for all states.");
    return feasible;
#else
    cerr << "You must build the planner with the #USE_LP symbol defined" << endl;
    exit_with(EXIT_CRITICAL_ERROR);
#endif
}

bool PotentialHeuristic::optimize_potential_for_samples(const vector<GlobalState> &samples) {
#ifdef USE_LP
    set_objective_for_states(samples);
    bool feasible = solve_lp();
    if (!feasible)
        ABORT("LP for samples infeasible. Please filter dead ends or limit the potentials.");
    return feasible;
#else
    unused_parameter(samples);
    cerr << "You must build the planner with the #USE_LP symbol defined" << endl;
    exit_with(EXIT_CRITICAL_ERROR);
#endif
}

void PotentialHeuristic::filter_dead_ends(const vector<GlobalState> &samples,
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

#ifdef USE_LP
void PotentialHeuristic::set_objective_for_state(const GlobalState &state) {
    assert(lp_solver);
    int num_vars = g_variable_domain.size();
    vector<double> objective(num_cols, 0);
    vector<int> indices(num_cols);
    iota(indices.begin(), indices.end(), 0);
    for (int var = 0; var < num_vars; ++var) {
        int value = state[var];
        objective[fact_lp_vars[var][value]] = 1;
    }
    lp_solver->setObjCoeffSet(&indices[0], &indices[num_cols], &objective[0]);
}
#endif

#ifdef USE_LP
void PotentialHeuristic::set_objective_for_states(const vector<GlobalState> &states) {
    int num_vars = g_variable_domain.size();
    vector<double> objective(num_cols, 0);
    vector<int> indices(num_cols);
    iota(indices.begin(), indices.end(), 0);
    for (size_t i = 0; i < states.size(); ++i) {
        const GlobalState &state = states[i];
        for (int var = 0; var < num_vars; ++var) {
            int value = state[var];
            objective[fact_lp_vars[var][value]] += 1;
        }
    }
    lp_solver->setObjCoeffSet(&indices[0], &indices[num_cols], &objective[0]);
}
#endif

void PotentialHeuristic::construct_lp() {
#ifdef USE_LP
    int lp_var = 0;
    fact_lp_vars.resize(g_variable_domain.size());
    const int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        fact_lp_vars[var].resize(g_variable_domain[var] + 1);
        for (int val = 0; val < g_variable_domain[var] + 1; ++val) {
            fact_lp_vars[var][val] = lp_var++;
        }
    }
    num_cols = lp_var;
    // TODO avoid duplication with OCP
    vector<MatrixEntry> matrix_entries;
    vector<double> variable_lower_bounds(num_cols, -lp_solver->getInfinity());
    double upper_bound = lp_solver->getInfinity();
    if (max_potential != numeric_limits<double>::infinity())
        upper_bound = max_potential;
    vector<double> variable_upper_bounds(num_cols, upper_bound);
    vector<double> constraint_lower_bounds;
    vector<double> constraint_upper_bounds;

    int constraint_id = 0;
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
            int pre_lp = fact_lp_vars[var][pre];
            int post_lp = fact_lp_vars[var][post];
            matrix_entries.push_back(MatrixEntry(constraint_id, pre_lp, 1));
            matrix_entries.push_back(MatrixEntry(constraint_id, post_lp, -1));
        }
        constraint_lower_bounds.push_back(-lp_solver->getInfinity());
        constraint_upper_bounds.push_back(op.get_cost());
        ++constraint_id;
    }
    vector<int> goal(g_variable_domain.size(), -1);
    for (size_t i = 0; i < g_goal.size(); ++i) {
        goal[g_goal[i].first] = g_goal[i].second;
    }
    for (int var = 0; var < num_vars; ++var) {
        int undef_val = g_variable_domain[var];
        if (goal[var] == -1)
            goal[var] = undef_val;
        int goal_val_lp = fact_lp_vars[var][goal[var]];
        int undef_val_lp = fact_lp_vars[var][undef_val];
        // Create Constraint (we use variable bounds to express this constraint):
        // P_{V=goal[V]} = 0
        variable_lower_bounds[goal_val_lp] = 0;
        variable_upper_bounds[goal_val_lp] = 0;
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            int val_lp = fact_lp_vars[var][val];
            // Create Constraint:
            // P_{V=v} <= P_{V=u}
            // Note that we could eliminate variables P_{V=u} if V is undefined in the goal.
            matrix_entries.push_back(MatrixEntry(constraint_id, val_lp, 1));
            matrix_entries.push_back(MatrixEntry(constraint_id, undef_val_lp, -1));
            constraint_lower_bounds.push_back(-lp_solver->getInfinity());
            constraint_upper_bounds.push_back(0);
            ++constraint_id;
        }
    }

    // Convert things into OSI format.
    int matrix_entry_count = matrix_entries.size();
    int *rowIndices = new int[matrix_entry_count];
    int *colIndices = new int[matrix_entry_count];
    double *elements = new double[matrix_entry_count];
    for (int i = 0; i < matrix_entry_count; ++i) {
        rowIndices[i] = matrix_entries[i].row;
        colIndices[i] = matrix_entries[i].col;
        elements[i] = matrix_entries[i].element;
    }
    CoinPackedMatrix matrix(false, rowIndices, colIndices, elements, matrix_entry_count);
    delete[] rowIndices;
    delete[] colIndices;
    delete[] elements;
    lp_solver->setObjSense(-1);
    // lower/upper bounds
    double *col_lb = &variable_lower_bounds[0];
    double *col_ub = &variable_upper_bounds[0];
    double *row_lb = &constraint_lower_bounds[0];
    double *row_ub = &constraint_upper_bounds[0];
    // Update objective for each state later.
    vector<double> obj(num_cols, 0);
    lp_solver->loadProblem(matrix, col_lb, col_ub, &obj[0], row_lb, row_ub);
    use_resolve = false;
#endif
}

#ifdef USE_LP
bool PotentialHeuristic::solve_lp() {
    try {
        if (use_resolve) {
            lp_solver->resolve();
        } else {
            lp_solver->initialSolve();
            use_resolve = true;
        }
        if (lp_solver->isAbandoned()) {
            // The documentation of OSI is not very clear here but memory seems
            // to be the most common cause for this in our case.
            cerr << "Abandoned LP during initial solve. "
                 << "Reasons include \"numerical difficuties\" and running out of memory." << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        if (lp_solver->isProvenDualInfeasible()) {
            return false;
        }
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
    extract_lp_solution();
    return true;
}

void PotentialHeuristic::dump_potentials() const {
    cout << "Fact potentials:" << endl;
    int num_vars = g_variable_domain.size();
    for (int var = 0; var < num_vars; ++var) {
        for (int val = 0; val < g_variable_domain[var]; ++val) {
            cout << g_fact_names[var][val] << ": " << fact_potential[var][val] << endl;
        }
        cout << endl;
    }
}

void PotentialHeuristic::extract_lp_solution() {
    try {
        assert(lp_solver->isProvenOptimal()
               && !lp_solver->isProvenPrimalInfeasible()
               && !lp_solver->isProvenDualInfeasible());
        const double *solution = lp_solver->getColSolution();
        int num_vars = g_variable_domain.size();
        for (int var = 0; var < num_vars; ++var) {
            for (int val = 0; val < g_variable_domain[var]; ++val) {
                fact_potential[var][val] = solution[fact_lp_vars[var][val]];
            }
        }
    } catch (CoinError &error) {
        handle_coin_error(error);
    }
}
#endif

// TODO: Avoid code duplication with iPDB.
void PotentialHeuristic::sample_states(StateRegistry &sample_registry,
                                       vector<GlobalState> &samples,
                                       int num_samples,
                                       Heuristic *heuristic) {
    CountdownTimer sampling_timer(max_sampling_time);

    bool using_potential_heuristic = false;
    if (!heuristic) {
        using_potential_heuristic = true;
        Options options;
        options.set<int>("optimization_function", INITIAL_STATE);
        options.set<double>("max_potential", max_potential);
        options.set<Heuristic *>("sampling_heuristic", 0);
        options.set<double>("sampling_steps_factor", -1);
        options.set<int>("num_samples", -1);
        options.set<double>("max_sampling_time", -1);
        options.set<double>("max_filtering_time", -1);
        options.set<int>("lpsolver", CPLEX);
        options.set<int>("cost_type", NORMAL);
        options.set<bool>("debug", false);
        heuristic = new PotentialHeuristic(options);
    }

    double average_operator_cost = 0;
    for (size_t i = 0; i < g_operators.size(); ++i)
        average_operator_cost += get_adjusted_action_cost(g_operators[i], cost_type);
    average_operator_cost /= g_operators.size();

    const GlobalState &initial_state = sample_registry.get_initial_state();
    heuristic->evaluate(initial_state);
    int h = heuristic->get_value();
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
                int random = g_rng.next(applicable_ops.size()); // [0..applicable_os.size())
                assert(applicable_ops[random]->is_applicable(current_state));
                current_state = sample_registry.get_successor_state(current_state, *applicable_ops[random]);
                // if current state is a dead end, then restart with initial state
                heuristic->evaluate(current_state);
                if (heuristic->is_dead_end())
                    current_state = initial_state;
            }
        }
        // last state of the random walk is used as sample
        samples.push_back(current_state);
    }
    if (using_potential_heuristic)
        delete heuristic;
    cout << "Time for sampling: " << sampling_timer << endl;
    cout << "Samples: " << samples.size() << endl;
}

#ifdef USE_LP
void PotentialHeuristic::release_memory() {
    if (lp_solver) {
        delete lp_solver;
        lp_solver = 0;
    }
    vector<vector<int> >().swap(fact_lp_vars);
}
#endif

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
        return 0;
    if (!opts.contains("sampling_heuristic")) {
        opts.set<Heuristic *>("sampling_heuristic", 0);
    }
    return new PotentialHeuristic(opts);
}

static Plugin<Heuristic> _plugin("potential", _parse);
}
