#include "globals.h"
#include "operator.h"
#include "option_parser.h"
#include "timer.h"
#include "utilities.h"


#include <iostream>
using namespace std;


void register_parsers();

int main(int argc, const char **argv) {
    register_event_handlers();

    string usage =
        "usage: \n" +
        string(argv[0]) + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
        "* SEARCH (SearchEngine): configuration of the search algorithm\n"
        "* OUTPUT (filename): preprocessor output\n\n"
        "Options:\n"
        "--heuristic HEURISTIC_PREDEFINITION\n"
        "    Predefines a heuristic that can afterwards be referenced\n"
        "    by the name that is specified in the definition.\n"
        "--random-seed SEED\n"
        "    Use random seed SEED\n\n"
        "See http://www.fast-downward.org/ for details.";

    if (argc < 2) {
        cout << usage << endl;
        exit(1);
    }

    // read prepropressor output first because we need to know the initial
    // state when we create a general lazy search engine
    bool poly_time_method = false;

    istream &in = cin;

    in >> poly_time_method;
    if (poly_time_method) {
        cout << "Poly-time method not implemented in this branch." << endl;
        cout << "Starting normal solver." << endl;
    }

    read_everything(in);

    SearchEngine *engine = 0;

    //the input will be parsed twice: 
    //once in dry-run mode, to check for simple input errors, 
    //then in normal mode
    OptionParser::parse_cmd_line(char **argv, true);
    OptionParser::parse_cmd_line(char **argv, false);
    

    Timer search_timer;
    engine->search();
    search_timer.stop();
    g_timer.stop();

    if (engine->found_solution())
        save_plan(engine->get_plan());
    engine->statistics();
    engine->heuristic_statistics();
    cout << "Search time: " << search_timer << endl;
    cout << "Total time: " << g_timer << endl;

    return engine->found_solution() ? 0 : 1;
}


