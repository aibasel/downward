#include "shrink_bisimulation.h"

#include "transition_system.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../utilities.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

/* A successor signature characterizes the behaviour of an abstract
   state in so far as bisimulation cares about it. States with
   identical successor signature are not distinguished by
   bisimulation.

   Each entry in the vector is a pair of (label, equivalence class of
   successor). The bisimulation algorithm requires that the vector is
   sorted and uniquified. */

typedef vector<pair<int, int> > SuccessorSignature;

/*
  The following class encodes all we need to know about a state for
  bisimulation: its h value, which equivalence class ("group") it currently
  belongs to, its successor signature (see above), and what the original
  state is.
*/

struct Signature {
    int h_and_goal; // -1 for goal states; h value for non-goal states
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h, bool is_goal, int group_,
              const SuccessorSignature &succ_signature_,
              int state_)
        : group(group_), succ_signature(succ_signature_), state(state_) {
        if (is_goal) {
            assert(h == 0);
            h_and_goal = -1;
        } else {
            h_and_goal = h;
        }
    }

    bool operator<(const Signature &other) const {
        if (h_and_goal != other.h_and_goal)
            return h_and_goal < other.h_and_goal;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }

    void dump() const {
        cout << "Signature(h_and_goal = " << h_and_goal
             << ", group = " << group
             << ", state = " << state
             << ", succ_sig = [";
        for (size_t i = 0; i < succ_signature.size(); ++i) {
            if (i)
                cout << ", ";
            cout << "(" << succ_signature[i].first
                 << "," << succ_signature[i].second
                 << ")";
        }
        cout << "])" << endl;
    }
};


ShrinkBisimulation::ShrinkBisimulation(const Options &opts)
    : ShrinkStrategy(opts),
      greedy(opts.get<bool>("greedy")),
      at_limit(AtLimit(opts.get_enum("at_limit"))) {
}

ShrinkBisimulation::~ShrinkBisimulation() {
}

int ShrinkBisimulation::initialize_groups(const TransitionSystem &ts,
                                          vector<int> &state_to_group) const {
    /* Group 0 holds all goal states.

       Each other group holds all states with one particular h value.

       Note that some goal state *must* exist because irrelevant und
       unreachable states are pruned before we shrink and we never
       perform the shrinking if that pruning shows that the problem is
       unsolvable.
    */

    typedef unordered_map<int, int> GroupMap;
    GroupMap h_to_group;
    int num_groups = 1; // Group 0 is for goal states.
    for (int state = 0; state < ts.get_size(); ++state) {
        int h = ts.get_goal_distance(state);
        assert(h >= 0 && h != INF);

        if (ts.is_goal_state(state)) {
            assert(h == 0);
            state_to_group[state] = 0;
        } else {
            pair<GroupMap::iterator, bool> result = h_to_group.insert(
                make_pair(h, num_groups));
            state_to_group[state] = result.first->second;
            if (result.second) {
                // We inserted a new element => a new group was started.
                ++num_groups;
            }
        }
    }
    return num_groups;
}

void ShrinkBisimulation::compute_signatures(
    const TransitionSystem &ts,
    vector<Signature> &signatures,
    const vector<int> &state_to_group) const {
    assert(signatures.empty());

    // Step 1: Compute bare state signatures (without transition information).
    signatures.push_back(Signature(-2, false, -1, SuccessorSignature(), -1));
    for (int state = 0; state < ts.get_size(); ++state) {
        int h = ts.get_goal_distance(state);
        assert(h >= 0 && h <= ts.get_max_h());
        Signature signature(h, ts.is_goal_state(state),
                            state_to_group[state], SuccessorSignature(),
                            state);
        signatures.push_back(signature);
    }
    signatures.push_back(Signature(INF, false, -1, SuccessorSignature(), -1));

    // Step 2: Add transition information.
    int label_group_counter = 0;
    /*
      Note that the final result of the bisimulation may depend on the
      order in which transitions are considered below.

      If label groups were sorted (every group by increasing label numbers,
      groups by smallest label number), then the following configuration
      gives a different result on parcprinter-08-strips:p06.pddl:
      astar(merge_and_shrink(merge_strategy=merge_dfp,
            shrink_strategy=shrink_bisimulation(max_states=50000,threshold=1,greedy=false),
            label_reduction=label_reduction(before_shrinking=true,before_merging=false)))

      The same behavioral difference can be obtained even without modifying
      the merge-and-shrink code: hg meld -r c66ee00a250a:d2e317621f2c.
      Running the above config on those two revisions yields the same difference.
    */
    for (TSConstIterator group_it = ts.begin();
         group_it != ts.end(); ++group_it) {
        const vector<Transition> &transitions = group_it.get_transitions();
        for (size_t i = 0; i < transitions.size(); ++i) {
            const Transition &trans = transitions[i];
            assert(signatures[trans.src + 1].state == trans.src);
            bool skip_transition = false;
            if (greedy) {
                int src_h = ts.get_goal_distance(trans.src);
                int target_h = ts.get_goal_distance(trans.target);
                int cost = group_it.get_cost();
                assert(target_h + cost >= src_h);
                skip_transition = (target_h + cost != src_h);
            }
            if (!skip_transition) {
                int target_group = state_to_group[trans.target];
                signatures[trans.src + 1].succ_signature.push_back(
                    make_pair(label_group_counter, target_group));
            }
        }
        ++label_group_counter;
    }

    /* Step 3: Canonicalize the representation. The resulting
       signatures must satisfy the following properties:

       1. Signature::operator< defines a total order with the correct
          sentinels at the start and end. The signatures vector is
          sorted according to that order.
       2. Goal states come before non-goal states, and low-h states come
          before high-h states.
       3. States that currently fall into the same group form contiguous
          subsequences.
       4. Two signatures compare equal according to Signature::operator<
          iff we don't want to distinguish their states in the current
          bisimulation round.
     */

    for (size_t i = 0; i < signatures.size(); ++i) {
        SuccessorSignature &succ_sig = signatures[i].succ_signature;
        ::sort(succ_sig.begin(), succ_sig.end());
        succ_sig.erase(::unique(succ_sig.begin(), succ_sig.end()),
                       succ_sig.end());
    }

    ::sort(signatures.begin(), signatures.end());
}

void ShrinkBisimulation::compute_abstraction(
    const TransitionSystem &ts,
    int target_size,
    StateEquivalenceRelation &equivalence_relation) const {
    int num_states = ts.get_size();

    vector<int> state_to_group(num_states);
    vector<Signature> signatures;
    signatures.reserve(num_states + 2);

    int num_groups = initialize_groups(ts, state_to_group);
    // cout << "number of initial groups: " << num_groups << endl;

    // assert(num_groups <= target_size); // TODO: We currently violate this; see issue250

    int max_h = ts.get_max_h();
    assert(max_h >= 0 && max_h != INF);

    bool stable = false;
    bool stop_requested = false;
    while (!stable && !stop_requested && num_groups < target_size) {
        stable = true;

        signatures.clear();
        compute_signatures(ts, signatures, state_to_group);

        // Verify size of signatures and presence of sentinels.
        assert(static_cast<int>(signatures.size()) == num_states + 2);
        assert(signatures[0].h_and_goal == -2);
        assert(signatures[num_states + 1].h_and_goal == INF);

        int sig_start = 1; // Skip over initial sentinel.
        while (true) {
            int h_and_goal = signatures[sig_start].h_and_goal;
            if (h_and_goal > max_h) {
                // We have hit the end sentinel.
                assert(h_and_goal == INF);
                assert(sig_start + 1 == static_cast<int>(signatures.size()));
                break;
            }

            // Compute the number of groups needed after splitting.
            int num_old_groups = 0;
            int num_new_groups = 0;
            int sig_end;
            for (sig_end = sig_start; true; ++sig_end) {
                if (signatures[sig_end].h_and_goal != h_and_goal)
                    break;

                const Signature &prev_sig = signatures[sig_end - 1];
                const Signature &curr_sig = signatures[sig_end];

                if (sig_end == sig_start)
                    assert(prev_sig.group != curr_sig.group);

                if (prev_sig.group != curr_sig.group) {
                    ++num_old_groups;
                    ++num_new_groups;
                } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                    ++num_new_groups;
                }
            }
            assert(sig_end > sig_start);

            if (at_limit == RETURN &&
                num_groups - num_old_groups + num_new_groups > target_size) {
                /* Can't split the group (or the set of groups for
                   this h value) -- would exceed bound on abstract
                   state number.
                */
                stop_requested = true;
                break;
            } else if (num_new_groups != num_old_groups) {
                // Split into new groups.
                stable = false;

                int new_group_no = -1;
                for (int i = sig_start; i < sig_end; ++i) {
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
                    if (num_groups == target_size)
                        break;
                }
                if (num_groups == target_size)
                    break;
            }
            sig_start = sig_end;
        }
    }

    /* Reduce memory pressure before generating the equivalence
       relation since this is one of the code parts relevant to peak
       memory. */
    release_vector_memory(signatures);

    // Generate final result.
    assert(equivalence_relation.empty());
    equivalence_relation.resize(num_groups);
    for (int state = 0; state < num_states; ++state) {
        int group = state_to_group[state];
        if (group != -1) {
            assert(group >= 0 && group < num_groups);
            equivalence_relation[group].push_front(state);
        }
    }
}

void ShrinkBisimulation::compute_equivalence_relation(
    const TransitionSystem &ts,
    int target,
    StateEquivalenceRelation &equivalence_relation) const {
    compute_abstraction(ts, target, equivalence_relation);
}

string ShrinkBisimulation::name() const {
    return "bisimulation";
}

void ShrinkBisimulation::dump_strategy_specific_options() const {
    cout << "Bisimulation type: " << (greedy ? "greedy" : "exact") << endl;
    cout << "At limit: ";
    if (at_limit == RETURN) {
        cout << "return";
    } else if (at_limit == USE_UP) {
        cout << "use up limit";
    } else {
        ABORT("Unknown setting for at_limit.");
    }
    cout << endl;
}

static shared_ptr<ShrinkStrategy>_parse(OptionParser &parser) {
    parser.document_synopsis(
        "Bismulation based shrink strategy",
        "This shrink strategy implements the algorithm described in the paper:\n\n"
        " * Raz Nissim, Joerg Hoffmann and Malte Helmert.<<BR>>\n"
        " [Computing Perfect Heuristics in Polynomial Time: On Bisimulation "
        "and Merge-and-Shrink Abstractions in Optimal Planning "
        "http://ai.cs.unibas.ch/papers/nissim-et-al-ijcai2011.pdf].<<BR>>\n "
        "In //Proceedings of the Twenty-Second International Joint Conference "
        "on Artificial Intelligence (IJCAI 2011)//, pp. 1983-1990. 2011.");
    parser.document_note(
        "shrink_bisimulation(max_states=infinity, threshold=1, greedy=true)",
        "Greedy bisimulation without size bound "
        "(called M&S-gop in the IJCAI 2011 paper)."
        "Combine this with the linear merge strategy "
        "REVERSE_LEVEL to match the heuristic in the paper. "
        "When we last ran experiments on interaction of shrink strategies "
        "with label reduction, this strategy performed best when used with "
        "label reduction before shrinking (and no label reduction before "
        "merging).");
    parser.document_note(
        "shrink_bisimulation(max_states=N, greedy=false)",
        "Exact bisimulation with a size limit "
        "(called DFP-bop in the IJCAI 2011 paper), "
        "where N is a numerical parameter for which sensible values "
        "include 1000, 10000, 50000, 100000 and 200000. "
        "Combine this with the linear merge strategy "
        "REVERSE_LEVEL to match the heuristic in the paper. "
        "When we last ran experiments on interaction of shrink strategies "
        "with label reduction, this strategy performed best when used with "
        "label reduction before shrinking (and no label reduction before "
        "merging).");

    ShrinkStrategy::add_options_to_parser(parser);
    parser.add_option<bool>("greedy", "use greedy bisimulation", "false");

    vector<string> at_limit;
    at_limit.push_back("RETURN");
    at_limit.push_back("USE_UP");
    parser.add_enum_option(
        "at_limit", at_limit,
        "what to do when the size limit is hit", "RETURN");

    Options opts = parser.parse();

    if (parser.help_mode())
        return nullptr;

    ShrinkStrategy::handle_option_defaults(opts);

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<ShrinkBisimulation>(opts);
}

static PluginShared<ShrinkStrategy> _plugin("shrink_bisimulation", _parse);
