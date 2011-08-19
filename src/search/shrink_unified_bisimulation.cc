#include "shrink_unified_bisimulation.h"

#include "option_parser.h"
#include "plugin.h"
#include "raz_abstraction.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;


static const int infinity = numeric_limits<int>::max();
static const int VERY_LARGE_BOUND = 1000000000; // TODO: Get rid of this


ShrinkUnifiedBisimulation::ShrinkUnifiedBisimulation(
    bool greedy_, bool has_mem_limit_)
    : greedy(greedy_),
      has_mem_limit(has_mem_limit_) {
}

ShrinkUnifiedBisimulation::ShrinkUnifiedBisimulation(const Options &opts)
    : greedy(opts.get<bool>("greedy")),
      has_mem_limit(opts.get<bool>("memory_limit")) {
}

ShrinkUnifiedBisimulation::~ShrinkUnifiedBisimulation() {
}

void ShrinkUnifiedBisimulation::shrink(
    Abstraction &abs, int threshold, bool force) {
    if(!must_shrink(abs, threshold, force))
        return;
    if (!has_mem_limit)
        threshold = VERY_LARGE_BOUND; // HACK: Override threshold.
    EquivalenceRelation equivalence_relation;
    compute_abstraction(abs, threshold, equivalence_relation);

    apply(abs, equivalence_relation, threshold);
}

int ShrinkUnifiedBisimulation::initialize_bisim(const Abstraction &abs) {
    bool exists_goal_state = false;
    bool exists_non_goal_state = false;
    for (int state = 0; state < abs.num_states; state++) {
        int h = abs.goal_distances[state];
        bool is_goal_state = abs.goal_states[state];
        if (h == infinity || abs.init_distances[state] == infinity) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= abs.max_h);
            if (is_goal_state) {
                state_to_group[state] = 0;
                exists_goal_state = true;
            } else {
                state_to_group[state] = 1;
                exists_non_goal_state = true;
            }
        }
    }

    int num_groups;
    if (exists_goal_state && exists_non_goal_state)
        num_groups = 2;
    else
        num_groups = 1;

    // Now all goal states are in group 0 and non-goal states are in
    // 1. Unreachable states are in -1.

    // TODO: Check logic. What if one or both of the groups are empty?

    group_done.resize(abs.num_states, false);

    return num_groups;
}

int ShrinkUnifiedBisimulation::initialize_dfp(const Abstraction &abs) {
    h_to_h_group.resize(abs.max_h + 1, -1);
    int num_of_used_h = 0;
    for (int state = 0; state < abs.num_states; state++) {
        int h = abs.goal_distances[state];
        if (h != infinity && abs.init_distances[state] != infinity) {
            if (h_to_h_group[h] == -1) {
                h_to_h_group[h] = num_of_used_h;
                num_of_used_h++;
            }
        }
    }
    cout << "number of used h values: " << num_of_used_h << endl;

    for (int state = 0; state < abs.num_states; state++) {
        int h = abs.goal_distances[state];
        if (h == infinity || abs.init_distances[state] == infinity) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= abs.max_h);
            int group = h_to_h_group[h];
            state_to_group[state] = group;
        }
    }
    int num_groups = num_of_used_h;

    h_group_done.resize(num_of_used_h, false);
    return num_groups;
}

void ShrinkUnifiedBisimulation::compute_abstraction(
    Abstraction &abs,
    int target_size,
    EquivalenceRelation &equivalence_relation) {

    state_to_group.clear();
    group_done.clear();
    signatures.clear();
    h_to_h_group.clear();
    h_group_done.clear();

    state_to_group.resize(abs.num_states);
    signatures.reserve(abs.num_states + 2);

    int num_groups;
    if (is_dfp())
        num_groups = initialize_dfp(abs);
    else
        num_groups = initialize_bisim(abs);

    // assert(num_groups <= target_size); // TODO: We currently violate this, right?

    bool done = false;
    while (!done) {
        done = true;
        // Compute state signatures.
        // Add sentinels to the start and end.
        signatures.clear();
        signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
        for (int state = 0; state < abs.num_states; state++) {
            int h = abs.goal_distances[state];
            if (h == infinity || abs.init_distances[state] == infinity) {
                h = -1;
                assert(state_to_group[state] == -1);
            }
            Signature signature(h, state_to_group[state], SuccessorSignature(),
                                state);
            signatures.push_back(signature);
        }
        signatures.push_back(Signature(abs.max_h + 1, -1, SuccessorSignature(), -1));

        // Initialize the successor signatures, which represent the
        // behaviour of each state in so far as bisimulation cares
        // about it.
        for (int op_no = 0; op_no < abs.transitions_by_op.size(); op_no++) {
            const vector<AbstractTransition> &transitions =
                abs.transitions_by_op[op_no];
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &trans = transitions[i];
                int src_group = state_to_group[trans.src];
                int target_group = state_to_group[trans.target];
                if (src_group != -1 && target_group != -1) {
                    assert(signatures[trans.src + 1].state == trans.src);
                    bool skip_transition = false;
                    if (greedy) {
                        int src_h = abs.goal_distances[trans.src];
                        int target_h = abs.goal_distances[trans.target];
                        skip_transition = (target_h >= src_h);
                    }
                    if (!skip_transition)
                        signatures[trans.src + 1].succ_signature.push_back(
                            make_pair(op_no, target_group));
                }
            }
        }

        for (int i = 0; i < signatures.size(); i++) {
            SuccessorSignature &succ_sig = signatures[i].succ_signature;
            ::sort(succ_sig.begin(), succ_sig.end());
            succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
                           succ_sig.end());
        }

        assert(signatures.size() == abs.num_states + 2);
        ::sort(signatures.begin(), signatures.end());
        // TODO: More efficient to sort an index set than to shuffle
        //       the whole signatures around?

        int sig_start = 0;
        while (true) {
            int h = signatures[sig_start].h;
            int group = signatures[sig_start].group;
            if (h > abs.max_h) {
                // We have hit the end sentinel.
                assert(h == abs.max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= -1);
            assert(h <= abs.max_h);

            // Skips all groups that cannot be split further (due to
            // memory restrictions).
            if (is_dfp()) {
                if (h == -1 || h_group_done[h_to_h_group[h]]) {
                    while (signatures[sig_start].h == h)
                        sig_start++;
                    continue;
                }
            } else {
                if (h == -1 || group_done[group]) {
                    while (signatures[sig_start].group == group)
                        sig_start++;
                    continue;
                }
            }

            // Compute the number of blocks after the splitting.
            int num_old_groups = 0;
            int num_new_groups = 0;
            int sig_end;
            for (sig_end = sig_start; true; sig_end++) {
                if (is_dfp()) {
                    if (signatures[sig_end].h != h)
                        break;
                } else {
                    if (signatures[sig_end].group != group)
                        break;
                }

                const Signature &prev_sig = signatures[sig_end - 1];
                const Signature &curr_sig = signatures[sig_end];

                if (sig_end == sig_start)
                    assert(prev_sig.group != curr_sig.group);

                if (prev_sig.group != curr_sig.group) {
                    num_old_groups++;
                    num_new_groups++;
                } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                    num_new_groups++;
                }
            }
            assert(sig_end > sig_start);

            if (num_groups - num_old_groups + num_new_groups > target_size) {
                // Can't split the group (or the set of groups for
                // this h value) -- would exceed bound on abstract
                // state number.
                if (is_dfp()) {
                    h_group_done[h_to_h_group[h]] = true;
                } else {
                    group_done[group] = true;
                }
            } else if (num_new_groups != num_old_groups) {
                // Split the group into the new groups, where if two
                // states are equivalent in any bisimulation, they will
                // be in the same group.
                done = false;

                int new_group_no = -1;
                for (int i = sig_start; i < sig_end; i++) {
                    const Signature &prev_sig = signatures[i - 1];
                    const Signature &curr_sig = signatures[i];

                    if (prev_sig.group != curr_sig.group) {
                        // Start first group of a block; keep old group no.
                        new_group_no = curr_sig.group;
                    } else if (prev_sig.succ_signature
                               != curr_sig.succ_signature) {
                        new_group_no = num_groups++;
                        assert(num_groups <= target_size);
                    }

                    assert(new_group_no != -1);
                    state_to_group[curr_sig.state] = new_group_no;
                }
            }
            sig_start = sig_end;
        }
    }

    assert(equivalence_relation.empty());
    equivalence_relation.resize(num_groups);
    for (int state = 0; state < abs.num_states; state++) {
        int group = state_to_group[state];
        if (group != -1) {
            assert(group >= 0 && group < num_groups);
            equivalence_relation[group].push_front(state);
        }
    }
}

bool ShrinkUnifiedBisimulation::has_memory_limit() const {
    return has_mem_limit;
}

bool ShrinkUnifiedBisimulation::is_bisimulation() const {
    return true;
}

bool ShrinkUnifiedBisimulation::is_dfp() const {
    if (has_memory_limit())
        return true;
    else
        return false;
}

string ShrinkUnifiedBisimulation::description() const {
    string descr = string("unified ");
    if (greedy)
        descr += "greedy ";
    descr += "bisimulation ";
    if (has_mem_limit)
        descr += "using ";
    else
        descr += "disregarding ";
    descr += " abstraction size limit";
    return descr;
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    parser.add_option<bool>("greedy");
    parser.add_option<bool>("memory_limit");
    Options opts = parser.parse();

    if(!parser.dry_run())
        return new ShrinkUnifiedBisimulation(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_unified_bisimulation", _parse);
