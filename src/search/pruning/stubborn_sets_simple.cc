#include "stubborn_sets_simple.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/markup.h"


using namespace std;

namespace stubborn_sets_simple {
StubbornSetsSimple::StubbornSetsSimple(const options::Options &opts)
    : StubbornSets(opts) {
}

void StubbornSetsSimple::initialize(const shared_ptr<AbstractTask> &task) {
    StubbornSets::initialize(task);
    interference_relation.resize(num_operators);
    interference_relation_computed.resize(num_operators, false);
    cout << "pruning method: stubborn sets simple" << endl;
}

const vector<int> &StubbornSetsSimple::get_interfering_operators(int op1_no) {
    /*
       TODO: as interference is symmetric, we only need to compute the
       relation for operators (o1, o2) with (o1 < o2) and add a lookup
       method that looks up (i, j) if i < j and (j, i) otherwise.
    */
    vector<int> &interfere_op1 = interference_relation[op1_no];
    if (!interference_relation_computed[op1_no]) {
        for (int op2_no = 0; op2_no < num_operators; ++op2_no) {
            if (op1_no != op2_no && interfere(op1_no, op2_no)) {
                interfere_op1.push_back(op2_no);
            }
        }
        interfere_op1.shrink_to_fit();
        interference_relation_computed[op1_no] = true;
    }
    return interfere_op1;
}

// Add all operators that achieve the fact (var, value) to stubborn set.
void StubbornSetsSimple::add_necessary_enabling_set(const FactPair &fact) {
    for (int op_no : achievers[fact.var][fact.value]) {
        mark_as_stubborn(op_no);
    }
}

// Add all operators that interfere with op.
void StubbornSetsSimple::add_interfering(int op_no) {
    for (int interferer_no : get_interfering_operators(op_no)) {
        mark_as_stubborn(interferer_no);
    }
}

void StubbornSetsSimple::initialize_stubborn_set(const State &state) {
    // Add a necessary enabling set for an unsatisfied goal.
    FactPair unsatisfied_goal = find_unsatisfied_goal(state);
    assert(unsatisfied_goal != FactPair::no_fact);
    add_necessary_enabling_set(unsatisfied_goal);
}

void StubbornSetsSimple::handle_stubborn_operator(const State &state,
                                                  int op_no) {
    FactPair unsatisfied_precondition = find_unsatisfied_precondition(op_no, state);
    if (unsatisfied_precondition == FactPair::no_fact) {
        /* no unsatisfied precondition found
           => operator is applicable
           => add all interfering operators */
        add_interfering(op_no);
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
        + utils::format_conference_reference(
            {"Yusra Alkhazraji", "Martin Wehrle", "Robert Mattmueller", "Malte Helmert"},
            "A Stubborn Set Algorithm for Optimal Planning",
            "https://ai.dmi.unibas.ch/papers/alkhazraji-et-al-ecai2012.pdf",
            "Proceedings of the 20th European Conference on Artificial Intelligence "
            "(ECAI 2012)",
            "891-892",
            "IOS Press",
            "2012")
        + utils::format_conference_reference(
            {"Martin Wehrle", "Malte Helmert"},
            "Efficient Stubborn Sets: Generalized Algorithms and Selection Strategies",
            "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS14/paper/view/7922/8042",
            "Proceedings of the 24th International Conference on Automated Planning "
            " and Scheduling (ICAPS 2014)",
            "323-331",
            "AAAI Press",
            "2014"));

    stubborn_sets::add_pruning_options(parser);

    Options opts = parser.parse();

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<StubbornSetsSimple>(opts);
}

static Plugin<PruningMethod> _plugin("stubborn_sets_simple", _parse);
}
