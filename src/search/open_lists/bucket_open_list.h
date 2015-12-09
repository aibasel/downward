#ifndef OPEN_LISTS_BUCKET_OPEN_LIST_H
#define OPEN_LISTS_BUCKET_OPEN_LIST_H

#include "open_list_factory.h"

#include "../option_parser_util.h"


/*
  Bucket-based implementation of an open list.

  Nodes with identical heuristic value are expanded in FIFO order.
*/

class BucketOpenListFactory : public OpenListFactory {
    Options options;
public:
    explicit BucketOpenListFactory(const Options &options);
    virtual ~BucketOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};

#endif
