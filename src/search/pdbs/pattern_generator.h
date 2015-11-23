#ifndef PDBS_PATTERN_GENERATOR_H
#define PDBS_PATTERN_GENERATOR_H

#include "types.h"

#include "../task_proxy.h"

#include <memory>


class PatternCollection {
    std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    std::shared_ptr<Patterns> patterns;
    std::shared_ptr<PDBCollection> pdbs;
    std::shared_ptr<PDBCliques> cliques;

    void construct_pdbs();
    void construct_cliques();
public:
    PatternCollection(
            std::shared_ptr<AbstractTask> task,
            std::shared_ptr<Patterns> patterns,
            std::shared_ptr<PDBCollection> pdbs = nullptr,
            std::shared_ptr<PDBCliques> cliques = nullptr);
    ~PatternCollection();

    std::shared_ptr<Patterns> get_patterns();
    std::shared_ptr<PDBCollection> get_pdbs();
    std::shared_ptr<PDBCliques> get_cliques();
};


class PatternCollectionGenerator {
public:
    virtual PatternCollection generate(std::shared_ptr<AbstractTask> task) = 0;
};

class PatternGenerator {
public:
    virtual Pattern generate(std::shared_ptr<AbstractTask> task) = 0;
};

#endif
