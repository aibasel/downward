#ifndef ALGORITHMS_NAMED_VECTOR_H
#define ALGORITHMS_NAMED_VECTOR_H

#include <string>
#include <vector>

namespace named_vector {
/*
  NamedVector is a vector-like collection with optional names to be associated with each element.
  It is optimized to have minimal overhead when there are no names.
  Any name which is not specified is assumed to be an empty string.
 */
template<typename T>
class NamedVector {
    std::vector<T> elements;
    std::vector<std::string> names;
public:
    NamedVector() = default;
    NamedVector(NamedVector<T> &&other) = default;
    NamedVector(NamedVector<T> &other) = default;

    template<typename ... _Args>
    void emplace_back(_Args && ... __args) {
        elements.emplace_back(std::forward<_Args>(__args) ...);
    }

    void push_back(const T &element) {
        elements.push_back(element);
    }

    T &operator[](int index) {
        return elements[index];
    }

    bool has_names() const {
        return !names.empty();
    }

    void set_name(int index, const std::string &name) {
        if (index >= names.size()) {
            if (name.empty()) {
                // All unspecified names are empty by default.
                return;
            }
            names.resize(index + 1, "");
        }
        names[index] = name;
    }

    std::string get_name(int index) const {
        int num_names = names.size();
        if (index < num_names) {
            return names[index];
        } else {
            // All unspecified names are empty by default.
            return "";
        }
    }

    int size() const {
        return elements.size();
    }

    typename std::vector<T>::reference back() {
        return elements.back();
    }

    typename std::vector<T>::iterator begin() {
        return elements.begin();
    }

    typename std::vector<T>::iterator end() {
        return elements.end();
    }

    typename std::vector<T>::const_iterator begin() const {
        return elements.begin();
    }

    typename std::vector<T>::const_iterator end() const {
        return elements.end();
    }

    void clear() {
        elements.clear();
        names.clear();
    }

    void reserve(int capacity) {
        /* No space is reserved in the names vector because it is kept at minimal length
           and space is only used when necessary. */
        elements.reserve(capacity);
    }

    void resize(int count, T value) {
        /* The names vector is not resized because it is kept at minimal length
           and only resized when necessary. */
        elements.resize(count, value);
    }
};
}

#endif
