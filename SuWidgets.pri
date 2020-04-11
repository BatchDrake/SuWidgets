
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
  QMAKE_CXXFLAGS += -std=gnu++11
} else {
  CONFIG += c++14
}

include(ctk.pri)
include(lcd.pri)
include(histogram.pri)
include(constellation.pri)
include(qverticallabel.pri)
include(symview.pri)
include(transition.pri)
include(waveform.pri)
include(waterfall.pri)
include(colorchooserbutton.pri)
include(frequencyspinbox.pri)

HEADERS += ThrottleableWidget.h \
    SuWidgetsHelpers.h

SOURCES += ThrottleableWidget.cpp \
    SuWidgetsHelpers.cpp

WIDGET_HEADERS += ThrottleableWidget.h SuWidgetsHelpers.h

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sigutils fftw3
