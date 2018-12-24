LANGUAGE	= C++

CONFIG	+= qt warn_on release
QT = core

LIBS += -lssl -lcrypto

TARGET=sysadm
target.path=/usr/local/bin

SOURCES	+= main.cpp session.cpp
HEADERS += session.h

manpage.path=/usr/local/man/man8
manpage.extra="gzip -c sysadm.8 > ${INSTALL_ROOT}/usr/local/man/man8/sysadm.8.gz"

INSTALLS += target manpage

include(../Core/core.pri)
