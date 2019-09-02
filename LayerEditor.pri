WIDGET_HEADERS += $$PWD/LayerEditor.h $$PWD/LayerItem.h

HEADERS += $$PWD/LayerEditor.h \
  $$PWD/LayerEditorModel.h \
  $$PWD/LayerItem.h \
  $$PWD/LayerItemDelegate.h
SOURCES += $$PWD/LayerEditor.cpp \
  $$PWD/LayerEditorModel.cpp \
  $$PWD/LayerItem.cpp \
  $$PWD/LayerItemDelegate.cpp

headers.path    = /usr/include/SuWidgets
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers
