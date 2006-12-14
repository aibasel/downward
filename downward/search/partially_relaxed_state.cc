#include "partially_relaxed_state.h"
#include "axioms.h"
#include "globals.h"
#include "operator.h"
#include "partial_relaxation.h"
#include "state.h"

#include <cassert>
using namespace std;

PartiallyRelaxedState::PartiallyRelaxedState(
    const PartialRelaxation &relax, const State &state)
    : relaxation(&relax) {

    relaxed_vars.resize(relaxation->get_max_relaxed_index(), false);
    unrelaxed_vars.resize(relaxation->get_max_unrelaxed_index());

    for(int var = 0; var < g_variable_domain.size(); var++)
	set(var, state[var]);
}

void PartiallyRelaxedState::set(int var_no, int value) {
    if(relaxation->is_relaxed(var_no)) {
	int index = relaxation->get_relaxed_index(var_no, value);
	relaxed_vars[index] = true;
    } else {
	int index = relaxation->get_unrelaxed_index(var_no);
	unrelaxed_vars[index] = value;
    }
}

int PartiallyRelaxedState::get(int var_no) const {
    assert(!relaxation->is_relaxed(var_no));
    int index = relaxation->get_unrelaxed_index(var_no);
    return unrelaxed_vars[index];
}

bool PartiallyRelaxedState::has_value(int var_no, int value) const {
    if(relaxation->is_relaxed(var_no)) {
	int index = relaxation->get_relaxed_index(var_no, value);
	return relaxed_vars[index];
    } else {
	return get(var_no) == value;
    }
}

PartiallyRelaxedState::PartiallyRelaxedState(
    const PartiallyRelaxedState &predecessor, const Operator &op)
    : relaxation(predecessor.relaxation),
      relaxed_vars(predecessor.relaxed_vars),
      unrelaxed_vars(predecessor.unrelaxed_vars) {

    assert(!op.is_axiom());

    // Update values affected by operator.
    for(int i = 0; i < op.get_pre_post().size(); i++) {
	const PrePost &pre_post = op.get_pre_post()[i];
	if(pre_post.does_fire(predecessor))
	    set(pre_post.var, pre_post.post);
    }

    g_axiom_evaluator->evaluate(*this);
}

void PartiallyRelaxedState::dump() const {
    int var_count = g_variable_domain.size();
    for(int var = 0; var < var_count; var++) {
	cout << "  " << g_variable_name[var] << ":";
	if(relaxation->is_relaxed(var)) {
	    for(int value = 0; value < g_variable_domain[var]; value++)
		if(has_value(var, value))
		    cout << " " << value;
	    cout << endl;
	} else {
	    cout << " " << unrelaxed_vars[var] << endl;
	}
    }
}

bool PartiallyRelaxedState::operator<(const PartiallyRelaxedState &other) const {
    for(int i = 0; i < relaxed_vars.size(); i++)
	if(relaxed_vars[i] != other.relaxed_vars[i])
	    return (relaxed_vars[i] < other.relaxed_vars[i]);
    for(int i = 0; i < unrelaxed_vars.size(); i++)
	if(unrelaxed_vars[i] != other.unrelaxed_vars[i])
	    return (unrelaxed_vars[i] < other.unrelaxed_vars[i]);
    return false;
}

const PartialRelaxation &PartiallyRelaxedState::get_relaxation() const {
    // TODO: This should disappear at some point; see comment in
    // partially_relaxed_state.h about partially relaxed states
    // carrying info about the relaxation.
    return *relaxation;
}
