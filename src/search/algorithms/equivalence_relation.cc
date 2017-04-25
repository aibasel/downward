#include "equivalence_relation.h"

#include <cassert>

using namespace std;

namespace equivalence_relation {
bool Block::empty() const {
    return elements.empty();
}

ElementListIter Block::insert(int element) {
    return elements.insert(elements.end(), element);
}

void Block::erase(ElementListIter it) {
    elements.erase(it);
}

EquivalenceRelation::EquivalenceRelation(int n)
    : num_elements(n) {
}

EquivalenceRelation::EquivalenceRelation(int n, const list<Block> &blocks_)
    : num_elements(n),
      blocks(blocks_) {
    for (BlockListIter it_block = blocks.begin();
         it_block != blocks.end(); ++it_block) {
        Block &block = *it_block;
        for (ElementListIter it_element = block.begin();
             it_element != block.end(); ++it_element) {
            element_positions[*it_element] = make_pair(it_block, it_element);
        }
    }
}

EquivalenceRelation::~EquivalenceRelation() {
}

int EquivalenceRelation::get_num_elements() const {
    return num_elements;
}

int EquivalenceRelation::get_num_explicit_elements() const {
    return element_positions.size();
}

int EquivalenceRelation::get_num_blocks() const {
    if (get_num_elements() == get_num_explicit_elements()) {
        return blocks.size();
    } else {
        return blocks.size() + 1;
    }
}

int EquivalenceRelation::get_num_explicit_blocks() const {
    return blocks.size();
}

BlockListIter EquivalenceRelation::add_empty_block() {
    Block empty;
    empty.it_intersection_block = blocks.end();
    return blocks.insert(blocks.end(), empty);
}

/*
  The relation is refined by using every block X from the other relation to
  split every blocks B in this relation into (B \cap X) and (B \setminus X).
  We can do this refinement with each block X individually.
*/
void EquivalenceRelation::refine(const EquivalenceRelation &other) {
    for (BlockListConstIter it = other.blocks.begin(); it != other.blocks.end(); ++it) {
        refine(*it);
    }
}

/*
  Refining along a block X means to split every blocks B in this relation into
  (B \cap X) and (B \setminus X).
  General idea:

  For every x \in X:
     Look up the block B containing x
     Remove x from B
     Add x to (B \cap X)

  The elements remaining in B are (B \setminus X).
  We associate the new block (B \cap X) with the block B to easily access it once
  we know block B. Block (B \cap X) is only created on demand, so it is never empty.
  We remember all blocks where at least one element was removed and remove those
  that become empty at the end of the loop.

  The sparse definition need special attention since there can be elements x \in X
  that are not specified in our relation. All unspecified elements are assumed
  to belong to a block U. Splitting U along X yields (U \cap X) and (U \setminus X).
  The block (U \setminus X) contains all elements that are neither specified
  in X nor in our relation -- they are kept unspecified.
  The block (U \cap X) contains all elements that are specified in X but not in
  our relation. If such elements exist, a new block is created for them.
*/
void EquivalenceRelation::refine(const Block &block_X) {
    refine(block_X.begin(), block_X.end());
}

void EquivalenceRelation::refine(ElementListConstIter block_X_begin,
                                 ElementListConstIter block_X_end) {
    // An iterator to the block (B \cap X) is stored in every block B that has
    // a non-empty intersection with X. This iterator has to be reset at the end
    // so all such blocks are stored.
    vector<BlockListIter> modified_blocks;
    // All elements that are specified in X but not in our relation belong in
    // the block (U \cap X) that is created on demand. This iterator refers to
    // the block if it was created already or to blocks.end() otherwise.
    BlockListIter it_unknown_elements = blocks.end();

    // For every x \in X:
    for (ElementListConstIter it_x = block_X_begin; it_x != block_X_end; ++it_x) {
        int x = *it_x;
        ElementPositionMap::iterator it_pos = element_positions.find(x);
        if (it_pos != element_positions.end()) {
            // Look up the block B containing x.
            ElementPosition &pos = it_pos->second;
            BlockListIter it_block_B = pos.first;
            Block &block_B = *it_block_B;
            ElementListIter it_previous_element = pos.second;
            // Create the block (B \cap X) on demand.
            if (block_B.it_intersection_block == blocks.end()) {
                block_B.it_intersection_block = add_empty_block();
                // Remember to reset the iterator at the end.
                modified_blocks.push_back(it_block_B);
            }
            // Remove x from B.
            block_B.erase(it_previous_element);
            // Add x to (B \cap X).
            ElementListIter it_element = block_B.it_intersection_block->insert(x);
            // Update the stored position of x.
            pos.first = block_B.it_intersection_block;
            pos.second = it_element;
        } else {
            // x is unspecified in our relation, so it belongs in block (U \cap X),
            // which is created on demand.
            if (it_unknown_elements == blocks.end()) {
                it_unknown_elements = add_empty_block();
            }
            ElementListIter it_element = it_unknown_elements->insert(x);
            // Store position of x.
            // NOTE using insert instead of find above could a second lookup
            //      but would complicate the code.
            element_positions[x] = make_pair(it_unknown_elements, it_element);
        }
    }
    // Reset the iterators referencing (B \cap X) for all modified blocks B and
    // remove any blocks B that became empty.
    for (size_t i = 0; i < modified_blocks.size(); ++i) {
        BlockListIter modified_block = modified_blocks[i];
        if (modified_block->empty()) {
            blocks.erase(modified_block);
        } else {
            modified_block->it_intersection_block = blocks.end();
        }
    }
}
}
