QT += core gui widgets network

TARGET   = QwenImageHarness
TEMPLATE = app

CONFIG += c++11

# Keep this list explicit: the project builds with qmake on Windows and Linux.
SOURCES += \
    Main.cpp \
    MainWindow.cpp \
    Backend.cpp \
    BackendFactory.cpp \
    ComfyBackend.cpp \
    CreatePage.cpp \
    GalleryPage.cpp \
    Icons.cpp \
    ImageSlots.cpp \
    Log.cpp \
    MaskCanvas.cpp \
    MockBackend.cpp \
    OutputPane.cpp \
    ParamPanel.cpp \
    Settings.cpp \
    SettingsPage.cpp \
    Shell.cpp \
    TerminalPage.cpp \
    Theme.cpp \
    Viewer.cpp \
    Widgets.cpp

HEADERS += \
    MainWindow.h \
    Backend.h \
    BackendFactory.h \
    ComfyBackend.h \
    CreatePage.h \
    GalleryPage.h \
    Icons.h \
    ImageSlots.h \
    Log.h \
    MaskCanvas.h \
    MockBackend.h \
    OutputPane.h \
    ParamPanel.h \
    Settings.h \
    SettingsPage.h \
    Shell.h \
    TerminalPage.h \
    Theme.h \
    Viewer.h \
    Widgets.h

INCLUDEPATH += .
