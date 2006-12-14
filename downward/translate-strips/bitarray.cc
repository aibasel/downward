#include "bitarray.h"

#include <algorithm>    // swap
using namespace std;

void BitArray::init(int size) {
  bitSize = size;
  chunkCount = (size + BITS - 1) / BITS;
  data = new unsigned long[chunkCount];
  for(int i = 0; i < chunkCount; i++)
    data[i] = 0;
}

void BitArray::assign(const BitArray &copy) {
  bitSize = copy.bitSize;
  chunkCount = copy.chunkCount;
  data = new unsigned long[chunkCount];
  for(int i = 0; i < chunkCount; i++)
    data[i] = copy.data[i];
}

void BitArray::swap(BitArray &other) {
  std::swap(data, other.data);
  std::swap(bitSize, other.bitSize);
  std::swap(chunkCount, other.chunkCount);
}

bool BitArray::operator==(const BitArray &comp) const {
  if(bitSize != comp.bitSize)
    return false;
  unsigned long *pos1 = data, *end1 = data + chunkCount;
  unsigned long *pos2 = comp.data;
  while(pos1 != end1)
    if(*pos1++ != *pos2++)
      return false;
  return true;
}

vector<int> BitArray::toIntVector() const {
  vector<int> back;
  toIntVector(back);
  return back;
}

BitArray::BitArray(int size, const vector<int> &vec) {
  init(size);
  for(int i = 0; i < vec.size(); i++)
    set(vec[i]);
}

void BitArray::toIntVector(vector<int> &vec) const {
  vec.clear();
  unsigned long *pos = data, *end = data + chunkCount;
  int i = 0;
  while(pos != end) {
    unsigned long val = *pos++;
    unsigned long mask = 1;
    do {
      if(val & mask)
        vec.push_back(i);
      mask <<= 1;
      i++;
    } while(mask);    // becomes 0 on shift overflow
  }
}
