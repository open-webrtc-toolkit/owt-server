#!/bin/bash
this=`dirname "$0"`
this=`cd "$this"; pwd`
ROOT=`cd "${this}/.."; pwd`

pack_rpm() {
  # Install necessary rpm build tools.
  sudo yum install rpm-build chrpath

  local RPMSPEC=$ROOT/scripts/webrtc-gateway.spec

  if [ ! -f $RPMSPEC ]; then
    echo "No RPM spec file; exiting."
    exit 1
  fi

  local PRODUCT=`grep "Name: " $RPMSPEC | awk '{print $2}'`
  local VERSION=`grep "Version: " $RPMSPEC | awk '{print $2}'`
  local RELEASE=`grep "Release: " $RPMSPEC | awk '{print $2}'`
  local INSTALLDIR=`grep "Prefix: " $RPMSPEC | awk '{print $2}'`

  # Prepare the rpmbuild source tar ball.
  cd $ROOT
  prelink -uf dist/lib/*.so*
  chrpath -d dist/lib/*.so*
  cp -r dist $PRODUCT-$VERSION
  tar zcvf $PRODUCT-$VERSION.tar.gz $PRODUCT-$VERSION > /dev/null
  rm -rf $PRODUCT-$VERSION

  # Create the rpmbuild working directory.
  local RPMBUILDDIR=`rpmbuild -E "%{_topdir}" $RPMSPEC`
  local ARCH=`rpmbuild -E "%{_arch}" $RPMSPEC`
  mkdir -p $RPMBUILDDIR/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

  # Build the RPM package.
  mv $PRODUCT-$VERSION.tar.gz $RPMBUILDDIR/SOURCES
  rpmbuild -bb $RPMSPEC

  # Copy the RPM package back to the project root directory.
  cp $RPMBUILDDIR/RPMS/$ARCH/$PRODUCT-$VERSION-$RELEASE.$ARCH.rpm $ROOT

  echo "==============================="
  echo "Install command:"
  echo "    rpm -ivh $PRODUCT-$VERSION-$RELEASE.$ARCH.rpm"
  echo "Uninstall command:"
  echo "    rpm -e $PRODUCT-$VERSION-$RELEASE.$ARCH"
  echo "The install path is ${INSTALLDIR}/${PRODUCT}"
}

pack_rpm
