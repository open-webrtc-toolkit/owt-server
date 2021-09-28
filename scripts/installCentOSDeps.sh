#!/bin/bash -e

install_glib2(){
  if [ -d $LIB_DIR ]; then
    # libffi
    cd $LIB_DIR
    wget -c ftp://sourceware.org/pub/libffi/libffi-3.0.13.tar.gz
    wget -c http://www.linuxfromscratch.org/patches/downloads/libffi/libffi-3.0.13-includedir-1.patch
    tar zxvf libffi-3.0.13.tar.gz
    cd libffi-3.0.13
    patch -Np1 -i ../libffi-3.0.13-includedir-1.patch &&
    ./configure --prefix=$PREFIX_DIR --disable-static &&
    make -s V=0 && make install
    # prce
    cd $LIB_DIR
    wget -c http://iweb.dl.sourceforge.net/project/pcre/pcre/8.34/pcre-8.34.tar.bz2
    tar jxvf pcre-8.34.tar.bz2
    cd pcre-8.34
    ./configure --prefix=$PREFIX_DIR --enable-utf8 --enable-unicode-properties --disable-static &&
    make -s V=0 && make install
    # glib2
    cd $LIB_DIR
    wget -c http://ftp.gnome.org/pub/gnome/sources/glib/2.32/glib-2.32.4.tar.xz
    tar xvf glib-2.32.4.tar.xz
    cd glib-2.32.4
    LIBFFI_CFLAGS="-I"$PREFIX_DIR"/include" LIBFFI_LIBS="-L"$PREFIX_DIR"/lib64 -lffi"  PCRE_CFLAGS="-I"$PREFIX_DIR"/include"    PCRE_LIBS="-L"$PREFIX_DIR"/lib -lpcre" ./configure --prefix=$PREFIX_DIR &&
    make -s V=0 && make install
  else
    mkdir -p $LIB_DIR
    install_glib2
  fi
}

installYumDeps(){
  ${SUDO} yum groupinstall " Development Tools" "Development Libraries " -y
  ${SUDO} yum install zlib-devel pkgconfig git libcurl-devel.x86_64 curl log4cxx-devel gcc gcc-c++ bzip2 bzip2-devel bzip2-libs python-devel nasm libXext-devel libXfixes-devel libpciaccess-devel libX11-devel yasm cmake -y
  ${SUDO} yum install rabbitmq-server mongodb mongodb-server java-1.7.0-openjdk gyp intel-gpu-tools which libtool freetype-devel -y
  ${SUDO} yum install glib2-devel boost-devel gstreamer1-plugins-base-devel -y
  ${SUDO} yum install centos-release-scl -y
  ${SUDO} yum install devtoolset-7-gcc* -y
  ${SUDO} yum install docbook2X -y
  ${SUDO} yum install libffi-devel -y
}

installRepo(){
  wget -c http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
  wget -c http://rpms.famillecollet.com/enterprise/remi-release-7.rpm
  if ${SUDO} rpm -Uvh remi-release-7*.rpm epel-release-latest-7*.rpm
  then
    echo "Successfully installed remi and epel"
  else
    echo "No need to upgrade or installation went wrong"
  fi
  ${SUDO} sed -i 's/https/http/g' /etc/yum.repos.d/epel.repo
  rm *.rpm
}

install_mediadeps_nonfree(){
  install_fdkaac
  install_ffmpeg
}

install_mediadeps(){
  install_ffmpeg
}

install_glibc(){
  cd $LIB_DIR
  wget http://ftp.gnu.org/gnu/glibc/glibc-2.18.tar.gz 
  tar zxf glibc-2.18.tar.gz
  cd glibc-2.18
  mkdir -p build && cd build/
  ../configure --prefix=/usr
  make -j4 && make install
}

install_python3(){
  if [ -f /usr/bin/python3 ]; then
    echo "python3 already installed." && return 0
  fi
  cd $LIB_DIR
  wget https://www.python.org/ftp/python/3.7.0/Python-3.7.0.tar.xz
  tar xvf Python-3.7.0.tar.xz
  cd Python-3.7.0
  ./configure prefix=/usr/local/python3
  make && make install
  ln -s /usr/local/python3/bin/python3 /usr/bin/python3
  cd ../ && rm -r Python-3.7*
}

cleanup(){
  cd $CURRENT_DIR
  rm *rpm*
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    rm -r x264*
    rm -r boost*
    rm -r glib*
    rm -r pcre*
    rm -r libffi*
    cd $CURRENT_DIR
  fi
  cleanup_common
}
