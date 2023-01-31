#include "equivalence_relation.h"

#include "../utils/system.h"

using namespace std;

namespace equivalence_relation {
EquivalenceRelation::EquivalenceRelation(const vector<int> &elements) {
    BlockListIter it_block = add_empty_block();
    for (const int &element : elements) {
        ElementListIter it_element = it_block->insert(element);
        element_positions[element] = make_pair(it_block, it_element);
    }
}

BlockListIter EquivalenceRelation::add_empty_block() {
    Block empty;
    empty.it_intersection_block = blocks.end();
    return blocks.insert(blocks.end(), empty);
}

/*
  Refining along a block X means to split every block B in this relation into
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
*/
void EquivalenceRelation::refine(const vector<int> &block) {
    /*
      An iterator to the block (B \cap X) is stored in every block B that has
      a non-empty intersection with X. This iterator has to be reset at the end
      so all such blocks are stored.
    */
    vector<BlockListIter> modified_blocks;

    for (int x : block) {
        typename ElementPositionMap::iterator it_pos = element_positions.find(x);
        if (it_pos == element_positions.end()) {
            ABORT("Element from given block not contained in equivalence "
                  "relation.");
        }

        // Look up the block B containing x.
        ElementPosition &pos = it_pos->second;
        BlockListIter it_block_B = pos.first;
        Block &block_B = *it_block_B;

        // Create the block (B \cap X) on demand.
        if (block_B.it_intersection_block == blocks.end()) {
            block_B.it_intersection_block = add_empty_block();
            // Remember to reset the iterator at the end.
            modified_blocks.push_back(it_block_B);
        }
        // Remove x from B.
        ElementListIter it_previous_pos_x = pos.second;
        block_B.erase(it_previous_pos_x);
        // Add x to (B \cap X).
        ElementListIter it_new_pos_x = block_B.it_intersection_block->insert(x);
        // Update the stored position of x.
        pos.first = block_B.it_intersection_block;
        pos.second = it_new_pos_x;
    }
    // Reset the iterators referencing (B \cap X) for all modified blocks B and
    // remove any blocks B that became empty.
    for (BlockListIter modified_block : modified_blocks) {
        if (modified_block->empty()) {
            blocks.erase(modified_block);
        } else {
            modified_block->it_intersection_block = blocks.end();
        }
    }
}
}
