#include <iostream>
using namespace std;

#include "data.h"
#include "options.h"
#include "tools.h"

int main(int argc, char *argv[]) {
  try {
    string domFile, probFile;
    ::options.read(argc, argv, domFile, probFile);
    Domain dom(domFile, probFile);
  } catch(StringException e) {
    cerr << e << endl;
    return -1;
  }
  return 0;
}
