# =============================================
# Core sysadm client class
# This class provides the communications interface with the main sysadm server
# =============================================
# To include in a client project file: "include(<path to this file>/core.pri)"
# =============================================
QT *= core network websockets concurrent

SOURCES += $$PWD/freenas-client.cpp

HEADERS += $$PWD/freenas-client.h
