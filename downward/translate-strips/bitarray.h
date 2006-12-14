#ifndef _BITARRAY_H
#define _BITARRAY_H

#include <vector>
using namespace std;

class BitArray {
  class reference {
    unsigned long mask;
    unsigned long &ref;
  public:
    reference(unsigned long m, unsigned long &r) : mask(m), ref(r) {}
    reference &operator=(bool val) {
      if(val)
        ref |= mask;
      else
        ref &= ~mask;
      return *this;
    }
    operator bool()   {return (ref & mask) != 0;}     // returns 0 or 1
    bool operator~()  {return !(bool(*this));}
  };

  enum {BITS = sizeof(unsigned long) * 8};
  unsigned long *data;
  int bitSize;
  int chunkCount;
  void init(int size);
  void assign(const BitArray &copy);
public:
  explicit BitArray(int size = 0)           {init(size);}
  BitArray(const BitArray &copy)            {assign(copy);}
  BitArray(int size, const vector<int> &set); // dual to toIntVector
  BitArray &operator=(const BitArray &copy) {delete[] data; assign(copy); return *this;}
  ~BitArray()                               {delete[] data;}
  void resize(int size)                     {delete[] data; init(size);}
  void swap(BitArray &other);
  bool operator==(const BitArray &comp) const;

  int size() const {return bitSize;}
  reference operator[](int idx) {return reference(1UL << (idx % BITS), data[idx / BITS]);}

  // methods for very fast manipulation and access
  unsigned long get(int idx) const {   // returns 0 or non-0
    return data[idx / BITS] & (1UL << (idx % BITS));
  }
  void set(int idx)   {data[idx / BITS] |= 1UL << (idx % BITS);}
  void clear(int idx) {data[idx / BITS] &= ~(1UL << (idx % BITS));}
  void flip(int idx)  {data[idx / BITS] ^= 1UL << (idx % BITS);}

  BitArray &operator|=(const BitArray &other) {
    if(bitSize != other.bitSize)
      throw InvalidSizeException();
    unsigned long *pos1 = data, *end1 = data + chunkCount;
    unsigned long *pos2 = other.data;
    while(pos1 != end1)
      *pos1++ |= *pos2++;
    return *this;
  }
  BitArray &operator&=(const BitArray &other) {
    if(bitSize != other.bitSize)
      throw InvalidSizeException();
    unsigned long *pos1 = data, *end1 = data + chunkCount;
    unsigned long *pos2 = other.data;
    while(pos1 != end1)
      *pos1++ &= *pos2++;
    return *this;
  }
  BitArray &operator^=(const BitArray &other) {
    if(bitSize != other.bitSize)
      throw InvalidSizeException();
    unsigned long *pos1 = data, *end1 = data + chunkCount;
    unsigned long *pos2 = other.data;
    while(pos1 != end1)
      *pos1++ ^= *pos2++;
    return *this;
  }

  vector<int> toIntVector() const;
  void toIntVector(vector<int> &vec) const;

  struct InvalidSizeException {};
};

#endif
