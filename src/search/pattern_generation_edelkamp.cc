#include "pattern_generation_edelkamp.h"
#include "globals.h"
#include "pdb_collection_heuristic.h"
#include "plugin.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using namespace std;

static ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run);
static ScalarEvaluatorPlugin pattern_generation_edelkamp_plugin("gapdb", create);

PatternGenerationEdelkamp::PatternGenerationEdelkamp(int initial_pdb_max_size) {
    initialize(initial_pdb_max_size);
}

PatternGenerationEdelkamp::~PatternGenerationEdelkamp() {
}

void PatternGenerationEdelkamp::initialize(int initial_pdb_max_size) {
    vector<int> variables;
    variables.resize(g_variable_domain.size());
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        variables[i] = i;
    }
    
    int MAX = 1;
    vector<vector<vector<int> > > pattern_collections;
    for (size_t num_pcs = 0; num_pcs < MAX; ++num_pcs) {
        vector<vector<int> > pattern_collection;
        int i = 0;
        while (i < variables.size()) {
            int current_size = 1;
            vector<int> pattern;
            while (i < variables.size() && current_size * g_variable_domain[variables[i]] < initial_pdb_max_size) {
                pattern.push_back(variables[i]);
                cout << "pushing variable " << variables[i] << endl;
                current_size *= g_variable_domain[variables[i]];
                ++i;
            }
            assert(!pattern.empty());
            pattern_collection.push_back(pattern);
            cout << "pushing pattern collection:" << endl;
            for (size_t j = 0; j < pattern.size(); ++j) {
                cout << pattern[j] << " ";
            }
            cout << endl;
        }
        pattern_collections.push_back(pattern_collection);
        random_shuffle(variables.begin(), variables.end());
        cout << "new variable ordering after shuffling:" << endl;
        for (size_t i = 0; i < g_variable_domain.size(); ++i) {
            cout << variables[i] << " ";
        }
        cout << endl;
    }
    
    cout << "finished" << endl;
    current_collection = new PDBCollectionHeuristic(pattern_collections[0]);
    cout << "test" << endl;
}

ScalarEvaluator *create(const vector<string> &config, int start, int &end, bool dry_run) {
    int initial_pdb_max_size = 32;
    //int max_collection_size = 20000000;
    if (config.size() > start + 2 && config[start + 1] == "(") {
        end = start + 2;
        if (config[end] != ")") {
            NamedOptionParser option_parser;
            option_parser.add_int_option("max_pdb_size", &initial_pdb_max_size, "maximum size per pdb");
            //option_parser.add_int_option("max_collection_size", &max_collection_size, "max collection size");
            option_parser.parse_options(config, end, end, dry_run);
            end++;
        }
        if (config[end] != ")")
            throw ParseError(end);
    } else {
        end = start;
    }
    
    // TODO: required meaningful value
    if (initial_pdb_max_size < 10) {
        cerr << "error: size per pdb must be at least 10" << endl;
        exit(2);
    }
    
    if (dry_run)
        return 0;
    
    PatternGenerationEdelkamp pge(initial_pdb_max_size);
    cout << "Edelkamp et al. done." << endl;
    return pge.get_pattern_collection_heuristic();
}
