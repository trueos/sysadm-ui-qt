TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release
QT = core
QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib

LIBS += -lssl -lcrypto

macx-*{ 
  INCLUDEPATH += /usr/local/ssl/include
}

TARGET=sysadm
target.path=/usr/local/bin

INSTALLS += target

SOURCES	+= main.cpp session.cpp
		
HEADERS += session.h

include(../Core/core.pri)
