#!/bin/bash
#
#  Copyright (C) 2023 Ángel Ruiz Fernández
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, version 3.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program.  If not, see
#  <http://www.gnu.org/licenses/>
#

if [ "$#" != "1" ]; then
    echo $0: Usage:
    echo "         $0 version"
    exit 1
fi

PKG_VERSION=$1
PKG_ARCH=`dpkg --print-architecture`
PKG_DEPENDS='libsigutils (>= 0.3.0-1), libqt5core5a (>= 5.15.2+dfsg-9)'
PKG_DEV_DEPENDS='libsigutils-dev (>= 0.3.0-1), qtcreator (>= 4.14.1-1)'

BINDIR=libsuwidgets_${PKG_VERSION}_${PKG_ARCH}
DEVDIR=libsuwidgets-dev_${PKG_VERSION}_${PKG_ARCH}
############################ Binary package ####################################
# create structure
rm -Rf $BINDIR
mkdir $BINDIR
cd $BINDIR
mkdir -p usr/lib/x86_64-linux-gnu/
mkdir -p DEBIAN/

# create debian thing
rm -f DEBIAN/control
cat <<EOF >>DEBIAN/control
Package: libsuwidgets
Version: $PKG_VERSION
Section: libs
Priority: optional
Architecture: $PKG_ARCH
Depends: $PKG_DEPENDS
Maintainer: arf20 <aruizfernandez05@gmail.com>
Description: Sigutils-related widgets
EOF

# copy files
cp ../libsuwidgets.so* usr/lib/x86_64-linux-gnu/

# set permissions
cd ..
chmod 755 -R $BINDIR/

# build deb
dpkg-deb --build $BINDIR

############################ Development package ###############################
# create structure
rm -Rf $DEVDIR
mkdir $DEVDIR
cd $DEVDIR
mkdir -p usr/include/x86_64-linux-gnu/qt5/SuWidgets/
mkdir -p usr/lib/x86_64-linux-gnu/qt5/plugins/designer/
mkdir -p DEBIAN/

# create debian thing
rm -f DEBIAN/control
cat <<EOF >>DEBIAN/control
Package: libsuwidgets-dev
Version: $PKG_VERSION
Section: libdevel
Priority: optional
Architecture: $PKG_ARCH
Depends: libsuwidgets (= $PKG_VERSION), $PKG_DEV_DEPENDS, pkg-config
Maintainer: arf20 <aruizfernandez05@gmail.com>
Description: Sigutils-related widgets development files
EOF

# copy files
cp ../ColorChooserButton.h ../Constellation.h ../ctkPimpl.h ../ctkRangeSlider.h ../Decider.h ../FrequencySpinBox.h ../GLWaterfall.h ../Histogram.h ../LCD.h ../MultiToolBox.h ../QVerticalLabel.h ../SuWidgetsHelpers.h ../SymView.h ../ThrottleableWidget.h ../TimeSpinBox.h ../Transition.h ../TVDisplay.h ../Version.h ../Waterfall.h ../Waveform.h ../WaveView.h ../WaveViewTree.h ../WFHelpers.h usr/include/x86_64-linux-gnu/qt5/SuWidgets/
cp ../libsuwidgetsplugin.so usr/lib/x86_64-linux-gnu/qt5/plugins/designer/

# set permissions
cd ..
chmod 755 -R $DEVDIR

# build deb
dpkg-deb --build $DEVDIR
