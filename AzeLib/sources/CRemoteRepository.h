
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QDateTime>

// qt-plus
#include "Macros.h"

// Application
#include "CRepository.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CRemoteRepository : public CObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CRepository*, p, localRepository, LocalRepository)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CRemoteRepository(CRepository* pLocalRepository, QObject* parent = nullptr);

    //!
    ~CRemoteRepository();

    //-------------------------------------------------------------------------------------------------
    // Command methods
    //-------------------------------------------------------------------------------------------------

    //! Returns the request for pull
    CXMLNode getPullRequest();

    //! Returns the request for push
    CXMLNode getPushRequest();

    //! Applies a push request to the local repository, returns true on success
    bool applyPushRequest(const CXMLNode& xRequest, QString& sErrorMessage);

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

    //! Stores the object nodes of a push request, returns true on success
    bool storeRequestObjects(const CXMLNode& xRequest, QString& sErrorMessage);

    //! Stores the commit nodes of a push request, returns stored commit ids
    QStringList storeRequestCommits(const CXMLNode& xRequest, QString& sErrorMessage);

    //! Updates a branch from pushed commits, returns true on success
    bool updateBranchFromPush(const QString& sBranchName, const QString& sTipId, const QStringList& lCommitIds, QString& sErrorMessage);
};

}
