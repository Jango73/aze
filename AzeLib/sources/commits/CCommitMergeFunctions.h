
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QObject>

// qt-plus
#include "Macros.h"

// Application
#include "../CError.h"
#include "../CFileHashCache.h"
#include "../objects/CCommit.h"
#include "../objects/CDatabase.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CCommitMergeFunctions : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CDatabase*, p, database, Database)
    Q_FAST_PROPERTY(CFileHashCache*, p, fileHashCache, FileHashCache)
    Q_FAST_PROPERTY(bool, b, silent, Silent)
    Q_FAST_PROPERTY(bool, b, debug, Debug)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CCommitMergeFunctions(CDatabase* pDatabase, CFileHashCache* pFileHashCache, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false);

    //!
    ~CCommitMergeFunctions();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Do a 3-way merge and add (if requested) the modified files to the stage
    CError::EErrorCode threeWayMerge(CCommit* pBaseCommit, CCommit* pFromTipCommit, CCommit* pToTipCommit, bool bAddToStage, CCommit* pStagingCommit);
};

}
