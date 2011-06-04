#ifndef PDBS_HEURISTIC_UTILITY_H
#define PDBS_HEURISTIC_UTILITY_H

#include <vector>

class OptionParser;

extern void check_parsed_pdbs(OptionParser &parser, std::vector<std::vector<int> > &pattern_collection);

#endif
