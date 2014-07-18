#include "lp_solver_interface.h"

using namespace std;

// TODO when merging this, the wiki page LPBuildInstructions should be updated
//      to explain how cplex and gurobi can be used.
void add_lp_solver_option_to_parser(OptionParser &parser) {
    parser.document_note(
        "Note",
        "to use an LP solver, you must build the planner with USE_LP=1. "
        "See LPBuildInstructions.");
    vector<string> lp_solvers;
    vector<string> lp_solvers_doc;
    lp_solvers.push_back("CLP");
    lp_solvers_doc.push_back("default LP solver shipped with the COIN library");
    lp_solvers.push_back("CPLEX");
    lp_solvers_doc.push_back("commercial solver by IBM");
    lp_solvers.push_back("GUROBI");
    lp_solvers_doc.push_back("commercial solver");
    parser.add_enum_option("lpsolver",
                           lp_solvers,
                           "external solver that should be used to solve linear programs",
                           "CLP",
                           lp_solvers_doc);
}

#ifdef USE_LP
OsiSolverInterface *create_lp_solver(LPSolverType solver_type) {
    string missing_symbol;
    switch (solver_type) {
    case CLP:
#ifdef COIN_HAS_CLP
        cout << "Using LP solver Clp" << endl;
        return new OsiClpSolverInterface();
#else
        missing_symbol = "COIN_HAS_CLP";
#endif
        break;
    case CPLEX:
#ifdef COIN_HAS_CPX
        {
            cout << "Using LP solver Cplex" << endl;
            OsiSolverInterface *lp_solver = new OsiCpxSolverInterface();
            lp_solver->passInMessageHandler(new ErrorCatchingCoinMessageHandler());
            return lp_solver;
        }
#else
        missing_symbol = "COIN_HAS_CPX";
#endif
        break;
    case GUROBI:
#ifdef COIN_HAS_GRB
        cout << "Using LP solver Gurobi" << endl;
        return new OsiGrbSolverInterface();
#else
        missing_symbol = "COIN_HAS_GRB";
#endif
        break;
    default:
        ABORT("Unknown LP solver type.");
    }
    cerr << "You must build the planner with the " << missing_symbol << " symbol defined" << endl;
    exit_with(EXIT_CRITICAL_ERROR);
}

extern void handle_coin_error(CoinError error) {
    cerr << "Coin threw exception: " << error.message() << endl
         << " from method " << error.methodName() << endl
         << " from class " << error.className() << endl;
    exit_with(EXIT_CRITICAL_ERROR);
}

// Cplex warning that is misleadingly reported with the severity of a critical error.
static const string CPLEX_WARNING_COMPRESS = "CPX0000  Compressing row and column files.";
static const string CPLEX_ERROR_OOM = "CPX0000  CPLEX Error  1001: Out of memory.";
static const string CPLEX_ERROR_OOM_PRE = "CPX0000  Insufficient memory for presolve.";

void ErrorCatchingCoinMessageHandler::checkSeverity() {
    // NOTE currentMessage_ should be used here but it doesn't help for clpex:
    //      currentMessage_.severity() is always "I"
    //      currentMessage_.externalNumber() is always 0
    //      currentMessage_.detail() is always empty
    //      currentMessage_.message() also is empty (NFI)
    if (messageBuffer_ == CPLEX_WARNING_COMPRESS) {
        CoinMessageHandler::checkSeverity();
    } else if (messageBuffer_ == CPLEX_ERROR_OOM || messageBuffer_ == CPLEX_ERROR_OOM_PRE) {
        exit_with(EXIT_OUT_OF_MEMORY);
    } else {
        exit_with(EXIT_CRITICAL_ERROR);
    }
}
#endif

