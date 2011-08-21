#include "raz_abstraction.h"
#include "option_parser.h"
#include "plugin.h"
#include "shrink_dfp.h"
#include <cassert>
#include <ext/slist>
#include <ios>
#include <limits>
#include <vector>

using namespace std;
using namespace __gnu_cxx;


static const int infinity = numeric_limits<int>::max();


ShrinkDFP::ShrinkDFP(const Options &opts)
    : ShrinkBisimulationBase(opts),
      dfp_style(DFPStyle(opts.get_enum("style"))) {
}

ShrinkDFP::~ShrinkDFP() {
}

string ShrinkDFP::name() const {
    return "Draeger/Finkbeiner/Podelski";
}

void ShrinkDFP::dump_strategy_specific_options() const {
    cout << "Enable greedy bisimulation: "
         << (dfp_style == ENABLE_GREEDY_BISIMULATION ? "yes" : "no")
         << endl;
}

void ShrinkDFP::shrink(Abstraction &abs, int threshold, bool force) {
    if(must_shrink(abs, threshold, force)) {
        vector<slist<AbstractStateRef> > collapsed_groups;
        bool greedy_bisim = (dfp_style == ENABLE_GREEDY_BISIMULATION);
        compute_abstraction_dfp_action_cost_support(
            abs, threshold, collapsed_groups, greedy_bisim);

        apply(abs, collapsed_groups, threshold);
    }
}

void ShrinkDFP::shrink_atomic(Abstraction &/*abs*/) {
    // We don't bisimulate here because the old code didn't, also
    // that was most probably an accident. TODO: Investigate the
    // effect of bisimulating here. The reason why we didn't just
    // add this is that it actually hurt performance on one of our
    // test cases, Sokoban-Opt-#12 with DFP-gop-200K (as well as
    // other DFP-based strategies). This may well be a random
    // mishap, but still it's certainly better to be careful here.
    cout << "DEBUG: I am DFP and I don't pre-bisimulate atomic abstractions."
         << endl;
}

void ShrinkDFP::compute_abstraction_dfp_action_cost_support(
    Abstraction &abs,
    int target_size,
    EquivalenceRelation &equivalence_relation,
    bool enable_greedy_bisimulation) const {
    vector<int> h_to_h_group(abs.max_h + 1, -1);
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

    vector<int> state_to_group(abs.num_states);     //vector containing the states' group number.
    vector<int> group_to_h(abs.num_states, -1);     //vector containing the groups' h-value.
    for (int state = 0; state < abs.num_states; state++) {
        int h = abs.goal_distances[state];
        if (h == infinity || abs.init_distances[state] == infinity) {
            state_to_group[state] = -1;
        } else {
            assert(h >= 0 && h <= abs.max_h);
            int group = h_to_h_group[h];
            state_to_group[state] = group;
            group_to_h[group] = group_to_h[group] == -1 ? h : ::min(h, group_to_h[group]);
        }
    }
    int num_groups = num_of_used_h;
    assert(num_groups <= target_size);
    vector<bool> h_group_done(num_of_used_h, false);
    vector<Signature> signatures;
    signatures.reserve(abs.num_states + 2);

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
            if (h > abs.max_h) {
                // We have hit the end sentinel.
                assert(h == abs.max_h + 1);
                assert(sig_start + 1 == signatures.size());
                break;
            }
            assert(h >= -1);
            assert(h <= abs.max_h);

            if (h == -1 || h_group_done[h_to_h_group[h]]) {
                while (signatures[sig_start].h == h)
                    sig_start++;
                continue;
            }


            int num_old_groups = 0;
            int num_new_groups = 0;
            int num_new_groups_greedy_bisimulation = 0;
            int sig_end;
            for (sig_end = sig_start; signatures[sig_end].h == h; sig_end++) {
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
                    num_new_groups_greedy_bisimulation++;
                } else if (prev_sig.succ_signature != curr_sig.succ_signature) {
                    num_new_groups++;
                    if (enable_greedy_bisimulation 
                        && !are_bisimilar(
                            prev_sig.succ_signature, curr_sig.succ_signature,
                            enable_greedy_bisimulation, group_to_h, h, curr_sig.h))
                        num_new_groups_greedy_bisimulation++;
                }
            }
            assert(sig_end > sig_start);

            bool use_greedy_bisimulation = false;

            if (num_groups - num_old_groups + num_new_groups > target_size) {
                // Can't split the groups for this h value -- would exceed
                // bound on abstract state number.
                //				cout << "can't split groups for h = " << h
                //						<< ", num_new_groups = " << num_new_groups << endl;
                h_group_done[h_to_h_group[h]] = true;
                //This is where dfp gives up on bisimulation for this group
                if (enable_greedy_bisimulation && num_groups - num_old_groups
                    + num_new_groups_greedy_bisimulation <= target_size) {
                    use_greedy_bisimulation = true;
                }
            }

            if ((use_greedy_bisimulation
                 && num_new_groups_greedy_bisimulation
                 != num_old_groups)
                || (!use_greedy_bisimulation
                    && !h_group_done[h_to_h_group[h]]
                    && num_new_groups != num_old_groups)) {
                // Split the groups.
                done = false;
                //this is needed since the checks on further reduction are not always done on the full set of reducible pairs
                bool performed_split = false;
                int new_group_no = -1;
                for (int i = sig_start; i < sig_end; i++) {
                    const Signature &prev_sig = signatures[i - 1];
                    const Signature &curr_sig = signatures[i];

                    assert(h == curr_sig.h);

                    if (prev_sig.group != curr_sig.group) {
                        // Start first group of a block; keep old group no.
                        new_group_no = curr_sig.group;
                    } else if ((!use_greedy_bisimulation
                                && prev_sig.succ_signature
                                != curr_sig.succ_signature)
                               || (use_greedy_bisimulation
                                   && !are_bisimilar(
                                       prev_sig.succ_signature,
                                       curr_sig.succ_signature,
                                       use_greedy_bisimulation,
                                       group_to_h,
                                       h, h))) {
                        new_group_no = num_groups++;
                        performed_split = true;
                        assert(num_groups <= target_size);
                    }

                    assert(new_group_no != -1);
                    state_to_group[curr_sig.state] = new_group_no;
                    group_to_h[new_group_no] = h;
                }
                if (use_greedy_bisimulation && performed_split)
                    h_group_done[h] = false;
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

static ShrinkStrategy *_parse(OptionParser &parser) {
    ShrinkStrategy::add_options_to_parser(parser);
    // TODO: Call this "greedy in {false, true}" as with ShrinkBisimulation?
    vector<string> styles;
    styles.push_back("DEFAULT");
    styles.push_back("ENABLE_GREEDY_BISIMULATION");
    parser.add_enum_option(
        "style", styles, "DEFAULT", "what kind of dfp strategy");
    Options opts = parser.parse();
    ShrinkStrategy::handle_option_defaults(opts);

    if(!parser.dry_run())
        return new ShrinkDFP(opts);
    else
        return 0;
}

static Plugin<ShrinkStrategy> _plugin("shrink_dfp", _parse);
