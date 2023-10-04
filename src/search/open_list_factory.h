#ifndef OPEN_LIST_FACTORY_H
#define OPEN_LIST_FACTORY_H

#include "component_map.h"
#include "open_list.h"

#include <memory>


class OpenListFactory : public Component {
public:
    OpenListFactory() = default;
    virtual ~OpenListFactory() = default;

    OpenListFactory(const OpenListFactory &) = delete;

    virtual std::shared_ptr<StateOpenList> create_state_open_list() = 0;
    virtual std::shared_ptr<EdgeOpenList> create_edge_open_list() = 0;

    /*
      The following template receives manual specializations (in the
      cc file) for the open list types we want to support. It is
      intended for templatized callers, e.g. the constructor of
      AlternationOpenList.
    */
    template<typename T>
    std::shared_ptr<OpenList<T>> create_open_list();
};

/*
  TODO: issue559 discuss indirection TaskIndependent_XYZ_Factory -> XYZ_Factory -> XYZ, instead TaskIndependent_XYZ -> XYZ
  Remove OpenListFactory completely. It is subsumed by TaskIndependentOpenListFactory that produces
  a TaskIndependentOpenList that produces an OpenList.
*/
class TaskIndependentOpenListFactory : public TaskIndependentComponent {
public:
    TaskIndependentOpenListFactory() = default;
    virtual ~TaskIndependentOpenListFactory() = default;

    TaskIndependentOpenListFactory(const TaskIndependentOpenListFactory &) = delete;

    virtual std::shared_ptr<Component> create_task_specific_Component(
        std::shared_ptr<AbstractTask> &task,
        std::shared_ptr<ComponentMap> &component_map, int depth = -1) override;

    virtual std::shared_ptr<OpenListFactory> create_task_specific_OpenListFactory(std::shared_ptr<AbstractTask> &task, int depth = -1);
    virtual std::shared_ptr<OpenListFactory> create_task_specific_OpenListFactory(std::shared_ptr<AbstractTask> &task, std::shared_ptr<ComponentMap> &component_map, int depth = -1);
};
#endif
