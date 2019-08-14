WIDGET_HEADERS += SymView.h Decider.h

HEADERS += SymView.h Decider.h
SOURCES += SymView.cpp Decider.cpp

headers.path    = /usr/include/SuWidgets
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers
