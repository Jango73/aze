
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBranch.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CTag : public CBranch
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CTag(QObject* parent = nullptr);

    //!
    virtual ~CTag() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Sets the tip once, refuses later moves as tags never move
    void setTipCommitId(const QString& sTipCommitId);
};

}
