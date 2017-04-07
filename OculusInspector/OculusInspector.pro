TEMPLATE = app
TARGET = OculusInspector
QT += core gui widgets gamepad
#CONFIG += console #debug



win32:INCLUDEPATH += C:/assimp-3.3.1/include
win32:LIBPATH += C:/assimp-3.3.1/lib
win32:INCLUDEPATH += C:/ovr_sdk_win_1.13.0/OculusSDK/LibOVR/Include
Release:win32:LIBPATH += C:/ovr_sdk_win_1.13.0/OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2015


win32:LIBS += -lassimp-vc140-mt
win32:LIBS += -lLibOVR

RESOURCES += shaders.qrc
HEADERS += OculusInspector.h 
SOURCES += OculusInspector.cpp

