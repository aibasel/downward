# -*- coding: utf-8 -*-

from collections import defaultdict

__all__ = ["get_sccs"]


def get_sccs(digraph):
    return StronglyConnectedComponentComputation(digraph).get_result()


class StronglyConnectedComponentComputation(object):
    """Tarjan's algorithm for maximal strongly connected components.

    Since the original recursive version exceeds python's maximal
    recursion depth on some planning instances, this is an iterative
    version with an explicit recursion stack (iter_stack).

    Note that the derived graph where each SCC is a single "supernode"
    is necessarily acyclic. The SCCs returned by get_result() are in a
    topological sort order with respect to this derived DAG.
    """

    def __init__(self, unweighted_graph):
        self.graph = unweighted_graph
        self.BEGIN, self.CONTINUE, self.RETURN = 0, 1, 2 # "recursion" handling

    def get_result(self):
        self.indices = dict()
        self.lowlinks = defaultdict(lambda: -1)
        self.stack_indices = dict()
        self.current_index = 0
        self.stack = []
        self.sccs = []

        for i in range(len(self.graph)):
            if i not in self.indices:
                self.visit(i)
        self.sccs.reverse()
        return self.sccs

    def visit(self, vertex):
        iter_stack = [(vertex, None, None, self.BEGIN)]
        while iter_stack:
            v, w, succ_index, state = iter_stack.pop()

            if state == self.BEGIN:
                self.current_index += 1
                self.indices[v] = self.current_index
                self.lowlinks[v] = self.current_index
                self.stack_indices[v] = len(self.stack)
                self.stack.append(v)

                iter_stack.append((v, None, 0, self.CONTINUE))
            elif state == self.CONTINUE:
                successors = self.graph[v]
                if succ_index == len(successors):
                    if self.lowlinks[v] == self.indices[v]:
                        stack_index = self.stack_indices[v]
                        scc = self.stack[stack_index:]
                        del self.stack[stack_index:]
                        for n in scc:
                            del self.stack_indices[n]
                        self.sccs.append(scc)
                else:
                    w = successors[succ_index]
                    if w not in self.indices:
                        iter_stack.append((v, w, succ_index, self.RETURN))
                        iter_stack.append((w, None, None, self.BEGIN))
                    else:
                        if w in self.stack_indices:
                            self.lowlinks[v] = min(self.lowlinks[v],
                                                   self.indices[w])
                        iter_stack.append(
                            (v, None, succ_index + 1, self.CONTINUE))
            elif state == self.RETURN:
                self.lowlinks[v] = min(self.lowlinks[v], self.lowlinks[w])
                iter_stack.append((v, None, succ_index + 1, self.CONTINUE))
