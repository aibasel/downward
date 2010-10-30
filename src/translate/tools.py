import itertools

try:
    from itertools import product # only available in Python 2.6+
except ImportError: # before Python 2.6
    def product(*sequences):
        """
        > product([a,b,c], [x,y]) => [(a,x), (a,y), (b,x), (b,y), (c,x), (c,y)]
        """
        result = [()]
        for sequence in sequences:
            result = [x + (y,) for x in result for y in sequence]
        return result
        

def cartesian_product(sequences):
    # TODO: Rename this. It's not good that we have two functions
    # called "product" and "cartesian_product", of which "product"
    # computes cartesian products, while "cartesian_product" does not.
    
    # This isn't actually a proper cartesian product because we
    # concatenate lists, rather than forming sequences of atomic elements.
    # We could probably also use something like
    # map(itertools.chain, product(*sequences))
    # but that does not produce the same results
    if not sequences:
        yield []
    else:
        temp = list(cartesian_product(sequences[1:]))
        for item in sequences[0]:
            for sequence in temp:
                yield item + sequence
    

def permutations(alist):
    # TODO: As of Python 2.6, there is a built-in way of computing
    # permutations, which unfortunately generates them in a different
    # order (lexicographic). We should probably ultimately switch to
    # that, but not without testing performance since it can change
    # the behaviour of invariant synthesis.
    
    # Note: The list is changed in place as the algorithm is performed.
    #       The original value is restored when we are done.
    #       Since this is a generator, the caller must be aware
    #       of this side effect and copy the input parameter if necessary.
    # This is Knuth's "Algorithm P" ("plain changes").
    N = len(alist)
    if N <= 1:   # Special-case small numbers for speed.
        yield alist
    elif N == 2:
        yield alist
        yield [alist[1], alist[0]]
    else:
        c = [0] * N
        d = [1] * N
        while True:
            yield alist
            s = 0
            for j in xrange(N - 1, -1, -1):
                q = c[j] + d[j]
                if q == j + 1:
                    if j == 0:
                        return
                    s += 1
                elif q >= 0:
                    index1, index2 = j - c[j] + s, j - q + s
                    alist[index1], alist[index2] = alist[index2], alist[index1]
                    c[j] = q
                    break
                d[j] = -d[j]
