TEMPLATE = app
TARGET = Phong
QT += core gui widgets
#win32:LIBS += -lopengl32
#LIBS += -lGL

RESOURCES += shaders.qrc
HEADERS += Molecules.h
SOURCES += Molecules.cpp \
    import.cpp \
    virtualreality.cpp

win32:INCLUDEPATH += C:/assimp-3.3.1/include
win32:LIBPATH += C:/assimp-3.3.1/lib
win32:INCLUDEPATH += C:/OculusSDK/LibOVR/Include
Release:LIBPATH += C:/OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2015
win32:LIBS += -lLibOVR
