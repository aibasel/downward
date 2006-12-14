#include "partial_relaxation.h"
#include "globals.h"

#include <cassert>
using namespace std;

/*
  var_is_relaxed is a vector index by a SAS variable number.
  var_is_relaxed[i] is true iff the i-th SAS variable should be relaxed.

  The meaning of var_to_index[i] depends on whether or variable i is a
  relaxed variable or not.

  For unrelaxed variables, var_to_index provides a consecutive
  numbering, so that e.g. if variable 2, 4, 5 are the only unrelaxed
  ones, var_to_index[2] == 0, var_to_index[4] == 1, and
  var_to_index[5] == 2.

  For relaxed variables, var_to_index provides the index for the
  *first value* for this variable in a vector of booleans. For
  example, if in the above example, variables 0, 1 and 3 are relaxed
  and have ranges 3, 5, 2, respectively, then var_to_index[0] == 0,
  var_to_index[1] == 3, and var_to_index[3] == 8.
 */

PartialRelaxation::PartialRelaxation(const vector<int> &relaxed_variables) {
    int var_count = g_variable_domain.size();
    var_is_relaxed.resize(var_count, false);
    for(int i = 0; i < relaxed_variables.size(); i++)
	var_is_relaxed[relaxed_variables[i]] = true;

    int next_relaxed_index = 0;
    int next_unrelaxed_index = 0;
    for(int i = 0; i < var_count; i++) {
	if(var_is_relaxed[i]) {
	    var_to_index.push_back(next_relaxed_index);
	    next_relaxed_index += g_variable_domain[i];
	} else {
	    var_to_index.push_back(next_unrelaxed_index);
	    next_unrelaxed_index++;
	}
    }

    max_relaxed_index = next_relaxed_index;
    max_unrelaxed_index = next_unrelaxed_index;
}

PartialRelaxation::~PartialRelaxation() {
}

bool PartialRelaxation::is_relaxed(int var_no) const {
    return var_is_relaxed[var_no];
}

int PartialRelaxation::get_unrelaxed_index(int var_no) const {
    assert(!var_is_relaxed[var_no]);
    return var_to_index[var_no];
}

int PartialRelaxation::get_relaxed_index(int var_no, int value) const {
    assert(var_is_relaxed[var_no]);
    return var_to_index[var_no] + value;
}

int PartialRelaxation::get_max_relaxed_index() const {
    return max_relaxed_index;
}

int PartialRelaxation::get_max_unrelaxed_index() const {
    return max_unrelaxed_index;
}
