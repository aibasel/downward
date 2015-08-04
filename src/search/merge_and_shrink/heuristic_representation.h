#ifndef MERGE_AND_SHRINK_HEURISTIC_REPRESENTATION_H
#define MERGE_AND_SHRINK_HEURISTIC_REPRESENTATION_H

#include <memory>
#include <vector>

class State;


class HeuristicRepresentation {
protected:
    int domain_size;

public:
    explicit HeuristicRepresentation(int domain_size);
    virtual ~HeuristicRepresentation() = 0;

    int get_domain_size() const;

    virtual int get_abstract_state(const State &state) const = 0;
    virtual void apply_abstraction_to_lookup_table(
        const std::vector<int> &abstraction_mapping) = 0;
};


class HeuristicRepresentationLeaf : public HeuristicRepresentation {
    const int var_id;

    std::vector<int> lookup_table;
public:
    HeuristicRepresentationLeaf(int var_id, int domain_size);
    virtual ~HeuristicRepresentationLeaf() = default;

    virtual void apply_abstraction_to_lookup_table(
        const std::vector<int> &abstraction_mapping) override;
    virtual int get_abstract_state(const State &state) const override;
};


class HeuristicRepresentationMerge : public HeuristicRepresentation {
    std::unique_ptr<HeuristicRepresentation> left_child;
    std::unique_ptr<HeuristicRepresentation> right_child;
    std::vector<std::vector<int> > lookup_table;
public:
    HeuristicRepresentationMerge(
        std::unique_ptr<HeuristicRepresentation> left_child,
        std::unique_ptr<HeuristicRepresentation> right_child);
    virtual ~HeuristicRepresentationMerge() = default;

    virtual void apply_abstraction_to_lookup_table(
        const std::vector<int> &abstraction_mapping) override;
    virtual int get_abstract_state(const State &state) const override;
};


#endif
