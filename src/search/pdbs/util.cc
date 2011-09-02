#include "util.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../utilities.h"

#include <vector>

using namespace std;

void parse_patterns(OptionParser &parser, Options &opts) {
    parser.add_list_option<vector<int> >("patterns", "the pattern collection", OptionFlags(false));
    opts = parser.parse();
    vector<vector<int> > pattern_collection;
    if (opts.contains("patterns"))
        pattern_collection = opts.get_list<vector<int> >("patterns");

    // check for correct patterns specification (only if not empty)
    if (parser.dry_run() && !pattern_collection.empty()) {
        // check if there are duplicates of patterns
        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            sort(pattern_collection[i].begin(), pattern_collection[i].end());
            // check if each pattern is valid
            int pat_old_size = pattern_collection[i].size();
            vector<int>::const_iterator it = unique(pattern_collection[i].begin(), pattern_collection[i].end());
            pattern_collection[i].resize(it - pattern_collection[i].begin());
            if (pattern_collection[i].size() != pat_old_size)
                parser.error("there are duplicates of variables in a pattern");
            if (!pattern_collection[i].empty()) {
                if (pattern_collection[i].front() < 0)
                    parser.error("there is a variable < 0 in a pattern");
                if (pattern_collection[i].back() >= g_variable_domain.size())
                    parser.error("there is a variable > number of variables in a pattern");
            }
        }
        sort(pattern_collection.begin(), pattern_collection.end());
        int coll_old_size = pattern_collection.size();
        vector<vector<int> >::const_iterator it = unique(pattern_collection.begin(), pattern_collection.end());
        pattern_collection.resize(it - pattern_collection.begin());
        if (pattern_collection.size() != coll_old_size)
            parser.error("there are duplicates of patterns in the pattern collection");

        for (size_t i = 0; i < pattern_collection.size(); ++i) {
            cout << pattern_collection[i] << endl;
        }
    }

    // if option "patterns" is not specified, use default
    if (!parser.dry_run() && !opts.contains("patterns")) {
        // Simple selection strategy. Take all goal variables as patterns.
        for (size_t i = 0; i < g_goal.size(); ++i) {
            pattern_collection.push_back(vector<int>(1, g_goal[i].first));
        }
        opts.set("patterns", pattern_collection);
    }
}
