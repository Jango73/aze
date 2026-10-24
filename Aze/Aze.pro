
QT += core xml network testlib
CONFIG += console

TEMPLATE = app
AZE_VERSION = $$cat($$PWD/../VERSION)
DEFINES += "AZE_VERSION_STRING=\"\\\"$$AZE_VERSION\\\"\""

# Dependencies
INCLUDEPATH += $$PWD/../qt-plus/source/cpp
INCLUDEPATH += $$PWD/../AzeLib/sources

DEPENDPATH += $$PWD/../qt-plus
DEPENDPATH += $$PWD/../AzeLib

# Directories
DESTDIR = $$OUT_PWD/bin

# Target binary name
CONFIG(debug, debug|release) {
    TARGET = aze-dbg
} else {
    TARGET = aze
}

# Sources
include(Aze.pri)

# Libraries
CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/../AzeLib/bin -laze-dbg
} else {
    LIBS += -L$$OUT_PWD/../AzeLib/bin -laze
}

# Installation (make install, used by the .deb package)
unix {
    target.path = /usr/bin
    INSTALLS += target
}
