#-------------------------------------------------
#
# Project created by QtCreator 2014-08-31T10:39:03
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qnwc
TEMPLATE = app


SOURCES += main.cpp \
    MainWindow.cpp \
    MainWidget.cpp \
    AngleDrawWidget.cpp \
    MyDoubleSpinBox.cpp \
    TCPWorker.cpp

HEADERS  += \
    MainWindow.hpp \
    MainWidget.hpp \
    AngleDrawWidget.hpp \
    MyDoubleSpinBox.hpp \
    TCPWorker.hpp
