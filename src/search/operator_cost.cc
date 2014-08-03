#include "operator_cost.h"

#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "utilities.h"

#include <cstdlib>
#include <vector>
using namespace std;

int get_adjusted_action_cost(const Operator &op, OperatorCost cost_type) {
    if (op.is_axiom())
        return 0;
    switch (cost_type) {
    case NORMAL:
        return op.get_cost();
    case ONE:
        return 1;
    case PLUSONE:
        if (is_unit_cost())
            return 1;
        else
            return op.get_cost() + 1;
    default:
        ABORT("Unknown cost type");
    }
}

void add_cost_type_option_to_parser(OptionParser &parser) {
    vector<string> cost_types;
    vector<string> cost_types_doc;
    cost_types.push_back("NORMAL");
    cost_types_doc.push_back(
        "all actions are accounted for with their real cost");
    cost_types.push_back("ONE");
    cost_types_doc.push_back(
        "all actions are accounted for as unit cost");
    cost_types.push_back("PLUSONE");
    cost_types_doc.push_back(
        "all actions are accounted for as their real cost + 1 "
        "(except if all actions have original cost 1, "
        "in which case cost 1 is used). "
        "This is the behaviour known for the heuristics of the LAMA planner. "
        "This is intended to be used by the heuristics, not search engines, "
        "but is supported for both.");
    parser.add_enum_option(
        "cost_type",
        cost_types,
        "Operator cost adjustment type. "
        "No matter what this setting is, axioms will always be considered "
        "as actions of cost 0 by the heuristics that treat axioms as actions.",
        "NORMAL",
        cost_types_doc);
}
