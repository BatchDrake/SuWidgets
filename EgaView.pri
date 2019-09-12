WIDGET_HEADERS += EgaView.h

HEADERS += EgaView.h \
    $$PWD/cpi.h \
    $$PWD/pearl-m68k.h
SOURCES += EgaView.cpp \
    $$PWD/cpi.c

headers.path    = /usr/include/SuWidgets
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers
