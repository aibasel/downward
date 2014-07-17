#ifndef LP_SOLVER_INTERFACE_H
#define LP_SOLVER_INTERFACE_H

#include "globals.h"
#include "option_parser.h"

#include <vector>

#ifdef USE_LP
#pragma GCC diagnostic ignored "-Wunused-parameter"
#ifdef COIN_HAS_CLP
#include <OsiClpSolverInterface.hpp>
#endif
#ifdef COIN_HAS_CPX
#include <OsiCpxSolverInterface.hpp>
#endif
#ifdef COIN_HAS_GRB
#include <OsiGrbSolverInterface.hpp>
#endif
#endif

enum LPSolverType {
    CLP, CPLEX, GUROBI
};

void add_lp_solver_option_to_parser(OptionParser &parser);

#ifdef USE_LP

// HACK Cplex sometimes does not report errors as exceptions, and only prints an
//      error message. This class will report any error messages as usual but will
//      exit with a critical error afterwards.
class ErrorCatchingCoinMessageHandler: public CoinMessageHandler {
public:
    ErrorCatchingCoinMessageHandler()
        : CoinMessageHandler() {
        setLogLevel(0);
    }

    virtual void checkSeverity();
};


OsiSolverInterface *create_lp_solver(LPSolverType solver_type);

extern void handle_coin_error(CoinError error) __attribute__((noreturn));
#endif

#endif
