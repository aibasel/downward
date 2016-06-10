#include "stubborn_sets_simple.h"

#include "../globals.h"
#include "../global_operator.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"


using namespace std;

namespace stubborn_sets_simple {
/* Implementation of simple instantiation of strong stubborn sets.
   Disjunctive action landmarks are computed trivially.*/
StubbornSetsSimple::StubbornSetsSimple() {
    compute_interference_relation();
    cout << "pruning method: stubborn sets simple" << endl;
}

void StubbornSetsSimple::compute_interference_relation() {
    OperatorsProxy operators = task_proxy.get_operators();
    interference_relation.resize(operators.size());

    /*
       TODO: as interference is symmetric, we only need to compute the
       relation for operators (o1, o2) with (o1 < o2) and add a lookup
       method that looks up (i, j) if i < j and (j, i) otherwise.
    */
    for (OperatorProxy op1 : operators) {
        vector<int> &interfere_op1 = interference_relation[op1.get_id()];
        for (OperatorProxy op2 : operators) {
            if (op1 != op2 && interfere(op1, op2)) {
                interfere_op1.push_back(op2.get_id());
            }
        }
    }
}

// Add all operators that achieve the fact (var, value) to stubborn set.
void StubbornSetsSimple::add_necessary_enabling_set(Fact fact) {
    for (int op_no : achievers[fact.var][fact.value]) {
        mark_as_stubborn(op_no);
    }
}

// Add all operators that interfere with op.
void StubbornSetsSimple::add_interfering(OperatorProxy op) {
    for (int interferer_no : interference_relation[op.get_id()]) {
        mark_as_stubborn(interferer_no);
    }
}

void StubbornSetsSimple::initialize_stubborn_set(const State &state) {
    // Add a necessary enabling set for an unsatisfied goal.
    Fact unsatisfied_goal = find_unsatisfied_goal(state);
    assert(unsatisfied_goal.var != -1);
    add_necessary_enabling_set(unsatisfied_goal);
}

void StubbornSetsSimple::handle_stubborn_operator(const State &state,
                                                  OperatorProxy op) {
    Fact unsatisfied_precondition = find_unsatisfied_precondition(op, state);
    if (unsatisfied_precondition.var == -1) {
        /* no unsatisfied precondition found
           => operator is applicable
           => add all interfering operators */
        add_interfering(op);
    } else {
        /* unsatisfied precondition found
           => add a necessary enabling set for it */
        add_necessary_enabling_set(unsatisfied_precondition);
    }
}

static shared_ptr<PruningMethod> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Stubborn sets simple",
        "Stubborn sets represent a state pruning method which computes a subset "
        "of applicable operators in each state such that completeness and "
        "optimality of the overall search is preserved. As stubborn sets rely "
        "on several design choices, there are different variants thereof. "
        "The variant 'StubbornSetsSimple' resolves the design choices in a "
        "straight-forward way. For details, see the following papers: "
        + utils::format_paper_reference(
            {"Yusra Alkhazraji", "Martin Wehrle", "Robert Mattmueller", "Malte Helmert"},
            "A Stubborn Set Algorithm for Optimal Planning",
            "http://ai.cs.unibas.ch/papers/alkhazraji-et-al-ecai2012.pdf",
            "Proceedings of the 20th European Conference on Artificial Intelligence "
            "(ECAI 2012)",
            "891-892",
            "IOS Press 2012")
        + utils::format_paper_reference(
            {"Martin Wehrle", "Malte Helmert"},
            "Efficient Stubborn Sets: Generalized Algorithms and Selection Strategies",
            "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS14/paper/view/7922/8042",
            "Proceedings of the 24th International Conference on Automated Planning "
            " and Scheduling (ICAPS 2014)",
            "323-331",
            "AAAI Press, 2014"));

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsSimple>();
}

static PluginShared<PruningMethod> _plugin("stubborn_sets_simple", _parse);
}
