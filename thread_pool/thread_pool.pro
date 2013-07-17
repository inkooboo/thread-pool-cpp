#-------------------------------------------------
#
# Project created by QtCreator 2013-07-17T18:32:50
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = thread_pool
CONFIG   += console

CONFIG += C++11

TEMPLATE = app

SOURCES += main.cpp \
    thread_pool.ipp \
    test_header_only.cpp

HEADERS += \
    noncopyable.hpp \
    thread_pool.hpp \
    mpsc_bounded_queue.hpp
