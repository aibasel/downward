#include "shrink_unified_bisimulation.h"

#include "option_parser.h"
#include "plugin.h"
#include "raz_abstraction.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;


static const int infinity = numeric_limits<int>::max();


ShrinkUnifiedBisimulation::ShrinkUnifiedBisimulation(const Options &opts)
    : ShrinkBisimulationBase(opts),
      greedy(opts.get<bool>("greedy")),
      threshold(opts.get<int>("threshold")),
      skip_atomic_bisimulation(opts.get<bool>("skip_atomic_bisimulation")),
      initialize_by_h(opts.get<bool>("initialize_by_h")),
      group_by_h(opts.get<bool>("group_by_h")) {
    if (initialize_by_h != group_by_h) {
        cerr << "initialize_by_h and group_by_h cannot be set independently "
             << "at the moment" << endl;
        exit(2);
    }
}

ShrinkUnifiedBisimulation::~ShrinkUnifiedBisimulation() {
}

string ShrinkUnifiedBisimulation::name() const {
    return "bisimulation";
}

void ShrinkUnifiedBisimulation::dump_strategy_specific_options() const {
    cout << "Bisimulation type: "
         << (greedy ? "greedy" : "exact")
         << endl
         << "Bisimulation threshold: " << threshold
         << endl;
}

void ShrinkUnifiedBisimulation::shrink(
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

void ShrinkUnifiedBisimulation::shrink_atomic(Abstraction &abs) {
    // Perform an exact bisimulation on all atomic abstractions.

     // TODO/HACK: Come up with a better way to do this than generating
    // a new shrinking class instance in this roundabout fashion. We
    // shouldn't need to generate a new instance at all.

    if (skip_atomic_bisimulation) {
        // We don't bisimulate here because the old code didn't, also
        // that was most probably an accident.
        //
        // TODO: Investigate the effect of bisimulating here. The
        // reason why we didn't just add this is that it actually hurt
        // performance on one of our test cases, Sokoban-Opt-#12 with
        // DFP-gop-200K (as well as other DFP-based strategies). This
        // may well be a random mishap, but still it's certainly
        // better to be careful here.
        cout << "DEBUG: I was asked to skip the atomic bisimulation." << endl;
        return;
    }

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

void ShrinkUnifiedBisimulation::shrink_before_merge(
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
    if (initialize_by_h)
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
            if (group_by_h) {
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
                if (group_by_h) {
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

ShrinkStrategy *ShrinkUnifiedBisimulation::create_default() {
    Options opts;
    opts.set("max_states", infinity);
    opts.set("max_states_before_merge", infinity);
    opts.set("greedy", false);

    opts.set("threshold", 1);
    opts.set("skip_atomic_bisimulation", false);
    opts.set("initialize_by_h", false);
    opts.set("group_by_h", false);

    return new ShrinkUnifiedBisimulation(opts);
}

static ShrinkStrategy *_parse(OptionParser &parser) {
    ShrinkStrategy::add_options_to_parser(parser);
    parser.add_option<bool>("greedy");
    parser.add_option<int>("threshold", -1);
    // NOTE: threshold == -1 activates legacy behaviour, in which all
    //       "modern" boolean options are overridden.
    parser.add_option<bool>("skip_atomic_bisimulation", false);
    parser.add_option<bool>("initialize_by_h", true);
    parser.add_option<bool>("group_by_h", false);
    // TODO: Remove the following legacy option.
    parser.add_option<bool>("memory_limit");

    Options opts = parser.parse();
    ShrinkStrategy::handle_option_defaults(opts);

    // BEGIN legacy option handling block
    if (opts.get<int>("threshold") == -1) {
        // TODO: Get rid of this, as well as the "memory_limit" option
        //       at some point, once we're happy with our code and no
        //       longer need to experiment with the old-style options.

        // The legacy "memory_limit" option determines whether we want
        // a ShrinkDFP-style heuristic or a ShrinkBisimulation-style
        // heuristic.

        if (opts.get<bool>("memory_limit")) {
            if (parser.dry_run())
                cout << "Legacy option support: setting options for DFP-bop."
                     << endl;

            opts.set("threshold", opts.get<int>("max_states"));
            opts.set("skip_atomic_bisimulation", true);
            opts.set("initialize_by_h", true);
            opts.set("group_by_h", true);
        } else {
            if (parser.dry_run())
                cout << "Legacy option support: setting options for "
                     << "M&S-bop/gop." << endl;

            if (opts.get<int>("max_states") != 1) {
                cerr << "legacy bisimulation wants max_states = 1!" << endl;
                exit(2);
            }

            opts.set("max_states", infinity);
            opts.set("max_states_before_merge", infinity);

            opts.set("threshold", 1);
            opts.set("skip_atomic_bisimulation", false);
            opts.set("initialize_by_h", false);
            opts.set("group_by_h", false);
        }
    }
    // END legacy option handling block

    int threshold = opts.get<int>("threshold");
    if (threshold < 1) {
        cerr << "error: bisimulation threshold must be at least 1" << endl;
        exit(2);
    }
    if (threshold > opts.get<int>("max_states")) {
        cerr << "error: bisimulation threshold must not be larger than "
             << "size limit" << endl;
        exit(2);
    }

    if(!parser.dry_run())
        return new ShrinkUnifiedBisimulation(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_unified_bisimulation", _parse);
