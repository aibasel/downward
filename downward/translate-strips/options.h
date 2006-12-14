#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <string>
using namespace std;

class Options {
public:
  enum {PARSING, CONSTANT, MERGE, EXPLORE, CODING,
	TRANSITION, PHASE_COUNT};
  enum {SILENT, NORMAL, VERBOSE, DEBUG};
private:
  static string usage;
  void error(string s);
  void setAll(int level);
  int outputLevel[PHASE_COUNT];
public:
  Options();
  void read(int argc, char *argv[], string &domFile, string &probFile);

  bool silent(int i)  {return outputLevel[i] == SILENT;}
  bool verbose(int i) {return outputLevel[i] >= VERBOSE;}
  bool debug(int i)   {return outputLevel[i] >= DEBUG;}
  bool allSilent();
};

extern Options options;

#endif
