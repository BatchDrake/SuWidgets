CONFIG      += debug_and_release
TARGET      = $$qtLibraryTarget(suwidgets)
TEMPLATE    = lib

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}
    
RESOURCES   = icons.qrc
LIBS        += -L.

target.path = $$[QT_INSTALL_LIBS]
INSTALLS    += target

include(SuWidgets.pri)

headers.path    = /usr/include/SuWidgets
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers
