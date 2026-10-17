
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QObject>
#include <QList>

// qt-plus
#include "Macros.h"
#include "File/CShardFileStore.h"

// Application
#include "../objects/CCommit.h"
#include "../objects/CFile.h"
#include "../objects/CDatabase.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

//-------------------------------------------------------------------------------------------------

class AZE_SHARED_EXPORT CCommitTreeList : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CDatabase*, p, database, Database)
    Q_FAST_PROPERTY(bool, b, silent, Silent)
    Q_FAST_PROPERTY(bool, b, debug, Debug)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CCommitTreeList(CDatabase* pDatabase, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false, bool bTextCache = false);

    //!
    ~CCommitTreeList() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    bool containsCommit(const QString& sCommitId) const;

    //!
    void registerCommitParents(const QString& sCommitId, const QStringList& lParentIds);

    //!
    void removeCommit(const QString& sCommitId);

    //!
    QList<CCommit*> commitParentList(const CCommit* pCommit, QObject* parent);

    //!
    QList<CCommit*> commitParentList(const QString& sCommitId, QObject* parent);

    //!
    QStringList commitParentIds(const QString& sCommitId);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CShardFileStore*    m_pShardStore;
};

}
