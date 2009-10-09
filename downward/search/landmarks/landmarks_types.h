/*********************************************************************
 * Authors: Matthias Westphal (westpham@informatik.uni-freiburg.de),
 *          Silvia Richter (silvia.richter@nicta.com.au)
 * (C) Copyright 2008 Matthias Westphal and NICTA
 *
 * This file is part of LAMA.
 *
 * LAMA is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the license, or (at your option) any later version.
 *
 * LAMA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *********************************************************************/

#ifndef LANDMARKS_TYPES_H
#define LANDMARKS_TYPES_H

#include <utility>
#include <ext/hash_set>
#include <tr1/functional>

class hash_int_pair {
public:
    size_t operator()(const std::pair<int, int> &key) const {
        return size_t(1337*key.first + key.second);
    }
};


class hash_pointer {
public:
    size_t operator()(const void* p) const {
        //return size_t(reinterpret_cast<int>(p));
        std::tr1::hash<const void*> my_hash_class;
        return my_hash_class(p);
    }
};


typedef __gnu_cxx::hash_set<std::pair<int, int>, hash_int_pair> lm_set;
#endif
