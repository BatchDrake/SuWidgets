CONFIG      += debug_and_release
TARGET      = $$qtLibraryTarget(suwidgets)
TEMPLATE    = lib
VERSION     = 0.1.1

darwin: QMAKE_SONAME_PREFIX = @rpath

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

isEmpty(SUWIDGETS_PREFIX) {
  SUWIDGETS_INSTALL_LIBS=$$[QT_INSTALL_LIBS]
  SUWIDGETS_INSTALL_HEADERS=$$[QT_INSTALL_HEADERS]/SuWidgets
} else {
  SUWIDGETS_INSTALL_LIBS=$$SUWIDGETS_PREFIX/lib
  SUWIDGETS_INSTALL_HEADERS=$$SUWIDGETS_PREFIX/include/SuWidgets
}


RESOURCES   = icons.qrc
LIBS        += -L.

target.path = $$SUWIDGETS_INSTALL_LIBS
INSTALLS    += target

include(SuWidgets.pri)

headers.path    = $$SUWIDGETS_INSTALL_HEADERS
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers
