#include "util.h"

#include "../option_parser.h"
#include "../utilities.h"

#include "../variable_order_finder.h"
#include <vector>

using namespace std;


void validate_and_normalize_pattern(const TaskProxy &task_proxy, vector<int> &pattern) {
    /*
      - Sort by variable number and remove duplicate variables.
      - Warn if duplicate variables exist.
      - Error if patterns contain out-of-range variable numbers.
    */
    sort(pattern.begin(), pattern.end());
    vector<int>::iterator it = unique(pattern.begin(), pattern.end());
    if (it != pattern.end()) {
        pattern.erase(it, pattern.end());
        cout << "Warning: duplicate variables in pattern have been removed" << endl;
    }
    if (!pattern.empty()) {
        if (pattern.front() < 0) {
            cerr << "Variable number too low in pattern" << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        int num_variables = task_proxy.get_variables().size();
        if (pattern.back() >= num_variables) {
            cerr << "Variable number too high in pattern" << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
    }
}

void validate_and_normalize_patterns(const TaskProxy &task_proxy, vector<vector<int>> &patterns) {
    /*
      - Validate and normalize each pattern (see there).
      - Sort collection lexicographically and remove duplicate patterns.
      - Warn if duplicate patterns exist.
    */
    for (auto &pattern : patterns)
        validate_and_normalize_pattern(task_proxy, pattern);
    sort(patterns.begin(), patterns.end());
    auto it = unique(patterns.begin(), patterns.end());
    if (it != patterns.end()) {
        patterns.erase(it, patterns.end());
        cout << "Warning: duplicate patterns have been removed" << endl;
    }
}


// TODO issue585: remove this function
static void validate_and_normalize_pattern(
    TaskProxy task_proxy, OptionParser &parser, vector<int> &pattern) {
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
        int num_variables = task_proxy.get_variables().size();
        if (pattern.back() >= num_variables)
            parser.error("variable number too high in pattern");
    }
}

// TODO issue585: remove this function
static void validate_and_normalize_patterns(
    TaskProxy task_proxy, OptionParser &parser,
    vector<vector<int>> &pattern_collection) {
    /*
      - Validate and normalize each pattern (see there).
      - Sort collection lexicographically and remove duplicate patterns.
      - Warn if duplicate patterns exist.
    */
    for (size_t i = 0; i < pattern_collection.size(); ++i)
        validate_and_normalize_pattern(task_proxy, parser,
                                       pattern_collection[i]);
    sort(pattern_collection.begin(), pattern_collection.end());
    vector<vector<int>>::iterator it = unique(
        pattern_collection.begin(), pattern_collection.end());
    if (it != pattern_collection.end()) {
        pattern_collection.erase(it, pattern_collection.end());
        parser.warning("duplicate patterns have been removed");
    }
}

static void build_singleton_patterns(
    TaskProxy task_proxy, vector<vector<int>> &pattern_collection) {
    // Build singleton pattern from each goal variable.
    assert(pattern_collection.empty());
    for (FactProxy goal : task_proxy.get_goals()) {
        int goal_var_id = goal.get_variable().get_id();
        pattern_collection.emplace_back(1, goal_var_id);
    }
}

void parse_patterns(OptionParser &parser, Options &opts) {
    if (!parser.help_mode())
        assert(parser.is_valid_option("transform"));
    parser.add_list_option<vector<int>>(
        "patterns",
        "list of patterns (which are lists of variable numbers of the planning "
        "task). Default: each goal variable is used as a single-variable "
        "pattern in the collection.",
        OptionParser::NONE);
    parser.add_option<bool>(
        "combo", "use the combo strategy", "false");
    parser.add_option<int>(
        "max_states",
        "maximum abstraction size for combo strategy",
        "1000000",
        Bounds("1", "infinity"));

    opts = parser.parse();
    if (parser.help_mode())
        return;

    const shared_ptr<AbstractTask> task = get_task_from_options(opts);
    TaskProxy task_proxy(*task);

    vector<vector<int>> pattern_collection;
    if (opts.contains("patterns")) {
        pattern_collection = opts.get_list<vector<int>>("patterns");
    } else if (opts.get<bool>("combo")) {
/*
        // TODO issue585: remove the method parse_patterns
        build_combo_patterns(
            task, parser, opts.get<int>("max_states"), pattern_collection);
*/
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
        build_singleton_patterns(task_proxy, pattern_collection);
    }

    /* Validation is only necessary at this stage if the patterns were
       manually specified, but does not hurt in other cases.
       Normalization is always useful. */
    validate_and_normalize_patterns(task_proxy, parser, pattern_collection);
    opts.set("patterns", pattern_collection);

    if (!parser.dry_run())
        cout << "pattern collection: " << pattern_collection << endl;
}
