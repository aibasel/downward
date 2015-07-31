#ifndef PDBS_UTIL_H
#define PDBS_UTIL_H

class OptionParser;
class Options;

/*
  NOTE that both of the following methods expect that the parser already has
  an option named "transform" to parse a task transformation.
  The methods will add parser options to parse one or many patterns, call
  parser.parse() and store the resulting Options object in the output parameter
  opts. The parsed patterns are validated and stored in opts.
*/
extern void parse_pattern(OptionParser &parser, Options &opts);
extern void parse_patterns(OptionParser &parser, Options &opts);

#endif
