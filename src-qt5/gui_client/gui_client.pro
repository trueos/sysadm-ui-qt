TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release
QT = core gui widgets svg
QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib

LIBS += -lssl -lcrypto

macx-*{ 
  INCLUDEPATH += /usr/local/ssl/include
}

TARGET=sysadm-client
target.path=/usr/local/bin

INSTALLS += target

freebsd-*{
  #Install the XDG registration files
  xdg.files=extras/sysadm-client.desktop \
	extras/appcafe.desktop \
	extras/pccontrol.desktop
  xdg.path=/usr/local/share/applications
  
  xdg_auto.files=extras/sysadm-client-auto.desktop
  xdg_auto.path=/usr/local/etc/xdg/autostart
  
  #Install the icon for the XDG files
  xdg_icon.files=extras/sysadm.svg \
	extras/pcbsd.png \
	extras/appcafe.png
  xdg_icon.path=/usr/local/share/pixmaps
  
  INSTALLS += xdg xdg_auto xdg_icon
}


RESOURCES += ../icons/icons.qrc \
			../styles/styles.qrc
			
SOURCES	+= main.cpp \
		TrayUI.cpp \
		MenuItem.cpp \
		mainUI.cpp \
		C_Manager.cpp \
		NewConnectionWizard.cpp \
		SettingsDialog.cpp
		
HEADERS += globals.h \
		TrayUI.h \
		MenuItem.h \
		PageWidget.h \
		mainUI.h \
		C_Manager.h \
		NewConnectionWizard.h \
		SettingsDialog.h \ 
		Messages.h

FORMS += mainUI.ui \
		C_Manager.ui \
		NewConnectionWizard.ui \
		SettingsDialog.ui

include(../Core/core.pri)
include(pages/pages.pri)
