# OpenH264 Library Download Script

download_openh264(){
  echo "Download OpenH264..."
  wget -c http://ciscobinary.openh264.org/libopenh264-1.6.0-linux64.3.so.bz2
  bzip2 -d libopenh264-1.6.0-linux64.3.so.bz2
  echo "Download libopenh264-1.6.0-linux64.3.so success."
}

# compile master instead of v1.6 release to support weighted_bipred_idc
# revert below after v1.7 released
compile_openh264(){
  echo "Download OpenH264 Source..."
  git clone https://github.com/cisco/openh264.git
  cd openh264
  git checkout 28db1bce906577c6e70d4b801a96882a33452462
  echo "Download OpenH264 Source success."

  echo "Compile OpenH264..."
  make -j4
  echo "Compile libopenh264.so.3 success."
}

compile_openh264
