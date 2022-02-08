#include "landmark_factory_h_m.h"

#include "exploration.h"
#include "landmark.h"

#include "../abstract_task.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/system.h"

using namespace std;
using utils::ExitCode;

namespace landmarks {
// alist = alist \cup other
template<typename T>
void union_with(list<T> &alist, const list<T> &other) {
    typename list<T>::iterator it1 = alist.begin();
    typename list<T>::const_iterator it2 = other.begin();

    while ((it1 != alist.end()) && (it2 != other.end())) {
        if (*it1 < *it2) {
            ++it1;
        } else if (*it1 > *it2) {
            alist.insert(it1, *it2);
            ++it2;
        } else {
            ++it1;
            ++it2;
        }
    }
    alist.insert(it1, it2, other.end());
}

// alist = alist \cap other
template<typename T>
void intersect_with(list<T> &alist, const list<T> &other) {
    typename list<T>::iterator it1 = alist.begin(), tmp;
    typename list<T>::const_iterator it2 = other.begin();

    while ((it1 != alist.end()) && (it2 != other.end())) {
        if (*it1 < *it2) {
            tmp = it1;
            ++tmp;
            alist.erase(it1);
            it1 = tmp;
        } else if (*it1 > *it2) {
            ++it2;
        } else {
            ++it1;
            ++it2;
        }
    }
    alist.erase(it1, alist.end());
}

// alist = alist \setminus other
template<typename T>
void set_minus(list<T> &alist, const list<T> &other) {
    typename list<T>::iterator it1 = alist.begin(), tmp;
    typename list<T>::const_iterator it2 = other.begin();

    while ((it1 != alist.end()) && (it2 != other.end())) {
        if (*it1 < *it2) {
            ++it1;
        } else if (*it1 > *it2) {
            ++it2;
        } else {
            tmp = it1;
            ++tmp;
            alist.erase(it1);
            it1 = tmp;
            ++it2;
        }
    }
}

// alist = alist \cup {val}
template<typename T>
void insert_into(list<T> &alist, const T &val) {
    typename list<T>::iterator it1 = alist.begin();

    while (it1 != alist.end()) {
        if (*it1 > val) {
            alist.insert(it1, val);
            return;
        } else if (*it1 < val) {
            ++it1;
        } else {
            return;
        }
    }
    alist.insert(it1, val);
}

template<typename T>
static bool contains(list<T> &alist, const T &val) {
    return find(alist.begin(), alist.end(), val) != alist.end();
}


// find partial variable assignments with size m or less
// (look at all the variables in the problem)
void LandmarkFactoryHM::get_m_sets_(const VariablesProxy &variables, int m, int num_included, int current_var,
                                    FluentSet &current,
                                    vector<FluentSet> &subsets) {
    int num_variables = variables.size();
    if (num_included == m) {
        subsets.push_back(current);
        return;
    }
    if (current_var == num_variables) {
        if (num_included != 0) {
            subsets.push_back(current);
        }
        return;
    }
    // include a value of current_var in the set
    for (int i = 0; i < variables[current_var].get_domain_size(); ++i) {
        bool use_var = true;
        FactPair current_var_fact(current_var, i);
        for (const FactPair &current_fact : current) {
            if (!interesting(variables, current_var_fact, current_fact)) {
                use_var = false;
                break;
            }
        }

        if (use_var) {
            current.push_back(current_var_fact);
            get_m_sets_(variables, m, num_included + 1, current_var + 1, current, subsets);
            current.pop_back();
        }
    }
    // don't include a value of current_var in the set
    get_m_sets_(variables, m, num_included, current_var + 1, current, subsets);
}

// find all size m or less subsets of superset
void LandmarkFactoryHM::get_m_sets_of_set(const VariablesProxy &variables,
                                          int m, int num_included,
                                          int current_var_index,
                                          FluentSet &current,
                                          vector<FluentSet> &subsets,
                                          const FluentSet &superset) {
    if (num_included == m) {
        subsets.push_back(current);
        return;
    }

    if (current_var_index == static_cast<int>(superset.size())) {
        if (num_included != 0) {
            subsets.push_back(current);
        }
        return;
    }

    bool use_var = true;
    for (const FactPair &fluent : current) {
        if (!interesting(variables, superset[current_var_index], fluent)) {
            use_var = false;
            break;
        }
    }

    if (use_var) {
        // include current fluent in the set
        current.push_back(superset[current_var_index]);
        get_m_sets_of_set(variables, m, num_included + 1, current_var_index + 1, current, subsets, superset);
        current.pop_back();
    }

    // don't include current fluent in set
    get_m_sets_of_set(variables, m, num_included, current_var_index + 1, current, subsets, superset);
}

// get subsets of superset1 \cup superset2 with size m or less,
// such that they have >= 1 elements from each set.
void LandmarkFactoryHM::get_split_m_sets(
    const VariablesProxy &variables,
    int m, int ss1_num_included, int ss2_num_included,
    int ss1_var_index, int ss2_var_index,
    FluentSet &current, vector<FluentSet> &subsets,
    const FluentSet &superset1, const FluentSet &superset2) {
    /*
       if( ((ss1_var_index == superset1.size()) && (ss1_num_included == 0)) ||
        ((ss2_var_index == superset2.size()) && (ss2_num_included == 0)) ) {
       return;
       }
     */

    int sup1_size = superset1.size();
    int sup2_size = superset2.size();

    if (ss1_num_included + ss2_num_included == m ||
        (ss1_var_index == sup1_size && ss2_var_index == sup2_size)) {
        // if set is empty, don't have to include from it
        if ((ss1_num_included > 0 || sup1_size == 0) &&
            (ss2_num_included > 0 || sup2_size == 0)) {
            subsets.push_back(current);
        }
        return;
    }

    bool use_var = true;

    if (ss1_var_index != sup1_size &&
        (ss2_var_index == sup2_size ||
         superset1[ss1_var_index] < superset2[ss2_var_index])) {
        for (const FactPair &fluent : current) {
            if (!interesting(variables, superset1[ss1_var_index], fluent)) {
                use_var = false;
                break;
            }
        }

        if (use_var) {
            // include
            current.push_back(superset1[ss1_var_index]);
            get_split_m_sets(variables, m, ss1_num_included + 1, ss2_num_included,
                             ss1_var_index + 1, ss2_var_index,
                             current, subsets, superset1, superset2);
            current.pop_back();
        }

        // don't include
        get_split_m_sets(variables, m, ss1_num_included, ss2_num_included,
                         ss1_var_index + 1, ss2_var_index,
                         current, subsets, superset1, superset2);
    } else {
        for (const FactPair &fluent : current) {
            if (!interesting(variables, superset2[ss2_var_index], fluent)) {
                use_var = false;
                break;
            }
        }

        if (use_var) {
            // include
            current.push_back(superset2[ss2_var_index]);
            get_split_m_sets(variables, m, ss1_num_included, ss2_num_included + 1,
                             ss1_var_index, ss2_var_index + 1,
                             current, subsets, superset1, superset2);
            current.pop_back();
        }

        // don't include
        get_split_m_sets(variables, m, ss1_num_included, ss2_num_included,
                         ss1_var_index, ss2_var_index + 1,
                         current, subsets, superset1, superset2);
    }
}

// use together is method that determines whether the two variables are interesting together,
// e.g. we don't want to represent (truck1-loc x, truck2-loc y) type stuff

// get partial assignments of size <= m in the problem
void LandmarkFactoryHM::get_m_sets(const VariablesProxy &variables, int m, vector<FluentSet> &subsets) {
    FluentSet c;
    get_m_sets_(variables, m, 0, 0, c, subsets);
}

// get subsets of superset with size <= m
void LandmarkFactoryHM::get_m_sets(const VariablesProxy &variables,
                                   int m, vector<FluentSet> &subsets,
                                   const FluentSet &superset) {
    FluentSet c;
    get_m_sets_of_set(variables, m, 0, 0, c, subsets, superset);
}

// second function to get subsets of size at most m that
// have at least one element in ss1 and same in ss2
// assume disjoint
void LandmarkFactoryHM::get_split_m_sets(
    const VariablesProxy &variables,
    int m, vector<FluentSet> &subsets,
    const FluentSet &superset1, const FluentSet &superset2) {
    FluentSet c;
    get_split_m_sets(variables, m, 0, 0, 0, 0, c, subsets, superset1, superset2);
}

// get subsets of state with size <= m
void LandmarkFactoryHM::get_m_sets(const VariablesProxy &variables, int m,
                                   vector<FluentSet> &subsets,
                                   const State &state) {
    FluentSet state_fluents;
    for (FactProxy fact : state) {
        state_fluents.push_back(fact.get_pair());
    }
    get_m_sets(variables, m, subsets, state_fluents);
}

void LandmarkFactoryHM::print_proposition(const VariablesProxy &variables, const FactPair &fluent) const {
    VariableProxy var = variables[fluent.var];
    FactProxy fact = var.get_fact(fluent.value);
    utils::g_log << fact.get_name()
                 << " (" << var.get_name() << "(" << fact.get_variable().get_id() << ")"
                 << "->" << fact.get_value() << ")";
}

static FluentSet get_operator_precondition(const OperatorProxy &op) {
    FluentSet preconditions = task_properties::get_fact_pairs(op.get_preconditions());
    sort(preconditions.begin(), preconditions.end());
    return preconditions;
}

// get facts that are always true after the operator application
// (effects plus prevail conditions)
static FluentSet get_operator_postcondition(int num_vars, const OperatorProxy &op) {
    FluentSet postconditions;
    EffectsProxy effects = op.get_effects();
    vector<bool> has_effect_on_var(num_vars, false);

    for (EffectProxy effect : effects) {
        FactProxy effect_fact = effect.get_fact();
        postconditions.push_back(effect_fact.get_pair());
        has_effect_on_var[effect_fact.get_variable().get_id()] = true;
    }

    for (FactProxy precondition : op.get_preconditions()) {
        if (!has_effect_on_var[precondition.get_variable().get_id()])
            postconditions.push_back(precondition.get_pair());
    }

    sort(postconditions.begin(), postconditions.end());
    return postconditions;
}


void LandmarkFactoryHM::print_pm_op(const VariablesProxy &variables, const PMOp &op) {
    set<FactPair> pcs, effs, cond_pc, cond_eff;
    vector<pair<set<FactPair>, set<FactPair>>> conds;

    for (int pc : op.pc) {
        for (const FactPair &fluent : h_m_table_[pc].fluents) {
            pcs.insert(fluent);
        }
    }
    for (int eff : op.eff) {
        for (const FactPair &fluent : h_m_table_[eff].fluents) {
            effs.insert(fluent);
        }
    }
    for (size_t i = 0; i < op.cond_noops.size(); ++i) {
        cond_pc.clear();
        cond_eff.clear();
        int pm_fluent;
        size_t j;
        utils::g_log << "PC:" << endl;
        for (j = 0; (pm_fluent = op.cond_noops[i][j]) != -1; ++j) {
            print_fluentset(variables, h_m_table_[pm_fluent].fluents);
            utils::g_log << endl;

            for (size_t k = 0; k < h_m_table_[pm_fluent].fluents.size(); ++k) {
                cond_pc.insert(h_m_table_[pm_fluent].fluents[k]);
            }
        }
        // advance to effects section
        utils::g_log << endl;
        ++j;

        utils::g_log << "EFF:" << endl;
        for (; j < op.cond_noops[i].size(); ++j) {
            int pm_fluent = op.cond_noops[i][j];

            print_fluentset(variables, h_m_table_[pm_fluent].fluents);
            utils::g_log << endl;

            for (size_t k = 0; k < h_m_table_[pm_fluent].fluents.size(); ++k) {
                cond_eff.insert(h_m_table_[pm_fluent].fluents[k]);
            }
        }
        conds.emplace_back(cond_pc, cond_eff);
        utils::g_log << endl << endl << endl;
    }

    utils::g_log << "Action " << op.index << endl;
    utils::g_log << "Precondition: ";
    for (const FactPair &pc : pcs) {
        print_proposition(variables, pc);
        utils::g_log << " ";
    }

    utils::g_log << endl << "Effect: ";
    for (const FactPair &eff : effs) {
        print_proposition(variables, eff);
        utils::g_log << " ";
    }
    utils::g_log << endl << "Conditionals: " << endl;
    int i = 0;
    for (const auto &cond : conds) {
        utils::g_log << "Cond PC #" << i++ << ":" << endl << "\t";
        for (const FactPair &pc : cond.first) {
            print_proposition(variables, pc);
            utils::g_log << " ";
        }
        utils::g_log << endl << "Cond Effect #" << i << ":" << endl << "\t";
        for (const FactPair &eff : cond.second) {
            print_proposition(variables, eff);
            utils::g_log << " ";
        }
        utils::g_log << endl << endl;
    }
}

void LandmarkFactoryHM::print_fluentset(const VariablesProxy &variables, const FluentSet &fs) {
    utils::g_log << "( ";
    for (const FactPair &fact : fs) {
        print_proposition(variables, fact);
        utils::g_log << " ";
    }
    utils::g_log << ")";
}

// check whether fs2 is a possible noop set for action with fs1 as effect
// sets cannot be 1) defined on same variable, 2) otherwise mutex
bool LandmarkFactoryHM::possible_noop_set(const VariablesProxy &variables,
                                          const FluentSet &fs1,
                                          const FluentSet &fs2) {
    FluentSet::const_iterator fs1it = fs1.begin(), fs2it = fs2.begin();

    while (fs1it != fs1.end() && fs2it != fs2.end()) {
        if (fs1it->var == fs2it->var) {
            return false;
        } else if (fs1it->var < fs2it->var) {
            ++fs1it;
        } else {
            ++fs2it;
        }
    }

    for (const FactPair &fluent1 : fs1) {
        FactProxy fact1 = variables[fluent1.var].get_fact(fluent1.value);
        for (const FactPair &fluent2 : fs2) {
            if (fact1.is_mutex(
                    variables[fluent2.var].get_fact(fluent2.value)))
                return false;
        }
    }

    return true;
}


// make the operators of the P_m problem
void LandmarkFactoryHM::build_pm_ops(const TaskProxy &task_proxy) {
    FluentSet pc, eff;
    vector<FluentSet> pc_subsets, eff_subsets, noop_pc_subsets, noop_eff_subsets;

    static int op_count = 0;
    int set_index, noop_index;

    OperatorsProxy operators = task_proxy.get_operators();
    pm_ops_.resize(operators.size());

    // set unsatisfied precondition counts, used in fixpoint calculation
    unsat_pc_count_.resize(operators.size());

    VariablesProxy variables = task_proxy.get_variables();

    // transfer ops from original problem
    // represent noops as "conditional" effects
    for (OperatorProxy op : operators) {
        PMOp &pm_op = pm_ops_[op.get_id()];
        pm_op.index = op_count++;

        pc_subsets.clear();
        eff_subsets.clear();

        // preconditions of P_m op are all subsets of original pc
        pc = get_operator_precondition(op);
        get_m_sets(variables, m_, pc_subsets, pc);
        pm_op.pc.reserve(pc_subsets.size());

        // set unsatisfied pc count for op
        unsat_pc_count_[op.get_id()].first = pc_subsets.size();

        for (const FluentSet &pc_subset : pc_subsets) {
            assert(set_indices_.find(pc_subset) != set_indices_.end());
            set_index = set_indices_[pc_subset];
            pm_op.pc.push_back(set_index);
            h_m_table_[set_index].pc_for.emplace_back(op.get_id(), -1);
        }

        // same for effects
        eff = get_operator_postcondition(variables.size(), op);
        get_m_sets(variables, m_, eff_subsets, eff);
        pm_op.eff.reserve(eff_subsets.size());

        for (const FluentSet &eff_subset : eff_subsets) {
            assert(set_indices_.find(eff_subset) != set_indices_.end());
            set_index = set_indices_[eff_subset];
            pm_op.eff.push_back(set_index);
        }

        noop_index = 0;

        // For all subsets used in the problem with size *<* m, check whether
        // they conflict with the effect of the operator (no need to check pc
        // because mvvs appearing in pc also appear in effect

        FluentSetToIntMap::const_iterator it = set_indices_.begin();
        while (static_cast<int>(it->first.size()) < m_
               && it != set_indices_.end()) {
            if (possible_noop_set(variables, eff, it->first)) {
                // for each such set, add a "conditional effect" to the operator
                pm_op.cond_noops.resize(pm_op.cond_noops.size() + 1);

                vector<int> &this_cond_noop = pm_op.cond_noops.back();

                noop_pc_subsets.clear();
                noop_eff_subsets.clear();

                // get the subsets that have >= 1 element in the pc (unless pc is empty)
                // and >= 1 element in the other set

                get_split_m_sets(variables, m_, noop_pc_subsets, pc, it->first);
                get_split_m_sets(variables, m_, noop_eff_subsets, eff, it->first);

                this_cond_noop.reserve(noop_pc_subsets.size() + noop_eff_subsets.size() + 1);

                unsat_pc_count_[op.get_id()].second.push_back(noop_pc_subsets.size());

                // push back all noop preconditions
                for (size_t j = 0; j < noop_pc_subsets.size(); ++j) {
                    assert(static_cast<int>(noop_pc_subsets[j].size()) <= m_);
                    assert(set_indices_.find(noop_pc_subsets[j]) != set_indices_.end());

                    set_index = set_indices_[noop_pc_subsets[j]];
                    this_cond_noop.push_back(set_index);
                    // these facts are "conditional pcs" for this action
                    h_m_table_[set_index].pc_for.emplace_back(op.get_id(), noop_index);
                }

                // separator
                this_cond_noop.push_back(-1);

                // and the noop effects
                for (size_t j = 0; j < noop_eff_subsets.size(); ++j) {
                    assert(static_cast<int>(noop_eff_subsets[j].size()) <= m_);
                    assert(set_indices_.find(noop_eff_subsets[j]) != set_indices_.end());

                    set_index = set_indices_[noop_eff_subsets[j]];
                    this_cond_noop.push_back(set_index);
                }

                ++noop_index;
            }
            ++it;
        }
        //    print_pm_op(pm_ops_[i]);
    }
}

bool LandmarkFactoryHM::interesting(const VariablesProxy &variables,
                                    const FactPair &fact1, const FactPair &fact2) const {
    // mutexes can always be safely pruned
    return !variables[fact1.var].get_fact(fact1.value).is_mutex(
        variables[fact2.var].get_fact(fact2.value));
}

LandmarkFactoryHM::LandmarkFactoryHM(const options::Options &opts)
    : m_(opts.get<int>("m")),
      conjunctive_landmarks(opts.get<bool>("conjunctive_landmarks")),
      use_orders(opts.get<bool>("use_orders")) {
}

void LandmarkFactoryHM::initialize(const TaskProxy &task_proxy) {
    utils::g_log << "h^m landmarks m=" << m_ << endl;
    if (!task_proxy.get_axioms().empty()) {
        cerr << "h^m landmarks don't support axioms" << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
    // Get all the m or less size subsets in the domain.
    vector<vector<FactPair>> msets;
    get_m_sets(task_proxy.get_variables(), m_, msets);

    // map each set to an integer
    for (size_t i = 0; i < msets.size(); ++i) {
        h_m_table_.emplace_back();
        set_indices_[msets[i]] = i;
        h_m_table_[i].fluents = msets[i];
    }
    utils::g_log << "Using " << h_m_table_.size() << " P^m fluents." << endl;

    build_pm_ops(task_proxy);
}

void LandmarkFactoryHM::postprocess(const TaskProxy &task_proxy) {
    if (!conjunctive_landmarks)
        discard_conjunctive_landmarks();
    lm_graph->set_landmark_ids();

    if (!use_orders)
        discard_all_orderings();

    calc_achievers(task_proxy);
    mk_acyclic_graph();
}

void LandmarkFactoryHM::discard_conjunctive_landmarks() {
    if (lm_graph->get_num_conjunctive_landmarks() > 0) {
        utils::g_log << "Discarding " << lm_graph->get_num_conjunctive_landmarks()
                     << " conjunctive landmarks" << endl;
        lm_graph->remove_node_if(
            [](const LandmarkNode &node) {return node.get_landmark().conjunctive;});
    }
}

void LandmarkFactoryHM::calc_achievers(const TaskProxy &task_proxy) {
    assert(!achievers_calculated);
    utils::g_log << "Calculating achievers." << endl;

    OperatorsProxy operators = task_proxy.get_operators();
    VariablesProxy variables = task_proxy.get_variables();
    // first_achievers are already filled in by compute_h_m_landmarks
    // here only have to do possible_achievers
    for (auto &lm_node : lm_graph->get_nodes()) {
        Landmark &landmark = lm_node->get_landmark();
        set<int> candidates;
        // put all possible adders in candidates set
        for (const FactPair &lm_fact : landmark.facts) {
            const vector<int> &ops = get_operators_including_eff(lm_fact);
            candidates.insert(ops.begin(), ops.end());
        }

        for (int op_id : candidates) {
            FluentSet post = get_operator_postcondition(variables.size(), operators[op_id]);
            FluentSet pre = get_operator_precondition(operators[op_id]);
            size_t j;
            for (j = 0; j < landmark.facts.size(); ++j) {
                const FactPair &lm_fact = landmark.facts[j];
                // action adds this element of lm as well
                if (find(post.begin(), post.end(), lm_fact) != post.end())
                    continue;
                bool is_mutex = false;
                for (const FactPair &fluent : post) {
                    if (variables[fluent.var].get_fact(fluent.value).is_mutex(
                            variables[lm_fact.var].get_fact(lm_fact.value))) {
                        is_mutex = true;
                        break;
                    }
                }
                if (is_mutex) {
                    break;
                }
                for (const FactPair &fluent : pre) {
                    // we know that lm_val is not added by the operator
                    // so if it incompatible with the pc, this can't be an achiever
                    if (variables[fluent.var].get_fact(fluent.value).is_mutex(
                            variables[lm_fact.var].get_fact(lm_fact.value))) {
                        is_mutex = true;
                        break;
                    }
                }
                if (is_mutex) {
                    break;
                }
            }
            if (j == landmark.facts.size()) {
                // not inconsistent with any of the other landmark fluents
                landmark.possible_achievers.insert(op_id);
            }
        }
    }
    achievers_calculated = true;
}

void LandmarkFactoryHM::free_unneeded_memory() {
    utils::release_vector_memory(h_m_table_);
    utils::release_vector_memory(pm_ops_);
    utils::release_vector_memory(unsat_pc_count_);

    set_indices_.clear();
    lm_node_table_.clear();
}

// called when a fact is discovered or its landmarks change
// to trigger required actions at next level
// newly_discovered = first time fact becomes reachable
void LandmarkFactoryHM::propagate_pm_fact(int factindex, bool newly_discovered,
                                          TriggerSet &trigger) {
    // for each action/noop for which fact is a pc
    for (const FactPair &info : h_m_table_[factindex].pc_for) {
        // a pc for the action itself
        if (info.value == -1) {
            if (newly_discovered) {
                --unsat_pc_count_[info.var].first;
            }
            // add to queue if unsatcount at 0
            if (unsat_pc_count_[info.var].first == 0) {
                // create empty set or clear prev entries -- signals do all possible noop effects
                trigger[info.var].clear();
            }
        }
        // a pc for a conditional noop
        else {
            if (newly_discovered) {
                --unsat_pc_count_[info.var].second[info.value];
            }
            // if associated action is applicable, and effect has become applicable
            // (if associated action is not applicable, all noops will be used when it first does)
            if ((unsat_pc_count_[info.var].first == 0) &&
                (unsat_pc_count_[info.var].second[info.value] == 0)) {
                // if not already triggering all noops, add this one
                if ((trigger.find(info.var) == trigger.end()) ||
                    (!trigger[info.var].empty())) {
                    trigger[info.var].insert(info.value);
                }
            }
        }
    }
}

void LandmarkFactoryHM::compute_h_m_landmarks(const TaskProxy &task_proxy) {
    // get subsets of initial state
    vector<FluentSet> init_subsets;
    get_m_sets(task_proxy.get_variables(), m_, init_subsets, task_proxy.get_initial_state());

    TriggerSet current_trigger, next_trigger;

    // for all of the initial state <= m subsets, mark level = 0
    for (size_t i = 0; i < init_subsets.size(); ++i) {
        int index = set_indices_[init_subsets[i]];
        h_m_table_[index].level = 0;

        // set actions to be applied
        propagate_pm_fact(index, true, current_trigger);
    }

    // mark actions with no precondition to be applied
    for (size_t i = 0; i < pm_ops_.size(); ++i) {
        if (unsat_pc_count_[i].first == 0) {
            // create empty set or clear prev entries
            current_trigger[i].clear();
        }
    }

    vector<int>::iterator it;
    TriggerSet::iterator op_it;

    list<int> local_landmarks;
    list<int> local_necessary;

    size_t prev_size;

    int level = 1;

    // while we have actions to apply
    while (!current_trigger.empty()) {
        for (op_it = current_trigger.begin(); op_it != current_trigger.end(); ++op_it) {
            local_landmarks.clear();
            local_necessary.clear();

            int op_index = op_it->first;
            PMOp &action = pm_ops_[op_index];

            // gather landmarks for pcs
            // in the set of landmarks for each fact, the fact itself is not stored
            // (only landmarks preceding it)
            for (it = action.pc.begin(); it != action.pc.end(); ++it) {
                union_with(local_landmarks, h_m_table_[*it].landmarks);
                insert_into(local_landmarks, *it);

                if (use_orders) {
                    insert_into(local_necessary, *it);
                }
            }

            for (it = action.eff.begin(); it != action.eff.end(); ++it) {
                if (h_m_table_[*it].level != -1) {
                    prev_size = h_m_table_[*it].landmarks.size();
                    intersect_with(h_m_table_[*it].landmarks, local_landmarks);

                    // if the add effect appears in local landmarks,
                    // fact is being achieved for >1st time
                    // no need to intersect for gn orderings
                    // or add op to first achievers
                    if (!contains(local_landmarks, *it)) {
                        insert_into(h_m_table_[*it].first_achievers, op_index);
                        if (use_orders) {
                            intersect_with(h_m_table_[*it].necessary, local_necessary);
                        }
                    }

                    if (h_m_table_[*it].landmarks.size() != prev_size)
                        propagate_pm_fact(*it, false, next_trigger);
                } else {
                    h_m_table_[*it].level = level;
                    h_m_table_[*it].landmarks = local_landmarks;
                    if (use_orders) {
                        h_m_table_[*it].necessary = local_necessary;
                    }
                    insert_into(h_m_table_[*it].first_achievers, op_index);
                    propagate_pm_fact(*it, true, next_trigger);
                }
            }

            // landmarks changed for action itself, have to recompute
            // landmarks for all noop effects
            if (op_it->second.empty()) {
                for (size_t i = 0; i < action.cond_noops.size(); ++i) {
                    // actions pcs are satisfied, but cond. effects may still have
                    // unsatisfied pcs
                    if (unsat_pc_count_[op_index].second[i] == 0) {
                        compute_noop_landmarks(op_index, i,
                                               local_landmarks,
                                               local_necessary,
                                               level, next_trigger);
                    }
                }
            }
            // only recompute landmarks for conditions whose
            // landmarks have changed
            else {
                for (set<int>::iterator noop_it = op_it->second.begin();
                     noop_it != op_it->second.end(); ++noop_it) {
                    assert(unsat_pc_count_[op_index].second[*noop_it] == 0);

                    compute_noop_landmarks(op_index, *noop_it,
                                           local_landmarks,
                                           local_necessary,
                                           level, next_trigger);
                }
            }
        }
        current_trigger.swap(next_trigger);
        next_trigger.clear();

        utils::g_log << "Level " << level << " completed." << endl;
        ++level;
    }
    utils::g_log << "h^m landmarks computed." << endl;
}

void LandmarkFactoryHM::compute_noop_landmarks(
    int op_index, int noop_index,
    list<int> const &local_landmarks,
    list<int> const &local_necessary,
    int level,
    TriggerSet &next_trigger) {
    list<int> cn_necessary, cn_landmarks;
    size_t prev_size;
    int pm_fluent;

    PMOp &action = pm_ops_[op_index];
    vector<int> &pc_eff_pair = action.cond_noops[noop_index];

    cn_landmarks.clear();

    cn_landmarks = local_landmarks;

    if (use_orders) {
        cn_necessary.clear();
        cn_necessary = local_necessary;
    }

    size_t i;
    for (i = 0; (pm_fluent = pc_eff_pair[i]) != -1; ++i) {
        union_with(cn_landmarks, h_m_table_[pm_fluent].landmarks);
        insert_into(cn_landmarks, pm_fluent);

        if (use_orders) {
            insert_into(cn_necessary, pm_fluent);
        }
    }

    // go to the beginning of the effects section
    ++i;

    for (; i < pc_eff_pair.size(); ++i) {
        pm_fluent = pc_eff_pair[i];
        if (h_m_table_[pm_fluent].level != -1) {
            prev_size = h_m_table_[pm_fluent].landmarks.size();
            intersect_with(h_m_table_[pm_fluent].landmarks, cn_landmarks);

            // if the add effect appears in cn_landmarks,
            // fact is being achieved for >1st time
            // no need to intersect for gn orderings
            // or add op to first achievers
            if (!contains(cn_landmarks, pm_fluent)) {
                insert_into(h_m_table_[pm_fluent].first_achievers, op_index);
                if (use_orders) {
                    intersect_with(h_m_table_[pm_fluent].necessary, cn_necessary);
                }
            }

            if (h_m_table_[pm_fluent].landmarks.size() != prev_size)
                propagate_pm_fact(pm_fluent, false, next_trigger);
        } else {
            h_m_table_[pm_fluent].level = level;
            h_m_table_[pm_fluent].landmarks = cn_landmarks;
            if (use_orders) {
                h_m_table_[pm_fluent].necessary = cn_necessary;
            }
            insert_into(h_m_table_[pm_fluent].first_achievers, op_index);
            propagate_pm_fact(pm_fluent, true, next_trigger);
        }
    }
}

void LandmarkFactoryHM::add_lm_node(int set_index, bool goal) {
    if (lm_node_table_.find(set_index) == lm_node_table_.end()) {
        const HMEntry &hm_entry = h_m_table_[set_index];
        vector<FactPair> facts(hm_entry.fluents);
        utils::sort_unique(facts);
        assert(!facts.empty());
        Landmark landmark(facts, false, (facts.size() > 1), goal);
        landmark.first_achievers.insert(
            hm_entry.first_achievers.begin(),
            hm_entry.first_achievers.end());
        lm_node_table_[set_index] = &lm_graph->add_landmark(move(landmark));
    }
}

void LandmarkFactoryHM::generate_landmarks(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    initialize(task_proxy);
    compute_h_m_landmarks(task_proxy);
    // now construct landmarks graph
    vector<FluentSet> goal_subsets;
    FluentSet goals = task_properties::get_fact_pairs(task_proxy.get_goals());
    VariablesProxy variables = task_proxy.get_variables();
    get_m_sets(variables, m_, goal_subsets, goals);
    list<int> all_lms;
    for (const FluentSet &goal_subset : goal_subsets) {
        assert(set_indices_.find(goal_subset) != set_indices_.end());

        int set_index = set_indices_[goal_subset];

        if (h_m_table_[set_index].level == -1) {
            utils::g_log << endl << endl << "Subset of goal not reachable !!." << endl << endl << endl;
            utils::g_log << "Subset is: ";
            print_fluentset(variables, h_m_table_[set_index].fluents);
            utils::g_log << endl;
        }

        // set up goals landmarks for processing
        union_with(all_lms, h_m_table_[set_index].landmarks);

        // the goal itself is also a lm
        insert_into(all_lms, set_index);

        // make a node for the goal, with in_goal = true;
        add_lm_node(set_index, true);
    }
    // now make remaining lm nodes
    for (int lm : all_lms) {
        add_lm_node(lm, false);
    }
    if (use_orders) {
        // do reduction of graph
        // if f2 is landmark for f1, subtract landmark set of f2 from that of f1
        for (int f1 : all_lms) {
            list<int> everything_to_remove;
            for (int f2 : h_m_table_[f1].landmarks) {
                union_with(everything_to_remove, h_m_table_[f2].landmarks);
            }
            set_minus(h_m_table_[f1].landmarks, everything_to_remove);
            // remove necessaries here, otherwise they will be overwritten
            // since we are writing them as greedy nec. orderings.
            if (use_orders)
                set_minus(h_m_table_[f1].landmarks, h_m_table_[f1].necessary);
        }

        // and add the edges

        for (int set_index : all_lms) {
            for (int lm : h_m_table_[set_index].landmarks) {
                assert(lm_node_table_.find(lm) != lm_node_table_.end());
                assert(lm_node_table_.find(set_index) != lm_node_table_.end());

                edge_add(*lm_node_table_[lm], *lm_node_table_[set_index], EdgeType::NATURAL);
            }
            if (use_orders) {
                for (int gn : h_m_table_[set_index].necessary) {
                    edge_add(*lm_node_table_[gn], *lm_node_table_[set_index], EdgeType::GREEDY_NECESSARY);
                }
            }
        }
    }
    free_unneeded_memory();

    postprocess(task_proxy);
}

bool LandmarkFactoryHM::computes_reasonable_orders() const {
    return false;
}

bool LandmarkFactoryHM::supports_conditional_effects() const {
    return false;
}

static shared_ptr<LandmarkFactory> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "h^m Landmarks",
        "The landmark generation method introduced by "
        "Keyder, Richter & Helmert (ECAI 2010).");
    parser.add_option<int>(
        "m", "subset size (if unsure, use the default of 2)", "2");
    parser.add_option<bool>(
        "conjunctive_landmarks",
        "keep conjunctive landmarks",
        "true");
    _add_use_orders_option_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return nullptr;

    parser.document_language_support("conditional_effects",
                                     "ignored, i.e. not supported");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkFactoryHM>(opts);
}

static Plugin<LandmarkFactory> _plugin("lm_hm", _parse);
}
