
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QObject>
#include <QStringList>

// qt-plus
#include "Macros.h"
#include "CXMLNode.h"

// Application
#include "../CError.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------
// Forward declarations
class CRepository;

//-------------------------------------------------------------------------------------------------

class AZE_SHARED_EXPORT CBaseCommand : public QObject
{
    Q_OBJECT

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(bool, b, calledInternally, CalledInternally)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CBaseCommand(CRepository* pRepository);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute();

    //-------------------------------------------------------------------------------------------------
    // Protected payload methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Stores the object nodes carried by a push payload or a pull response
    CError::EErrorCode storePayloadObjects(const CXMLNode& xPayload, QString& sDetail, CError::EErrorCode eInvalidCode);

    //! Stores the commit nodes carried by a push payload or a pull response into lCommitIds
    CError::EErrorCode storePayloadCommits(const CXMLNode& xPayload, QStringList& lCommitIds, QString& sDetail, CError::EErrorCode eInvalidCode);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CRepository*    m_pRepository;
};

}
