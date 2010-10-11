#include "landmark_cost_assignment.h"
#include <limits>


LandmarkCostAssignment::LandmarkCostAssignment(LandmarksGraph &graph, bool exc_ALM_eff)
    : lm_graph(graph), exclude_ALM_effects(exc_ALM_eff) {
}

LandmarkCostAssignment::~LandmarkCostAssignment() {
}


const set<int> &LandmarkCostAssignment::get_achievers(int lmn_status, LandmarkNode &lmn) {
    /*
     * Return the achievers of the landmark, according to its status.
     * Returns first achievers for non-reached landmarks
     * Returns all achievers for needed again landmarks
     * Return an empty set for reached landmarks (that are not accepted)
     */

    if (lmn_status == lm_not_reached) {
        return lmn.first_achievers;
    } else if (lmn_status == lm_needed_again) {
        return lmn.possible_achievers;
    }

    return empty;
}


void LandmarkCostAssignment::get_landmark_effects(const Operator &op, set<pair<int, int> > &eff) {
    /*
     * Get the effects of op that are landmarks that
     * are not accepted or required again
     */
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        pair<int, int> prop = make_pair(op.get_pre_post()[i].var, op.get_pre_post()[i].post);

        LandmarkNode *lmn = NULL;
        if (lm_graph.simple_landmark_exists(prop)) {
            lmn = &lm_graph.get_simple_lm_node(prop);
        } else {
            set<pair<int, int> > lm_set;
            lm_set.insert(prop);
            if (lm_graph.disj_landmark_exists(lm_set)) {
                lmn = &lm_graph.get_disj_lm_node(prop);
            }
        }

        if ((lmn != NULL) && (lmn->get_status(exclude_ALM_effects) != lm_reached)) {
            eff.insert(prop);
        }
    }
}


void LandmarkCostAssignment::get_effect_landmarks(const Operator &op, set<LandmarkNode *> &eff) {
    /*
     * Get the landmarks that are effects of op and
     * are not accepted or required again
     */
    for (int i = 0; i < op.get_pre_post().size(); i++) {
        pair<int, int> prop = make_pair(op.get_pre_post()[i].var, op.get_pre_post()[i].post);

        LandmarkNode *lmn = NULL;
        if (lm_graph.simple_landmark_exists(prop)) {
            lmn = &lm_graph.get_simple_lm_node(prop);
        } else {
            set<pair<int, int> > lm_set;
            lm_set.insert(prop);
            if (lm_graph.disj_landmark_exists(lm_set)) {
                lmn = &lm_graph.get_disj_lm_node(prop);
            }
        }

        if ((lmn != NULL) && (lmn->get_status(exclude_ALM_effects) != lm_reached)) {
            eff.insert(lmn);
        }
    }
}


/* Uniform cost partioning */
LandmarkUniformSharedCostAssignment::LandmarkUniformSharedCostAssignment(
    LandmarksGraph &graph, bool exc_ALM_eff)
    : LandmarkCostAssignment(graph, exc_ALM_eff) {
}

LandmarkUniformSharedCostAssignment::~LandmarkUniformSharedCostAssignment() {
}

void LandmarkUniformSharedCostAssignment::assign_costs() {
    const set<LandmarkNode *> &nodes = lm_graph.get_nodes();

    set<LandmarkNode *>::iterator node_it;
    for (node_it = nodes.begin(); node_it != nodes.end(); ++node_it) {
        LandmarkNode &node = **node_it;
        //if (g_verbose) dump_node(&node);
        int lmn_status = node.get_status(exclude_ALM_effects);
        if (lmn_status != lm_reached) {
            //if (g_verbose) cout << "calculating cost " << endl;
            double min_cost = numeric_limits<double>::max();

            const set<int> &achievers = get_achievers(lmn_status, node);

            if (achievers.size() > 0) {
                set<int>::iterator fa_it;
                for (fa_it = achievers.begin(); fa_it != achievers.end(); ++fa_it) {
                    int op_id = *fa_it;
                    const Operator &op = lm_graph.get_operator_for_lookup_index(op_id);

                    set<pair<int, int> > current_lm_effects;

                    get_landmark_effects(op, current_lm_effects);
                    assert(current_lm_effects.size() > 0);
                    double cost = ((double)op.get_cost()) / ((double)current_lm_effects.size());

                    //if (g_verbose) cout << "\tcost from " << op.get_name() << " = " << cost << endl;

                    if (cost < min_cost) {
                        min_cost = cost;
                    }
                }

                node.shared_cost = min_cost;

                //if (g_verbose) cout << "\tcost = " << node.shared_cost << endl;
            } else {
                node.shared_cost = 0; //(double) INT_MAX;

                /*
                if (g_verbose) {
                    cout << "\tno achievers - DEAD END" << endl;
                    dump_node(&node);
                }
                */
                //assert(false);
            }
        } else {
            node.shared_cost = 0;

            //if (g_verbose) cout << "\treached and not needed again (or excluded by action landmark) - cost 0" << endl;
        }
    }
}



/* Optimal cost partioning */
LandmarkOptimalSharedCostAssignment::LandmarkOptimalSharedCostAssignment(
    LandmarksGraph &graph, bool exc_ALM_eff)
    : LandmarkCostAssignment(graph, exc_ALM_eff) {
#ifdef USE_LP
    si = new OsiXxxSolverInterface();
#endif
}

LandmarkOptimalSharedCostAssignment::~LandmarkOptimalSharedCostAssignment() {
#ifdef USE_LP
    delete si;
#endif
}

void LandmarkOptimalSharedCostAssignment::assign_costs() {
#ifdef USE_LP
    try{
        struct tms start, end_build, end_solve, end_all;
        times(&start);

        //Number of variables (columns) in problem is:
        // 1 for each landmark, and 1 for each action/landmark pair
        const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
        int n_cols = lm_graph.number_of_landmarks() * (1 + g_operators.size());
        double *objective = new double[n_cols]; //the objective coefficients
        double *col_lb = new double[n_cols];   //the column lower bounds
        double *col_ub = new double[n_cols];   //the column upper bounds
        vector<CoinPackedVector *> added_rows;

        // we need to number the landmarks, so put them in a vector
        vector<LandmarkNode *> lm_vector;
        for (set<LandmarkNode *>::iterator node_it = nodes.begin(); node_it != nodes.end(); ++node_it) {
            lm_vector.push_back(*node_it);
        }

        for (int i = 0; i < n_cols; i++) {
            // Define the objective coefficients.
            // we want to maximize 1*cost(lm_1) + 1*cost(lm_2) + ... + 1*cost(lm_n)
            if (i < lm_graph.number_of_landmarks())
                objective[i] = 1;
            else
                objective[i] = 0.0;

            // Define bounds
            // all costs are between 0 and infinity
            col_lb[i] = 0.0;
            col_ub[i] = si->getInfinity();
        }


        //Define the constraint matrix.
        CoinPackedMatrix *matrix = new CoinPackedMatrix(false, 0, 0);
        matrix->setDimensions(0, n_cols);

        // the number of constraints is not known in advance (it depends on first achievers)
        // there is one for each operator, and we will increment further
        int n_rows = g_operators.size();

        // Add action cost constraints:
        // cost(a_i, lm_i1) + cost(a_i, lm_i2) + ... + cost(a_i, lm_in) = cost(a_i)
        // for current landmarks where a_i is a first achiever
        for (int a_i = 0; a_i < g_operators.size(); a_i++) {
            CoinPackedVector &row = *new CoinPackedVector(true);
            added_rows.push_back(&row);
        }

        for (int lm_j = 0; lm_j < lm_graph.number_of_landmarks(); lm_j++) {
            LandmarkNode *lmn = lm_vector[lm_j];

            int lmn_status = lmn->get_status(exclude_ALM_effects);
            if (lmn_status != lm_reached) {
                const set<int> achievers = get_achievers(lmn_status, *lmn);
                set<int>::const_iterator op_it;
                for (op_it = achievers.begin(); op_it != achievers.end(); ++op_it) {
                    int a_i = *op_it;

                    added_rows[a_i]->insert(cost_a_lm_i(a_i, lm_j), 1.0);
                }
            }
        }

        /*
        for (int a_i = 0; a_i < g_operators.size(); a_i++) {
            matrix->appendRow(*added_rows[a_i]);
        }
        */

        // this stands in for cost(lm_i) = min_{a_j} cost(a_j, lm_i)
        // for first achievers a_j of lm_i
        for (int lm_i = 0; lm_i < lm_graph.number_of_landmarks(); lm_i++) {
            LandmarkNode *lmn = lm_vector[lm_i];

            int lmn_status = lmn->get_status(exclude_ALM_effects);
            if (lmn_status != lm_reached) {
                const set<int> achievers = get_achievers(lmn_status, *lmn);
                //get_first_achievers(*lmn, first_achievers);

                set<int>::iterator fa_it;
                for (fa_it = achievers.begin(); fa_it != achievers.end(); ++fa_it) {
                    int a_i = *fa_it;
                    CoinPackedVector &row = *new CoinPackedVector(true);
                    added_rows.push_back(&row);

                    row.insert(cost_lm_i(lm_i), 1.0);
                    row.insert(cost_a_lm_i(a_i, lm_i), -1.0);
                    //matrix->appendRow(row);
                    n_rows++;
                }
            } else {
                CoinPackedVector &row = *new CoinPackedVector(true);
                added_rows.push_back(&row);

                row.insert(cost_lm_i(lm_i), 1.0);
                //matrix->appendRow(row);
                n_rows++;
            }
        }

        CoinPackedVectorBase **rows = new CoinPackedVectorBase *[added_rows.size()];
        for (int i = 0; i < added_rows.size(); i++) {
            rows[i] = added_rows[i];
        }
        matrix->appendRows(added_rows.size(), rows);

        double *row_lb = new double[n_rows]; //the row lower bounds
        double *row_ub = new double[n_rows]; //the row upper bounds

        for (int a_i = 0; a_i < g_operators.size(); a_i++) {
            const Operator &op = lm_graph.get_operator_for_lookup_index(a_i);

            row_lb[a_i] = (double)op.get_cost();
            row_ub[a_i] = (double)op.get_cost();
        }
        for (int i = g_operators.size(); i < n_rows; i++) {
            row_lb[i] = -1.0 * si->getInfinity();
            row_ub[i] = 0.0;
        }


        //load the problem to OSI
        si->loadProblem(*matrix, col_lb, col_ub, objective, row_lb, row_ub);

        //we want to maximize the objective function
        si->setObjSense(-1);

        si->messageHandler()->setLogLevel(0);
        //solve the linear program

        times(&end_build);
        si->initialSolve();
        times(&end_solve);


        const double *solution = si->getColSolution();

        // LP solved, assign shared cost to nodes
        for (int i = 0; i < lm_graph.number_of_landmarks(); i++) {
            lm_vector[i]->shared_cost = solution[cost_lm_i(i)];
        }

        //free the memory
        if (objective != 0) {
            delete [] objective;
            objective = 0;
        }
        if (col_lb != 0) {
            delete [] col_lb;
            col_lb = 0;
        }
        if (col_ub != 0) {
            delete [] col_ub;
            col_ub = 0;
        }
        if (row_lb != 0) {
            delete [] row_lb;
            row_lb = 0;
        }
        if (row_ub != 0) {
            delete [] row_ub;
            row_ub = 0;
        }
        for (int i = 0; i < added_rows.size(); i++)
            delete added_rows[i];
        if (matrix != 0) {
            delete matrix;
            matrix = 0;
        }
        //delete solution;
        delete rows;

        times(&end_all);

        //int total_ms = (end_all.tms_utime - start.tms_utime) * 10;
        //int build_ms = (end_build.tms_utime - start.tms_utime) * 10;
        //int solve_ms = (end_solve.tms_utime - end_build.tms_utime) * 10;

        //cout << "Build: " << build_ms << " , Solve: " << solve_ms << " , Total: " << total_ms << "  , Iterations: " << si->getIterationCount() << endl;;
    }catch (CoinError &ex) {
        cerr << "Exception:" << ex.message() << endl
             << " from method " << ex.methodName() << endl
             << " from class " << ex.className() << endl;
        exit(0);
    }
    ;
#else
    // TODO: Use same error message everywhere (or better:
    //       try to avoid having such #ifdefs in different places).
    //       See the nicer error message in landmarks_count_heuristic.cc.
    cout << "Please define USE_LP first" << endl;
    exit(2);
#endif
}

LandmarkEfficientOptimalSharedCostAssignment::LandmarkEfficientOptimalSharedCostAssignment(
    LandmarksGraph &graph, bool exc_ALM_eff) : LandmarkCostAssignment(graph, exc_ALM_eff) {
#ifdef USE_LP
    si = new OsiXxxSolverInterface();
#endif
}

LandmarkEfficientOptimalSharedCostAssignment::~LandmarkEfficientOptimalSharedCostAssignment() {
#ifdef USE_LP
    delete si;
#endif
}

void LandmarkEfficientOptimalSharedCostAssignment::assign_costs() {
#ifdef USE_LP
    try{
        struct tms start, end_build, end_solve, end_all;
        times(&start);


        //Number of variables (columns) in problem is 1 for each landmark
        const set<LandmarkNode *> &nodes = lm_graph.get_nodes();
        int n_cols = lm_graph.number_of_landmarks();
        double *objective = new double[n_cols]; //the objective coefficients
        double *col_lb = new double[n_cols];   //the column lower bounds
        double *col_ub = new double[n_cols];   //the column upper bounds
        vector<CoinPackedVector *> added_rows;

        // we need to number the landmarks, so put them in a vector
        LandmarkNode **lm_vector = new LandmarkNode *[lm_graph.number_of_landmarks()];
        for (set<LandmarkNode *>::iterator node_it = nodes.begin(); node_it != nodes.end(); ++node_it) {
            LandmarkNode *lmn = *node_it;
            lm_vector[lmn->id] = lmn;
        }

        for (int i = 0; i < n_cols; i++) {
            // Define the objective coefficients.
            // we want to maximize 1*cost(lm_1) + 1*cost(lm_2) + ... + 1*cost(lm_n)
            objective[i] = 1.0;

            // Define bounds
            // all costs are between 0 and infinity for participating (non-reached) landmarks
            col_lb[i] = 0.0;
            if ((lm_vector[i]->get_status(exclude_ALM_effects)) != lm_reached) {
                col_ub[i] = si->getInfinity();
            }
            // reached landmarks have a cost of 0
            else {
                col_ub[i] = 0.0;
            }
        }

        //Define the constraint matrix.
        CoinPackedMatrix *matrix = new CoinPackedMatrix(false, 0, 0);
        int n_rows = g_operators.size();
        matrix->setDimensions(0, n_cols);

        // Add constraints:
        // cost(lm_i1) + cost(lm_i2) + ... + cost(lm_in) <= cost(o)
        // where lm_i1 ... lm_in are landmarks achieved by operator o
        for (int a_i = 0; a_i < g_operators.size(); a_i++) {
            //cout << a_i << " : ";
            CoinPackedVector &row = *new CoinPackedVector(true);

            set<LandmarkNode *> current_lm_effects;
            get_effect_landmarks(g_operators[a_i], current_lm_effects);

            set<LandmarkNode *>::iterator it;
            for (it = current_lm_effects.begin(); it != current_lm_effects.end(); it++) {
                LandmarkNode *lmn = *it;
                row.insert(lmn->id, 1.0);
                //cout << lmn->id << " ";
            }

            added_rows.push_back(&row);
            //cout << endl;
        }


        CoinPackedVectorBase **rows = new CoinPackedVectorBase *[added_rows.size()];
        for (int i = 0; i < added_rows.size(); i++) {
            rows[i] = added_rows[i];
        }
        matrix->appendRows(added_rows.size(), rows);


        double *row_lb = new double[n_rows]; //the row lower bounds
        double *row_ub = new double[n_rows]; //the row upper bounds

        for (int a_i = 0; a_i < g_operators.size(); a_i++) {
            const Operator &op = lm_graph.get_operator_for_lookup_index(a_i);

            row_lb[a_i] = 0;
            row_ub[a_i] = (double)op.get_cost();
        }

        //load the problem to OSI
        si->loadProblem(*matrix, col_lb, col_ub, objective, row_lb, row_ub);


        //we want to maximize the objective function
        si->setObjSense(-1);

        times(&end_build);

        //solve the linear program
        si->messageHandler()->setLogLevel(0);
        si->initialSolve();
        times(&end_solve);

        const double *solution = si->getColSolution();

        // LP solved, assign shared cost to nodes
        for (int i = 0; i < lm_graph.number_of_landmarks(); i++) {
            lm_vector[i]->shared_cost = solution[i];
        }

        //free the memory
        if (objective != 0) {
            delete [] objective;
            objective = 0;
        }
        if (col_lb != 0) {
            delete [] col_lb;
            col_lb = 0;
        }
        if (col_ub != 0) {
            delete [] col_ub;
            col_ub = 0;
        }
        if (row_lb != 0) {
            delete [] row_lb;
            row_lb = 0;
        }
        if (row_ub != 0) {
            delete [] row_ub;
            row_ub = 0;
        }
        for (int i = 0; i < added_rows.size(); i++)
            delete added_rows[i];
        if (matrix != 0) {
            delete matrix;
            matrix = 0;
        }
        if (lm_vector != 0) {
            delete lm_vector;
            lm_vector = 0;
        }
        //delete solution;
        if (rows != 0) {
            delete rows;
            rows = 0;
        }


        times(&end_all);
        /*
        int total_ms = (end_all.tms_utime - start.tms_utime) * 10;
        int build_ms = (end_build.tms_utime - start.tms_utime) * 10;
        int solve_ms = (end_solve.tms_utime - end_build.tms_utime) * 10;

        cout << "Build: " << build_ms << " , Solve: " << solve_ms << " , Total: " << total_ms << endl;
        */
    }catch (CoinError &ex) {
        cerr << "Exception:" << ex.message() << endl
             << " from method " << ex.methodName() << endl
             << " from class " << ex.className() << endl;
        exit(0);
    }
    ;
#else
    // TODO: Use same error message everywhere (or better:
    //       try to avoid having such #ifdefs in different places).
    //       See the nicer error message in landmarks_count_heuristic.cc.
    cout << "Please define USE_LP first" << endl;
    exit(2);
#endif
}
