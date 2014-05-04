#include "util.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../utilities.h"

#include "../merge_and_shrink/variable_order_finder.h"
#include <vector>

using namespace std;


static void validate_and_normalize_pattern(
    OptionParser &parser, vector<int> &pattern) {
    /*
      - Sort by variable number and remove duplicate variables.
      - Warn if duplicate variables exist.
      - Error if patterns contain out-of-range variable numbers.
    */
    sort(pattern.begin(), pattern.end());
    vector<int>::iterator it = unique(pattern.begin(), pattern.end());
    if (it != pattern.end()) {
        pattern.erase(it, pattern.end());
        parser.warning("duplicate variables in pattern have been removed");
    }
    if (!pattern.empty()) {
        if (pattern.front() < 0)
            parser.error("variable number too low in pattern");
        if (pattern.back() >= g_variable_domain.size())
            parser.error("variable number too high in pattern");
    }
}

static void validate_and_normalize_patterns(
    OptionParser &parser, vector<vector<int> > &pattern_collection) {
    /*
      - Validate and normalize each pattern (see there).
      - Sort collection lexicographically and remove duplicate patterns.
      - Warn if duplicate patterns exist.
     */

    for (size_t i = 0; i < pattern_collection.size(); ++i)
        validate_and_normalize_pattern(parser, pattern_collection[i]);
    sort(pattern_collection.begin(), pattern_collection.end());
    vector<vector<int> >::iterator it = unique(
        pattern_collection.begin(), pattern_collection.end());
    if (it != pattern_collection.end()) {
        pattern_collection.erase(it, pattern_collection.end());
        parser.warning("duplicate patterns have been removed");
    }
}

static void build_pattern_for_size_limit(
    OptionParser &parser, int size_limit, vector<int> &pattern) {
    /*
       - Error if size_limit is invalid (< 1).
       - Pattern is normalized, i.e., variable numbers are sorted.
    */
    assert(pattern.empty());

    if (size_limit < 1)
        parser.error("abstraction size must be at least 1");

    VariableOrderFinder order(GOAL_CG_LEVEL);
    size_t size = 1;
    while (true) {
        if (order.done())
            break;
        int next_var = order.next();
        int next_var_size = g_variable_domain[next_var];

        // Test if (size * next_var_size > size_limit) while guarding
        // against overflow.
        if ((size_limit - 1) / next_var_size <= size)
            break;

        pattern.push_back(next_var);
        size *= next_var_size;
    }

    validate_and_normalize_pattern(parser, pattern);
}

static void build_combo_patterns(
    OptionParser &parser, int size_limit,
    vector<vector<int> > &pattern_collection) {
    // Take one large pattern and then single-variable patterns for
    // all goal variables that are not in the large pattern.
    assert(pattern_collection.empty());
    vector<int> large_pattern;
    build_pattern_for_size_limit(parser, size_limit, large_pattern);
    pattern_collection.push_back(large_pattern);
    set<int> used_vars(large_pattern.begin(), large_pattern.end());
    for (size_t i = 0; i < g_goal.size(); ++i) {
        int goal_var = g_goal[i].first;
        if (!used_vars.count(goal_var))
            pattern_collection.push_back(vector<int>(1, goal_var));
    }
}

static void build_singleton_patterns(
    vector<vector<int> > &pattern_collection) {
    // Build singleton pattern from each goal variable.
    assert(pattern_collection.empty());
    for (size_t i = 0; i < g_goal.size(); ++i)
        pattern_collection.push_back(vector<int>(1, g_goal[i].first));
}

void parse_pattern(OptionParser &parser, Options &opts) {
    parser.add_option<int>("max_states",
        "maximal number of abstract states in the pattern database",
        "1000000");
    parser.add_list_option<int>("pattern",
        "list of variable numbers of the planning task that should be used as pattern. "
        "Default: the variables are selected automatically based on a simple greedy strategy.",
        "",
        OptionFlags(false));

    opts = parser.parse();
    if (parser.help_mode())
        return;

    vector<int> pattern;
    if (opts.contains("pattern")) {
        pattern = opts.get_list<int>("pattern");
    } else {
        build_pattern_for_size_limit(
            parser, opts.get<int>("max_states"), pattern);
    }
    validate_and_normalize_pattern(parser, pattern);
    opts.set("pattern", pattern);

    if (!parser.dry_run())
        cout << "pattern: " << pattern << endl;
}

void parse_patterns(OptionParser &parser, Options &opts) {
    parser.add_list_option<vector<int> >(
        "patterns",
        "list of patterns (which are lists of variable numbers of the planning task) "
        "Default: each goal variable is used as a single-variable pattern in the collection.",
        "",
        OptionFlags(false));
    parser.add_option<bool>(
        "combo", "use the combo strategy", "false");
    parser.add_option<int>(
        "max_states", "maximum abstraction size for combo strategy", "1000000");

    opts = parser.parse();
    if (parser.help_mode())
        return;

    vector<vector<int> > pattern_collection;
    if (opts.contains("patterns")) {
        pattern_collection = opts.get_list<vector<int> >("patterns");
    } else if (opts.get<bool>("combo")) {
        build_combo_patterns(
            parser, opts.get<int>("max_states"), pattern_collection);
    } else {
        /*
          // TODO: We'd like to have a test like in the following lines,
          // but unfortunately this would currently trigger when max_states
          // is *not* specified due to its default value. For this, we
          // would need to distinguish between the case where a parameter
          // was specified manually or got its value as a default.
        if (opts.contains("max_states"))
            parser.error("max_states cannot be specified with combo=false");
        */
        build_singleton_patterns(pattern_collection);
    }

    /* Validation is only necessary at this stage if the patterns were
       manually specified, but does not hurt in other cases.
       Normalization is always useful. */
    validate_and_normalize_patterns(parser, pattern_collection);
    opts.set("patterns", pattern_collection);

    if (!parser.dry_run())
        cout << "pattern collection: " << pattern_collection << endl;
}
