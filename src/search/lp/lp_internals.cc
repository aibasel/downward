#include "lp_internals.h"

#ifdef USE_LP
#include "lp_solver.h"

#include "../utils/system.h"

#ifdef __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Woverflow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wconstant-conversion"
#endif
#include <OsiSolverInterface.hpp>

#ifdef COIN_HAS_CLP
#include <OsiClpSolverInterface.hpp>
#endif

#ifdef COIN_HAS_CPX
#include <OsiCpxSolverInterface.hpp>
#include <cplex.h>
#endif

#ifdef COIN_HAS_GRB
#include <OsiGrbSolverInterface.hpp>
#endif

#ifdef COIN_HAS_SPX
#include <OsiSpxSolverInterface.hpp>
#include <spxout.h>
#endif

#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif

using namespace std;
using utils::ExitCode;

namespace lp {
// CPLEX warning that is misleadingly reported with the severity of a critical error.
static const string CPLEX_WARNING_COMPRESS = "CPX0000  Compressing row and column files.";
// CPLEX warning from writeMps if no column names are defined.
static const string CPLEX_WARNING_WRITE_MPS_COLUMNS = "CPX0000  Default column names x1, x2 ... being created.";
static const string CPLEX_WARNING_WRITE_MPS_ROWS = "CPX0000  Default row    names c1, c2 ... being created.";
static const string CPLEX_ERROR_OOM = "CPX0000  CPLEX Error  1001: Out of memory.";
static const string CPLEX_ERROR_OOM_PRE = "CPX0000  Insufficient memory for presolve.";
static const string CPLEX_ERROR_OOM_DEVEX = "CPX0000  Not enough memory for devex.";
static const string COIN_CPLEX_ERROR_OOM = "returned error 1001";

/*
  CPLEX sometimes does not report errors as exceptions and only prints an
  error message. This class will report any error messages as usual but will
  exit with a critical error afterwards.
*/
class ErrorCatchingCoinMessageHandler : public CoinMessageHandler {
public:
    ErrorCatchingCoinMessageHandler() {
        setLogLevel(0);
    }

    ~ErrorCatchingCoinMessageHandler() {
    }

    virtual void checkSeverity() {
        /*
          Note that currentMessage_ should be used here but it doesn't help for clpex:
            currentMessage_.severity() is always "I"
            currentMessage_.externalNumber() is always 0
            currentMessage_.detail() is always empty
            currentMessage_.message() also is empty (NFI)
        */
        if (messageBuffer_ == CPLEX_WARNING_COMPRESS ||
            messageBuffer_ == CPLEX_WARNING_WRITE_MPS_COLUMNS ||
            messageBuffer_ == CPLEX_WARNING_WRITE_MPS_ROWS) {
            CoinMessageHandler::checkSeverity();
        } else if (messageBuffer_ == CPLEX_ERROR_OOM ||
                   messageBuffer_ == CPLEX_ERROR_OOM_PRE ||
                   messageBuffer_ == CPLEX_ERROR_OOM_DEVEX) {
            utils::exit_with(ExitCode::SEARCH_OUT_OF_MEMORY);
        } else {
            utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
};

unique_ptr<OsiSolverInterface> create_lp_solver(LPSolverType solver_type) {
    string missing_symbol;
    OsiSolverInterface *lp_solver = 0;
    switch (solver_type) {
    case LPSolverType::CLP:
#ifdef COIN_HAS_CLP
        lp_solver = new OsiClpSolverInterface;
#else
        missing_symbol = "COIN_HAS_CLP";
#endif
        break;
    case LPSolverType::CPLEX:
#ifdef COIN_HAS_CPX
        {
            OsiCpxSolverInterface *cpx_solver = new OsiCpxSolverInterface;
            CPXsetintparam(cpx_solver->getEnvironmentPtr(), CPX_PARAM_THREADS, 1);
            cpx_solver->passInMessageHandler(new ErrorCatchingCoinMessageHandler);
            lp_solver = cpx_solver;
        }
#else
        missing_symbol = "COIN_HAS_CPX";
#endif
        break;
    case LPSolverType::GUROBI:
#ifdef COIN_HAS_GRB
        lp_solver = new OsiGrbSolverInterface;
#else
        missing_symbol = "COIN_HAS_GRB";
#endif
        break;
    case LPSolverType::SOPLEX:
#ifdef COIN_HAS_SPX
        {
            OsiSpxSolverInterface *spx_solver = new OsiSpxSolverInterface;
            spx_solver->getSPxOut()->setVerbosity(soplex::SPxOut::ERROR);
            lp_solver = spx_solver;
        }
#else
        missing_symbol = "COIN_HAS_SPX";
#endif
        break;
    default:
        ABORT("Unknown LP solver type.");
    }
    if (lp_solver) {
        lp_solver->messageHandler()->setLogLevel(0);
        return unique_ptr<OsiSolverInterface>(lp_solver);
    } else {
        cerr << "You must build the planner with the " << missing_symbol << " symbol defined" << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

NO_RETURN
void handle_coin_error(const CoinError &error) {
    if (error.message().find(COIN_CPLEX_ERROR_OOM) != string::npos) {
        cout << "CPLEX ran out of memory during OSI method." << endl
             << "Coin exception: " << error.message() << endl
             << " from method " << error.methodName() << endl
             << " from class " << error.className() << endl;
        utils::exit_with(ExitCode::SEARCH_OUT_OF_MEMORY);
    } else {
        cerr << "Coin threw exception: " << error.message() << endl
             << " from method " << error.methodName() << endl
             << " from class " << error.className() << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
}
}

#endif
