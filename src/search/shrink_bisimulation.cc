#include "abstraction.h"
#include "shrink_bisimulation.h"
#include <cassert>

using namespace std;

ShrinkBisimulation::ShrinkBisimulation(bool gr, bool ml)
    : greedy(gr),
      has_mem_limit(ml) {
}

void ShrinkBisimulation::compute_abstraction_action_cost_support(
    Abstraction &abs,
    int target_size, vector<slist<AbstractStateRef> > &collapsed_groups,
    bool enable_greedy_bisimulation) const {
    int num_groups;
    vector<int> state_to_group(abs.num_states);
    vector<int> group_to_h(abs.num_states, -1);

    bool exists_goal_state = false;
    bool exists_non_goal_state = false;
    for (int state = 0; state < abs.num_states; state++) {
        int h = abs.goal_distances[state];
        bool isGoalState = abs.goal_states[state];
        if (h == QUITE_A_LOT || abs.init_distances[state] == QUITE_A_LOT) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= abs.max_h);
            if (h == 0 && isGoalState) {
                state_to_group[state] = 0;
                group_to_h[0] = 0;
                exists_goal_state = true;
            } else if (h >= 0) {
                state_to_group[state] = 1;
                group_to_h[1] = group_to_h[1] == -1 ? h : ::min(h,
                                                                group_to_h[1]);
                exists_non_goal_state = true;
            }
        }
    }

    if (exists_goal_state && exists_non_goal_state)
        num_groups = 2;
    else
        num_groups = 1;

    //Now all goal states are in group 0 and non-goal states are in 1. Unreachable states are in -1.

    vector<bool> group_done(abs.num_states, false);

    vector<Signature> signatures;
    signatures.reserve(abs.num_states + 2);

    bool done = false;
    while (!done) {
        done = true;
        //TODO!!!!!!!- maybe need to update group_to_h vector with new values!!!!
        // Compute state signatures.
        // Add sentinels to the start and end.
        signatures.clear();
        signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
        for (int state = 0; state < abs.num_states; state++) {
            int h = abs.goal_distances[state];
            if (h == QUITE_A_LOT || abs.init_distances[state] == QUITE_A_LOT) {
                h = -1;
                assert(state_to_group[state] == -1);
            }
            Signature signature(h, state_to_group[state], SuccessorSignature(),
                                state);
            signatures.push_back(signature);
        }
        signatures.push_back(Signature(abs.max_h + 1, -1, SuccessorSignature(), -1));

        //Adds to the succ_sig of every signature, the pair <op_no, target_group>
        //reachable by the transition op_no on the state of the signature.
        for (int op_no = 0; op_no < abs.transitions_by_op.size(); op_no++) {
            const vector<AbstractTransition> &transitions =
                abs.transitions_by_op[op_no];
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &trans = transitions[i];
                int src_group = state_to_group[trans.src];
                int target_group = state_to_group[trans.target];
                if (src_group != -1 && target_group != -1) {
                    assert(signatures[trans.src + 1].state == trans.src);
                    signatures[trans.src + 1].succ_signature.push_back(
                        make_pair(op_no, target_group));
                }
            }
        }
        if (enable_greedy_bisimulation)
            for (int i = 0; i < group_to_h.size(); i++)             //TODO- this is something I added (update of the group_to_h)
                group_to_h[i] = -1;

        for (int i = 0; i < signatures.size(); i++) {
            SuccessorSignature &succ_sig = signatures[i].succ_signature;
            ::sort(succ_sig.begin(), succ_sig.end());
            succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
                           succ_sig.end());
            //TODO - this is something I added (update of the group_to_h)
            if (enable_greedy_bisimulation && signatures[i].group > -1)
                group_to_h[signatures[i].group] =
                    group_to_h[signatures[i].group] == -1 ? signatures[i].h
                    : min(signatures[i].h,
                          group_to_h[signatures[i].group]);
        }
        assert(signatures.size() == abs.num_states + 2);
        ::sort(signatures.begin(), signatures.end());
        // TODO: More efficient to sort an index set than to shuffle
        //       the whole signatures around?


        int sig_start = 0;
        while (true) {
            int h = signatures[sig_start].h;
            int group = signatures[sig_start].group;
            // cout << sig_start << " *** " << h << endl;
            if (h > abs.max_h) {
                // We have hit the end sentinel.
                assert(h == abs.max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= -1);
            assert(h <= abs.max_h);

            //this code skips all groups that cannot be further split
            //(due to memory restrictions).
            if (h == -1 || group_done[group]) {
                while (signatures[sig_start].group == group)
                    sig_start++;
                continue;
            }

            //This next piece of code is for computing the number of blocks after the splitting.
            //changed the condition in the for loop to equality of groups and not h values.
            int num_old_groups = 0;
            int num_new_groups = 0;
            int num_new_groups_label_reduction_or_greedy_bisimulation = 0;
            int sig_end;
            for (sig_end = sig_start; signatures[sig_end].group == group; sig_end++) {
                // cout << "@" << sig_start << "@" << sig_end << flush;

                const Signature &prev_sig = signatures[sig_end - 1];
                const Signature &curr_sig = signatures[sig_end];

                /*
                 cout << "@" << prev_sig.group
                 << "@" << curr_sig.group << endl;
                 */

                if (sig_end == sig_start)
                    assert(prev_sig.group != curr_sig.group);

                if (prev_sig.group != curr_sig.group) {
                    num_old_groups++;
                    num_new_groups++;
                    num_new_groups_label_reduction_or_greedy_bisimulation++;
                } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                    num_new_groups++;
                    if (enable_greedy_bisimulation && !abs.are_bisimilar(
                            prev_sig.succ_signature, curr_sig.succ_signature,
                            false, enable_greedy_bisimulation, false,
                            group_to_h, group_to_h[prev_sig.group],
                            group_to_h[curr_sig.group],
                            vector<pair<int, int> > ()))                            //TODO - changed h to group_to_h
                        num_new_groups_label_reduction_or_greedy_bisimulation++;
                }
            }
            assert(sig_end > sig_start);

            bool use_label_reduction_or_greedy_bisimulation =
                enable_greedy_bisimulation;

            if (num_groups - num_old_groups + num_new_groups > target_size) {
                // Can't split the group -- would exceed
                // bound on abstract state number.
                group_done[group] = true;
                if (enable_greedy_bisimulation && num_groups - num_old_groups
                    + num_new_groups_label_reduction_or_greedy_bisimulation
                    <= target_size) {
                    use_label_reduction_or_greedy_bisimulation = true;
                }
            }
            if ((use_label_reduction_or_greedy_bisimulation
                 && num_new_groups_label_reduction_or_greedy_bisimulation
                 != num_old_groups)
                || (!use_label_reduction_or_greedy_bisimulation
                    && !group_done[group] && num_new_groups
                    != num_old_groups)) {
                // Split the group into the new groups, where if two states are equivalent
                //in any bisimulation, they will be in the same group.
                done = false;

                bool performed_split = false;
                int new_group_no = -1;
                for (int i = sig_start; i < sig_end; i++) {
                    const Signature &prev_sig = signatures[i - 1];
                    const Signature &curr_sig = signatures[i];
                    if (prev_sig.group != curr_sig.group) {
                        // Start first group of a block; keep old group no.
                        new_group_no = curr_sig.group;
                    } else if ((!use_label_reduction_or_greedy_bisimulation
                                && prev_sig.succ_signature
                                != curr_sig.succ_signature)
                               || (use_label_reduction_or_greedy_bisimulation
                                   && !abs.are_bisimilar(prev_sig.succ_signature,
                                                     curr_sig.succ_signature, false,
                                                     enable_greedy_bisimulation, false,
                                                     group_to_h,
                                                     group_to_h[prev_sig.group],
                                                     group_to_h[curr_sig.group], vector<
                                                         pair<int, int> > ()))) {                                                //TODO - changed h to group_to_h
                        new_group_no = num_groups++;
                        performed_split = true;
                        assert(num_groups <= target_size);
                    }
                    assert(new_group_no != -1);
                    state_to_group[curr_sig.state] = new_group_no;
                    group_to_h[new_group_no] = curr_sig.h;                     //TODO - changed from just h.
                }
                if (use_label_reduction_or_greedy_bisimulation
                    && performed_split)
                    group_done[group] = false;
            }

            sig_start = sig_end;
        }
    }
    assert(collapsed_groups.empty());
    collapsed_groups.resize(num_groups);
    // int total_size = 0;
    for (int state = 0; state < abs.num_states; state++) {
        int group = state_to_group[state];
        if (group != -1) {
            assert(group >= 0 && group < num_groups);
            collapsed_groups[group].push_front(state);
            // total_size++;
        }
    }
    int zero_sized_groups = 0;
    int geq_one_sized_groups = 0;
    for (int i = 0; i < collapsed_groups.size(); i++) {
        if (collapsed_groups[i].size() == 0)
            zero_sized_groups++;
        if (collapsed_groups[i].size() > 1)
            geq_one_sized_groups++;
    }
//	if (zero_sized_groups > 0)
//		cout << "NUM OF 0 SIZE GROUPS: " << zero_sized_groups << endl;
//	if (geq_one_sized_groups > 0)
//		cout << "NUM OF GEQ 1 GROUPS: " << geq_one_sized_groups << endl;
}


bool ShrinkBisimulation::has_memory_limit() {
    return has_mem_limit;
}

bool ShrinkBisimulation::is_bisimulation() {
    return true;
}

bool ShrinkBisimulation::is_dfp() {
    return false;
}
