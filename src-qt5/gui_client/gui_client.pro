LANGUAGE	= C++

!win64{
CONFIG  += qt warn_on release
}else{
CONFIG += qt warn_on release console
}

QT = core gui widgets svg

LIBS += -lssl -lcrypto

TARGET=sysadm-client
target.path=/usr/local/bin

INSTALLS += target

freebsd-*{
  #Install the XDG registration files
  xdg.files=extras/sysadm-client.desktop \
	extras/appcafe.desktop \
	extras/pccontrol.desktop \
	extras/sysadm-updatemanager.desktop \
	extras/sysadm-moused.desktop \
	extras/sysadm-devicemanager.desktop \
	extras/sysadm-servicemanager.desktop \
	extras/sysadm-taskmanager.desktop \
	extras/sysadm-usermanager.desktop
  xdg.path=/usr/local/share/applications
  
  #Install the icon for the XDG files
  xdg_icon.files=extras/sysadm.svg \
	extras/trueos.png \
	extras/appcafe.png
  xdg_icon.path=/usr/local/share/pixmaps
  
  INSTALLS += xdg xdg_icon
}


RESOURCES += icons/icons.qrc \
			styles/styles.qrc
			
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

TRANSLATIONS =  i18n/SysAdm_af.ts \
		i18n/SysAdm_ar.ts \
		i18n/SysAdm_az.ts \
		i18n/SysAdm_bg.ts \
		i18n/SysAdm_bn.ts \
		i18n/SysAdm_bs.ts \
		i18n/SysAdm_ca.ts \
		i18n/SysAdm_cs.ts \
		i18n/SysAdm_cy.ts \
		i18n/SysAdm_da.ts \
		i18n/SysAdm_de.ts \
		i18n/SysAdm_el.ts \
		i18n/SysAdm_es.ts \
		i18n/SysAdm_en.ts \
		i18n/SysAdm_en_US.ts \
		i18n/SysAdm_en_GB.ts \
		i18n/SysAdm_en_AU.ts \
		i18n/SysAdm_en_ZA.ts \
		i18n/SysAdm_et.ts \
		i18n/SysAdm_eu.ts \
		i18n/SysAdm_fa.ts \
		i18n/SysAdm_fi.ts \
		i18n/SysAdm_fr.ts \
		i18n/SysAdm_fr_CA.ts \
		i18n/SysAdm_fur.ts \
		i18n/SysAdm_gl.ts \
		i18n/SysAdm_he.ts \
		i18n/SysAdm_hi.ts \
		i18n/SysAdm_hr.ts \
		i18n/SysAdm_hu.ts \
		i18n/SysAdm_id.ts \
		i18n/SysAdm_is.ts \
		i18n/SysAdm_it.ts \
		i18n/SysAdm_ja.ts \
		i18n/SysAdm_ka.ts \
		i18n/SysAdm_ko.ts \
		i18n/SysAdm_lt.ts \
		i18n/SysAdm_lv.ts \
		i18n/SysAdm_mk.ts \
		i18n/SysAdm_mn.ts \
		i18n/SysAdm_ms.ts \
		i18n/SysAdm_mt.ts \
		i18n/SysAdm_nb.ts \
		i18n/SysAdm_ne.ts \
		i18n/SysAdm_nl.ts \
		i18n/SysAdm_pa.ts \
		i18n/SysAdm_pl.ts \
		i18n/SysAdm_pt.ts \
		i18n/SysAdm_pt_BR.ts \
		i18n/SysAdm_ro.ts \
		i18n/SysAdm_ru.ts \
		i18n/SysAdm_sa.ts \
		i18n/SysAdm_sk.ts \
		i18n/SysAdm_sl.ts \
		i18n/SysAdm_sr.ts \
		i18n/SysAdm_sv.ts \
		i18n/SysAdm_sw.ts \
		i18n/SysAdm_ta.ts \
		i18n/SysAdm_tg.ts \
		i18n/SysAdm_th.ts \
		i18n/SysAdm_tr.ts \
		i18n/SysAdm_uk.ts \
		i18n/SysAdm_ur.ts \
		i18n/SysAdm_uz.ts \
		i18n/SysAdm_vi.ts \
		i18n/SysAdm_zh_CN.ts \
		i18n/SysAdm_zh_HK.ts \
		i18n/SysAdm_zh_TW.ts \
		i18n/SysAdm_zu.ts

include(../Core/core.pri)
include(pages/pages.pri)

#Some conf to redirect intermediate stuff in separate dirs
UI_DIR=./.build/ui/
MOC_DIR=./.build/moc/
OBJECTS_DIR=./.build/obj
RCC_DIR=./.build/rcc
QMAKE_DISTCLEAN += -r ./.build
