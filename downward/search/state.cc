#include "state.h"

#include "axioms.h"
#include "globals.h"
#include "operator.h"

#include <algorithm>
#include <iostream>
#include <cassert>
using namespace std;

State::State(istream &in) {
    check_magic(in, "begin_state");
    for(int i = 0; i < g_variable_domain.size(); i++) {
	int var;
	cin >> var;
	vars.push_back(var);
    }
    check_magic(in, "end_state");

    g_default_axiom_values = vars;
}

State::State(const State &predecessor, const Operator &op)
    : vars(predecessor.vars) {
    assert(!op.is_axiom());

    // Update values affected by operator.
    for(int i = 0; i < op.get_pre_post().size(); i++) {
	const PrePost &pre_post = op.get_pre_post()[i];
	if(pre_post.does_fire(predecessor))
	    vars[pre_post.var] = pre_post.post;
    }

    g_axiom_evaluator->evaluate(*this);
}

void State::dump() const {
    for(int i = 0; i < vars.size(); i++)
	cout << "  " << g_variable_name[i] << ": " << vars[i] << endl;
}

bool State::operator<(const State &other) const {
    return lexicographical_compare(vars.begin(), vars.end(),
				   other.vars.begin(), other.vars.end());
}
