
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QList>

// qt-plus
#include "Macros.h"

// Application
#include "objects/CCommit.h"
#include "objects/CFile.h"
#include "objects/CDatabase.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

// A map of commit parent Ids
// Key is commit Id
// Value is the commit's parent Ids
typedef QMap<QString, QStringList> QCommitParentList;

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
    Q_FAST_PROPERTY(bool, b, dirty, Dirty)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CCommitTreeList(CDatabase* pDatabase, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false, bool bTextCache = false);

    //!
    CCommitTreeList(const CCommitTreeList& target);

    //!
    ~CCommitTreeList();

    //-------------------------------------------------------------------------------------------------
    // Operators
    //-------------------------------------------------------------------------------------------------

    //!
    CCommitTreeList& operator = (const CCommitTreeList& target);

    //!
    bool operator == (const CCommitTreeList& target);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    CXMLNode toNode();

    //!
    void fromNode(const CXMLNode& xNode);

    //!
    bool containsCommit(const QString& sCommitId) const;

    //!
    void registerCommitParents(const QString& sCommitId, const QStringList& lParentIds);

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

    QString             m_sFileName;
    QCommitParentList   m_mCommitParentList;
};

}
