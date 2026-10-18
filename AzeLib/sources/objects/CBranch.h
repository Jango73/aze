
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QObject>
#include <QDateTime>

// qt-plus
#include "Macros.h"
#include "CXMLNode.h"

// Application
#include "CObject.h"
#include "../CEnums.h"
#include "../CStrings.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CBranch : public CObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // QML properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CEnums::EBranchType, e, type, Type)
    Q_FAST_PROPERTY(QString, s, author, Author)
    Q_FAST_PROPERTY(QString, s, date, Date)
    Q_FAST_PROPERTY(QString, s, rootCommitId, RootCommitId)
    Q_FAST_PROPERTY(QString, s, tipCommitId, TipCommitId)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CBranch(QObject* parent = nullptr);

    //!
    virtual ~CBranch() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CXMLNode toNode() const override;

    //!
    bool isTag() const { return m_eType == CEnums::eTag; }

    //-------------------------------------------------------------------------------------------------
    // Static control methods
    //-------------------------------------------------------------------------------------------------

public:

    //! Creates a branch or a tag from a type value
    static CBranch* create(CEnums::EBranchType eType, QObject* parent);

    //! Returns the branch type for a stored type name, eBranch by default
    static CEnums::EBranchType typeFromString(const QString& sType);

    //!
    static CBranch* fromNode(const CXMLNode& xNode, QObject* parent);

    //!
    static CBranch* fromFile(const QString& sFileName, QObject* parent);
};

}
