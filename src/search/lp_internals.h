#ifndef LP_INTERNALS_H
#define LP_INTERNALS_H

/*
  This file provides some internal functions for the LP solver code.
  They could be implemented in linear_program.cc but we moved them here
  to avoid a long and complex file. They should not be necessary outside
  of linear_program.cc. If you need them, think about extending the
  LP class instead.
*/

#include "utilities.h"

#include <memory>

class CoinError;
class OsiSolverInterface;
enum class LPSolverType;

std::unique_ptr<OsiSolverInterface> create_lp_solver(LPSolverType solver_type);

/*
  Print the CoinError and then exit with EXIT_CRITICAL_ERROR.
  Note that out-of-memory conditions occurring within CPLEX code cannot
  be caught by a try/catch block. When CPLEX runs out of memory,
  the planner will attempt to terminate gracefully, like it does with
  uncaught out-of-memory exceptions in other parts of the code.
*/
NO_RETURN
void handle_coin_error(const CoinError &error);

#endif
