#include "pho_constraints.h"

#include "../lp_solver.h"
#include "../plugin.h"

#include "../pdbs/canonical_pdbs_heuristic.h"
#include "../pdbs/pattern_generation_haslum.h"
#include "../pdbs/pattern_generation_systematic.h"
#include "../pdbs/pdb_heuristic.h"
#include "../pdbs/util.h"

namespace operator_counting {
PhOConstraints::PhOConstraints(const Options &opts)
    : options(opts),
      pdb_source(nullptr) {
}

PhOConstraints::~PhOConstraints() {
    if (!pdb_source) {
        for (PatternDatabase *pdb : pdbs) {
            delete pdb;
        }
    }
}

void PhOConstraints::generate_pdbs(const shared_ptr<AbstractTask> task) {
    options.set<shared_ptr<AbstractTask>>("transform", task);
    options.set<int>("cost_type", NORMAL);
    TaskProxy task_proxy(*task);
    if (options.contains("min_improvement")) {
        // iPDB patterns
        options.set<bool>("cache_estimates", false);
        PatternGenerationHaslum pgh(options);
        pdb_source = unique_ptr<CanonicalPDBsHeuristic>(
            pgh.get_pattern_collection_heuristic());
        pdbs = pdb_source->get_pattern_databases();

    } else {
        vector<vector<int> > patterns;
        if (options.contains("patterns")) {
            // Manually specified patterns
            patterns = options.get<vector<vector<int> > >("patterns");
        } else {
            // Systematically generated patterns
            if (options.get<bool>("only_interesting_patterns")) {
                PatternGenerationSystematic pattern_generator(options);
                patterns = pattern_generator.get_patterns();
            } else {
                PatternGenerationSystematicNaive pattern_generator(options);
                patterns = pattern_generator.get_patterns();
            }
        }
        for (const vector<int> &pattern : patterns) {
            if (pdbs.size() % 1000 == 0) {
                cout << "Generated " << pdbs.size() << "/"
                     << patterns.size() << " PDBs" << endl;
            }
            PatternDatabase *pdb = new PatternDatabase(task_proxy, pattern);
            pdbs.push_back(pdb);
        }
        cout << "Generated " << pdbs.size() << "/"
             << patterns.size() << " PDBs" << endl;
    }
}

void PhOConstraints::initialize_constraints(
    const std::shared_ptr<AbstractTask> task, vector<LPConstraint> &constraints,
    double infinity) {
    generate_pdbs(task);
    TaskProxy task_proxy(*task);
    constraint_offset = constraints.size();
    for (PatternDatabase *pdb : pdbs) {
        constraints.emplace_back(0, infinity);
        LPConstraint &constraint = constraints.back();
        for (OperatorProxy op : task_proxy.get_operators()) {
            if (pdb->is_operator_relevant(op)) {
                constraint.insert(op.get_id(), op.get_cost());
            }
        }
    }
}

bool PhOConstraints::update_constraints(const State &state,
                                        LPSolver &lp_solver) {
    for (size_t i = 0; i < pdbs.size(); ++i) {
        int constraint_id = constraint_offset + i;
        PatternDatabase *pdb = pdbs[i];
        int h = pdb->get_value(state);
        if (h == numeric_limits<int>::max()) {
            return true;
        }
        lp_solver.set_constraint_lower_bound(constraint_id, h);
    }
    return false;
}

static ConstraintGenerator *_parse_manual(OptionParser &parser) {
    Options opts;
    parse_patterns(parser, opts);
    if (parser.dry_run())
        return 0;
    return new PhOConstraints(opts);
}

static ConstraintGenerator *_parse_systematic(OptionParser &parser) {
    parser.document_synopsis(
        "Posthoc optimization constraints for systematically generated patterns",
        "All (interesting) patterns with up to pattern_max_size variables are "
        "generated. The generator will compute a PDB for each pattern and add "
        "the constraint h(s) <= sum_{o in relevant(h)} Count_o.");

    parser.add_option<int>(
        "pattern_max_size",
        "max number of variables per pattern",
        "1");
    parser.add_option<bool>(
        "only_interesting_patterns",
        "Only consider the union of two disjoint patterns if the union has "
        "more information than the individual patterns. Warning: switching off "
        "the option leads to the generation of a lot of useless patterns.",
        "true");

    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    return new PhOConstraints(opts);
}

static ConstraintGenerator *_parse_ipdb(OptionParser &parser) {
    parser.add_option<int>(
        "pdb_max_size",
        "maximal number of states per pattern database ",
        "2000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "collection_max_size",
        "maximal number of states in the pattern collection",
        "20000000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "num_samples",
        "number of samples (random states) on which to evaluate each "
        "candidate pattern collection",
        "1000",
        Bounds("1", "infinity"));
    parser.add_option<int>(
        "min_improvement",
        "minimum number of samples on which a candidate pattern "
        "collection must improve on the current one to be considered "
        "as the next pattern collection ",
        "10",
        Bounds("1", "infinity"));
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds for improving the initial pattern "
        "collection via hill climbing. If set to 0, no hill climbing "
        "is performed at all.",
        "infinity",
        Bounds("0.0", "infinity"));

    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    if (opts.get<int>("min_improvement") > opts.get<int>("num_samples"))
        parser.error("minimum improvement must not be higher than number of "
                     "samples");

    if (parser.dry_run())
        return 0;
    return new PhOConstraints(opts);
}

/* TODO: Once we have a common interface for pattern generation, we only need
         one plugin (see issue585). This will also undo the code duplication in
         the option parsers. */
static Plugin<ConstraintGenerator> _plugin_manual(
    "pho_constraints_manual", _parse_manual);
static Plugin<ConstraintGenerator> _plugin_systematic(
    "pho_constraints_systematic", _parse_systematic);
static Plugin<ConstraintGenerator> _plugin_ipdb(
    "pho_constraints_ipdb", _parse_ipdb);
}
