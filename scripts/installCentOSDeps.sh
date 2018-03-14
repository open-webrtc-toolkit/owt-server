#!/bin/bash

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

install_boost(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget -c http://iweb.dl.sourceforge.net/project/boost/boost/1.50.0/boost_1_50_0.tar.bz2
    tar xvf boost_1_50_0.tar.bz2
    cd boost_1_50_0
    chmod +x bootstrap.sh
    ./bootstrap.sh
    ./b2 && ./b2 install --prefix=$PREFIX_DIR
  else
    mkdir -p $LIB_DIR
    install_boost
  fi
}

enable_intel_gpu_top() {
  # make intel-gpu-tools accessable by non-root users.
  sudo chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*
  # make the above change effect at every system startup.
  sudo chmod +x /etc/rc.local /etc/rc.d/rc.local
  if sudo grep -RInqs "chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*" /etc/rc.local; then
     echo "intel-gpu-tools has been authorised to non-root users."
  else
     sudo sh -c "echo \"chmod a+rw /sys/devices/pci0000:00/0000:00:02.0/resource*\" >> /etc/rc.local"
  fi
}

installYumDeps(){
  sudo -E yum groupinstall " Development Tools" "Development Libraries " -y
  sudo -E yum install zlib-devel pkgconfig git libcurl-devel.x86_64 curl log4cxx-devel gcc gcc-c++ bzip2 bzip2-devel bzip2-libs python-devel nasm libXext-devel libXfixes-devel libpciaccess-devel libX11-devel yasm cmake -y
  sudo -E yum install rabbitmq-server mongodb mongodb-server java-1.7.0-openjdk gyp intel-gpu-tools SDL2-devel which libtool -y
  enable_intel_gpu_top

  if [[ "$OS" =~ .*6.* ]] # CentOS 6.x
  then
    install_glib2
    install_boost
    install_gcc
  elif [[ "$OS" =~ .*7.* ]] # CentOS 7.x
  then
    sudo -E yum install glib2-devel boost-devel -y
  fi
}

installRepo(){
  if [[ "$OS" =~ .*6.* ]] # CentOS 6.x
  then
    wget -c http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
    wget -c http://rpms.famillecollet.com/enterprise/remi-release-6.rpm
    sudo rpm -Uvh remi-release-6*.rpm epel-release-6*.rpm
    wget -c http://dl.atrpms.net/el6-x86_64/atrpms/stable/atrpms-repo-6-7.el6.x86_64.rpm
    sudo rpm -Uvh atrpms-repo*rpm
    sudo -E wget -c -qO- http://people.redhat.com/bkabrda/scl_python27.repo >> /etc/yum.repos.d/scl.repo
    rm *.rpm
  elif [[ "$OS" =~ .*7.* ]] # CentOS 7.x
  then
    wget -c http://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
    wget -c http://rpms.famillecollet.com/enterprise/remi-release-7.rpm
    sudo rpm -Uvh remi-release-7*.rpm epel-release-latest-7*.rpm
    sudo sed -i 's/https/http/g' /etc/yum.repos.d/epel.repo
    rm *.rpm
  fi
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
  wget -c http://gnu.mirrors.pair.com/gnu/libc/glibc-2.14.tar.xz
  tar xvf glibc-2.14.tar.xz
  cd glibc-2.14
  mkdir build && cd build/
  ../configure --prefix=$PREFIX_DIR
  make -j4 -s && make install
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
