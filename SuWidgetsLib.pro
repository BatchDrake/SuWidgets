CONFIG      += debug_and_release
TARGET      = $$qtLibraryTarget(suwidgets)
TEMPLATE    = lib
VERSION     = 0.3.0

darwin: QMAKE_SONAME_PREFIX = @rpath
darwin: QMAKE_LFLAGS += -Wl,-export_dynamic

QT += widgets opengl
win32:QT += gui
win32:LIBS += -lopengl32

isEmpty(PREFIX) {
  SUWIDGETS_INSTALL_LIBS=$$[QT_INSTALL_LIBS]
  SUWIDGETS_INSTALL_HEADERS=$$[QT_INSTALL_HEADERS]/SuWidgets
} else {
  SUWIDGETS_INSTALL_LIBS=$$PREFIX/lib
  SUWIDGETS_INSTALL_HEADERS=$$PREFIX/include/SuWidgets
}

!isEmpty(PKGVERSION) {
  QMAKE_CXXFLAGS += "-DSUWIDGETS_PKGVERSION='\""$$PKGVERSION"\"'"
}


RESOURCES   = icons.qrc
LIBS        += -L.

target.path = $$SUWIDGETS_INSTALL_LIBS
INSTALLS    += target

include(SuWidgets.pri)

headers.path    = $$SUWIDGETS_INSTALL_HEADERS
headers.files   += $$WIDGET_HEADERS

INSTALLS       += headers

DISTFILES += \
  polarizationview.pri
