#-------------------------------------------------
#
# Project created by QtCreator 2017-01-22T18:32:42
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PerfView
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    perfdisplay.cpp \
    perfdata.cpp \
    errordialog.cpp

HEADERS  += mainwindow.h \
    perfdisplay.h \
    perfdata.h \
    errordialog.h

FORMS    += mainwindow.ui \
    errordialog.ui
