
QT += core xml network testlib

TEMPLATE = lib
AZELIB_VERSION = $$cat($$PWD/../VERSION)
DEFINES += "AZELIB_VERSION_STRING=\"\\\"$$AZELIB_VERSION\\\"\""
DEFINES += AZE_LIBRARY

# Dependencies
INCLUDEPATH += $$PWD/../qt-plus/source/cpp
INCLUDEPATH += $$PWD/sources/google

# Sources
include(AzeLib.pri)
include($$PWD/../qt-plus/qt-plus-nolib-web.pri)

# Functions
include($$PWD/../qt-plus/functions.pri)

# Directories
DESTDIR = $$OUT_PWD/bin

# Target
CONFIG(debug, debug|release) {
    TARGET = aze-dbg
} else {
    TARGET = aze
}
