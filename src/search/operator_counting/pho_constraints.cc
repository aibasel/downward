#include "pho_constraints.h"

#include "../lp_solver.h"
#include "../plugin.h"

#include "../pdbs/canonical_pdbs_heuristic.h"
#include "../pdbs/pattern_generation_haslum.h"
#include "../pdbs/pattern_generation_systematic.h"
#include "../pdbs/pdb_heuristic.h"
#include "../pdbs/util.h"

namespace OperatorCounting {
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
            pgh.extract_pattern_collection_heuristic());
        pdbs = pdb_source->get_pattern_databases();
    } else {
        vector<vector<int>> patterns;
        if (options.contains("patterns")) {
            // Manually specified patterns
            patterns = options.get<vector<vector<int>>>("patterns");
        } else {
            // Systematically generated patterns
            PatternGenerationSystematic pattern_generator(options);
            patterns = pattern_generator.get_patterns();
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

static shared_ptr<ConstraintGenerator> _parse_manual(OptionParser &parser) {
    parser.document_synopsis(
        "Posthoc optimization constraints for manually specified patterns",
        "The generator will compute a PDB for each pattern and add the "
        "constraint h(s) <= sum_{o in relevant(h)} Count_o. For details, see\n"
        " * Florian Pommerening, Gabriele Roeger and Malte Helmert.<<BR>>\n"
        " [Getting the Most Out of Pattern Databases for Classical Planning "
        "http://ijcai.org/papers13/Papers/IJCAI13-347.pdf].<<BR>>\n "
        "In //Proceedings of the Twenty-Third International Joint "
        "Conference on Artificial Intelligence (IJCAI 2013)//, "
        "pp. 2357-2364. 2013.\n\n\n");

    Options opts;
    parse_patterns(parser, opts);
    if (parser.dry_run())
        return nullptr;
    return make_shared<PhOConstraints>(opts);
}

static shared_ptr<ConstraintGenerator> _parse_systematic(OptionParser &parser) {
    parser.document_synopsis(
        "Posthoc optimization constraints for systematically generated patterns",
        "All (interesting) patterns with up to pattern_max_size variables are "
        "generated. The generator will compute a PDB for each pattern and add "
        "the constraint h(s) <= sum_{o in relevant(h)} Count_o. For details, "
        "see\n"
        " * Florian Pommerening, Gabriele Roeger and Malte Helmert.<<BR>>\n"
        " [Getting the Most Out of Pattern Databases for Classical Planning "
        "http://ijcai.org/papers13/Papers/IJCAI13-347.pdf].<<BR>>\n "
        "In //Proceedings of the Twenty-Third International Joint "
        "Conference on Artificial Intelligence (IJCAI 2013)//, "
        "pp. 2357-2364. 2013.\n\n\n");

    PatternGenerationSystematic::add_systematic_pattern_options(parser);
    Options opts = parser.parse();

    if (parser.help_mode())
        return nullptr;
    PatternGenerationSystematic::check_systematic_pattern_options(parser, opts);
    if (parser.dry_run())
        return nullptr;
    return make_shared<PhOConstraints>(opts);
}

static shared_ptr<ConstraintGenerator> _parse_ipdb(OptionParser &parser) {
    parser.document_synopsis(
        "Posthoc optimization constraints for iPDB patterns",
        "A pattern collection is discovered, using iPDB hillclimbing (see "
        "[Doc/Heuristic#iPDB])."
        "The generator will compute a PDB for each pattern and add the "
        "constraint h(s) <= sum_{o in relevant(h)} Count_o. For details, see\n"
        " * Florian Pommerening, Gabriele Roeger and Malte Helmert.<<BR>>\n"
        " [Getting the Most Out of Pattern Databases for Classical Planning "
        "http://ijcai.org/papers13/Papers/IJCAI13-347.pdf].<<BR>>\n "
        "In //Proceedings of the Twenty-Third International Joint "
        "Conference on Artificial Intelligence (IJCAI 2013)//, "
        "pp. 2357-2364. 2013.\n\n\n");

    PatternGenerationHaslum::add_hillclimbing_options(parser);
    Options opts = parser.parse();

    if (parser.help_mode())
        return nullptr;
    PatternGenerationHaslum::check_hillclimbing_options(parser, opts);
    if (parser.dry_run())
        return nullptr;

    return make_shared<PhOConstraints>(opts);
}

/* TODO: Once we have a common interface for pattern generation, we only need
         one plugin (see issue585). This will also undo the code duplication in
         the option parsers. */
static PluginShared<ConstraintGenerator> _plugin_manual(
    "pho_constraints_manual", _parse_manual);
static PluginShared<ConstraintGenerator> _plugin_systematic(
    "pho_constraints_systematic", _parse_systematic);
static PluginShared<ConstraintGenerator> _plugin_ipdb(
    "pho_constraints_ipdb", _parse_ipdb);
}
