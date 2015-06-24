#include "h_m_landmarks.h"
#include "../plugin.h"
#include "../exact_timer.h"


std::ostream & operator<<(std::ostream &os, const Fluent &p) {
    return os << "(" << p.first << ", " << p.second << ")";
}

std::ostream &operator<<(std::ostream &os, const FluentSet &fs) {
    FluentSet::const_iterator it;
    os << "[";
    for (it = fs.begin(); it != fs.end(); ++it) {
        os << *it << " ";
    }
    os << "]";
    return os;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const std::list<T> &alist) {
    typename std::list<T>::const_iterator it;

    os << "(";
    for (it = alist.begin(); it != alist.end(); ++it) {
        os << *it << " ";
    }
    os << ")";
    return os;
}

// alist = alist \cup other
template<typename T>
void union_with(std::list<T> &alist, const std::list<T> &other) {
    typename std::list<T>::iterator it1 = alist.begin();
    typename std::list<T>::const_iterator it2 = other.begin();

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
void intersect_with(std::list<T> &alist, const std::list<T> &other) {
    typename std::list<T>::iterator it1 = alist.begin(), tmp;
    typename std::list<T>::const_iterator it2 = other.begin();

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
void set_minus(std::list<T> &alist, const std::list<T> &other) {
    typename std::list<T>::iterator it1 = alist.begin(), tmp;
    typename std::list<T>::const_iterator it2 = other.begin();

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
void insert_into(std::list<T> &alist, const T &val) {
    typename std::list<T>::iterator it1 = alist.begin();

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
bool contains(std::list<T> &alist, const T &val) {
    typename std::list<T>::iterator it1 = alist.begin();

    for (; it1 != alist.end(); ++it1) {
        if (*it1 == val) {
            return true;
        }
    }
    return false;
}


// find partial variable assignments with size m or less
// (look at all the variables in the problem)
void HMLandmarks::get_m_sets_(int m, int num_included, int current_var,
                              FluentSet &current,
                              std::vector<FluentSet > &subsets) {
    int num_variables = g_variable_domain.size();
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
    for (int i = 0; i < g_variable_domain[current_var]; ++i) {
        bool use_var = true;
        for (size_t j = 0; j < current.size(); ++j) {
            if (!interesting(current_var, i, current[j].first, current[j].second)) {
                use_var = false;
                break;
            }
        }

        if (use_var) {
            current.push_back(std::make_pair(current_var, i));
            get_m_sets_(m, num_included + 1, current_var + 1, current, subsets);
            current.pop_back();
        }
    }
    // don't include a value of current_var in the set
    get_m_sets_(m, num_included, current_var + 1, current, subsets);
}

// find all size m or less subsets of superset
void HMLandmarks::get_m_sets_of_set_(int m, int num_included, int current_var_index,
                                     FluentSet &current,
                                     std::vector<FluentSet > &subsets,
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
    for (size_t i = 0; i < current.size(); ++i) {
        if (!interesting(superset[current_var_index].first, superset[current_var_index].second,
                         current[i].first, current[i].second)) {
            use_var = false;
            break;
        }
    }

    if (use_var) {
        // include current fluent in the set
        current.push_back(superset[current_var_index]);
        get_m_sets_of_set_(m, num_included + 1, current_var_index + 1, current, subsets, superset);
        current.pop_back();
    }

    // don't include current fluent in set
    get_m_sets_of_set_(m, num_included, current_var_index + 1, current, subsets, superset);
}

// get subsets of superset1 \cup superset2 with size m or less,
// such that they have >= 1 elements from each set.
void HMLandmarks::get_split_m_sets_(
    int m, int ss1_num_included, int ss2_num_included,
    int ss1_var_index, int ss2_var_index,
    FluentSet &current, std::vector<FluentSet> &subsets,
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
        for (size_t i = 0; i < current.size(); ++i) {
            if (!interesting(superset1[ss1_var_index].first,
                             superset1[ss1_var_index].second,
                             current[i].first, current[i].second)) {
                use_var = false;
                break;
            }
        }

        if (use_var) {
            // include
            current.push_back(superset1[ss1_var_index]);
            get_split_m_sets_(m, ss1_num_included + 1, ss2_num_included,
                              ss1_var_index + 1, ss2_var_index,
                              current, subsets, superset1, superset2);
            current.pop_back();
        }

        // don't include
        get_split_m_sets_(m, ss1_num_included, ss2_num_included,
                          ss1_var_index + 1, ss2_var_index,
                          current, subsets, superset1, superset2);
    } else {
        for (size_t i = 0; i < current.size(); ++i) {
            if (!interesting(superset2[ss2_var_index].first, superset2[ss2_var_index].second,
                             current[i].first, current[i].second)) {
                use_var = false;
                break;
            }
        }

        if (use_var) {
            // include
            current.push_back(superset2[ss2_var_index]);
            get_split_m_sets_(m, ss1_num_included, ss2_num_included + 1,
                              ss1_var_index, ss2_var_index + 1,
                              current, subsets, superset1, superset2);
            current.pop_back();
        }

        // don't include
        get_split_m_sets_(m, ss1_num_included, ss2_num_included,
                          ss1_var_index, ss2_var_index + 1,
                          current, subsets, superset1, superset2);
    }
}

// use together is method that determines whether the two variables are interesting together,
// e.g. we don't want to represent (truck1-loc x, truck2-loc y) type stuff

// get partial assignments of size <= m in the problem
void HMLandmarks::get_m_sets(int m, std::vector<FluentSet> &subsets) {
    FluentSet c;
    get_m_sets_(m, 0, 0, c, subsets);
}

// get subsets of superset with size <= m
void HMLandmarks::get_m_sets(int m, std::vector<FluentSet> &subsets, const FluentSet &superset) {
    FluentSet c;
    get_m_sets_of_set_(m, 0, 0, c, subsets, superset);
}

// second function to get subsets of size at most m that
// have at least one element in ss1 and same in ss2
// assume disjoint
void HMLandmarks::get_split_m_sets(
    int m, std::vector<FluentSet> &subsets,
    const FluentSet &superset1, const FluentSet &superset2) {
    FluentSet c;
    get_split_m_sets_(m, 0, 0, 0, 0, c, subsets, superset1, superset2);
}

// get subsets of state with size <= m
void HMLandmarks::get_m_sets(int m,
                             std::vector<FluentSet> &subsets,
                             const GlobalState &s) {
    FluentSet state_fluents;
    for (size_t i = 0; i < g_variable_domain.size(); ++i) {
        state_fluents.push_back(std::make_pair(i, s[i]));
    }
    get_m_sets(m, subsets, state_fluents);
}

void HMLandmarks::print_proposition(const pair<int, int> &fluent) const {
    cout << g_fact_names[fluent.first][fluent.second]
    << " (" << g_variable_name[fluent.first] << "(" << fluent.first << ")"
    << "->" << fluent.second << ")";
}

void get_operator_precondition(int op_index, FluentSet &pc) {
    GlobalOperator &op = g_operators[op_index];

    const std::vector<GlobalCondition> &preconditions = op.get_preconditions();
    for (size_t i = 0; i < preconditions.size(); ++i)
        pc.push_back(make_pair(preconditions[i].var, preconditions[i].val));

    std::sort(pc.begin(), pc.end());
}

// get facts that are always true after the operator application
// (effects plus prevail conditions)
void get_operator_postcondition(int op_index, FluentSet &post) {
    GlobalOperator &op = g_operators[op_index];

    const std::vector<GlobalCondition> &preconditions = op.get_preconditions();
    const std::vector<GlobalEffect> &effects = op.get_effects();
    std::vector<bool> has_effect_on_var(g_variable_domain.size(), false);

    for (size_t i = 0; i < effects.size(); ++i) {
        post.push_back(make_pair(effects[i].var, effects[i].val));
        has_effect_on_var[effects[i].var] = true;
    }

    for (size_t i = 0; i < preconditions.size(); ++i) {
        if (!has_effect_on_var[preconditions[i].var])
            post.push_back(make_pair(preconditions[i].var, preconditions[i].val));
    }

    std::sort(post.begin(), post.end());
}


void HMLandmarks::print_pm_op(const PMOp &op) {
    std::set<Fluent> pcs, effs, cond_pc, cond_eff;
    std::vector<std::pair<std::set<Fluent>, std::set<Fluent> > > conds;
    std::set<Fluent>::iterator it;

    std::vector<int>::const_iterator v_it;

    for (v_it = op.pc.begin(); v_it != op.pc.end(); ++v_it) {
        for (size_t j = 0; j < h_m_table_[*v_it].fluents.size(); ++j) {
            pcs.insert(h_m_table_[*v_it].fluents[j]);
        }
    }
    for (v_it = op.eff.begin(); v_it != op.eff.end(); ++v_it) {
        for (size_t j = 0; j < h_m_table_[*v_it].fluents.size(); ++j) {
            effs.insert(h_m_table_[*v_it].fluents[j]);
        }
    }
    for (size_t i = 0; i < op.cond_noops.size(); ++i) {
        cond_pc.clear();
        cond_eff.clear();
        int pm_fluent;
        size_t j;
        std::cout << "PC:" << std::endl;
        for (j = 0; (pm_fluent = op.cond_noops[i][j]) != -1; ++j) {
            print_fluentset(h_m_table_[pm_fluent].fluents);
            std::cout << std::endl;

            for (size_t k = 0; k < h_m_table_[pm_fluent].fluents.size(); ++k) {
                cond_pc.insert(h_m_table_[pm_fluent].fluents[k]);
            }
        }
        // advance to effects section
        std::cout << std::endl;
        ++j;

        std::cout << "EFF:" << std::endl;
        for (; j < op.cond_noops[i].size(); ++j) {
            int pm_fluent = op.cond_noops[i][j];

            print_fluentset(h_m_table_[pm_fluent].fluents);
            std::cout << std::endl;

            for (size_t k = 0; k < h_m_table_[pm_fluent].fluents.size(); ++k) {
                cond_eff.insert(h_m_table_[pm_fluent].fluents[k]);
            }
        }
        conds.push_back(std::make_pair(cond_pc, cond_eff));
        std::cout << std::endl << std::endl << std::endl;
    }

    std::cout << "Action " << op.index << std::endl;
    std::cout << "Precondition: ";
    for (it = pcs.begin(); it != pcs.end(); ++it) {
        print_proposition(*it);
        std::cout << " ";
    }

    std::cout << std::endl << "Effect: ";
    for (it = effs.begin(); it != effs.end(); ++it) {
        print_proposition(*it);
        std::cout << " ";
    }
    std::cout << std::endl << "Conditionals: " << std::endl;
    for (size_t i = 0; i < conds.size(); ++i) {
        std::cout << "Cond PC #" << i << ":" << std::endl << "\t";
        for (it = conds[i].first.begin(); it != conds[i].first.end(); ++it) {
            print_proposition(*it);
            std::cout << " ";
        }
        std::cout << std::endl << "Cond Effect #" << i << ":" << std::endl << "\t";
        for (it = conds[i].second.begin(); it != conds[i].second.end(); ++it) {
            print_proposition(*it);
            std::cout << " ";
        }
        std::cout << std::endl << std::endl;
    }
}

void HMLandmarks::print_fluentset(const FluentSet &fs) {
    FluentSet::const_iterator it;

    std::cout << "( ";
    for (it = fs.begin(); it != fs.end(); ++it) {
        print_proposition(*it);
        std::cout << " ";
    }
    std::cout << ")";
}

// check whether fs2 is a possible noop set for action with fs1 as effect
// sets cannot be 1) defined on same variable, 2) otherwise mutex
bool HMLandmarks::possible_noop_set(const FluentSet &fs1, const FluentSet &fs2) {
    FluentSet::const_iterator fs1it = fs1.begin(), fs2it = fs2.begin();

    while (fs1it != fs1.end() && fs2it != fs2.end()) {
        if (fs1it->first == fs2it->first) {
            return false;
        } else if (fs1it->first < fs2it->first) {
            ++fs1it;
        } else {
            ++fs2it;
        }
    }

    for (fs1it = fs1.begin(); fs1it != fs1.end(); ++fs1it) {
        for (fs2it = fs2.begin(); fs2it != fs2.end(); ++fs2it) {
            if (are_mutex(make_pair(fs1it->first, fs1it->second), make_pair(fs2it->first, fs2it->second)))
                return false;
        }
    }

    return true;
}


// make the operators of the P_m problem
void HMLandmarks::build_pm_ops() {
    FluentSet pc, eff;
    std::vector<FluentSet> pc_subsets, eff_subsets, noop_pc_subsets, noop_eff_subsets;

    static int op_count = 0;
    int set_index, noop_index;

    pm_ops_.resize(g_operators.size());

    // set unsatisfied precondition counts, used in fixpoint calculation
    unsat_pc_count_.resize(g_operators.size());

    // transfer ops from original problem
    // represent noops as "conditional" effects
    for (size_t i = 0; i < g_operators.size(); ++i) {
        PMOp &op = pm_ops_[i];
        op.index = op_count++;

        pc.clear();
        eff.clear();

        pc_subsets.clear();
        eff_subsets.clear();

        // preconditions of P_m op are all subsets of original pc
        get_operator_precondition(i, pc);
        get_m_sets(m_, pc_subsets, pc);
        op.pc.reserve(pc_subsets.size());

        // set unsatisfied pc count for op
        unsat_pc_count_[i].first = pc_subsets.size();

        for (size_t j = 0; j < pc_subsets.size(); ++j) {
            assert(set_indices_.find(pc_subsets[j]) != set_indices_.end());
            set_index = set_indices_[pc_subsets[j]];
            op.pc.push_back(set_index);
            h_m_table_[set_index].pc_for.push_back(std::make_pair(i, -1));
        }

        // same for effects
        get_operator_postcondition(i, eff);
        get_m_sets(m_, eff_subsets, eff);
        op.eff.reserve(eff_subsets.size());

        for (size_t j = 0; j < eff_subsets.size(); ++j) {
            assert(set_indices_.find(eff_subsets[j]) != set_indices_.end());
            set_index = set_indices_[eff_subsets[j]];
            op.eff.push_back(set_index);
        }

        noop_index = 0;

        // For all subsets used in the problem with size *<* m, check whether
        // they conflict with the effect of the operator (no need to check pc
        // because mvvs appearing in pc also appear in effect

        FluentSetToIntMap::const_iterator it = set_indices_.begin();
        while (static_cast<int>(it->first.size()) < m_
               && it != set_indices_.end()) {
            if (possible_noop_set(eff, it->first)) {
                // for each such set, add a "conditional effect" to the operator
                op.cond_noops.resize(op.cond_noops.size() + 1);

                std::vector<int> &this_cond_noop = op.cond_noops.back();

                noop_pc_subsets.clear();
                noop_eff_subsets.clear();

                // get the subsets that have >= 1 element in the pc (unless pc is empty)
                // and >= 1 element in the other set

                get_split_m_sets(m_, noop_pc_subsets, pc, it->first);
                get_split_m_sets(m_, noop_eff_subsets, eff, it->first);

                this_cond_noop.reserve(noop_pc_subsets.size() + noop_eff_subsets.size() + 1);

                unsat_pc_count_[i].second.push_back(noop_pc_subsets.size());

                // push back all noop preconditions
                for (size_t j = 0; j < noop_pc_subsets.size(); ++j) {
                    assert(static_cast<int>(noop_pc_subsets[j].size()) <= m_);
                    assert(set_indices_.find(noop_pc_subsets[j]) != set_indices_.end());

                    set_index = set_indices_[noop_pc_subsets[j]];
                    this_cond_noop.push_back(set_index);
                    // these facts are "conditional pcs" for this action
                    h_m_table_[set_index].pc_for.push_back(std::make_pair(i, noop_index));
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

bool HMLandmarks::interesting(int var1, int val1, int var2, int val2) {
    // mutexes can always be safely pruned
    return !are_mutex(make_pair(var1, val1), make_pair(var2, val2));
}

HMLandmarks::HMLandmarks(const Options &opts)
    : LandmarkFactory(opts),
      m_(opts.get<int>("m")) {
    std::cout << "H_m_Landmarks(" << m_ << ")" << std::endl;
    if (!g_axioms.empty()) {
        cerr << "H_m_Landmarks do not support axioms" << endl;
        exit_with(EXIT_UNSUPPORTED);
    }
    // need this to be able to print propositions for debugging
    // already called in global.cc
    //  read_external_inconsistencies();

    // moving all code from here to init(), this can be called in generate
    // we can then free all unneeded memory after computation is done.
}

void HMLandmarks::init() {
    // get all the m or less size subsets in the domain
    std::vector<std::vector<Fluent> > msets;
    get_m_sets(m_, msets);
    //  std::cout << "P^m index\tP fluents" << std::endl;

    // map each set to an integer
    for (size_t i = 0; i < msets.size(); ++i) {
        h_m_table_.push_back(HMEntry());
        set_indices_[msets[i]] = i;
        h_m_table_[i].fluents = msets[i];
        /*
           std::cout << i << "\t";
           print_fluentset(h_m_table_[i].fluents);
           std::cout << std::endl;
         */
    }
    std::cout << "Using " << h_m_table_.size() << " P^m fluents."
    << std::endl;

    // unsatisfied pc counts are now in build pm ops

    build_pm_ops();
    //  std::cout << "Built P(m) ops, total: " << pm_ops_.size() << "." << std::endl;
}

void HMLandmarks::calc_achievers() {
    std::cout << "Calculating achievers." << std::endl;

    // first_achievers are already filled in by compute_h_m_landmarks
    // here only have to do possible_achievers
    for (std::set<LandmarkNode *>::iterator it = lm_graph->get_nodes().begin();
         it != lm_graph->get_nodes().end(); ++it) {
        LandmarkNode &lmn = **it;

        std::set<int> candidates;
        // put all possible adders in candidates set
        for (size_t i = 0; i < lmn.vars.size(); ++i) {
            const std::vector<int> &ops =
                lm_graph->get_operators_including_eff(std::make_pair(lmn.vars[i],
                                                                     lmn.vals[i]));
            candidates.insert(ops.begin(), ops.end());
        }

        for (std::set<int>::iterator cands_it = candidates.begin();
             cands_it != candidates.end(); ++cands_it) {
            int op = *cands_it;

            FluentSet post, pre;
            get_operator_postcondition(op, post);
            get_operator_precondition(op, pre);
            size_t j;
            for (j = 0; j < lmn.vars.size(); ++j) {
                std::pair<int, int> lm_val = std::make_pair(lmn.vars[j], lmn.vals[j]);
                // action adds this element of lm as well
                if (std::find(post.begin(), post.end(), lm_val) != post.end())
                    continue;
                size_t k;
                for (k = 0; k < post.size(); ++k) {
                    if (are_mutex(post[k], lm_val)) {
                        break;
                    }
                }
                if (k != post.size()) {
                    break;
                }
                for (k = 0; k < pre.size(); ++k) {
                    // we know that lm_val is not added by the operator
                    // so if it incompatible with the pc, this can't be an achiever
                    if (are_mutex(pre[k], lm_val)) {
                        break;
                    }
                }
                if (k != pre.size()) {
                    break;
                }
            }
            if (j == lmn.vars.size()) {
                // not inconsistent with any of the other landmark fluents
                lmn.possible_achievers.insert(op);
            }
        }
    }
}

void HMLandmarks::free_unneeded_memory() {
    release_vector_memory(h_m_table_);
    release_vector_memory(pm_ops_);
    release_vector_memory(interesting_);
    release_vector_memory(unsat_pc_count_);

    set_indices_.clear();
    lm_node_table_.clear();
}

// called when a fact is discovered or its landmarks change
// to trigger required actions at next level
// newly_discovered = first time fact becomes reachable
void HMLandmarks::propagate_pm_fact(int factindex, bool newly_discovered,
                                    TriggerSet &trigger) {
    // for each action/noop for which fact is a pc
    for (size_t i = 0; i < h_m_table_[factindex].pc_for.size(); ++i) {
        std::pair<int, int> const &info = h_m_table_[factindex].pc_for[i];

        // a pc for the action itself
        if (info.second == -1) {
            if (newly_discovered) {
                --unsat_pc_count_[info.first].first;
            }
            // add to queue if unsatcount at 0
            if (unsat_pc_count_[info.first].first == 0) {
                // create empty set or clear prev entries -- signals do all possible noop effects
                trigger[info.first].clear();
            }
        }
        // a pc for a conditional noop
        else {
            if (newly_discovered) {
                --unsat_pc_count_[info.first].second[info.second];
            }
            // if associated action is applicable, and effect has become applicable
            // (if associated action is not applicable, all noops will be used when it first does)
            if ((unsat_pc_count_[info.first].first == 0) &&
                (unsat_pc_count_[info.first].second[info.second] == 0)) {
                // if not already triggering all noops, add this one
                if ((trigger.find(info.first) == trigger.end()) ||
                    (!trigger[info.first].empty())) {
                    trigger[info.first].insert(info.second);
                }
            }
        }
    }
}

void HMLandmarks::compute_h_m_landmarks() {
    // get subsets of initial state
    std::vector<FluentSet> init_subsets;
    get_m_sets(m_, init_subsets, g_initial_state());

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

    std::vector<int>::iterator it;
    TriggerSet::iterator op_it;

    std::list<int> local_landmarks;
    std::list<int> local_necessary;

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

                if (lm_graph->use_orders()) {
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
                        if (lm_graph->use_orders()) {
                            intersect_with(h_m_table_[*it].necessary, local_necessary);
                        }
                    }

                    if (h_m_table_[*it].landmarks.size() != prev_size)
                        propagate_pm_fact(*it, false, next_trigger);
                } else {
                    h_m_table_[*it].level = level;
                    h_m_table_[*it].landmarks = local_landmarks;
                    if (lm_graph->use_orders()) {
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
                for (std::set<int>::iterator noop_it = op_it->second.begin();
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

        std::cout << "Level " << level << " completed." << std::endl;
        ++level;
    }
    std::cout << "h^m landmarks computed." << std::endl;
}

void HMLandmarks::compute_noop_landmarks(
    int op_index, int noop_index,
    std::list<int> const &local_landmarks,
    std::list<int> const &local_necessary,
    int level,
    TriggerSet &next_trigger) {
    std::list<int> cn_necessary, cn_landmarks;
    size_t prev_size;
    int pm_fluent;

    PMOp &action = pm_ops_[op_index];
    std::vector<int> &pc_eff_pair = action.cond_noops[noop_index];

    cn_landmarks.clear();

    cn_landmarks = local_landmarks;

    if (lm_graph->use_orders()) {
        cn_necessary.clear();
        cn_necessary = local_necessary;
    }

    size_t i;
    for (i = 0; (pm_fluent = pc_eff_pair[i]) != -1; ++i) {
        union_with(cn_landmarks, h_m_table_[pm_fluent].landmarks);
        insert_into(cn_landmarks, pm_fluent);

        if (lm_graph->use_orders()) {
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
                if (lm_graph->use_orders()) {
                    intersect_with(h_m_table_[pm_fluent].necessary, cn_necessary);
                }
            }

            if (h_m_table_[pm_fluent].landmarks.size() != prev_size)
                propagate_pm_fact(pm_fluent, false, next_trigger);
        } else {
            h_m_table_[pm_fluent].level = level;
            h_m_table_[pm_fluent].landmarks = cn_landmarks;
            if (lm_graph->use_orders()) {
                h_m_table_[pm_fluent].necessary = cn_necessary;
            }
            insert_into(h_m_table_[pm_fluent].first_achievers, op_index);
            propagate_pm_fact(pm_fluent, true, next_trigger);
        }
    }
}

void HMLandmarks::add_lm_node(int set_index, bool goal) {
    std::set<std::pair<int, int> > lm;

    std::map<int, LandmarkNode *>::iterator it = lm_node_table_.find(set_index);

    if (it == lm_node_table_.end()) {
        for (FluentSet::iterator it = h_m_table_[set_index].fluents.begin();
             it != h_m_table_[set_index].fluents.end(); ++it) {
            lm.insert(*it);
        }
        LandmarkNode *node;
        if (lm.size() > 1) { // conjunctive landmark
            node = &lm_graph->landmark_add_conjunctive(lm);
        } else { // simple landmark
            node = &lm_graph->landmark_add_simple(h_m_table_[set_index].fluents[0]);
        }
        node->in_goal = goal;
        node->first_achievers.insert(h_m_table_[set_index].first_achievers.begin(),
                                     h_m_table_[set_index].first_achievers.end());
        lm_node_table_[set_index] = node;
    }
}

void HMLandmarks::generate_landmarks() {
    int set_index;
    init();
    compute_h_m_landmarks();
    // now construct landmarks graph
    std::vector<FluentSet> goal_subsets;
    get_m_sets(m_, goal_subsets, g_goal);
    std::list<int> all_lms;
    for (size_t i = 0; i < goal_subsets.size(); ++i) {
        /*
           std::cout << "Goal set: ";
           print_fluentset(goal_subsets[i]);
           std::cout << " -- ";
           for(size_t j = 0; j < goal_subsets[i].size(); ++j) {
           std::cout << goal_subsets[i][j] << " ";
           }
           std::cout << std::endl;
         */

        assert(set_indices_.find(goal_subsets[i]) != set_indices_.end());

        set_index = set_indices_[goal_subsets[i]];

        if (h_m_table_[set_index].level == -1) {
            std::cout << std::endl << std::endl << "Subset of goal not reachable !!." << std::endl << std::endl << std::endl;
            std::cout << "Subset is: ";
            print_fluentset(h_m_table_[set_index].fluents);
            std::cout << std::endl;
        }

        // set up goals landmarks for processing
        union_with(all_lms, h_m_table_[set_index].landmarks);

        // the goal itself is also a lm
        insert_into(all_lms, set_index);

        // make a node for the goal, with in_goal = true;
        add_lm_node(set_index, true);
        /*
           std::cout << "Goal subset: ";
           print_fluentset(h_m_table_[set_index].fluents);
           std::cout << std::endl;
         */
    }
    // now make remaining lm nodes
    for (std::list<int>::iterator it = all_lms.begin(); it != all_lms.end(); ++it) {
        add_lm_node(*it, false);
    }
    if (lm_graph->use_orders()) {
        // do reduction of graph
        // if f2 is landmark for f1, subtract landmark set of f2 from that of f1
        for (std::list<int>::iterator f1 = all_lms.begin(); f1 != all_lms.end(); ++f1) {
            std::list<int> everything_to_remove;
            for (std::list<int>::iterator f2 = h_m_table_[*f1].landmarks.begin();
                 f2 != h_m_table_[*f1].landmarks.end(); ++f2) {
                union_with(everything_to_remove, h_m_table_[*f2].landmarks);
            }
            set_minus(h_m_table_[*f1].landmarks, everything_to_remove);
            // remove necessaries here, otherwise they will be overwritten
            // since we are writing them as greedy nec. orderings.
            if (lm_graph->use_orders())
                set_minus(h_m_table_[*f1].landmarks, h_m_table_[*f1].necessary);
        }

        // and add the edges

        for (std::list<int>::iterator it = all_lms.begin(); it != all_lms.end(); ++it) {
            set_index = *it;
            for (std::list<int>::iterator lms_it = h_m_table_[set_index].landmarks.begin();
                 lms_it != h_m_table_[set_index].landmarks.end(); ++lms_it) {
                assert(lm_node_table_.find(*lms_it) != lm_node_table_.end());
                assert(lm_node_table_.find(set_index) != lm_node_table_.end());

                edge_add(*lm_node_table_[*lms_it], *lm_node_table_[set_index], natural);
            }
            if (lm_graph->use_orders()) {
                for (std::list<int>::iterator gn_it = h_m_table_[set_index].necessary.begin();
                     gn_it != h_m_table_[set_index].necessary.end(); ++gn_it) {
                    edge_add(*lm_node_table_[*gn_it], *lm_node_table_[set_index], greedy_necessary);
                }
            }
        }
    }
    free_unneeded_memory();
}

static LandmarkGraph *_parse(OptionParser &parser) {
    parser.document_synopsis(
        "h^m Landmarks",
        "The landmark generation method introduced by "
        "Keyder, Richter & Helmert (ECAI 2010).");
    parser.document_note(
        "Relevant options",
        "m, reasonable_orders, conjunctive_landmarks, no_orders");
    parser.add_option<int>(
        "m", "subset size (if unsure, use the default of 2)", "2");
    LandmarkGraph::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.help_mode())
        return 0;

    opts.set("explor", new Exploration(opts));

    parser.document_language_support("conditional_effects",
                                     "ignored, i.e. not supported");
    opts.set<bool>("supports_conditional_effects", false);

    if (parser.dry_run()) {
        return 0;
    } else {
        HMLandmarks lm_graph_factory(opts);
        LandmarkGraph *graph = lm_graph_factory.compute_lm_graph();
        return graph;
    }
}

static Plugin<LandmarkGraph> _plugin(
    "lm_hm", _parse);
