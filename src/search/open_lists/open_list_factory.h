#ifndef OPEN_LISTS_OPEN_LIST_FACTORY_H
#define OPEN_LISTS_OPEN_LIST_FACTORY_H

#include <memory>


class StateID;

template<typename T>
class OpenList;


class OpenListFactory {
public:
    OpenListFactory() = default;
    virtual ~OpenListFactory() = default;

    OpenListFactory(const OpenListFactory &) = delete;

    virtual std::unique_ptr<OpenList<StateID> > create_state_open_list() = 0;
};

#endif
