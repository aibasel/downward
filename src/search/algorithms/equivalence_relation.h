#ifndef ALGORITHMS_EQUIVALENCE_RELATION_H
#define ALGORITHMS_EQUIVALENCE_RELATION_H

#include <algorithm>
#include <list>
#include <unordered_map>
#include <vector>

namespace equivalence_relation {
class Block;
class EquivalenceRelation;

using ElementListIter = std::list<int>::iterator;
using ElementListConstIter = std::list<int>::const_iterator;
using BlockListIter = std::list<Block>::iterator;
using BlockListConstIter = std::list<Block>::const_iterator;


class Block {
    std::list<int> elements;
    /*
      During the refinement step of EquivalenceRelation, every existing block B
      is split along every new block X into the intersection and difference of
      B and X. The way the algorithm is set up, the difference remains in the
      block that previously represented B. To store the intersection, a new block
      is created and stored in B for easier access.
    */
    friend class EquivalenceRelation;
    BlockListIter it_intersection_block;
public:
    bool empty() const {
        return elements.empty();
    }

    ElementListIter insert(int element) {
        return elements.insert(elements.end(), element);
    }

    void erase(ElementListIter it) {
        elements.erase(it);
    }

    ElementListIter begin() {
        return elements.begin();
    }

    ElementListIter end() {
        return elements.end();
    }

    ElementListConstIter begin() const {
        return elements.begin();
    }

    ElementListConstIter end() const {
        return elements.end();
    }
};

class EquivalenceRelation {
    std::list<Block> blocks;
    /*
      With each element we associate a pair of iterators (block_it, element_it).
      block_it is an iterator from the list blocks pointing to the block that
      contains the element and element_it is an iterator from the list in this
      block and points to the element within it.
    */
    using ElementPosition = std::pair<BlockListIter, ElementListIter>;
    using ElementPositionMap = std::unordered_map<int, ElementPosition>;
    ElementPositionMap element_positions;

    BlockListIter add_empty_block();
public:
    explicit EquivalenceRelation(const std::vector<int> &elements);

    BlockListConstIter begin() const {return blocks.begin();}
    BlockListConstIter end() const {return blocks.end();}

    /*
      Refining a relation with a block X is equivalent to splitting every block B
      into two blocks (B \cap X) and (B \setminus X). After refining, two items A
      and B are in the same block if and only if they were in the same block
      before and they are in one block in the other relation. The amortized
      runtime is linear in the number of elements specified in other.
    */
    void refine(const std::vector<int> &block);
};
}

#endif
