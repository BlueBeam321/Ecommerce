
#include "CHashTable.h"


    CHashTable::CHashTable()
    {
    }

    CHashTable::~CHashTable()
    {
    }

    CHashTable::Iterator::Iterator()
    {
    }

    CHashTable::Iterator::~Iterator()
    {
    }

    void*
        CHashTable::RemoveNext()
    {
        Iterator* iter = Iterator::create(*this);
        char const* key;
        void* removedValue = iter->next(key);
        if (removedValue != 0) Remove(key);

        delete iter;
        return removedValue;
    }
