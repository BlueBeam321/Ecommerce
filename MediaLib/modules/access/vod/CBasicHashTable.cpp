#include "CBasicHashTable.h"


#include <stdio.h>
#include <string.h>

#define REBUILD_MULTIPLIER 3

    CBasicHashTable::CBasicHashTable(int keyType)
        : m_Buckets(m_StaticBuckets), m_NumBuckets(SMALL_HASH_TABLE_SIZE),
        m_NumEntries(0), m_RebuildSize(SMALL_HASH_TABLE_SIZE*REBUILD_MULTIPLIER),
        m_DownShift(28), m_Mask(0x3), m_KeyType(keyType)
    {
        for (unsigned i = 0; i < SMALL_HASH_TABLE_SIZE; ++i)
        {
            m_StaticBuckets[i] = NULL;
        }
    }

    CBasicHashTable::~CBasicHashTable()
    {
        for (unsigned i = 0; i < m_NumBuckets; ++i)
        {
            CTableEntry* entry;
            while ((entry = m_Buckets[i]) != NULL)
            {
                deleteEntry(i, entry);
            }
        }

        if (m_Buckets != m_StaticBuckets) delete[] m_Buckets;
    }

    void*
        CBasicHashTable::Add(char const* key, void* value)
    {
        void* oldValue;
        unsigned index;

        CTableEntry* entry = lookupKey(key, index);
        if (entry != NULL)
        {
            oldValue = entry->value;
        }
        else
        {
            entry = insertNewEntry(index, key);
            oldValue = NULL;
        }
        entry->value = value;

        if (m_NumEntries >= m_RebuildSize) rebuild();

        return oldValue;
    }

    bool
        CBasicHashTable::Remove(char const* key)
    {
        unsigned index;

        CTableEntry* entry = lookupKey(key, index);
        if (entry == NULL) return false;

        deleteEntry(index, entry);

        return true;
    }

    void*
        CBasicHashTable::Lookup(char const* key) const
    {
        unsigned index;

        CTableEntry* entry = lookupKey(key, index);
        if (entry == NULL) return NULL;

        return entry->value;
    }

    bool
        CBasicHashTable::IsEmpty() const
    {
        return bool(m_NumEntries == 0);
    }

    CBasicHashTable::Iterator::Iterator(CBasicHashTable& table)
        :m_Table(table), m_NextIndex(0), m_NextEntry(NULL)
    {
    }

    void*
        CBasicHashTable::Iterator::next(char const*& key)
    {
        while (m_NextEntry == NULL) {
            if (m_NextIndex >= m_Table.m_NumBuckets) return NULL;

            m_NextEntry = m_Table.m_Buckets[m_NextIndex++];
        }

        CBasicHashTable::CTableEntry* entry = m_NextEntry;
        m_NextEntry = entry->m_Next;

        key = entry->key;
        return entry->value;
    }

    ////////// CHashTable  //////////
    CHashTable* CHashTable::create(int keyType)
    {
        return new CBasicHashTable(keyType);
    }

    CHashTable::Iterator*
        CHashTable::Iterator::create(CHashTable& hashTable)
    {
        return new CBasicHashTable::Iterator((CBasicHashTable&)hashTable);
    }

    CBasicHashTable::CTableEntry*
        CBasicHashTable::lookupKey(char const* key, unsigned& index) const
    {
        CTableEntry* entry;
        index = hashIndexFromKey(key);

        for (entry = m_Buckets[index]; entry != NULL; entry = entry->m_Next)
        {
            if (keyMatches(key, entry->key)) break;
        }

        return entry;
    }

    bool
        CBasicHashTable::keyMatches(char const* key1, char const* key2) const
    {
        if (m_KeyType == STRING_HASH_KEYS)
        {
            return bool(strcmp(key1, key2) == 0);
        }
        else if (m_KeyType == ONE_WORD_HASH_KEYS)
        {
            return bool(key1 == key2);
        }
        else
        {
            unsigned* k1 = (unsigned*)key1;
            unsigned* k2 = (unsigned*)key2;

            for (int i = 0; i < m_KeyType; ++i)
            {
                if (k1[i] != k2[i]) return false;
            }
            return true;
        }
    }

    CBasicHashTable::CTableEntry*
        CBasicHashTable::insertNewEntry(unsigned index, char const* key)
    {
        CTableEntry* entry = new CTableEntry();
        entry->m_Next = m_Buckets[index];
        m_Buckets[index] = entry;

        ++m_NumEntries;
        assignKey(entry, key);

        return entry;
    }

    void
        CBasicHashTable::assignKey(CTableEntry* entry, char const* key)
    {
        if (m_KeyType == STRING_HASH_KEYS)
        {
            entry->key = strdup(key);
        }
        else if (m_KeyType == ONE_WORD_HASH_KEYS)
        {
            entry->key = key;
        }
        else if (m_KeyType > 0)
        {
            entry->key = (char const*)(new unsigned[m_KeyType]);

            memcpy((void*)entry->key, key, m_KeyType*sizeof(unsigned));//RMC
        }
    }

    void
        CBasicHashTable::deleteEntry(unsigned index, CTableEntry* entry)
    {
        CTableEntry** ep = &m_Buckets[index];

        bool foundIt = false;
        while (*ep != NULL)
        {
            if (*ep == entry)
            {
                foundIt = true;
                *ep = entry->m_Next;
                break;
            }
            ep = &((*ep)->m_Next);
        }

        if (!foundIt)
        {
        }

        --m_NumEntries;
        deleteKey(entry);
        delete entry;
    }

    void
        CBasicHashTable::deleteKey(CTableEntry* entry)
    {
        if (m_KeyType == ONE_WORD_HASH_KEYS)
        {
            entry->key = NULL;
        }
        else
        {
            delete[](char*)entry->key;
            entry->key = NULL;
        }
    }

    void
        CBasicHashTable::rebuild()
    {
        unsigned oldSize = m_NumBuckets;
        CTableEntry** oldBuckets = m_Buckets;

        m_NumBuckets *= 4;
        m_Buckets = new CTableEntry*[m_NumBuckets];
        for (unsigned i = 0; i < m_NumBuckets; ++i)
        {
            m_Buckets[i] = NULL;
        }
        m_RebuildSize *= 4;
        m_DownShift -= 2;
        m_Mask = (m_Mask << 2) | 0x3;

        for (CTableEntry** oldChainPtr = oldBuckets; oldSize > 0;
            --oldSize, ++oldChainPtr)
        {
            for (CTableEntry* hPtr = *oldChainPtr; hPtr != NULL;
                hPtr = *oldChainPtr)
            {
                *oldChainPtr = hPtr->m_Next;

                unsigned index = hashIndexFromKey(hPtr->key);

                hPtr->m_Next = m_Buckets[index];
                m_Buckets[index] = hPtr;
            }
        }

        if (oldBuckets != m_StaticBuckets) delete[] oldBuckets;
    }

    unsigned CBasicHashTable::hashIndexFromKey(char const* key) const
    {
        unsigned result = 0;

        if (m_KeyType == STRING_HASH_KEYS)
        {
            while (1)
            {
                char c = *key++;
                if (c == 0) break;
                result += (result << 3) + (unsigned)c;
            }
            result &= m_Mask;
        }
        else if (m_KeyType == ONE_WORD_HASH_KEYS)
        {
            result = randomIndex((unsigned long)key);
        }
        else
        {
            unsigned* k = (unsigned*)key;
            unsigned long sum = 0;
            for (int i = 0; i < m_KeyType; ++i)
            {
                sum += k[i];
            }
            result = randomIndex(sum);
        }

        return result;
    }
