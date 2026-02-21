QT       += core gui widgets
TARGET = app
TEMPLATE = app

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS += -Ofast -march=native -mtune=native -flto
QMAKE_LFLAGS_RELEASE -= -Wl,-O1
QMAKE_LFLAGS += -Ofast -Wl,-Ofast -march=native -mtune=native -flto

CONFIG += c++20

BUILD_DIR = out

OBJECTS_DIR = $$BUILD_DIR/obj

MOC_DIR = $$BUILD_DIR/moc

UI_DIR = $$BUILD_DIR/ui


INCLUDEPATH += $$PWD/inc \
               $$UI_DIR

DEPENDPATH  += $$PWD/inc

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/Renderer.cpp \
    src/RenderWidget.cpp \
    src/Scene.cpp \
    src/Camera.cpp \
    src/Gear.cpp

HEADERS += \
    inc/MainWindow.h \
    inc/Renderer.h \
    inc/RenderWidget.h \
    inc/Scene.h \
    inc/Geom.h \
    inc/Camera.h \
    inc/Gear.h

FORMS += \
    forms/MainWindow.ui
