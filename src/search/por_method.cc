#include "por_method.h"

#include "option_parser.h"
#include "plugin.h"

#include <iostream>

using namespace std;

PORMethod::PORMethod() {}

PORMethod::~PORMethod() {}

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
    return nullptr;
    // if (option == NO_POR_METHOD) {
    //     return new NullPORMethod;
    // } else if (option == SIMPLE_STUBBORN_SETS) {
    //     return new SimpleStubbornSets;
    // } else if (option == SSS_EXPANSION_CORE) {
    //     return new SSS_ExpansionCore;
    // } else {
    //     cerr << "internal error: unknown POR method " << option << endl;
    //     abort();
    // }
}

static PluginTypePlugin<PORMethod> _type_plugin(
    "PORMethod",
    "Partial order reduction method");
