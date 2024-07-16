#ifndef ALGORITHMS_DYNAMIC_BITSET_H
#define ALGORITHMS_DYNAMIC_BITSET_H

#include <cassert>
#include <limits>
#include <vector>
#include <bit>

/*
  Poor man's version of boost::dynamic_bitset, mostly copied from there.
*/

namespace dynamic_bitset {
template<typename Block = unsigned int>
class DynamicBitset {
    static_assert(
        !std::numeric_limits<Block>::is_signed,
        "Block type must be unsigned");

    std::vector<Block> blocks;
    std::size_t num_bits;

    static const Block zeros;
    static const Block ones;

    static const int bits_per_block = std::numeric_limits<Block>::digits;

    static int compute_num_blocks(std::size_t num_bits) {
        return num_bits / bits_per_block +
               static_cast<int>(num_bits % bits_per_block != 0);
    }

    static std::size_t block_index(std::size_t pos) {
        return pos / bits_per_block;
    }

    static std::size_t bit_index(std::size_t pos) {
        return pos % bits_per_block;
    }

    static Block bit_mask(std::size_t pos) {
        return Block(1) << bit_index(pos);
    }

    int count_bits_in_last_block() const {
        return bit_index(num_bits);
    }

    void zero_unused_bits() {
        const int bits_in_last_block = count_bits_in_last_block();

        if (bits_in_last_block != 0) {
            assert(!blocks.empty());
            blocks.back() &= ~(ones << bits_in_last_block);
        }
    }

public:
    explicit DynamicBitset()
        : blocks(0, zeros),
          num_bits(0) {
    }
    explicit DynamicBitset(std::size_t num_bits)
        : blocks(compute_num_blocks(num_bits), zeros),
          num_bits(num_bits) {
    }
    // copy constructor
    DynamicBitset(const DynamicBitset &other)
	: blocks(other.blocks),
	  num_bits(other.num_bits) {
    }
    // copy assignment operator
    DynamicBitset& operator=(const DynamicBitset& other) {
        if (this == &other) {
            return *this;
        }
	blocks = other.blocks;
	num_bits = other.num_bits;
        return *this;
    }


    std::size_t size() const {
        return num_bits;
    }

    void resize(std::size_t _num_bits){
	if (num_bits != _num_bits){
	    num_bits = _num_bits;
	    std::size_t num_blocks = compute_num_blocks(num_bits);
	    blocks.resize(num_blocks, zeros);
	}
    }

    /*
      Count the number of set bits.

      The computation could be made faster by using a more sophisticated
      algorithm (see https://en.wikipedia.org/wiki/Hamming_weight).
    */
    int count() const {
        int result = 0;
        // for (std::size_t pos = 0; pos < num_bits; ++pos) {
        //     result += static_cast<int>(test(pos));
        // }
	for (Block blk : blocks){
	    result += std::popcount(blk);
	}
        return result;
    }

    void set() {
        std::fill(blocks.begin(), blocks.end(), ones);
        zero_unused_bits();
    }

    void reset() {
        std::fill(blocks.begin(), blocks.end(), zeros);
    }

    void set(std::size_t pos) {
        assert(pos < num_bits);
        blocks[block_index(pos)] |= bit_mask(pos);
    }

    void reset(std::size_t pos) {
        assert(pos < num_bits);
        blocks[block_index(pos)] &= ~bit_mask(pos);
    }

    bool test(std::size_t pos) const {
        assert(pos < num_bits);
        return (blocks[block_index(pos)] & bit_mask(pos)) != 0;
    }

    bool operator[](std::size_t pos) const {
        return test(pos);
    }

    bool intersects(const DynamicBitset &other) const {
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i] & other.blocks[i])
                return true;
        }
        return false;
    }

    bool is_subset_of(const DynamicBitset &other) const {
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i] & ~other.blocks[i])
                return false;
        }
        return true;
    }

    void update_not() {
        for (std::size_t i = 0; i < blocks.size(); ++i) {
	    blocks[i] = ~blocks[i];
        }
    }
    void update_and(const DynamicBitset &other) {
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
	    blocks[i] &= other.blocks[i];
        }
    }
    void update_or(const DynamicBitset &other) {
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
	    blocks[i] |= other.blocks[i];
        }
    }
    void update_andc(const DynamicBitset &other) { // and complement
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
	    blocks[i] &= ~(other.blocks[i]);
        }
    }
    void update_orc(const DynamicBitset &other) { // or complement
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
	    blocks[i] |= ~(other.blocks[i]);
        }
    }
    void update_xor(const DynamicBitset &other) {
        assert(size() == other.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
	    blocks[i] ^= other.blocks[i];
        }
    }
    DynamicBitset& operator&=(const DynamicBitset &other){
	update_and(other);
	return *this;
    }
    DynamicBitset& operator|=(const DynamicBitset &other){
	update_or(other);
	return *this;
    }
    DynamicBitset& operator^=(const DynamicBitset &other){
	update_xor(other);
	return *this;
    }
};

template<typename Block>
const Block DynamicBitset<Block>::zeros = Block(0);

template<typename Block>
// MSVC's bitwise negation always returns a signed type.
const Block DynamicBitset<Block>::ones = Block(~Block(0));
}

/*
This source file was derived from the boost::dynamic_bitset library
version 1.54. Original copyright statement and license for this
original source follow.

Copyright (c) 2001-2002 Chuck Allison and Jeremy Siek
Copyright (c) 2003-2006, 2008 Gennaro Prota

Distributed under the Boost Software License, Version 1.0.

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#endif
