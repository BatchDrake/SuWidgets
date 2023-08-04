equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
  QMAKE_CXXFLAGS += -std=gnu++11
} else {
  CONFIG += c++14
}

greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

include(ctk.pri)
include(lcd.pri)
include(led.pri)
include(histogram.pri)
include(constellation.pri)
include(phaseview.pri)
include(qverticallabel.pri)
include(symview.pri)
include(transition.pri)
include(waveform.pri)
include(waterfall.pri)
include(colorchooserbutton.pri)
include(contextawarespinbox.pri)
include(scispinbox.pri)
include(frequencyspinbox.pri)
include(tvdisplay.pri)
include(timespinbox.pri)
include(multitoolbox.pri)
include(glwaterfall.pri)

HEADERS += ThrottleableWidget.h \
    Version.h \
    SuWidgetsHelpers.h \
    WFHelpers.h

SOURCES += ThrottleableWidget.cpp \
    SuWidgetsHelpers.cpp \
    WFHelpers.cpp

WIDGET_HEADERS += ThrottleableWidget.h SuWidgetsHelpers.h Version.h WFHelpers.h

CONFIG += link_pkgconfig
PKGCONFIG += sigutils fftw3
