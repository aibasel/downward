#ifndef EQUIVALENCE_RELATION_H
#define EQUIVALENCE_RELATION_H

#include <algorithm>
#include <cmath>
#include <ext/hash_map>
#include <list>
#include <vector>

class Block;
typedef std::list<int>::iterator ElementListIter;
typedef std::list<int>::const_iterator ElementListConstIter;
typedef std::list<Block>::iterator BlockListIter;
typedef std::list<Block>::const_iterator BlockListConstIter;
class EquivalenceRelation;

struct DoubleEpsilonEquality {
    bool operator() (const double &d1, const double &d2) {
        // TODO avoid code duplication with landmark count heuristic
        static const double epsilon = 0.01;
        return abs(d1 - d2) < epsilon;
    }
};

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
    bool empty() const;
    ElementListIter insert(int element);
    void erase(ElementListIter it);
    ElementListIter begin() {return elements.begin(); }
    ElementListIter end() {return elements.end(); }
    ElementListConstIter begin() const {return elements.begin(); }
    ElementListConstIter end() const {return elements.end(); }
};

class EquivalenceRelation {
    int num_elements;
    std::list<Block> blocks;
    /*
      With each element we associate a pair of iterators (block_it, element_it).
      block_it is an iterator from the list blocks pointing to the block that
      contains the element and element_it is an iterator from the list in this
      block and points to the element within it.
    */
    typedef std::pair<BlockListIter, ElementListIter> ElementPosition;
    typedef __gnu_cxx::hash_map<int, ElementPosition> ElementPositionMap;
    ElementPositionMap element_positions;

    /*
      Refining a relation with a block X is equivalent to splitting every block B
      into two blocks (B \cap X) and (B \setminus X).
    */
    void refine(const Block &block);

    BlockListIter add_empty_block();
public:
    EquivalenceRelation(int n);
    EquivalenceRelation(int n, const std::list<Block> &blocks_);
    ~EquivalenceRelation();

    int get_num_elements() const;
    int get_num_explicit_elements() const;
    int get_num_blocks() const;
    int get_num_explicit_blocks() const;
    // TODO: There may or may not be an implicitly defined Block. Should this be
    //       created and returned, too?
    //       The same question goes for get_num_blocks().
    //       This is also a problem with get_num_elements() as there can be less
    //       explicitly specified elements than num_elements.
    BlockListConstIter begin() const {return blocks.begin(); }
    BlockListConstIter end() const {return blocks.end(); }

    /*
      Refines the current relation with an other relation.
      After refining, two items A and B are in the same block if and only if
      they were in the same block before and they are in one block in the other
      relation. For both relations, items that are not in any block are assumed
      to be in one implicitly defined block.
      The amortized runtime is linear in the number of elements specified in other.
    */
    void refine(const EquivalenceRelation &other);

    /*
      Creates an equivalence relation over the numbers 0 to n -1.
      The vector annotated_elements cointains pairs (A, e) where A is an arbitrary
      annotation for element e. Elements must not occur more than once in this vector.
      Two elements are equivalent iff they are annotated with an equivalent annotation.

      All elements that are not mentioned in this vector are assumed to have one
      specific annotation that does not occur in annotated_elements, i.e. they are
      equivalent to each other, but not equivalent to anything mentioned in
      annotated_elements.
      The vector annotated_elements will be sorted by the constructor.
    */
    // NOTE unfortunately this is not possible as a constructor, since c++
    //      does not support templated constructors that only use the template
    //      parameter in a nested context.
    //      Also, default parameters are not allowed for function templates in
    //      the current c++ standard.
    template<class T>
    static EquivalenceRelation *from_annotated_elements(
            int n,
            std::vector<std::pair<T, int> > &annotated_elements);
    template<class T, class Equal>
    static EquivalenceRelation *from_annotated_elements(
            int n,
            std::vector<std::pair<T, int> > &annotated_elements);
};

template<class T>
EquivalenceRelation *EquivalenceRelation::from_annotated_elements(int n,
    std::vector<std::pair<T, int> > &annotated_elements) {
    return EquivalenceRelation::from_annotated_elements<T, std::equal_to<T> >(n, annotated_elements);
}

template<class T, class Equal>
EquivalenceRelation *EquivalenceRelation::from_annotated_elements(int n,
    std::vector<std::pair<T, int> > &annotated_elements) {
    EquivalenceRelation *relation = new EquivalenceRelation(n);
    if (!annotated_elements.empty()) {
        sort(annotated_elements.begin(), annotated_elements.end());
        Equal equal;
        T current_class_label = annotated_elements[0].first;
        BlockListIter it_current_block = relation->add_empty_block();
        for (size_t i = 0; i < annotated_elements.size(); ++i) {
            T label = annotated_elements[i].first;
            int element = annotated_elements[i].second;
            if (!equal(label, current_class_label)) {
                current_class_label = label;
                it_current_block = relation->add_empty_block();
            }
            ElementListIter it_element = it_current_block->insert(element);
            relation->element_positions[element] = make_pair(it_current_block, it_element);
        }
    }
    return relation;
}

#endif
