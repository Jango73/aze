
// Application
#include "CTestAze.h"
#include "CTestWorkflow.h"
#include "CTestMerge.h"
#include "CTestDiffPatch.h"
#include "CTestRemote.h"
#include "CTestServe.h"
#include "CTestTag.h"
#include "CTestCache.h"
#include "CTestPrune.h"
#include "CTestMassive.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestAze::CTestAze(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Runs every test suite sequentially.
* \return Combined status, 0 when every suite passes.
*/
/***************************************************************************/
int CTestAze::runAllTests()
{
    int iStatus = 0;

    {
        CTestWorkflow oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestMerge oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestDiffPatch oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestRemote oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestServe oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestTag oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestCache oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestPrune oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    {
        CTestMassive oTest(m_sArgument0);
        iStatus |= QTest::qExec(&oTest);
    }

    clearRepository();

    return iStatus;
}

//-------------------------------------------------------------------------------------------------
