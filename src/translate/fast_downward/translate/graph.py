#! /usr/bin/env python3

from collections import defaultdict


def transitive_closure(pairs):
    """Compute the transitive closure of a relation.

    The input must be a sequence of pairs.
    The output is a sorted list of pairs."""

    successors = defaultdict(list)
    sources = set()
    for u, v in pairs:
        sources.add(u)
        successors[u].append(v)

    result = []
    for source in sources:
        # Perform a depth-first search from this source. The
        # implementation is non-recursive to avoid hitting Python's
        # recursion limit.

        # Note that we don't want reflexive closure, so we add to the
        # result when processing "successor", not when processing
        # "current".
        reached = set()
        pending = [source]
        while pending:
            current = pending.pop()
            for successor in successors[current]:
                if successor not in reached:
                    result.append((source, successor))
                    reached.add(successor)
                    pending.append(successor)
    result.sort()
    return result


class Graph:
    def __init__(self, nodes):
        self.nodes = nodes
        self.neighbours = {u: set() for u in nodes}
    def connect(self, u, v):
        self.neighbours[u].add(v)
        self.neighbours[v].add(u)
    def connected_components(self):
        remaining_nodes = set(self.nodes)
        result = []
        def dfs(node):
            result[-1].append(node)
            remaining_nodes.remove(node)
            for neighbour in self.neighbours[node]:
                if neighbour in remaining_nodes:
                    dfs(neighbour)
        while remaining_nodes:
            node = next(iter(remaining_nodes))
            result.append([])
            dfs(node)
            result[-1].sort()
        return sorted(result)


if __name__ == "__main__":
    g = Graph([1, 2, 3, 4, 5, 6])
    g.connect(1, 2)
    g.connect(1, 3)
    g.connect(4, 5)
    assert g.connected_components() == [[1, 2, 3], [4, 5], [6]]

    assert transitive_closure([
        ("A", "B"),
        ("A", "C"),
        ("B", "C"),
        ("C", "B"),
        ("C", "D")]) == [
        ("A", "B"),
        ("A", "C"),
        ("A", "D"),
        ("B", "B"),
        ("B", "C"),
        ("B", "D"),
        ("C", "B"),
        ("C", "C"),
        ("C", "D")]
