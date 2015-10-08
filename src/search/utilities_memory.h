#ifndef UTILITIES_MEMORY_H
#define UTILITIES_MEMORY_H

/*
  Reserve some memory that we can release and be able to continue
  afterwards, once we hit the memory limit. Due to memory fragmentation
  the planner often doesn't have enough memory to continue if we
  reserve less than 50 MB. On the other hand, amounts greater than 100
  MB seem to be unnecessarily large. Reserving around 75 MB seems to be
  a good compromise.
*/
void reserve_extra_memory_padding(int memory_in_mb = 75);
void release_extra_memory_padding();
bool extra_memory_padding_is_reserved();

#endif
