TEMPLATE = app
TARGET = Phong
QT += core gui widgets
#win32:LIBS += -lopengl32
#LIBS += -lGL

RESOURCES += shaders.qrc
HEADERS += Molecules.h
SOURCES += Molecules.cpp

