CONFIG      += plugin debug_and_release
TARGET      = $$qtLibraryTarget(suwidgetsplugin)
TEMPLATE    = lib

HEADERS     = ConstellationPlugin.h TransitionPlugin.h HistogramPlugin.h LCDPlugin.h WaveformPlugin.h SymViewPlugin.h SuWidgets.h \
    LayerEditorPlugin.h \
    WaterfallPlugin.h ctkRangeSliderPlugin.h \
    ColorChooserButtonPlugin.h \
    EgaViewPlugin.h \
    EgaConsolePlugin.h
SOURCES     = ConstellationPlugin.cpp TransitionPlugin.cpp HistogramPlugin.cpp LCDPlugin.cpp WaveformPlugin.cpp SymViewPlugin.cpp SuWidgets.cpp \
    LayerEditorPlugin.cpp \
    WaterfallPlugin.cpp ctkRangeSliderPlugin.cpp \
    ColorChooserButtonPlugin.cpp \
    EgaConsolePlugin.cpp \
    EgaViewPlugin.cpp

RESOURCES   = icons.qrc
LIBS        += -L.

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += designer
} else {
    CONFIG += designer
}

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS    += target

include(SuWidgets.pri)
