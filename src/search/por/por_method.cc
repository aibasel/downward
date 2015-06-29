#include "por_method.h"

// TODO: We can get rid of the following includes once we have the
//       plugin mechanism in place for this.
#include "simple_stubborn_sets.h"
#include "sss_expansion_core.h"
#include "../option_parser.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace POR {
PORMethod::PORMethod() {}

PORMethod::~PORMethod() {}

NullPORMethod::NullPORMethod() {}

NullPORMethod::~NullPORMethod() {}

void NullPORMethod::dump_options() const {
    cout << "partial order reduction method: none" << endl;
}

PORMethodWithStatistics::PORMethodWithStatistics()
    : unpruned_successors_generated(0),
      pruned_successors_generated(0) {
}

PORMethodWithStatistics::~PORMethodWithStatistics() {
}

void PORMethodWithStatistics::prune_operators(
    const GlobalState &state, std::vector<const GlobalOperator *> &ops) {
    unpruned_successors_generated += ops.size();
    do_pruning(state, ops);
    pruned_successors_generated += ops.size();
}

void PORMethodWithStatistics::dump_statistics() const {
    cout << "total successors before partial-order reduction: "
         << unpruned_successors_generated << endl
         << "total successors after partial-order reduction: "
         << pruned_successors_generated << endl;
}

void add_parser_option(OptionParser &parser, const string &option_name) {
    // TODO: Use the plug-in mechanism here.
    vector<string> por_methods;
    vector<string> por_methods_doc;
    
    por_methods.push_back("NONE");
    por_methods_doc.push_back("no partial order reduction");
    por_methods.push_back("SIMPLE_STUBBORN_SETS");
    por_methods_doc.push_back("simple stubborn sets");
    por_methods.push_back("SSS_EXPANSION_CORE");
    por_methods_doc.push_back("strong stubborn sets that dominate expansion core");
    parser.add_enum_option(option_name, por_methods, 
			   "partial-order reduction method to be used", 
			   "NONE", 
			   por_methods_doc);
}

PORMethod *create(int option) {
    if (option == NO_POR_METHOD) {
        return new NullPORMethod;
    } else if (option == SIMPLE_STUBBORN_SETS) {
        return new SimpleStubbornSets;
    } else if (option == SSS_EXPANSION_CORE) {
        return new SSS_ExpansionCore;
    } else {
        cerr << "internal error: unknown POR method " << option << endl;
        abort();
    }
}
}
