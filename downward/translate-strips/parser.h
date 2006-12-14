#ifndef _PARSER_H
#define _PARSER_H

#include <vector>
#include <string>
#include <iostream>
using namespace std;

class LispEntity {
  bool atom;
  string value;             // this should be a union, but that is
  vector<LispEntity> list;  // not allowed for copy-constructor types
public:
  explicit LispEntity(string val) : atom(true), value(val) {}
  explicit LispEntity(const vector<LispEntity> &l) : atom(false), list(l) {}
  ~LispEntity() {}
  bool isString() {return atom;}
  bool isList() {return !atom;}
  string getString();
  vector<LispEntity> &getList();
  vector<LispEntity> getAndList();
  string toString();
};

class LispScanner {
  istream *stream;
  string fileName;
  int lineNo;

  int currToken, nextToken;
  string currId, nextId;

  void readToken();
  void eatSpace();
public:
  enum {LEFT, RIGHT, ID, END};
  LispScanner(string fileName);
  ~LispScanner();
  int peekToken();
  int getToken();
  string getValue();
  void error(string str);
};

class LispParser {
  LispEntity parseEntity(LispScanner &s);
public:
  LispParser() {}
  ~LispParser() {}
  LispEntity parseFile(string fileNm);
};

#endif
