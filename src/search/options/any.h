#ifndef OPTIONS_ANY_H
#define OPTIONS_ANY_H

#include <algorithm>
#include <exception>
#include <typeinfo>

/*
  Poor-man's version of boost::any, mostly copied from there.
  Does not support
    - moving
    - references as values
    - perfect forwarding
  All of these features can be added if we need them (see boost::any)
*/
class Any {
    class Placeholder {
    public:
        virtual ~Placeholder() {}
        virtual Placeholder *clone() const = 0;
        virtual const std::type_info& type() const = 0;
    };

    template<typename ValueType>
    class Holder : public Placeholder {
        Holder &operator=(const Holder&) = delete;
    public:
        ValueType held;

        Holder(const ValueType &value)
          : held(value) {
        }

        virtual Placeholder *clone() const {
            return new Holder(held);
        }

        virtual const std::type_info& type() const {
            return typeid(ValueType);
        }
    };

    template<typename ValueType>
    friend ValueType *any_cast(Any *);

    Placeholder *content;

public:
    Any() : content(nullptr) {}

    Any(const Any &other)
        : content(other.content ? other.content->clone() : 0) {
    }

    template<typename ValueType>
    Any(ValueType &value)
        : content(new Holder<ValueType>(value)) {
    }

    ~Any() {
        delete content;
    }

    template<typename ValueType>
    Any &operator=(const ValueType &rhs) {
        Any(rhs).swap(*this);
        return *this;
    }

    Any &operator=(Any rhs) {
        rhs.swap(*this);
        return *this;
    }

    Any &swap(Any &rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    const std::type_info &type() const {
        return content ? content->type() : typeid(void);
    }
};

class BadAnyCast : public std::bad_cast {
public:
    virtual const char *what() const noexcept {
        return "BadAnyCast: failed conversion using any_cast";
    }
};


template<typename ValueType>
ValueType *any_cast(Any *operand) {
    return operand && operand->type() == typeid(ValueType)
        ? &static_cast<Any::Holder<ValueType> *>(operand->content)->held
        : 0;
}

template<typename ValueType>
inline const ValueType *any_cast(const Any *operand) {
    return any_cast<ValueType>(const_cast<Any *>(operand));
}


template<typename ValueType>
ValueType any_cast(Any &operand) {
    ValueType *result = any_cast<ValueType>(&operand);
    if(!result)
        throw BadAnyCast();
    return *result;
}

template<typename ValueType>
inline ValueType any_cast(const Any &operand) {
    return any_cast<const ValueType>(const_cast<Any &>(operand));
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#endif
