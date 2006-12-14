#include "tools.h"

#include <cstdio>
#include <algorithm>
#include <iomanip>
using namespace std;

Timer::Timer() {
  time(&startTime);
  lastTime = startTime;
  startClock = lastClock = ::clock();
}

Timer::TimeSpan Timer::stop() {
  long s;
  time(&s);
  long c = ::clock();
  TimeSpan back(s - lastTime, c - lastClock);
  lastClock = c;
  return back;
}

Timer::TimeSpan Timer::total() {
  long s;
  time(&s);
  return TimeSpan(s - startTime, ::clock() - startClock);
}

ostream &operator<<(ostream &os, Timer::TimeSpan span) {
  // This is not as it should be.
  // "fixed" manipulator isn't supported by our old g++,
  // and flags set by manipulators like "setprecision" do not
  // reset automatically. Thus, ugly as it is, sprintf is used.
  int sec = span.seconds;
  char buf[32];
  if(sec >= 1000) {
    // The reason for this uglyness: clocks overflow
    // (at least on my machine) after 2^31/1000
    // (about 2100) seconds.
    sprintf(buf, "%d", sec);
  } else {
    double clocks = double(span.clocks) / CLOCKS_PER_SEC;
    sprintf(buf, "%.2fs", clocks);
  }
  os << buf;
  return os;
}

ProgressBar::ProgressBar(ostream &s, int tot, int dots)
  : stream(s), total(tot), dotsPrinted(0), totalDots(dots) {
  calcThreshold();
}

void ProgressBar::calcThreshold() {
  int granularity = total / totalDots;
  threshold = min((dotsPrinted + 1) * granularity, total);
}

void ProgressBar::display(int count) {
  while(count >= threshold && dotsPrinted < totalDots) {
    stream << '.' << flush;
    ++dotsPrinted;
    calcThreshold();
  }
}

void ProgressBar::done() {
  while(dotsPrinted++ < totalDots)
    stream << '.';
  stream << "done!" << flush;
}

void toDecimalString(int val, char *dest) {
  static char buf[16];
  char *ptr = buf;
  do {
    *ptr++ = '0' + val % 10;
    val /= 10;
  } while(val);
  while(--ptr >= buf)
    *dest++ = *ptr;
  *dest = 0;
}

string toString(int val) {
  // "ostringstream" not supported by our old g++, thus
  // old-fashioned C-style is used ("itoa" is non-standard).
  char buf[16];
  toDecimalString(val, buf);
  return string(buf);
}

string toString(unsigned int val) {
  return toString((int) val);
}

string toString(double val) {
  char buf[32];
  sprintf(buf, "%f", val);
  return string(buf);
}

int pow(int x, int y) {
  int back = 1;
  while(y--)
    back *= x;
  return back;
}

int log2(int n) {
  int back = 0;
  n--;
  while(n) {
    back++;
    n >>= 1;
  }
  return back;  
}

vector<vector<int> > getPerms(vector<int> vec) {
  vector<vector<int> > back;
  do {
    back.push_back(vec);
  } while(next_permutation(vec.begin(), vec.end()));
  return back;
}

vector<int> firstTuple(int count) {
  vector<int> back;
  back.resize(count, 0);
  return back;
}

void nextTuple(vector<int> &vec, int maxCount) {
  if(vec.size() == 0)
    vec.push_back(maxCount);  // create past-the-end tuple
  else {
    int pos = vec.size() - 1;
    while(pos >= 0 && ++vec[pos] == maxCount)
      vec[pos--] = 0;
    if(pos < 0)
      vec[0] = maxCount;  // past-the-end value
  }
 return;
}

bool lastTuple(const vector<int> &vec, int maxCount) {
  return vec.size() > 0 && vec[0] == maxCount;
}

vector<int> fromTo(int from, int to, int step) {
  vector<int> back;
  back.reserve((to - from) / step + 1);
  if(step > 0) {
    while(from < to) {
      back.push_back(from);
      from += step;
    }
  } else {
    while(from > to) {
      back.push_back(from);
      from += step;
    }
  }
  return back;
}

ostream &operator<<(ostream &os, StringException e) {
  os << e.toString();
  return os;
}

void error(string str) {
  throw StringException(str);
}
