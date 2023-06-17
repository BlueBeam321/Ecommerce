
#ifndef __CMedia_VOD_H__
#define __CMedia_VOD_H__

#include "CUsageEnvironment.h"
#include "CHashTable.h"


#define mediumNameMaxLen 30

    class CMedium
    {
    public:
        CMedium(CUsageEnvironment* inEnv);
        virtual ~CMedium();

        static bool lookupByName(CUsageEnvironment* inEnv, char const* inMediumName, CMedium*& outResultMedium);

        CUsageEnvironment* env();

        char const* name() const { return m_MediumName; }

        virtual bool isRTPSource() const;
        virtual bool isRTCPInstance() const;
        virtual bool isRTSPClient() const;
        virtual bool isMediaSession() const;

    private:
        friend class CMediaLookupTable;

        CUsageEnvironment*	m_Env;
        char					  m_MediumName[mediumNameMaxLen];
    };

    class CMediaLookupTable
    {
    public:
        CMediaLookupTable(CUsageEnvironment* inEnv);
        virtual ~CMediaLookupTable();

        static CMediaLookupTable* ourMedia(CUsageEnvironment* inEnv);

        CMedium* lookup(char const* name) const;

        void addNew(CMedium* medium, char* mediumName);
        void remove(char const* name);

        void generateNewName(char* mediumName, unsigned maxLen);


    private:
        CUsageEnvironment*	m_Env;
        CHashTable*			m_Table;
        unsigned				  m_NameGenerator;
    };

    class C_Tables
    {
    public:
        C_Tables(CUsageEnvironment* inEnv);
        virtual ~C_Tables();

        static C_Tables* getOurTables(CUsageEnvironment* inEnv);

        CMediaLookupTable* m_MediaTable;


    private:
        CUsageEnvironment* m_Env;
    };

#endif // __CMedia_VOD_H__