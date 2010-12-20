#include "pattern_generation_haslum.h"
#include "plugin.h"
#include "pdb_collection_heuristic.h"
#include "globals.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <cassert>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_haslum_plugin("haslum", create);

PatternGenerationHaslum::PatternGenerationHaslum(int max_pdb_memory) {
    cout << max_pdb_memory << endl;
    hill_climbing();
}

PatternGenerationHaslum::~PatternGenerationHaslum() {
}

void PatternGenerationHaslum::hill_climbing() {
    assert(pattern_collection.empty());
    // initial collection: a pdb for each goal variable
    for (size_t i = 0; i < g_goal.size(); ++i) {
        pattern_collection.push_back(vector<int>(1, g_goal[i].first));
    }
    //while (true) {
        // if termination criterion -> end and return pattern collection
        // else compute "successor" pattern collections
        // evaluate successors and set new current collection
    //}
}

const vector<vector<int> > &PatternGenerationHaslum::get_pattern_collection() {
    return pattern_collection;
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    //OptionParser::instance()->set_end_for_simple_config(config, start, end);
    if (dry_run)
        return 0;
    
    int max_pdb_memory = -1;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_pdb_memory", &max_pdb_memory, "maximum pdbs size");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }
    // TODO: Default value
    if (max_pdb_memory == -1) {
        max_pdb_memory = 1000000;
    }
    // TODO: required meaningful value
    if (max_pdb_memory < 1) {
        cerr << "error: abstraction size must be at least 1" << endl;
        exit(2);
    }
    
    PatternGenerationHaslum pgh(max_pdb_memory);
    return new PDBCollectionHeuristic(pgh.get_pattern_collection());
}