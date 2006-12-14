#include "parser.h"
#include "tools.h"

#include <fstream>
using namespace std;

string LispEntity::toString() {
  if(atom)
    return value;
  string back;
  for(int i = 0; i < list.size(); i++) {
    back += (i == 0) ? '(' : ' ';
    back += list[i].toString();
  }
  return back + ')';
}

string LispEntity::getString() {
  if(!atom)
    ::error("atom required in expression " + toString());
  return value;
}

vector<LispEntity> &LispEntity::getList() {
  if(atom)
    ::error("list required in expression " + toString());
  return list;
}

vector<LispEntity> LispEntity::getAndList() {
  // returns list consisting of single entity if this is no
  // and list or returns list of entities if this is an and list
  if(!atom && list.size() > 0 && list[0].atom && list[0].value == "and")
    return vector<LispEntity>(list.begin() + 1, list.end());
  vector<LispEntity> back;
  back.push_back(*this);
  return back;
}

void LispScanner::eatSpace() {
  while(!stream->eof() && isspace(stream->peek()))
    if(stream->get() == '\n')
      lineNo++;
}

void LispScanner::readToken() {
  int c = stream->peek();
  while(c == ';') {
    while(!stream->eof() && stream->peek() != '\n')
      stream->get();
    eatSpace();
    c = stream->peek();
  }
  if(c == EOF) {
    nextToken = END;
    return;
  }

  if(c == '(') {
    stream->get();
    nextToken = LEFT;
  } else if(c == ')') {
    stream->get();
    nextToken = RIGHT;
  } else if(isalnum(c) || c == ':' || c == '-' || c == '_' || c == '?') {
    nextId = "";
    do {
      if(isupper(c))
	c = c - 'A' + 'a';
      nextId = nextId + char(c);
      stream->get();
      c = stream->peek();
    } while(!stream->eof() && (isalnum(c) || c == '-' || c == '_'));
    nextToken = ID;
  } else {
    error(string("unexpected character ") + char(c));
  }
  eatSpace();
}

LispScanner::LispScanner(string fileNm) {
  fileName = fileNm;
  lineNo = 1;
  stream = new ifstream(fileName.c_str());
  if(stream->fail())
    ::error("Error: Could not open file " + fileNm + ".\n");
  eatSpace();
  readToken();
}

LispScanner::~LispScanner() {
  delete stream;
}

int LispScanner::peekToken() {
  return nextToken;
}

int LispScanner::getToken() {
  currToken = nextToken;
  currId = nextId;
  readToken();
  return currToken;
}

string LispScanner::getValue() {
  return currId;
}

void LispScanner::error(string str) {
  ::error("Error: " + str + "\n"
	  "       in file: " + fileName + "\n"
	  "       in line: " + ::toString(lineNo) + "\n");
}

LispEntity LispParser::parseEntity(LispScanner &s) {
  int c = s.peekToken();
  if(c == LispScanner::END)
    s.error("unexpected end of file");
  if(c == LispScanner::RIGHT)
    s.error("unbalanced closing paranthesis");
  if(c == LispScanner::ID) {
    s.getToken();
    return LispEntity(s.getValue());
  }
  s.getToken(); // eat LEFT (only possibility left)
  vector<LispEntity> vec;
  while(s.peekToken() != LispScanner::RIGHT)
    vec.push_back(parseEntity(s));
  s.getToken(); // eat RIGHT
  return LispEntity(vec);
}

LispEntity LispParser::parseFile(string fileNm) {
  try {
    LispScanner scan(fileNm);
    LispEntity back = parseEntity(scan);
    if(scan.getToken() != LispScanner::END)
      scan.error("end of file expected");
      return back;
  } catch(StringException e) {
    cerr << e << endl;
    exit(-1);
  }
}
