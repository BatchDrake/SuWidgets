WIDGET_HEADERS += LCD.h

HEADERS += LCD.h
SOURCES += LCD.cpp

headers.path    = /usr/include/SuWidgets
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers
