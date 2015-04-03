#!/bin/bash

install_glib2(){
  if [ -d $LIB_DIR ]; then
    # libffi
    cd $LIB_DIR
    wget ftp://sourceware.org/pub/libffi/libffi-3.0.13.tar.gz
    wget http://www.linuxfromscratch.org/patches/downloads/libffi/libffi-3.0.13-includedir-1.patch
    tar zxvf libffi-3.0.13.tar.gz
    cd libffi-3.0.13
    patch -Np1 -i ../libffi-3.0.13-includedir-1.patch &&
    ./configure --prefix=$PREFIX_DIR --disable-static &&
    make -s V=0 && make install
    # prce
    cd $LIB_DIR
    wget http://iweb.dl.sourceforge.net/project/pcre/pcre/8.34/pcre-8.34.tar.bz2
    tar jxvf pcre-8.34.tar.bz2
    cd pcre-8.34
    ./configure --prefix=$PREFIX_DIR --enable-utf8 --enable-unicode-properties --disable-static &&
    make -s V=0 && make install
    # glib2
    cd $LIB_DIR
    wget http://ftp.gnome.org/pub/gnome/sources/glib/2.32/glib-2.32.4.tar.xz
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
    wget http://superb-dca2.dl.sourceforge.net/project/boost/boost/1.50.0/boost_1_50_0.tar.bz2
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

installYumDeps(){
  sudo -E yum groupinstall " Development Tools" "Development Libraries " -y
  sudo -E yum install zlib-devel pkgconfig git subversion libcurl-devel.x86_64 curl log4cxx-devel -y
  sudo -E yum install gcc gcc-c++ bzip2 bzip2-devel bzip2-libs python-devel gyp nodejs npm nasm -y
  sudo -E yum install rabbitmq-server mongodb mongodb-server java-1.7.0-openjdk -y

  read -p "Installing glib2... [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [Yy]* ) install_glib2;;
    * ) install_glib2;;
  esac

  read -p "Installing boost... [Yes/no]" yn
  case $yn in
    [Nn]* ) ;;
    [yY]* ) install_boost;;
    * ) install_boost;;
  esac

  read -p "Install nvm/node? [No/yes] NOTE: This will install a specific Nodejs version managed by nvm. You're highly recommended to just PRESS ENTER to skip this step." yn
  case $yn in
    [Yy]* ) install_node;;
    [Nn]* ) ;;
    * ) ;;
  esac

  read -p "Installing glibc... [No/yes] WARNING: You're highly recommended to just PRESS ENTER to skip this step. Press Y/y ONLY IF YOU EXACTLY KNOW WHAT YOU'RE DOING!" yn
  case $yn in
    [Yy]* ) install_glibc;;
    [nN]* ) ;;
    * ) ;;
  esac
}

installRepo(){
  wget http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
  wget http://rpms.famillecollet.com/enterprise/remi-release-6.rpm
  sudo rpm -Uvh remi-release-6*.rpm epel-release-6*.rpm
  wget http://dl.atrpms.net/el6-x86_64/atrpms/stable/atrpms-repo-6-7.el6.x86_64.rpm
  sudo rpm -Uvh atrpms-repo*rpm
  sudo -E yum --enablerepo=atrpms-testing install cmake -y
  sudo -E wget -qO- http://people.redhat.com/bkabrda/scl_python27.repo >> /etc/yum.repos.d/scl.repo
  rm *.rpm
}

install_mediadeps_nogpl(){
  sudo -E yum install yasm
  install_libvpx
  install_libav
}

install_mediadeps(){
  sudo -E yum install yasm
  install_libvpx
  # x264
  mkdir -p $LIB_DIR
  cd $LIB_DIR
  git clone git://git.videolan.org/x264.git
  cd x264
  ./configure --enable-static --enable-shared
  make;sudo make install
  sudo ldconfig

  install_libav
}

install_glibc(){
  cd $LIB_DIR
  wget http://gnu.mirrors.pair.com/gnu/libc/glibc-2.14.tar.xz
  tar xvf glibc-2.14.tar.xz
  cd glibc-2.14
  mkdir build && cd build/
  ../configure --prefix=$PREFIX_DIR
  make -j4 -s && make install
}

install_node() {
  local NODE_VERSION=
  . ${PATHNAME}/.conf
  echo -e "\x1b[32mInstalling nvm...\x1b[0m"
  NVM_DIR="${HOME}/.nvm"
 
  #install nvm
  bash "${PATHNAME}/install_nvm.sh"
  #install node
  [[ -s "${NVM_DIR}/nvm.sh" ]] && . "${NVM_DIR}/nvm.sh"
  echo -e "\x1b[32mInstalling node ${NODE_VERSION}...\x1b[0m"
  if ! grep -qc 'nvm use' ~/.bash_profile; then
   echo -e 'nvm use '${NODE_VERSION} >> ~/.bash_profile
  fi
  nvm install ${NODE_VERSION}
  nvm use ${NODE_VERSION}
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
