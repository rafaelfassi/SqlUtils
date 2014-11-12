#-------------------------------------------------
#
# Project created by QtCreator 2014-11-08T14:51:16
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtoci
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fsqlrelation.cpp \
    fbasesqltablemodel.cpp \
    sqlutil.cpp

HEADERS  += mainwindow.h \
    sqlutil.h \
    fsqlrelation.h \
    fbasesqltablemodel.h \
    fsqlrelationaldelegate.h

FORMS    += mainwindow.ui
