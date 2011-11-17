#include "shrink_bisimulation.h"

#include "abstraction.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;


static const int infinity = numeric_limits<int>::max();


/* A successor signature characterizes the behaviour of an abstract
   state in so far as bisimulation cares about it. States with
   identical successor signature are not distinguished by
   bisimulation.

   Each entry in the vector is a pair of (label, equivalence class of
   successor). The bisimulation algorithm requires that the vector is
   sorted and uniquified. */

typedef std::vector<std::pair<int, int> > SuccessorSignature;

/* TODO: The following class should probably be renamed. It encodes
   all we need to know about a state for bisimulation: its h value,
   which equivalence class ("group") it currently belongs to, its
   signature (see above), and what the original state is. */

struct Signature {
    int h; // TODO: Also take into count if it is a goal!
    int group;
    SuccessorSignature succ_signature;
    int state;

    Signature(int h_, int group_, const SuccessorSignature &succ_signature_,
              int state_)
        : h(h_), group(group_), succ_signature(succ_signature_), state(state_) {
    }

    bool operator<(const Signature &other) const {
        if (h != other.h)
            return h < other.h;
        if (group != other.group)
            return group < other.group;
        if (succ_signature != other.succ_signature)
            return succ_signature < other.succ_signature;
        return state < other.state;
    }

    void dump() const {
        cout << "Signature(h = " << h
             << ", group = " << group
             << ", state = " << state
             << ", succ_sig = [";
        for (int i = 0; i < succ_signature.size(); ++i) {
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
      greediness(Greediness(opts.get_enum("greedy"))),
      threshold(opts.get<int>("threshold")),
      initialize_by_h(opts.get<bool>("initialize_by_h")),
      group_by_h(opts.get<bool>("group_by_h")),
      at_limit(AtLimit(opts.get_enum("at_limit"))) {
}

ShrinkBisimulation::~ShrinkBisimulation() {
}

string ShrinkBisimulation::name() const {
    return "bisimulation";
}

void ShrinkBisimulation::dump_strategy_specific_options() const {
    cout << "Bisimulation type: ";
    if (greediness == NOT_GREEDY)
        cout << "exact";
    else if (greediness == SOMEWHAT_GREEDY)
        cout << "somewhat greedy";
    else if (greediness == GREEDY)
        cout << "greedy";
    else
        abort();
    cout << endl;
    cout << "Bisimulation threshold: " << threshold << endl;
    cout << "Initialize by h: " << (initialize_by_h ? "yes" : "no") << endl;
    cout << "Group by h: " << (group_by_h ? "yes" : "no") << endl;
    cout << "At limit: ";
    if (at_limit == RETURN)
        cout << "return";
    else if (at_limit == USE_UP)
        cout << "use up limit";
    else
        abort();
    cout << endl;
}

bool ShrinkBisimulation::reduce_labels_before_shrinking() const {
    return true;
}

void ShrinkBisimulation::shrink(
    Abstraction &abs, int target, bool force) {
    if (abs.size() == 1 && greediness != NOT_GREEDY) {
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

int ShrinkBisimulation::initialize_bisim(const Abstraction &abs,
                                         vector<int> &state_to_group) {

    /* Group 0 holds all goal states.

       Group 1 holds all non-goal states if any non-goal states exist.

       Note that some goal state *must* exist because irrelevant und
       unreachable states are pruned before we shrink and we never
       perform the shrinking if that pruning shows that the problem is
       unsolvable.
    */

    int num_groups = 1;

    for (int state = 0; state < abs.size(); ++state) {
        assert(abs.get_goal_distance(state) >= 0 &&
               abs.get_goal_distance(state) <= abs.get_max_h());
        if (abs.is_goal_state(state)) {
            state_to_group[state] = 0;
        } else {
            state_to_group[state] = 1;
            num_groups = 2; // There exists a non-goal state.
        }
    }

    return num_groups;
}

int ShrinkBisimulation::initialize_dfp(const Abstraction &abs,
                                       vector<int> &state_to_group) {

    /* Group 0 holds all goal states.

       All other groups hold all states with one particular h value.

       See the comments for initialize_bisim to see why group 0 must
       be nonempty and we don't need to bother about irrelevant or
       unreachable states. */

    vector<int> h_to_h_group(abs.get_max_h() + 1, -1);
    int num_of_used_h = 0;
    for (int state = 0; state < abs.size(); ++state) {
        int h = abs.get_goal_distance(state);
        assert(h >= 0 && h != infinity);
        assert(abs.get_init_distance(state) != infinity);

        if (abs.is_goal_state(state)) {
            assert(h == 0);
            state_to_group[state] = 0;
        } else {
            int &h_group = h_to_h_group[h];
            if (h_group == -1) {
                h_group = num_of_used_h;
                num_of_used_h++;
            }
            state_to_group[state] = h_group + 1;
        }
    }

    int num_groups = num_of_used_h + 1;
    cout << "number of initial groups: " << num_groups << endl;
    return num_groups;
}

void ShrinkBisimulation::compute_abstraction(
    Abstraction &abs,
    int target_size,
    EquivalenceRelation &equivalence_relation) {
    int num_states = abs.size();

    vector<int> state_to_group(num_states);
    vector<Signature> signatures;
    signatures.reserve(num_states + 2);

    int num_groups;
    if (initialize_by_h)
        num_groups = initialize_dfp(abs, state_to_group);
    else
        num_groups = initialize_bisim(abs, state_to_group);

    // assert(num_groups <= target_size); // TODO: We currently violate this; see issue250

    int max_h = abs.get_max_h();
    assert(max_h >= 0 && max_h != infinity);

    bool done = false;
    bool stop_after_iteration = false;
    while (!done && !stop_after_iteration) {
        done = true;
        // Compute state signatures.
        // Add sentinels to the start and end.
        signatures.clear();
        signatures.push_back(Signature(-1, -1, SuccessorSignature(), -1));
        for (int state = 0; state < num_states; state++) {
            int h = abs.get_goal_distance(state);
            if (h == infinity || abs.get_init_distance(state) == infinity) {
                h = -1;
                assert(state_to_group[state] == -1);
            }
            Signature signature(h, state_to_group[state], SuccessorSignature(),
                                state);
            signatures.push_back(signature);
        }
        signatures.push_back(Signature(max_h + 1, -1, SuccessorSignature(), -1));

        // Initialize the successor signatures, which represent the
        // behaviour of each state in so far as bisimulation cares
        // about it.
        int num_ops = abs.get_num_ops();
        for (int op_no = 0; op_no < num_ops; op_no++) {
            const vector<AbstractTransition> &transitions =
                abs.get_transitions_for_op(op_no);
            for (int i = 0; i < transitions.size(); i++) {
                const AbstractTransition &trans = transitions[i];
                int src_group = state_to_group[trans.src];
                int target_group = state_to_group[trans.target];
                if (src_group != -1 && target_group != -1) {
                    assert(signatures[trans.src + 1].state == trans.src);
                    bool skip_transition = false;
                    if (greediness != NOT_GREEDY) {
                        int src_h = abs.get_goal_distance(trans.src);
                        int target_h = abs.get_goal_distance(trans.target);
                        if (greediness == SOMEWHAT_GREEDY)
                            skip_transition = (target_h > src_h);
                        else if (greediness == GREEDY)
                            skip_transition = (target_h >= src_h);
                        else
                            abort();
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

        assert(signatures.size() == num_states + 2);
        ::sort(signatures.begin(), signatures.end());
        // TODO: More efficient to sort an index set than to shuffle
        //       the whole signatures around?

        assert(signatures[0].h == -1);
        int sig_start = 1; // Skip over initial sentinel.
        while (true) {
            int h = signatures[sig_start].h;
            int group = signatures[sig_start].group;
            if (h > max_h) {
                // We have hit the end sentinel.
                assert(h == max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= 0);
            assert(h <= max_h);

            // Compute the number of blocks after the splitting.
            int num_old_groups = 0;
            int num_new_groups = 0;
            int sig_end;
            for (sig_end = sig_start; true; sig_end++) {
                if (group_by_h) {
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
                if (at_limit == RETURN) {
                    stop_after_iteration = true;
                    break;
                } else {
                    assert(at_limit == USE_UP);

                    stop_after_iteration = true;
                    // TODO: Get rid of code duplication between this
                    // and the following code

                    int new_group_no = -1;
                    for (int i = sig_start; i < sig_end; i++) {
                        const Signature &prev_sig = signatures[i - 1];
                        const Signature &curr_sig = signatures[i];

                        if (prev_sig.group != curr_sig.group) {
                            // Start first group of a block; keep old group no.
                            new_group_no = curr_sig.group;
                        } else if (prev_sig.succ_signature
                                   != curr_sig.succ_signature) {
                            if (num_groups < target_size) {
                                // Only start a new group if we're within
                                // budget!
                                new_group_no = num_groups++;
                                assert(num_groups <= target_size);
                            }
                        }

                        assert(new_group_no != -1);
                        state_to_group[curr_sig.state] = new_group_no;
                    }
                    assert(num_groups == target_size);
                    break;
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

    /* Reduce memory pressure before generating the equivalence
       relation since this is one of the code parts relevant to peak
       memory. */
    release_memory(signatures);

    // Generate final result.
    assert(equivalence_relation.empty());
    equivalence_relation.resize(num_groups);
    for (int state = 0; state < num_states; state++) {
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
    opts.set<int>("greedy", NOT_GREEDY);
    opts.set("threshold", 1);
    opts.set("initialize_by_h", false);
    opts.set("group_by_h", false);
    opts.set<int>("at_limit", RETURN);

    return new ShrinkBisimulation(opts);
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    ShrinkStrategy::add_options_to_parser(parser);
    vector<string> greediness;
    greediness.push_back("false");
    greediness.push_back("somewhat");
    greediness.push_back("true");
    parser.add_enum_option(
        "greedy", greediness, "NOT_GREEDY",
        "use exact, somewhat greedy or greedy bisimulation");
    parser.add_option<int>("threshold", -1); // default: same as max_states
    parser.add_option<bool>("initialize_by_h", true);
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
    if (threshold < 1) {
        cerr << "error: bisimulation threshold must be at least 1" << endl;
        exit(2);
    }
    if (threshold > opts.get<int>("max_states")) {
        cerr << "error: bisimulation threshold must not be larger than "
             << "size limit" << endl;
        exit(2);
    }

    if (!parser.dry_run())
        return new ShrinkBisimulation(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_bisimulation", _parse);
