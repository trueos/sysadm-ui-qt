TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release
QT = core
QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib

TARGET=sysadm-client
target.path=/usr/local/bin

INSTALLS += target




SOURCES	+= main.cpp 

include(core/core.pri)



