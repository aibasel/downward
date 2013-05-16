#include "shrink_bisimulation.h"

#include "abstraction.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <ext/hash_map>

using namespace std;
using namespace __gnu_cxx;


static const int infinity = numeric_limits<int>::max();


/* A successor signature characterizes the behaviour of an abstract
   state in so far as bisimulation cares about it. States with
   identical successor signature are not distinguished by
   bisimulation.

   Each entry in the vector is a pair of (label, equivalence class of
   successor). The bisimulation algorithm requires that the vector is
   sorted and uniquified. */

typedef vector<pair<int, int> > SuccessorSignature;

/* TODO: The following class should probably be renamed. It encodes
   all we need to know about a state for bisimulation: its h value,
   which equivalence class ("group") it currently belongs to, its
   signature (see above), and what the original state is. */

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


// TODO: This is a general tool that probably belongs somewhere else.
template<class T>
void release_memory(vector<T> &vec) {
    vector<T>().swap(vec);
}

ShrinkBisimulation::ShrinkBisimulation(const Options &opts)
    : ShrinkStrategy(opts),
      greedy(opts.get<bool>("greedy")),
      threshold(opts.get<int>("threshold")),
      group_by_h(opts.get<bool>("group_by_h")),
      at_limit(AtLimit(opts.get_enum("at_limit"))) {
}

ShrinkBisimulation::~ShrinkBisimulation() {
}

string ShrinkBisimulation::name() const {
    return "bisimulation";
}

void ShrinkBisimulation::dump_strategy_specific_options() const {
    cout << "Bisimulation type: " << (greedy ? "greedy" : "exact") << endl;
    cout << "Bisimulation threshold: " << threshold << endl;
    cout << "Group by h: " << (group_by_h ? "yes" : "no") << endl;
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

bool ShrinkBisimulation::reduce_labels_before_shrinking() const {
    return true;
}

void ShrinkBisimulation::shrink(
    Abstraction &abs, int target, bool force) {
    if (abs.size() == 1 && greedy) {
        cout << "Special case: do not greedily bisimulate an atomic abstration."
             << endl;
        return;
    }

    // TODO: Explain this min(target, threshold) stuff. Also, make the
    //       output clearer, which right now is rubbish, calling the
    //       min(...) "threshold". The reasoning behind this is that
    //       we need to shrink if we're above the threshold or if
    //       *after* composition we'd be above the size limit, so
    //       target can either be less or larger than threshold.
    if (must_shrink(abs, min(target, threshold), force)) {
        EquivalenceRelation equivalence_relation;
        compute_abstraction(abs, target, equivalence_relation);
        apply(abs, equivalence_relation, target);
    }
}

void ShrinkBisimulation::shrink_atomic(Abstraction &abs) {
    // Perform an exact bisimulation on all atomic abstractions.

    // TODO/HACK: Come up with a better way to do this than generating
    // a new shrinking class instance in this roundabout fashion. We
    // shouldn't need to generate a new instance at all.

    int old_size = abs.size();
    ShrinkStrategy *strategy = create_default();
    strategy->shrink(abs, abs.size(), true);
    delete strategy;
    if (abs.size() != old_size) {
        cout << "Atomic abstraction simplified "
             << "from " << old_size
             << " to " << abs.size()
             << " states." << endl;
    }
}

void ShrinkBisimulation::shrink_before_merge(
    Abstraction &abs1, Abstraction &abs2) {
    pair<int, int> new_sizes = compute_shrink_sizes(abs1.size(), abs2.size());
    int new_size1 = new_sizes.first;
    int new_size2 = new_sizes.second;

    // HACK: The output is based on the assumptions of a linear merge
    //       strategy. It would be better (and quite possible) to
    //       treat both abstractions exactly the same here by amending
    //       the output a bit.
    if (new_size2 != abs2.size())
        cout << "atomic abstraction too big; must shrink" << endl;
    shrink(abs2, new_size2);
    shrink(abs1, new_size1);
}

int ShrinkBisimulation::initialize_groups(const Abstraction &abs,
                                          vector<int> &state_to_group) {
    /* Group 0 holds all goal states.

       Each other group holds all states with one particular h value.

       Note that some goal state *must* exist because irrelevant und
       unreachable states are pruned before we shrink and we never
       perform the shrinking if that pruning shows that the problem is
       unsolvable.
    */

    typedef hash_map<int, int> GroupMap;
    GroupMap h_to_group;
    int num_groups = 1; // Group 0 is for goal states.
    for (int state = 0; state < abs.size(); ++state) {
        int h = abs.get_goal_distance(state);
        assert(h >= 0 && h != infinity);
        assert(abs.get_init_distance(state) != infinity);

        if (abs.is_goal_state(state)) {
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
    const Abstraction &abs,
    vector<Signature> &signatures,
    vector<int> &state_to_group) {
    assert(signatures.empty());

    // Step 1: Compute bare state signatures (without transition information).
    signatures.push_back(Signature(-2, false, -1, SuccessorSignature(), -1));
    for (int state = 0; state < abs.size(); ++state) {
        int h = abs.get_goal_distance(state);
        assert(h >= 0 && h <= abs.get_max_h());
        Signature signature(h, abs.is_goal_state(state),
                            state_to_group[state], SuccessorSignature(),
                            state);
        signatures.push_back(signature);
    }
    signatures.push_back(Signature(infinity, false, -1, SuccessorSignature(), -1));

    // Step 2: Add transition information.
    int num_ops = abs.get_num_ops();
    for (int op_no = 0; op_no < num_ops; ++op_no) {
        const vector<AbstractTransition> &transitions =
            abs.get_transitions_for_op(op_no);
        int op_cost = abs.get_cost_for_op(op_no);
        for (size_t i = 0; i < transitions.size(); ++i) {
            const AbstractTransition &trans = transitions[i];
            assert(signatures[trans.src + 1].state == trans.src);
            bool skip_transition = false;
            if (greedy) {
                int src_h = abs.get_goal_distance(trans.src);
                int target_h = abs.get_goal_distance(trans.target);
                assert(target_h + op_cost >= src_h);
                skip_transition = (target_h + op_cost != src_h);
            }
            if (!skip_transition) {
                int target_group = state_to_group[trans.target];
                signatures[trans.src + 1].succ_signature.push_back(
                    make_pair(op_no, target_group));
            }
        }
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
    /* TODO: Should we sort an index set rather than shuffle the whole
       signatures around? But since swapping vectors is fast, we
       probably don't have to worry about that. */
}

void ShrinkBisimulation::compute_abstraction(
    Abstraction &abs,
    int target_size,
    EquivalenceRelation &equivalence_relation) {
    int num_states = abs.size();

    vector<int> state_to_group(num_states);
    vector<Signature> signatures;
    signatures.reserve(num_states + 2);

    int num_groups = initialize_groups(abs, state_to_group);
    // cout << "number of initial groups: " << num_groups << endl;

    // assert(num_groups <= target_size); // TODO: We currently violate this; see issue250

    int max_h = abs.get_max_h();
    assert(max_h >= 0 && max_h != infinity);

    bool stable = false;
    bool stop_requested = false;
    while (!stable && !stop_requested && num_groups < target_size) {
        stable = true;

        signatures.clear();
        compute_signatures(abs, signatures, state_to_group);

        // Verify size of signatures and presence of sentinels.
        assert(signatures.size() == num_states + 2);
        assert(signatures[0].h_and_goal == -2);
        assert(signatures[num_states + 1].h_and_goal == infinity);

        int sig_start = 1; // Skip over initial sentinel.
        while (true) {
            int h_and_goal = signatures[sig_start].h_and_goal;
            int group = signatures[sig_start].group;
            if (h_and_goal > max_h) {
                // We have hit the end sentinel.
                assert(h_and_goal == infinity);
                assert(sig_start + 1 == signatures.size());
                break;
            }

            // Compute the number of groups needed after splitting.
            int num_old_groups = 0;
            int num_new_groups = 0;
            int sig_end;
            for (sig_end = sig_start; true; ++sig_end) {
                if (group_by_h) {
                    if (signatures[sig_end].h_and_goal != h_and_goal)
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
                for (size_t i = sig_start; i < sig_end; ++i) {
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
    release_memory(signatures);

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

ShrinkStrategy *ShrinkBisimulation::create_default() {
    Options opts;
    opts.set("max_states", infinity);
    opts.set("max_states_before_merge", infinity);
    opts.set<bool>("greedy", false);
    opts.set("threshold", 1);
    opts.set("group_by_h", false);
    opts.set<int>("at_limit", RETURN);

    return new ShrinkBisimulation(opts);
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    ShrinkStrategy::add_options_to_parser(parser);
    parser.add_option<bool>("greedy", false, "use greedy bisimulation");
    parser.add_option<int>("threshold", -1); // default: same as max_states
    parser.add_option<bool>("group_by_h", false);

    vector<string> at_limit;
    at_limit.push_back("RETURN");
    at_limit.push_back("USE_UP");
    parser.add_enum_option(
        "at_limit", at_limit, "RETURN",
        "what to do when the size limit is hit");

    Options opts = parser.parse();
    ShrinkStrategy::handle_option_defaults(opts);

    int threshold = opts.get<int>("threshold");
    if (threshold == -1) {
        threshold = opts.get<int>("max_states");
        opts.set("threshold", threshold);
    }
    if (threshold < 1)
        parser.error("bisimulation threshold must be at least 1");
    if (threshold > opts.get<int>("max_states"))
        parser.error("bisimulation threshold must not be larger than size limit");

    if (!parser.dry_run())
        return new ShrinkBisimulation(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_bisimulation", _parse);
