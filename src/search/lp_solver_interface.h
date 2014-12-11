#ifndef LP_SOLVER_INTERFACE_H
#define LP_SOLVER_INTERFACE_H

#ifdef USE_LP
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <OsiSolverInterface.hpp>
#pragma GCC diagnostic pop
#endif

class OptionParser;

enum LPSolverType {
    CLP, CPLEX, GUROBI
};

void add_lp_solver_option_to_parser(OptionParser &parser);

#ifdef USE_LP
OsiSolverInterface *create_lp_solver(LPSolverType solver_type);
#endif

#endif
