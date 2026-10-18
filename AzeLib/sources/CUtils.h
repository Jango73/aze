
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QDateTime>

// qt-plus
#include "Macros.h"
#include "CXMLNode.h"

// Application
#include "CEnums.h"

//-------------------------------------------------------------------------------------------------

#define PRINTABLE_ISO_DATE(a)   (QDateTime::fromString(a, Qt::ISODate).toUTC().toString("yyyy-MM-dd hh:mm"))

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CUtils
{
public:

    //-------------------------------------------------------------------------------------------------
    // Public static methods
    //-------------------------------------------------------------------------------------------------

    //!
    static bool sameIds(const QString& sId1, const QString& sId2);

    //! Creates the path specified in sFileName if it does not exist
    static void ensureFilePathExists(const QString& sFileName);

    //! Moves a file from sSource to sTarget
    static bool moveFile(const QString& sSource, const QString& sTarget);

    //! Returns a relative file name given a root path and an absolute path to a file
    static QString relativeFileName(const QString& sRootPath, const QString& sFileName);

    //! Returns an absolute file name given a root path and a relative path to a file
    static QString absoluteFileName(const QString& sRootPath, const QString& sFileName);

    //! Returns true if the file sRelativeFileName relative to sRootPath exists
    static bool fileExists(const QString& sRootPath, const QString& sRelativeFileName);

    //! Returns binary contents of sFileName
    static QByteArray getBinaryFileContent(const QString& sFileName);

    //! Returns text contents of sFileName
    static QString getTextFileContent(const QString& sFileName);

    //! Writes baContent into sFileName
    static bool putBinaryFileContent(const QString& sFileName, const QByteArray& baContent);

    //! Writes sContent into sFileName
    static bool putTextFileContent(const QString& sFileName, const QString& sContent);

    //! Returns xNode as a dictionary
    static QDictionary dictionaryFromNode(const CXMLNode& xNode);

    //! Returns xDict as an XML node
    static CXMLNode nodeFromDictionary(const QDictionary& xDict);

    //! Returns a string as an UTF8-Base64 string
    static QString serializeBase64(const QString& sText);

    //! Returns an UTF8-Base64 string as a string
    static QString deserializeBase64(const QString& sText);

    //! Returns the SHAxxx for baData
    static QString idFromByteArray(const QByteArray& baData);

    //! Returns the SHAxxx for sText as UTF8
    static QString idFromString(const QString& sText);

    //! Returns the SHAxxx for the contents of sFilename (calls idFromByteArray)
    static QString idFromFileContent(const QString& sFilename);

    //! Returns true if sId has the length of a SHAxxx string
    static bool idValid(const QString& sId);

    //! Returns a string that joins sFilename and sId with '@'
    static QString packFileAndId(const QString& sFilename, const QString& sFilePath);

    //! Unjoins sPack with '@', yielding sFilename and sId
    static void unpackFileAndId(const QString& sPack, QString& sFilename, QString& sId);

    //!
    static std::vector<std::string> textToStdStringVector(const QString& sText);

    //!
    static QString fileDiffHeader(const QString& sLeft, const QString& sRight);

    //! Returns a unified diff of sText1 and sText2 (using diff-match-patch)
    static QString unifiedDiff(const QString& sText1, const QString& sText2);

    //! Applies a unified diff to sText (using diff-match-patch)
    static bool applyUnifiedDiff(const QString& sText, const QString& sDiff, QString& sResult);

    //! Applies a unified diff to sText (using diff-match-patch)
    static bool applyThreeWayMerge(const QString& sBase, const QString& sTitle1, const QString& sText1, const QString& sTitle2, const QString& sText2, QString& sResult);

    //! Returns a printable version of sText
    static QString printableUnifiedDiff(const QString& sText);

    //! Splits a full diff into a file diff list
    //! 1st = file name, 2nd = diff
    static QList<QPair<QString, QString> > splitDiff(const QString& sFullDiff);

    //! Returns sFileName without a trailing sExtension suffix
    //! Keeps dotted names intact, unlike QFileInfo::baseName cutting at the first dot
    static QString fileNameWithoutExtension(const QString& sFileName, const QString& sExtension);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    static float s_fMatchThreshold;
};

}
