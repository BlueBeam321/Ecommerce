
#ifndef __CBasicHashTable_VOD_H__
#define __CBasicHashTable_VOD_H__
#include "CHashTable.h"


#define SMALL_HASH_TABLE_SIZE 4

    class CBasicHashTable : public CHashTable
    {
    private:
        class CTableEntry;

    public:
        CBasicHashTable(int keyType);
        virtual ~CBasicHashTable();

        class Iterator; friend class Iterator;
        class Iterator : public CHashTable::Iterator
        {
        public:
            Iterator(CBasicHashTable& table);

        private:
            void* next(char const*& key);

        private:
            CBasicHashTable& m_Table;
            unsigned m_NextIndex;
            CTableEntry* m_NextEntry;
        };

    private:
        virtual void* Add(char const* key, void* value);
        virtual bool Remove(char const* key);
        virtual void* Lookup(char const* key) const;
        virtual bool IsEmpty() const;

    private:
        class CTableEntry
        {
        public:
            CTableEntry* m_Next;
            char const* key;
            void* value;
        };

        CTableEntry* lookupKey(char const* key, unsigned& index) const;
        bool keyMatches(char const* key1, char const* key2) const;

        CTableEntry* insertNewEntry(unsigned index, char const* key);
        void assignKey(CTableEntry* entry, char const* key);

        void deleteEntry(unsigned index, CTableEntry* entry);
        void deleteKey(CTableEntry* entry);

        void rebuild();

        unsigned hashIndexFromKey(char const* key) const;

        unsigned randomIndex(unsigned long i) const
        {
            return (((i * 1103515245) >> m_DownShift) & m_Mask);
        }

    private:
        CTableEntry** m_Buckets;
        CTableEntry*	m_StaticBuckets[SMALL_HASH_TABLE_SIZE];
        unsigned m_NumBuckets, m_NumEntries, m_RebuildSize, m_DownShift, m_Mask;
        int m_KeyType;
    };

#endif // __CBasicHashTable_VOD_H__