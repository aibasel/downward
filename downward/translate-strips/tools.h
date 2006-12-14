#ifndef _TOOLS_H
#define _TOOLS_H

#include <string>
#include <vector>
#include <iostream>
#include <ctime>
using namespace std;

extern void toDecimalString(int val, char *dest);
extern string toString(int val);
extern string toString(unsigned int val);
extern string toString(double val);
extern int pow(int x, int y);
extern int log2(int n); // rounds up
extern vector<vector<int> > getPerms(vector<int> vec);
extern void error(string str);    // throws StringException

// generate vector <from, from + step, from + 2 * step, ..., to - 1>
extern vector<int> fromTo(int from, int to, int step = 1);

// generate tuples (0,...,0), (0,...,1), ..., (0,...,max-1), (0,...,1,0)
extern vector<int> firstTuple(int size);
extern void nextTuple(vector<int> &vec, int maxCount);
extern bool lastTuple(const vector<int> &vec, int maxCount);

class Timer {
  time_t startTime;
  time_t lastTime;
  long startClock;
  long lastClock;
public:
  struct TimeSpan {
    TimeSpan(long s, long c) : seconds(s), clocks(c) {}
    long seconds;
    long clocks;
  };
  Timer();
  TimeSpan stop();    // stop and restart timer; return last time span
  TimeSpan total();   // return time span from first start to now
};

ostream &operator<<(ostream &os, Timer::TimeSpan span);

class ProgressBar {
  ostream &stream;
  int total;
  int dotsPrinted;
  int totalDots;

  int threshold;
  void calcThreshold();
public:
  ProgressBar(ostream &s, int tot, int dots = 20);
  void display(int count);
  void done();
};

class StringException {
  string text;
public:
  StringException(string str) : text(str) {}
  string toString() {return text;}
};

extern ostream &operator<<(ostream &os, StringException e);

#endif
