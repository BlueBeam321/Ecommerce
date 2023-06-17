
#ifndef __CHashTable_VOD_H__
#define __CHashTable_VOD_H__


    class CHashTable
    {
    public:
        virtual ~CHashTable();

        static CHashTable* create(int keyType);

        virtual void* Add(char const* key, void* value) = 0;
        virtual bool Remove(char const* key) = 0;
        // Returns 0 if not found
        virtual void* Lookup(char const* key) const = 0;
        virtual bool IsEmpty() const = 0;

        // Used to iterate through the members of the table:
        class Iterator
        {
        public:
            // The following must be implemented by a particular
            // implementation (subclass):
            static Iterator* create(CHashTable& hashTable);

            virtual ~Iterator();

            virtual void* next(char const*& key) = 0;// returns 0 if none

        protected:
            Iterator(); // abstract base class
        };

        // A shortcut that can be used to successively remove each of
        // the entries in the table (e.g., so that their values can be
        // deleted, if they happen to be pointers to allocated memory).
        void* RemoveNext();

    protected:
        CHashTable(); // abstract base class
    };

    int const STRING_HASH_KEYS = 0;
    int const ONE_WORD_HASH_KEYS = 1;

#endif // __CHashTable_VOD_H__