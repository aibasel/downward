#ifndef OPEN_LIST_FACTORY_H
#define OPEN_LIST_FACTORY_H

#include "open_list.h"

#include <memory>


class OpenListFactory {
public:
    OpenListFactory() = default;
    virtual ~OpenListFactory() = default;

    OpenListFactory(const OpenListFactory &) = delete;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() = 0;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() = 0;

    /*
      The following template receives manual specializations (in the
      cc file) for the open list types we want to support. It is
      intended for templatized callers, e.g. the constructor of
      AlternationOpenList.
    */
    template<typename T>
    std::unique_ptr<OpenList<T>> create_open_list();
};


class TaskIndependentOpenListFactory { //TODO559 remove indirection TI_XYZ_Factory -> XYZ_Factory -> XYZ, instead TI_XYZ -> XYZ
public:
    TaskIndependentOpenListFactory() = default;
    virtual ~TaskIndependentOpenListFactory() = default;

    TaskIndependentOpenListFactory(const TaskIndependentOpenListFactory &) = delete;

    virtual std::unique_ptr<TaskIndependentStateOpenList> create_task_independent_state_open_list() = 0;
    virtual std::unique_ptr<TaskIndependentEdgeOpenList> create_task_independent_edge_open_list() = 0;
};
#endif
