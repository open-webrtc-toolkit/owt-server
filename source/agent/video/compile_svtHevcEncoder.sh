# SVT HEVC Encoder Library Install Script

detect_OS() {
  lsb_release >/dev/null 2>/dev/null
  if [ $? = 0 ]
  then
    lsb_release -ds | sed 's/^\"//g;s/\"$//g'
  # a bunch of fallbacks if no lsb_release is available
  # first trying /etc/os-release which is provided by systemd
  elif [ -f /etc/os-release ]
  then
    source /etc/os-release
    if [ -n "${PRETTY_NAME}" ]
    then
      printf "${PRETTY_NAME}\n"
    else
      printf "${NAME}"
      [[ -n "${VERSION}" ]] && printf " ${VERSION}"
      printf "\n"
    fi
  # now looking at distro-specific files
  elif [ -f /etc/arch-release ]
  then
    printf "Arch Linux\n"
  elif [ -f /etc/gentoo-release ]
  then
    cat /etc/gentoo-release
  elif [ -f /etc/fedora-release ]
  then
    cat /etc/fedora-release
  elif [ -f /etc/redhat-release ]
  then
    cat /etc/redhat-release
  elif [ -f /etc/debian_version ]
  then
    printf "Debian GNU/Linux " ; cat /etc/debian_version
  else
    printf "Unknown\n"
  fi
}

OS=$(detect_OS | awk '{print tolower($0)}')
this=$(pwd)
DOWNLOAD_DIR="${this}/svt_hevc_encoder"

install_svt_hevc(){
    pushd $DOWNLOAD_DIR
    rm -rf SVT-HEVC
    git clone https://github.com/intel/SVT-HEVC.git

    pushd SVT-HEVC
    git checkout 5b3feae186f83942d8039ba574a97aa2f4789f90

    pushd Build
    pushd linux
    chmod +x build.sh
    ./build.sh debug
    popd
    popd

    popd
    cp -v ./SVT-HEVC/Bin/Debug/libSvtHevcEnc.so.1 ./

    popd
}

if [[ "$OS" =~ .*ubuntu.* ]]
then
    [[ ! -d ${DOWNLOAD_DIR} ]] && mkdir ${DOWNLOAD_DIR};
    pushd ${DOWNLOAD_DIR}
    install_svt_hevc
    popd
fi
