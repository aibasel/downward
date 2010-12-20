#ifndef PATTERN_GENERATION_HASLUM_H
#define PATTERN_GENERATION_HASLUM_H

#include <vector>

// Implementation of the pattern generation algorithm by Haslum et al.
class PatternGenerationHaslum {
    std::vector<std::vector<int> > pattern_collection;
    void hill_climbing();
public:
    PatternGenerationHaslum(int max_pdb_memory);
    virtual ~PatternGenerationHaslum();
    const std::vector<std::vector<int> > &get_pattern_collection();
};

#endif
