TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release
QT = core gui widgets svg
QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib

TARGET=sysadm-client
target.path=/usr/local/bin

INSTALLS += target



RESOURCES += ../icons/icons.qrc \
			../styles/styles.qrc
			
SOURCES	+= main.cpp \
		TrayUI.cpp \
		MenuItem.cpp \
		mainUI.cpp \
		C_Manager.cpp
		
HEADERS += globals.h \
		TrayUI.h \
		MenuItem.h \
		PageWidget.h \
		mainUI.h \
		C_Manager.h

FORMS += mainUI.ui \
		C_Manager.ui

include(../Core/core.pri)
include(pages/pages.pri)



