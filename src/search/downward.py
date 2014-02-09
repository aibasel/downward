#! /usr/bin/env python

import argparse
import os.path
import resource
import sys

import input_analyzer


BASE_DIR = os.path.abspath(os.path.dirname(__file__))

DESCRIPTION = """Wrapper for the Fast Downward search component.
Arguments that do not have a special meaning for the wrapper (see below)
are passed on to the actual search component."""


def set_memory_limit():
    # We don't want Python to gobble up too much memory in case we're
    # running a portfolio, so we set a 50 MB memory limit for
    # ourselves. We set a "soft" limit, which limits ourselves, but
    # not a "hard" limit, which would prevent us from raising the
    # limit for our child processes. If an existing soft limit is
    # already lower, we leave it unchanged.

    # On the maia cluster, 20 MB have been tested to be sufficient; 18
    # MB were insufficent. With 50 MB, we should have a bit of a
    # safety net.

    ALLOWED_MEMORY = 50 * 1024 * 1024
    soft, hard = resource.getrlimit(resource.RLIMIT_AS)
    if soft == resource.RLIM_INFINITY or soft > ALLOWED_MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (ALLOWED_MEMORY, hard))


def check_arg_conflicts(parser, possible_conflicts):
    for pos, (name1, is_specified1) in enumerate(possible_conflicts):
        for name2, is_specified2 in possible_conflicts[pos + 1:]:
            if is_specified1 and is_specified2:
                parser.error("cannot combine %s with %s" % (name1, name2))


def parse_args():
    parser = argparse.ArgumentParser(description=DESCRIPTION)
    parser.add_argument("--alias",
                        help="run a config with an alias (e.g. seq-sat-lama-2011)")
    parser.add_argument("--debug", action="store_true",
                        help="run search component in debug mode")
    parser.add_argument("--file", default="output",
                        help="preprocessed input file (default: output)")
    parser.add_argument("--help-search", action="store_true",
                        help="pass --help argument to search component")
    parser.add_argument("--ipc", dest="alias",
                        help="same as --alias")
    parser.add_argument("--portfolio", metavar="FILE",
                        help="run a portfolio specified in FILE")
    parser.add_argument("--show-aliases", action="store_true",
                        help="show the known aliases; don't run search")
    args, search_args = parser.parse_known_args()

    check_arg_conflicts(parser, [
            ("--alias", args.alias is not None),
            ("--help-search", args.help_search),
            ("--portfolio", args.portfolio is not None),
            ("arguments to search component", bool(search_args))])

    if args.help_search:
        search_args = ["--help"]
    del args.help_search

    return args, search_args


def main():
    set_memory_limit()
    args, search_args = parse_args()

    ## TODO: take into account args.debug

    print args
    print search_args

    if args.show_aliases:
        # This takes precedence over other options.
        # TODO: Implement this.
        raise NotImplementedError
        sys.exit()

    # TODO: Only do complex input analysis if we care about the
    # unit-cost/non-unit-cost question? We only do with LAMA-2011.
    is_unit_cost = input_analyzer.read_unit_cost(args.file)
    print "is_unit_cost:", is_unit_cost

    state_var_size = input_analyzer.read_state_var_size(args.file)
    print "state_var_size:", state_var_size

    if args.alias:
        assert not search_args
        # TODO: Implement this.
        raise NotImplementedError

    elif args.portfolio:
        assert not search_args
        # TODO: Implement this.
        raise NotImplementedError
    else:
        # TODO: Implement this.
        raise NotImplementedError

    # TODO: When not running a portfolio, we can run the actual search
    # code with exec rather than forking. In that case, we should not
    # lower the memory limit. Probably best to move the
    # set_memory_limit call to the portfolio case.


if __name__ == "__main__":
    main()


## TODO: Take into account the time used by *this* process too for the
## portfolio time slices. After all, we used to take into account time
## for dispatch script and unitcost script, and these are now
## in-process.


## TODO: The stuff below are parts of the bash script that remain to
## be translated.

"""
PLANNER="$BASEDIR/downward-$STATE_SIZE"

function run_portfolio {
    PORTFOLIO="$1"
    shift
    # Set soft memory limit of 50 MB to avoid Python using too much space.
    # On the maia cluster, 20 MB have been tested to be sufficient; 18 MB are not.
    ulimit -Sv 51200
    "$PORTFOLIO" "$TEMPFILE" "$UNIT_COST" "$PLANNER" "$@"
    # Explicit is better than implicit: return portfolio's exit code.
    return $?
}

if [[ "$1" == "ipc" ]]; then
    CONFIG="$2"
    shift 2
    PORTFOLIO_SCRIPT="$BASEDIR/downward-$CONFIG.py"
    if [[ -e "$PORTFOLIO_SCRIPT" ]]; then
        # Handle configs seq-{sat,opt}-fdss-{1,2} and seq-opt-merge-and-shrink.
        run_portfolio "$PORTFOLIO_SCRIPT" "$@"
    elif [[ "$CONFIG" == "seq-sat-fd-autotune-1" ]]; then
        "$PLANNER" \
            --heuristic "hFF=ff(cost_type=1)" \
            --heuristic "hCea=cea(cost_type=0)" \
            --heuristic "hCg=cg(cost_type=2)" \
            --heuristic "hGoalCount=goalcount(cost_type=0)" \
            --heuristic "hAdd=add(cost_type=0)" \
            --search "iterated([
                lazy(alt([single(sum([g(),weight(hFF, 10)])),
                          single(sum([g(),weight(hFF, 10)]),pref_only=true)],
                          boost=2000),
                     preferred=hFF,reopen_closed=false,cost_type=1),
                lazy(alt([single(sum([g(),weight(hAdd, 7)])),
                          single(sum([g(),weight(hAdd, 7)]),pref_only=true),
                          single(sum([g(),weight(hCg, 7)])),
                          single(sum([g(),weight(hCg, 7)]),pref_only=true),
                          single(sum([g(),weight(hCea, 7)])),
                          single(sum([g(),weight(hCea, 7)]),pref_only=true),
                          single(sum([g(),weight(hGoalCount, 7)])),
                          single(sum([g(),weight(hGoalCount, 7)]),pref_only=true)],
                          boost=1000),
                     preferred=[hCea,hGoalCount],
                     reopen_closed=false,cost_type=1),
                lazy(alt([tiebreaking([sum([g(),weight(hAdd, 3)]),hAdd]),
                          tiebreaking([sum([g(),weight(hAdd, 3)]),hAdd],pref_only=true),
                          tiebreaking([sum([g(),weight(hCg, 3)]),hCg]),
                          tiebreaking([sum([g(),weight(hCg, 3)]),hCg],pref_only=true),
                          tiebreaking([sum([g(),weight(hCea, 3)]),hCea]),
                          tiebreaking([sum([g(),weight(hCea, 3)]),hCea],pref_only=true),
                          tiebreaking([sum([g(),weight(hGoalCount, 3)]),hGoalCount]),
                          tiebreaking([sum([g(),weight(hGoalCount, 3)]),hGoalCount],pref_only=true)],
                         boost=5000),
                     preferred=[hCea,hGoalCount],reopen_closed=false,cost_type=0),
                eager(alt([tiebreaking([sum([g(),weight(hAdd, 10)]),hAdd]),
                           tiebreaking([sum([g(),weight(hAdd, 10)]),hAdd],pref_only=true),
                           tiebreaking([sum([g(),weight(hCg, 10)]),hCg]),
                           tiebreaking([sum([g(),weight(hCg, 10)]),hCg],pref_only=true),
                           tiebreaking([sum([g(),weight(hCea, 10)]),hCea]),
                           tiebreaking([sum([g(),weight(hCea, 10)]),hCea],pref_only=true),
                           tiebreaking([sum([g(),weight(hGoalCount, 10)]),hGoalCount]),
                           tiebreaking([sum([g(),weight(hGoalCount, 10)]),hGoalCount],pref_only=true)],
                          boost=500),
                      preferred=[hCea,hGoalCount],reopen_closed=true,
                      pathmax=true,cost_type=0)],
                repeat_last=true,continue_on_fail=true)" "$@" < $TEMPFILE
    elif [[ "$CONFIG" == "seq-sat-fd-autotune-2" ]]; then
        "$PLANNER" \
            --heuristic "hCea=cea(cost_type=2)" \
            --heuristic "hCg=cg(cost_type=1)" \
            --heuristic "hGoalCount=goalcount(cost_type=2)" \
            --heuristic "hFF=ff(cost_type=0)" \
            --search "iterated([
                ehc(hCea, preferred=hCea,preferred_usage=0,cost_type=0),
                lazy(alt([single(sum([weight(g(), 2),weight(hFF, 3)])),
                          single(sum([weight(g(), 2),weight(hFF, 3)]),pref_only=true),
                          single(sum([weight(g(), 2),weight(hCg, 3)])),
                          single(sum([weight(g(), 2),weight(hCg, 3)]),pref_only=true),
                          single(sum([weight(g(), 2),weight(hCea, 3)])),
                          single(sum([weight(g(), 2),weight(hCea, 3)]),pref_only=true),
                          single(sum([weight(g(), 2),weight(hGoalCount, 3)])),
                          single(sum([weight(g(), 2),weight(hGoalCount, 3)]),pref_only=true)],
                         boost=200),
                     preferred=[hCea,hGoalCount],reopen_closed=false,cost_type=1),
                lazy(alt([single(sum([g(),weight(hFF, 5)])),
                          single(sum([g(),weight(hFF, 5)]),pref_only=true),
                          single(sum([g(),weight(hCg, 5)])),
                          single(sum([g(),weight(hCg, 5)]),pref_only=true),
                          single(sum([g(),weight(hCea, 5)])),
                          single(sum([g(),weight(hCea, 5)]),pref_only=true),
                          single(sum([g(),weight(hGoalCount, 5)])),
                          single(sum([g(),weight(hGoalCount, 5)]),pref_only=true)],
                         boost=5000),
                     preferred=[hCea,hGoalCount],reopen_closed=true,cost_type=0),
                lazy(alt([single(sum([g(),weight(hFF, 2)])),
                          single(sum([g(),weight(hFF, 2)]),pref_only=true),
                          single(sum([g(),weight(hCg, 2)])),
                          single(sum([g(),weight(hCg, 2)]),pref_only=true),
                          single(sum([g(),weight(hCea, 2)])),
                          single(sum([g(),weight(hCea, 2)]),pref_only=true),
                          single(sum([g(),weight(hGoalCount, 2)])),
                          single(sum([g(),weight(hGoalCount, 2)]),pref_only=true)],
                         boost=1000),
                     preferred=[hCea,hGoalCount],reopen_closed=true,cost_type=1)],
                repeat_last=true,continue_on_fail=true)" "$@" < $TEMPFILE
    elif [[ "$CONFIG" == "seq-sat-lama-2008" ]]; then
        echo "The seq-sat-lama-2008 planner should not use this code."
        exit 2
    elif [[ "$CONFIG" == "seq-sat-lama-2011" ]]; then
        if [[ "$UNIT_COST" == "unit" ]]; then
            "$PLANNER" \
                --heuristic "hlm,hff=lm_ff_syn(lm_rhw(
                    reasonable_orders=true,lm_cost_type=2,cost_type=2))" \
                --search "iterated([
                    lazy_greedy([hff,hlm],preferred=[hff,hlm]),
                    lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),
                    lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),
                    lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),
                    lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)],
                    repeat_last=true,continue_on_fail=true)" \
                "$@" < $TEMPFILE
        elif [[ "$UNIT_COST" == "nonunit" ]]; then
            "$PLANNER" \
                --heuristic "hlm1,hff1=lm_ff_syn(lm_rhw(
                    reasonable_orders=true,lm_cost_type=1,cost_type=1))" \
                --heuristic "hlm2,hff2=lm_ff_syn(lm_rhw(
                    reasonable_orders=true,lm_cost_type=2,cost_type=2))" \
                --search "iterated([
                    lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],
                                cost_type=1,reopen_closed=false),
                    lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],
                                reopen_closed=false),
                    lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),
                    lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),
                    lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),
                    lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)],
                    repeat_last=true,continue_on_fail=true)" \
                "$@" < $TEMPFILE
        else
            echo "Something is seriously messed up!"
            exit 2
        fi
    elif [[ "$CONFIG" == "seq-opt-fd-autotune" ]]; then
        "$PLANNER" \
            --heuristic "hLMCut=lmcut()" \
            --heuristic "hHMax=hmax()" \
            --heuristic "hCombinedSelMax=selmax(
                [hLMCut,hHMax],alpha=4,classifier=0,conf_threshold=0.85,
                training_set=10,sample=0,uniform=true)" \
            --search "astar(hCombinedSelMax,mpd=false,
                            pathmax=true,cost_type=0)" "$@" < $TEMPFILE
    elif [[ "$CONFIG" == "seq-opt-selmax" ]]; then
        "$PLANNER" --search "astar(selmax([lmcut(),lmcount(lm_merged([lm_hm(m=1),lm_rhw()]),admissible=true)],training_set=1000),mpd=true)" "$@" < $TEMPFILE
    elif [[ "$CONFIG" == "seq-opt-bjolp" ]]; then
        "$PLANNER" --search "astar(lmcount(lm_merged([lm_rhw(),lm_hm(m=1)]),admissible=true),mpd=true)" "$@" < $TEMPFILE
    elif [[ "$CONFIG" == "seq-opt-lmcut" ]]; then
        "$PLANNER" --search "astar(lmcut())" "$@" < $TEMPFILE
    else
        echo "unknown IPC planner name: $CONFIG"
        exit 2
    fi
elif [[ "$1" == "--portfolio" ]]; then
    # Portfolio files must reside in the search directory.
    PORTFOLIO="$2"
    shift 2
    run_portfolio "$BASEDIR/$PORTFOLIO" "$@"
else
    "$PLANNER" "$@" < $TEMPFILE
fi
EXITCODE=$?
rm -f $TEMPFILE
exit $EXITCODE
"""
