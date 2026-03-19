#-------------------------------------------------
#
# Project created by QtCreator 2016-04-26T19:28:36
#
#-------------------------------------------------

QT       += core testlib

TARGET    = test
CONFIG   += testcase
CONFIG   -= app_bundle

TEMPLATE = app

unix:!mac{
LIBS += -lX11
}

DEPENDPATH += ../src/OBJ

HEADERS += \
    tst_notedata.h \
    tst_notemodel.h

SOURCES += \
    main.cpp \
    tst_notedata.cpp \
    tst_notemodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"
