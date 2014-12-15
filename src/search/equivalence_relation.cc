#include "equivalence_relation.h"

#include "utilities.h"

#include <cassert>
#include <iostream>

using namespace std;

Block::Block(BlockListIter block_it)
    : it_intersection_block(block_it) {
}

bool Block::empty() const {
    return elements.empty();
}

ElementListIter Block::insert(int element) {
    return elements.insert(elements.end(), element);
}

ElementListIter Block::erase(ElementListIter it) {
    return elements.erase(it);
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

int EquivalenceRelation::replace_elements_by_new_one(
    const std::vector<int> &existing_elements,
    int new_element) {
    assert(new_element == num_elements);

    /*
      Remove all existing elements from their block and remove the entry in
      element_positions.
    */
    BlockListIter canonical_block_it = element_positions[existing_elements[0]].first;
    for (size_t i = 0; i < existing_elements.size(); ++i) {
        int old_element = existing_elements[i];
        assert(element_positions[old_element].first == canonical_block_it);
        ElementListIter element_it = element_positions[old_element].second;
        canonical_block_it->erase(element_it);
        element_positions.erase(old_element);
    }

    if (canonical_block_it->empty()) {
        /*
          Note that if all labels from a block are replaced by a single new
          one, we could use the same block, but for the sake of the blocks
          being orderd (by the smallest element contained in the block), we
          delete the old empty block and create a new one.

          TODO: due to the incremental way of calling this, all old elements
          of a block can be removed one after the other, leaving the block
          with only new elements, breaking the order.
        */
        blocks.erase(canonical_block_it);
        canonical_block_it = add_empty_block();
    }

    /*
      Insert the new element into the block of the removed elements, if it
      did not become empty, or into the newly created block (see above).
    */
    ElementListIter elem_it = canonical_block_it->insert(new_element);
    element_positions[new_element] = make_pair(canonical_block_it, elem_it);
    ++num_elements;
    return *canonical_block_it->begin();
}

void EquivalenceRelation::remove_elements(const vector<int> &existing_elements) {
    /*
      Remove all existing elements from their block (and the block itself if
      it becomes empty) and remove their entries in element_positions.
    */
    for (size_t i = 0; i < existing_elements.size(); ++i) {
        int old_element = existing_elements[i];
        BlockListIter block_it = element_positions[old_element].first;
        ElementListIter element_it = element_positions[old_element].second;
        block_it->erase(element_it);
        element_positions.erase(old_element);
        if (block_it->empty()) {
            blocks.erase(block_it);
        }
    }
}

void EquivalenceRelation::insert(int new_element, int existing_element) {
    BlockListIter block_it;
    if (existing_element == -1) {
        block_it = add_empty_block();
    } else {
        block_it = element_positions[existing_element].first;
    }
    ElementListIter elem_it = block_it->insert(new_element);
    element_positions[new_element] = make_pair(block_it, elem_it);
    assert(new_element == num_elements);
    ++num_elements;
}

void EquivalenceRelation::integrate_second_into_first(BlockListIter block1_it,
                                                      BlockListIter block2_it) {
    vector<int> elements;
    elements.reserve(block1_it->size() + block2_it->size());
    ElementListIter elem_it = block1_it->begin();
    while (elem_it != block1_it->end()) {
        int element = *elem_it;
        elements.push_back(element);
        elem_it = block1_it->erase(elem_it);
    }
    assert(block1_it->empty());
    elem_it = block2_it->begin();
    while (elem_it != block2_it->end()) {
        int element = *elem_it;
        elements.push_back(element);
        elem_it = block2_it->erase(elem_it);
    }
    assert(block2_it->empty());
    sort(elements.begin(), elements.end());
    BlockListIter new_block_it = block1_it;
    for (size_t i = 0; i < elements.size(); ++i) {
        int element = elements[i];
        ElementListIter elem_it = new_block_it->insert(element);
        element_positions[element] = make_pair(block1_it, elem_it);
    }
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

void EquivalenceRelation::sort_blocks() {
    blocks.sort();
}

bool EquivalenceRelation::are_blocks_sorted() const {
    int previous_element = -1;
    for (BlockListConstIter block_it = blocks.begin();
         block_it != blocks.end(); ++block_it) {
        if (*block_it->begin() <= previous_element) {
            return false;
        }
        previous_element = *block_it->begin();
    }
    return true;
}

BlockListIter EquivalenceRelation::erase(BlockListIter block_it) {
    return blocks.erase(block_it);
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
    // An iterator to the block (B \cap X) is stored in every block B that has
    // a non-empty intersection with X. This iterator has to be reset at the end
    // so all such blocks are stored.
    vector<BlockListIter> modified_blocks;
    // All elements that are specified in X but not in our relation belong in
    // the block (U \cap X) that is created on demand. This iterator refers to
    // the block if it was created already or to blocks.end() otherwise.
    BlockListIter it_unknown_elements = blocks.end();

    // For every x \in X:
    for (ElementListConstIter it_x = block_X.begin(); it_x != block_X.end(); ++it_x) {
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

void EquivalenceRelation::dump() const {
    cout << "equivalence relation: " << endl;
    for (BlockListConstIter it_block = blocks.begin();
         it_block != blocks.end(); ++it_block) {
        const Block &block = *it_block;
        cout << "block: ";
        for (ElementListConstIter it_element = block.begin();
             it_element != block.end(); ++it_element) {
            cout << *it_element << ", ";
        }
        cout << endl;
    }
}
