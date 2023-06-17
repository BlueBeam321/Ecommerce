
#include "CMedia.h"
#include "CHashTable.h"
#include "CUsageEnvironment.h"


    ////////// CMedium //////////
    CMedium::CMedium(CUsageEnvironment* inEnv)
        : m_Env(inEnv)
    {
        CMediaLookupTable::ourMedia(inEnv)->generateNewName(m_MediumName, mediumNameMaxLen);

        CMediaLookupTable::ourMedia(inEnv)->addNew(this, m_MediumName);
    }

    CMedium::~CMedium()
    {
        CMediaLookupTable::ourMedia(env())->remove(m_MediumName);
    }

    bool
        CMedium::lookupByName(CUsageEnvironment* inEnv, char const* mediumName, CMedium*& resultMedium)
    {
        resultMedium = CMediaLookupTable::ourMedia(inEnv)->lookup(mediumName);
        if (resultMedium == NULL)
        {
            inEnv->setResultMsg(11000102, "CMedium ", mediumName, " does not exist");
            return false;
        }

        return true;
    }

    CUsageEnvironment*
        CMedium::env()
    {
        return m_Env;
    }

    bool
        CMedium::isRTPSource() const
    {
        return false;
    }

    bool
        CMedium::isRTCPInstance() const
    {
        return false;
    }

    bool
        CMedium::isRTSPClient() const
    {
        return false;
    }

    bool
        CMedium::isMediaSession() const
    {
        return false;
    }

    ////////// CMediaLookupTable //////////
    CMediaLookupTable*
        CMediaLookupTable::ourMedia(CUsageEnvironment* inEnv)
    {
        C_Tables* ourTables = C_Tables::getOurTables(inEnv);
        if (ourTables->m_MediaTable == NULL)
        {
            ourTables->m_MediaTable = new CMediaLookupTable(inEnv);
        }
        return (CMediaLookupTable*)(ourTables->m_MediaTable);
    }

    CMedium*
        CMediaLookupTable::lookup(char const* name) const
    {
        return (CMedium*)(m_Table->Lookup(name));
    }

    void
        CMediaLookupTable::addNew(CMedium* medium, char* mediumName)
    {
        m_Table->Add(mediumName, (void*)medium);
    }

    /*******************************************************************************/
    /*
    */
    /*******************************************************************************/
    void
        CMediaLookupTable::remove(char const* name)
    {
        CMedium* medium = lookup(name);
        if (medium != NULL)
        {
            m_Table->Remove(name);
            /*
                if (m_Table->IsEmpty())
                {
                C_Tables* ourTables = C_Tables::getOurTables(m_Env);
                delete this;
                ourTables->m_MediaTable = NULL;
                ourTables->reclaimIfPossible();
                }
                */
            //    delete medium; 20050921
        }
    }

    void
        CMediaLookupTable::generateNewName(char* mediumName, unsigned /*maxLen*/)
    {
        sprintf(mediumName, "SobaksuMedia%d", m_NameGenerator++);
    }

    CMediaLookupTable::CMediaLookupTable(CUsageEnvironment* inEnv)
        : m_Env(inEnv), m_Table(CHashTable::create(STRING_HASH_KEYS)), m_NameGenerator(0)
    {
    }

    CMediaLookupTable::~CMediaLookupTable()
    {
        delete m_Table;
    }

    ////////// C_Tables implementation //////////
    C_Tables*
        C_Tables::getOurTables(CUsageEnvironment* inEnv)
    {
        if (inEnv->m_LiveMediaPriv == NULL)
        {
            inEnv->m_LiveMediaPriv = new C_Tables(inEnv);
        }
        return (C_Tables*)(inEnv->m_LiveMediaPriv);
    }

    /*******************************************************************************/
    /*
    */
    /*******************************************************************************/
    /*
    void
    C_Tables::reclaimIfPossible()
    {
    if (m_MediaTable == NULL)
    {
    m_Env->m_LiveMediaPriv = NULL;
    delete this;
    }
    }
    */
    C_Tables::C_Tables(CUsageEnvironment* inEnv)
        : m_MediaTable(NULL), m_Env(inEnv)
    {
    }

    C_Tables::~C_Tables()
    {
        if (m_MediaTable)
            delete m_MediaTable;
    }
