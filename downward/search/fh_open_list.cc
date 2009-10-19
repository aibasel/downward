// #include "open_list_buckets.h"

// HACK! Ignore this if used as a top-level compile target.
#ifdef FH_OPEN_LIST_H

#include <cassert>
using namespace std;

/*
  Bucket-based implementation of an open list.
  Nodes with identical heuristic value are expanded in FIFO order.

  It would be easy to templatize the "State *" and "Operator *"
  datatypes, because these are only used as anonymous data. However,
  there is little point in applying such generalizations before there
  is any need for them.
*/

template<class Entry>
FhOpenList<Entry>::FhOpenList() {
    lowest_fbucket = 0;
    size = 0;
}

template<class Entry>
FhOpenList<Entry>::~FhOpenList() {
}


template<class Entry>
void FhOpenList<Entry>::insert(int fkey, int hkey, const Entry &entry) {
  //  std::deque<Entry>::iterator it;
    assert(fkey >= 0);
    if(fkey >= fbuckets.size()) {
	fbuckets.resize(fkey + 1);
	fsize.resize(fkey + 1, 0);
	lowest_hbucket.resize(fkey + 1, 0);
    }
    else if(fkey < lowest_fbucket)
	lowest_fbucket = fkey;

    assert(hkey >= 0);
    if(hkey >= fbuckets[fkey].size()) {
	fbuckets[fkey].resize(hkey + 1);
    }
    else if(hkey < lowest_hbucket[fkey])
	lowest_hbucket[fkey] = hkey;
    
    // push to back, ie tie breaking in queue-style
    fbuckets[fkey][hkey].push_back(entry);
    fsize[fkey]++;
    size++;
}


template<class Entry>
Entry FhOpenList<Entry>::remove_min() {
    assert(size > 0);
    while(fsize[lowest_fbucket] == 0 )
	lowest_fbucket++;
    while(fbuckets[lowest_fbucket][lowest_hbucket[lowest_fbucket]].empty())
	lowest_hbucket[lowest_fbucket]++;
    size--;
    fsize[lowest_fbucket]--;
    Entry result = fbuckets[lowest_fbucket][lowest_hbucket[lowest_fbucket]].front();
    fbuckets[lowest_fbucket][lowest_hbucket[lowest_fbucket]].pop_front();
    return result;
}

template<class Entry>
bool FhOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void FhOpenList<Entry>::clear() {
    fbuckets.clear();
    lowest_hbucket.clear();
    lowest_fbucket = 0;
    size = 0;
}

template<class Entry>
void FhOpenList<Entry>::print() {

  cout << "lowest fbucket: " << lowest_fbucket << endl;
  for ( int i = 0; i < fbuckets.size(); i++ ) {
    cout << "fbucket " << i << " size: " << fsize[lowest_fbucket] << endl;
    if ( fsize[lowest_fbucket] == 0 ) continue;
    cout << "lowest hbucket: " << lowest_hbucket[i] << endl;
    for ( int j = 0; j < fbuckets[i].size(); j++ ) {
      cout << "hbucket " << j << " size: "  << fbuckets[i][j].size() << endl;
    }
  }

}

#endif
