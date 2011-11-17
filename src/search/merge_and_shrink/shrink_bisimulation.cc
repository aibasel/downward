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
   signatures (see above), and what the original state is. */

struct Signature {
    int h;
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
    if (initialize_by_h != group_by_h) {
        cerr << "initialize_by_h and group_by_h cannot be set independently "
             << "at the moment" << endl;
        exit(2);
    }
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
    else if (at_limit == SKIP_AND_COMPLETE_ITERATION)
        cout << "skip and complete iteration";
    else if (at_limit == SKIP_AND_KEEP_GOING)
        cout << "skip and keep going";
    else
        abort();
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

int ShrinkBisimulation::initialize_bisim(const Abstraction &abs) {
    bool exists_goal_state = false;
    bool exists_non_goal_state = false;
    for (int state = 0; state < abs.size(); state++) {
        int h = abs.get_goal_distance(state);
        bool is_goal_state = abs.is_goal_state(state);
        if (h == infinity || abs.get_init_distance(state) == infinity) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= abs.get_max_h());
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

    if (at_limit == SKIP_AND_KEEP_GOING)
        group_done.resize(abs.size(), false);

    return num_groups;
}

int ShrinkBisimulation::initialize_dfp(const Abstraction &abs) {
    h_to_h_group.resize(abs.get_max_h() + 1, -1);
    int num_of_used_h = 0;
    for (int state = 0; state < abs.size(); state++) {
        int h = abs.get_goal_distance(state);
        if (h != infinity && abs.get_init_distance(state) != infinity) {
            if (h_to_h_group[h] == -1) {
                h_to_h_group[h] = num_of_used_h;
                num_of_used_h++;
            }
        }
    }
    cout << "number of used h values: " << num_of_used_h << endl;

    for (int state = 0; state < abs.size(); state++) {
        int h = abs.get_goal_distance(state);
        if (h == infinity || abs.get_init_distance(state) == infinity) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= abs.get_max_h());
            int group = h_to_h_group[h];
            state_to_group[state] = group;
        }
    }
    int num_groups = num_of_used_h;

    if (at_limit == SKIP_AND_KEEP_GOING)
        h_group_done.resize(num_of_used_h, false);
    else
        release_memory(h_to_h_group);
    return num_groups;
}

void ShrinkBisimulation::compute_abstraction(
    Abstraction &abs,
    int target_size,
    EquivalenceRelation &equivalence_relation) {
    int num_states = abs.size();

    state_to_group.clear();
    group_done.clear();
    signatures.clear();
    h_to_h_group.clear();
    h_group_done.clear();

    state_to_group.resize(num_states);
    signatures.reserve(num_states + 2);

    int num_groups;
    if (initialize_by_h)
        num_groups = initialize_dfp(abs);
    else
        num_groups = initialize_bisim(abs);

    // assert(num_groups <= target_size); // TODO: We currently violate this, right?

    int max_h = abs.get_max_h();

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

        int sig_start = 0;
        while (true) {
            int h = signatures[sig_start].h;
            int group = signatures[sig_start].group;
            if (h > max_h) {
                // We have hit the end sentinel.
                assert(h == max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= -1);
            assert(h <= max_h);

            // Skips all groups that cannot be split further (due to
            // memory restrictions).
            if (group_by_h) {
                if (h == -1 || (at_limit == SKIP_AND_KEEP_GOING &&
                                h_group_done[h_to_h_group[h]])) {
                    while (signatures[sig_start].h == h)
                        sig_start++;
                    continue;
                }
            } else {
                if (h == -1 || (at_limit == SKIP_AND_KEEP_GOING &&
                                group_done[group])) {
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
                if (at_limit == SKIP_AND_KEEP_GOING) {
                    if (group_by_h) {
                        h_group_done[h_to_h_group[h]] = true;
                    } else {
                        group_done[group] = true;
                    }
                } else if (at_limit == SKIP_AND_COMPLETE_ITERATION) {
                    stop_after_iteration = true;
                } else if (at_limit == RETURN) {
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

    // TODO: As we're releasing these anyway, they should probably
    // not be instance variables but local variables. We previously
    // did not release them, but that cost us several hundreds of MB
    // in peak memory, and hence is probably a bad idea.
    release_memory(group_done);
    release_memory(signatures);
    release_memory(h_to_h_group);
    release_memory(h_group_done);

    assert(equivalence_relation.empty());
    equivalence_relation.resize(num_groups);
    for (int state = 0; state < num_states; state++) {
        int group = state_to_group[state];
        if (group != -1) {
            assert(group >= 0 && group < num_groups);
            equivalence_relation[group].push_front(state);
        }
    }

    release_memory(state_to_group);
}

ShrinkStrategy *ShrinkBisimulation::create_default() {
    Options opts;
    opts.set("max_states", infinity);
    opts.set("max_states_before_merge", infinity);
    opts.set<int>("greedy", NOT_GREEDY);
    opts.set("threshold", 1);
    opts.set("initialize_by_h", false);
    opts.set("group_by_h", false);
    opts.set<int>("at_limit", SKIP_AND_KEEP_GOING);

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
    at_limit.push_back("SKIP_AND_COMPLETE_ITERATION");
    at_limit.push_back("SKIP_AND_KEEP_GOING");
    parser.add_enum_option(
        "at_limit", at_limit, "SKIP_AND_KEEP_GOING",
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
