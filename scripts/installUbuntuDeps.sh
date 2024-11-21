#!/bin/bash -e

install_apt_deps(){
  ${SUDO} apt-get update -y
  ${SUDO} apt-get install git make gcc g++ libglib2.0-dev docbook2x pkg-config -y
  ${SUDO} apt-get install libboost-regex-dev libboost-thread-dev libboost-system-dev liblog4cxx-dev -y
  ${SUDO} apt-get install openjdk-8-jre curl libboost-test-dev nasm yasm gyp libx11-dev libkrb5-dev intel-gpu-tools -y
  ${SUDO} apt-get install m4 autoconf libtool automake cmake libfreetype6-dev libgstreamer-plugins-base1.0-dev tcl -y
  if [ "$GITHUB_ACTIONS" != "true" ]; then
    ${SUDO} apt-get install rabbitmq-server mongodb -y
  else
    if [ -d $LIB_DIR ]; then
      echo "Installing mongodb-org from tar"
      ${SUDO} apt-get install -y libcurl4 openssl liblzma5
      wget -q -P $LIB_DIR https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-ubuntu1804-4.4.6.tgz
      tar -zxvf $LIB_DIR/mongodb-linux-x86_64-ubuntu1804-4.4.6.tgz -C $LIB_DIR
      ${SUDO} ln -s $LIB_DIR/mongodb-linux-x86_64-ubuntu1804-4.4.6/bin/* /usr/local/bin/
    else
      mkdir -p $LIB_DIR
      install_mongodb
    fi
  fi
}

install_gcc_7(){
  ${SUDO} apt-get install gcc-7 g++-7
  ${SUDO} update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 100
  ${SUDO} update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100
  ${SUDO} update-alternatives --set g++ /usr/bin/g++-7
  ${SUDO} update-alternatives --set gcc /usr/bin/gcc-7
}

install_mediadeps_nonfree(){
  install_srt
  install_fdkaac
  install_ffmpeg
}

install_mediadeps(){
  install_srt
  install_ffmpeg
}

cleanup(){
  cleanup_common
}
