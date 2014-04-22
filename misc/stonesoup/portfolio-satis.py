#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import sys


TIMEOUT = 1800
EPSILON = 0.0001 # Don't care about this little time.


class Results(object):
    def __init__(self, file):
        self.configs, self.problems, self.data = self._parse_entries(file)
        self._add_missing_entries()
        assert len(self.data) == len(self.configs) * len(self.problems)
        self._discard_costs()

    def _parse_entries(self, file):
        configs = set()
        problems = set()
        data = {}
        best_costs = {}
        for entry_ in self._parse(file):
            entry = self._massage_entry(entry_)

            config = entry["config"]
            problem = entry["problem"]
            cost = entry["cost"]

            if "_wa" in config or "alt" in config or "fdss" in config:
                continue
            if "08" not in entry_["domain"]:
                if config.endswith("_0"):
                    config = config[:-2] + "_1"
            else:
                if config.endswith(("_0", "_2")):
                    continue

#            if not (entry["domain"].endswith("08'") or
#                entry["domain"].endswith("08-strips'")):
#                continue
#            if (entry["domain"].endswith("08'") or
#                entry["domain"].endswith("08-strips'")):
#                continue

            configs.add(config)
            problems.add(problem)
            if cost is not None:
                if problem not in best_costs:
                    best_costs[problem] = entry["cost"]
                else:
                    best_costs[problem] = min(best_costs[problem], cost)
            data[config, problem] = (entry["time"], entry["cost"])

        for (config, problem), (time, cost) in data.iteritems():
            ipc_score = None
            if time is not None:
                best_cost = best_costs[problem]
                assert cost is not None
                assert best_cost <= cost
                ipc_score = float(best_cost)/cost
            data[config, problem] = (time, cost, ipc_score)
        return sorted(configs), sorted(problems), data

    def _parse(self, file):
        properties = ["config", "domain", "problem",
                      "status", "total_time", "cost"]
        data = {}
        for line in file:
            left, match, right = line.partition("=")
            left = left.strip()
            right = right.strip()
            if not match:
                assert line.startswith("[")
                if not line.startswith("[["):
                    if data:
                        yield data
                    data = {}
            elif left in properties:
                assert left not in data
                data[left] = right

    def _massage_entry(self, entry):
        assert "config" in entry, entry
        assert "domain" in entry, entry
        assert "problem" in entry, entry
        assert "status" in entry, entry
        config = entry["config"].strip("'")
        if config.startswith("WORK-"):
            config = config[5:]
        domain = entry["domain"].strip("'")
        problem = entry["problem"].strip("'")
        status = entry["status"]
        assert status in ["'ok'", "'unsolved'"], entry

        if "08" in domain and "0000" in config:
            # M&S config with no action cost support
            # print "IGNORED:", entry
            time = None
            cost = None
        elif status == "'ok'":
            assert "total_time" in entry, entry
            assert "cost" in entry, entry
            time = float(entry["total_time"])
            assert time <= TIMEOUT + EPSILON, entry
            cost = int(entry["cost"])
        else:
            assert "cost" not in entry, entry
            time = None
            cost = None
        return dict(
            config=config,
            problem="%s:%s" % (domain, problem),
            time=time,
            cost=cost)

    def _add_missing_entries(self):
        for config in self.configs:
            for problem in self.problems:
                if (config, problem) not in self.data:
                    print "MISSING:", config, problem
                    self.data[config, problem] = (None, None, None)

    def _discard_costs(self):
        for problem in self.problems:
            for config in self.configs:
                time = self.data[config, problem][0]
                ipc_score = self.data[config, problem][2]
                self.data[config, problem] = (time, ipc_score)

    def solution_times(self, problem):
        result = {}
        for config in self.configs:
            time = self.data[config, problem][0]
            if time is not None:
                result[config] = time
        return result

    def solution_scores(self, problem):
        result = {}
        for config in self.configs:
            ipc_score = self.data[config, problem][1]
            if ipc_score is not None:
                result[config] = ipc_score
        return result

    def _total_solved(self):
        return sum(1 for problem in self.problems
                   if self.solution_times(problem))

    def dump_statistics(self):
        print len(self.data), "results"
        print len(self.configs), "configs"
        print len(self.problems), "problems"
        print self._total_solved(), "problems solved by someone"
        for config in sorted(self.configs):
            num_solved = len([problem for problem in self.problems
                              if self.data[config, problem][0] is not None])
            print num_solved, "problems solved by", config
        for config in sorted(self.configs):
            ipc_score = sum([self.data[config, problem][1]
                             for problem in self.problems
                             if self.data[config, problem][1] is not None])
            print "%.2f" % ipc_score, "IPC score by", config


class Portfolio(object):
    def __init__(self, results, timeouts):
        self.results = results
        self.timeouts = timeouts
        self.configs = sorted(self.timeouts.iterkeys())

    def solves_problem(self, problem):
        solution_times = self.results.solution_times(problem)
        for solver, time in solution_times.iteritems():
            if time < self.timeouts[solver] + EPSILON:
                return True
        return False

    def total_timeout(self):
        return sum(self.timeouts.itervalues())

    def num_solved(self):
        return sum(1 for problem in self.results.problems
                   if self.solves_problem(problem))

    def score(self, problem):
        solution_times = self.results.solution_times(problem)
        solution_scores = self.results.solution_scores(problem)
        best_score = 0.0
        for solver, time in solution_times.iteritems():
            if time < self.timeouts[solver] + EPSILON:
                if solution_scores[solver] > best_score:
                    best_score = solution_scores[solver]
        return best_score

    def ipc_score(self):
        return sum(self.score(problem)
                   for problem in self.results.problems)

    def successors(self, granularity):
        for config in self.configs:
            succ_timeouts = self.timeouts.copy()
            succ_timeouts[config] += granularity
            yield Portfolio(self.results, succ_timeouts)

    def reduce(self, granularity):
        num_solved = self.num_solved()
        for config in self.configs:
            # Reduce timeout for this config until we hit zero or lose
            # a problem.
            while self.timeouts[config] > granularity + EPSILON:
                self.timeouts[config] -= granularity
                if self.num_solved() < num_solved:
                    # Undo last change and stop reducing this timeout.
                    self.timeouts[config] += granularity
                    break

    def reduce_score_based(self, granularity):
        score = self.ipc_score()
        for config in self.configs:
            # Reduce timeout for this config until we hit zero or lose
            # a problem.
            while self.timeouts[config] > granularity + EPSILON:
                self.timeouts[config] -= granularity
                if self.ipc_score() < score:
                    # Undo last change and stop reducing this timeout.
                    self.timeouts[config] += granularity
                    break

    def dump_marginal_contributions(self):
        print "Marginal contributions:"
        num_solved = self.num_solved()
        score = self.ipc_score()
        for config in self.configs:
            if self.timeouts[config] > EPSILON:
                succ_timeouts = self.timeouts.copy()
                succ_timeouts[config] = 0
                succ = Portfolio(self.results, succ_timeouts)
                num_lost = num_solved - succ.num_solved()
                score_lost = score - succ.ipc_score()
                print "   %3d problems / %.2f score from %s" % (num_lost,
                        score_lost, config)

    def dump_detailed_marginal_contributions(self, granularity):
        print "Detailed marginal contributions:"
        num_solved = self.num_solved()
        for config in self.configs:
            if self.timeouts[config] > EPSILON:
                reduction = 0
                prev_num_solved = num_solved
                lost_at = []
                while reduction <= self.timeouts[config] + EPSILON:
                    reduced_timeout = self.timeouts[config] - reduction
                    succ_timeouts = self.timeouts.copy()
                    succ_timeouts[config] = reduced_timeout
                    succ = Portfolio(self.results, succ_timeouts)
                    succ_solved = succ.num_solved()
                    if succ_solved < prev_num_solved:
                        for _ in xrange(prev_num_solved - succ_solved):
                            lost_at.append(str(reduction))
                        prev_num_solved = succ_solved
                    reduction += granularity
                print "   %s: problems lost at %s" % (
                    config, ", ".join(lost_at))

    def dump(self):
        print "portfolio for %.2f seconds solves %d problems with score %.2f:" % (
            self.total_timeout(), self.num_solved(), self.ipc_score())
        for config in self.configs:
            print "   %7.2f seconds for %s" % (self.timeouts[config], config)

    def dump_unsolved(self):
        print "Unsolved problems:"
        count = 0
        for problem in self.results.problems:
            solution_times = self.results.solution_times(problem)
            if solution_times and not self.solves_problem(problem):
                formatted_solution_times = "{%s}" % ", ".join(
                    "%s: %.2f" % pair
                    for pair in sorted(solution_times.items()))
                print "   %-35s %s" % (problem, formatted_solution_times)
                count += 1
        print "(%d problems)" % count


def compute_portfolio(results, granularity):
    portfolio = Portfolio(results, dict.fromkeys(results.configs, 0))
    while True:
        portfolio.dump()
        print
        best_successor = None
        best_solved = -1
        for successor in portfolio.successors(granularity):
            succ_solved = successor.num_solved()
            if succ_solved > best_solved:
                best_solved = succ_solved
                best_successor = successor
        if best_successor.total_timeout() > TIMEOUT + EPSILON:
            return portfolio
        portfolio = best_successor

def compute_portfolio_using_ipc_scores(results, granularity):
    portfolio = Portfolio(results, dict.fromkeys(results.configs, 0))
    while True:
        portfolio.dump()
        print
        best_successor = None
        best_score = -1
        for successor in portfolio.successors(granularity):
            succ_score = successor.ipc_score()
            if succ_score > best_score:
                best_score = succ_score
                best_successor = successor
        if best_successor.total_timeout() > TIMEOUT + EPSILON:
            return portfolio
        portfolio = best_successor

def main():
    if len(sys.argv) >= 3:
        granularity = int(sys.argv[2])
    results = Results(open(sys.argv[1]))
    results.dump_statistics()
#    print "Computing portfolio using coverage..."
#    portfolio = compute_portfolio(results, granularity=120)
#    print
#    portfolio.dump()
#    # pprint.pprint(results.data)
#    print
#    print "Reducing portfolio..."
#    portfolio.reduce(granularity=1)
#    portfolio.dump()
#    print
#    portfolio.dump_unsolved()
#    print
#    portfolio.dump_marginal_contributions()
#    print
#    portfolio.dump_detailed_marginal_contributions(granularity=1)
#    print
#    print "Computing portfolio using IPC scores..."
    portfolio = compute_portfolio_using_ipc_scores(results, granularity=granularity)
    print
    portfolio.dump()
    # pprint.pprint(results.data)
    print
    print "Reducing portfolio..."
    portfolio.reduce_score_based(granularity=1)
    portfolio.dump()
    print
    portfolio.dump_unsolved()
    print
    portfolio.dump_marginal_contributions()
    print
    portfolio.dump_detailed_marginal_contributions(granularity=1)
    print


if __name__ == "__main__":
    main()
