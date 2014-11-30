#!/bin/bash

SCRIPT=`pwd`/$0
FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
ROOT=$PATHNAME/..
BUILD_DIR=$ROOT/build
CURRENT_DIR=`pwd`

LIB_DIR=$BUILD_DIR/libdeps
PREFIX_DIR=$LIB_DIR/build

install_gcc(){
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget ftp://gcc.gnu.org/pub/gcc/infrastructure/{gmp-4.3.2.tar.bz2,mpc-0.8.1.tar.gz,mpfr-2.4.2.tar.bz2}
    wget http://ftp.gnu.org/gnu/gcc/gcc-4.6.3/gcc-4.6.3.tar.bz2
    tar jxf gmp-4.3.2.tar.bz2 && cd gmp-4.3.2/
    ./configure --prefix=${PREFIX_DIR}
    make -s && make install
    cd ..

    tar jxf mpfr-2.4.2.tar.bz2 ;cd mpfr-2.4.2/
    ./configure --prefix=${PREFIX_DIR} -with-gmp=${PREFIX_DIR}
    make -s && make install
    cd ..

    tar xzf mpc-0.8.1.tar.gz ;cd mpc-0.8.1
    ./configure --prefix=${PREFIX_DIR} -with-mpfr=${PREFIX_DIR} -with-gmp=${PREFIX_DIR}
    make -s && make install
    cd ..

    tar jxf gcc-4.6.3.tar.bz2 ;cd gcc-4.6.3
    ./configure --prefix=${PREFIX_DIR} -enable-threads=posix -disable-checking -disable-multilib -enable-languages=c,c++ -with-gmp=${PREFIX_DIR} -with-mpfr=${PREFIX_DIR} -with-mpc=${PREFIX_DIR}

    if
    [ $? -eq 0 ];then
    echo "this gcc configure is success"
    else
    echo "this gcc configure is failed"
    fi

    LD_LIBRARY_PATH=${PREFIX_DIR}/lib:$LD_LIBRARY_PATH make -s && make install

    [ $? -eq 0 ] && echo install success

    cd $LIB_DIR
    rm *.tar.bz2 *.tar.gz
  else
    mkdir -p $LIB_DIR
    install_gcc
  fi
}

install_glib2(){
  if [ -d $LIB_DIR ]; then
    # libffi
    cd $LIB_DIR
    wget ftp://sourceware.org/pub/libffi/libffi-3.0.13.tar.gz
    wget http://svn.exactcode.de/t2/trunk/package/develop/libffi/libffi-3.0.13-includedir.patch
    tar zxvf libffi-3.0.13.tar.gz
    cd libffi-3.0.13
    patch -Np1 -i ../libffi-3.0.13-includedir.patch &&
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

  read -p "Installing gcc-4.6... [No/yes]" yn
  case $yn in
    [Yy]* ) install_gcc;;
    [Nn]* ) ;;
    * ) ;;
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

pause() {
  read -p "$*"
}

parse_arguments(){
  while [ "$1" != "" ]; do
    case $1 in
      "--enable-gpl")
        ENABLE_GPL=true
        ;;
      "--cleanup")
        CLEANUP=true
        ;;
    esac
    shift
  done
}

install_mediadeps_nogpl(){
  sudo -E yum install yasm
  if [ -d $LIB_DIR ]; then
    cd $LIB_DIR
    wget https://webm.googlecode.com/files/libvpx-v1.3.0.tar.bz2
    tar -xvf libvpx-v1.3.0.tar.bz2
    cd libvpx-v1.3.0
    ./configure --prefix=/usr --enable-shared
    make -s V=0 && sudo make install
    sudo ldconfig
    cd $LIB_DIR
    wget https://www.libav.org/releases/libav-9.9.tar.gz
    tar -zxvf libav-9.9.tar.gz
    cd libav-9.9
    CFLAGS=-D__STDC_CONSTANT_MACROS ./configure --prefix=$PREFIX_DIR --enable-shared --enable-libvpx
    make -s V=0 && make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_mediadeps_nogpl
  fi
}

install_mediadeps(){
  sudo -E yum install yasm
  if [ -d $LIB_DIR ]; then
    # x264
    cd $LIB_DIR
    git clone git://git.videolan.org/x264.git
    cd x264
    ./configure --enable-static --enable-shared
    make;sudo make install
    sudo ldconfig
    # libvpx
    cd $LIB_DIR
    wget https://webm.googlecode.com/files/libvpx-v1.3.0.tar.bz2
    tar -xvf libvpx-v1.3.0.tar.bz2
    cd libvpx-v1.3.0
    ./configure --enable-shared
    make -s V=0;sudo make install
    #libav
    cd $LIB_DIR
    wget https://www.libav.org/releases/libav-9.9.tar.gz
    tar -zxvf libav-9.9.tar.gz
    cd libav-9.9
    CFLAGS=-D__STDC_CONSTANT_MACROS ./configure --prefix=$PREFIX_DIR --enable-shared --enable-gpl --enable-libvpx --enable-libx264
    make -s V=0;make install
    cd $CURRENT_DIR
  else
    mkdir -p $LIB_DIR
    install_mediadeps
  fi
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
    rm -r openssl*
    rm -r boost*
    rm -r glib*
    rm -r pcre*
    rm -r lib*
    rm *.tar*
    cd $CURRENT_DIR
  fi
}

parse_arguments $*

mkdir -p $PREFIX_DIR

read -p "Add EPEL repository to yum? [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) installRepo;;
  * ) installRepo;;
esac

read -p "Installing deps via yum [Yes/no]" yn
case $yn in
  [Nn]* ) ;;
  [Yy]* ) installYumDeps;;
  * ) installYumDeps;;
esac

if [ "$ENABLE_GPL" = "true" ]; then
  pause "GPL libraries enabled"
  install_mediadeps
else
  pause "No GPL libraries enabled, this disables h264 transcoding, to enable gpl please use the --enable-gpl option"
  install_mediadeps_nogpl
fi

cd $PATHNAME
./installCommonDeps.sh

if [ "$CLEANUP" = "true" ]; then
  echo "Cleaning up..."
  cleanup
fi
