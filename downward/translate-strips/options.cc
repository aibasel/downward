#include "options.h"
#include "tools.h"

int getPos(const char *str, char c) {
  for(int i = 0; str[i] != 0; i++)
    if(str[i] == c)
      return i;
  return -1;
}

Options::Options() {
  setAll(NORMAL);
}

void Options::setAll(int level) {
  for(int i = 0; i < PHASE_COUNT; i++)
    outputLevel[i] = level;
}

void Options::read(int argc, char *argv[], string &domFile, string &probFile) {
  vector<string> remaining;
  for(int i = 1; i < argc; i++) {
    const char *s = argv[i];
    if(s[0] == '-') {
      char c = s[1];
      int level = getPos("snvd", c);
      if(level != -1) {
	if(s[2] == 0) {
	  setAll(level);
	} else {
	  for(const char *c = s + 2; *c != 0; c++) {
	    int opt = getPos("pcmeot", *c);
	    if(opt == -1)
	      error(string("invalid phase <") + *c + ">");
	    outputLevel[opt] = level;
	  }
	}
      } else if(getPos("?h", c) != -1) {
	::error(usage);
      } else if(c == 0) {
	error("missing option");
      } else {
	error(string("invalid option -") + s[1]);
      }
    } else {
      remaining.push_back(s);
    }
  }
  int s = remaining.size();
  if(s == 0) {
    probFile = "problem.pddl";
    domFile = "domain.pddl";
  } else if(s == 1) {
    probFile = remaining[0];
    domFile = "domain.pddl";
  } else if(s == 2) {
    domFile = remaining[0];
    probFile = remaining[1];
  } else {
    error("more than two files in argument list");
  }
}

void Options::error(string s) {
  ::error(s + "\nType mips -? for help.\n");
}

bool Options::allSilent() {
  for(int i = 0; i < PHASE_COUNT; i++)
    if(!silent(i))
      return false;
  return true;
}

Options options;

string Options::usage =
  "usage: mips [<option> ...]\n"
  "       mips [<option> ...] <problem file>\n"
  "       mips [<option> ...] <domain file> <problem file>\n\n"
  "valid options are:\n"
  "-s: silent mode (minimal output)\n"
  "-n: normal mode\n"
  "-v: verbose output\n"
  "-d: debugging output\n\n"
  "Add characters from [pcmeobt] to restrict those options to\n"
  "parsing, detecting constants, merging, exploring, encoding,\n"
  "and transition function creation, respectively.\n\n"
  "If no domain file is specified, domain.pddl serves as a default.\n"
  "If no problem file is specified, problem.pddl serves as a default.\n";
