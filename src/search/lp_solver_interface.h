#ifndef LP_SOLVER_INTERFACE_H
#define LP_SOLVER_INTERFACE_H

#ifdef USE_LP
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <OsiSolverInterface.hpp>
#endif

class OptionParser;

enum LPSolverType {
    CLP, CPLEX, GUROBI
};

void add_lp_solver_option_to_parser(OptionParser &parser);

#ifdef USE_LP
OsiSolverInterface *create_lp_solver(LPSolverType solver_type);

/*
  Print the CoinError and then exit with EXIT_CRITICAL_ERROR.
  Note that out-of-memory conditions occurring within CPLEX code cannot
  be caught by a try/catch block. When CPLEX runs out of memory,
  the planner will attempt to terminate gracefully, like it does with
  uncaught out-of-memory exceptions in other parts of the code.
*/
void handle_coin_error(const CoinError &error) __attribute__((noreturn));
#endif

#endif
