#pragma once

#include <iostream>

#include <tuple>
#include <utility>		// for pair

#include <vector>
#include <deque>
#include <array>

#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include <memory>

namespace utils {

template<class T, class S>
std::ostream& operator<<(std::ostream &os, const std::pair<T,S> &thing) {
    os << "[" << thing.first << ", " << thing.second << "]" ;
    return os;
}

// Helper functions to iterate over tuples
template <typename Tuple>
struct TupleIterator {
    template <std::size_t I, std::size_t... Is>
    static void print(std::ostream& os, const Tuple& tuple, std::index_sequence<I, Is...>) {
	os << std::get<0>(tuple);
	((os << ", " << std::get<Is>(tuple)), ...);
    }
};

template <typename... Args>
std::ostream& operator<<(std::ostream& os, const std::tuple<Args...>& tuple) {
    constexpr std::size_t tuple_size = std::tuple_size<std::tuple<Args...>>::value;
    os << "[";
    if constexpr (tuple_size > 0) {
        TupleIterator<std::tuple<Args...>>::print(os, tuple, std::index_sequence_for<Args...>());
    }
    os << "]";
    return os;
}


template<class T>
std::ostream& operator<<(std::ostream &os, const std::deque<T> &thing) {
    os << "[";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem;
        first = false;
    }
    os << "]" ;
    return os;
}


template<class T>
std::ostream& operator<<(std::ostream &os, const std::vector<T> &thing) {
    os << "[";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem;
        first = false;
    }
    os << "]" ;
    return os;
}


template<class T, std::size_t N>
std::ostream& operator<<(std::ostream &os, const std::array<T, N> &thing) {
    os << "[";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem;
        first = false;
    }
    os << "]" ;
    return os;
}


template<class T, typename... Args>
std::ostream& operator<<(std::ostream &os, const std::set<T,Args...> &thing) {
    os << "{";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem;
        first = false;
    }
    os << "}" ;
    return os;
}

template<class T, typename... Args>
std::ostream& operator<<(std::ostream &os, const std::unordered_set<T,Args...> &thing) {
    os << "{";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem;
        first = false;
    }
    os << "}" ;
    return os;
}

template<class K, class V, typename... Args>
std::ostream& operator<<(std::ostream &os, const std::map<K,V,Args...> &thing) {
    os << "{";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem.first
	   << " : "
	   << elem.second ;
        first = false;
    }
    os << "}" ;
    return os;
}

template<class K, class V, typename... Args>
std::ostream& operator<<(std::ostream &os, const std::unordered_map<K,V,Args...> &thing) {
    os << "{";
    bool first = true;
    for (auto elem : thing) {
        if (! first) {
            os << ", ";
        }
        os << elem.first
	   << " : "
	   << elem.second ;
        first = false;
    }
    os << "}" ;
    return os;
}


template<class T>
std::ostream& operator<<(std::ostream &os, const std::shared_ptr<T> &thing) {
    os << "@(" << reinterpret_cast<std::size_t>(thing.get()) << ", c=" << thing.use_count() << ")";
    return os;
}

// template<class T>
// std::ostream& operator<<(std::ostream &os, const T* thing) {
//     os << "@"s << (thing);
//     return os;
// }

}
