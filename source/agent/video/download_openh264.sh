# OpenH264 Library Download Script

download_openh264(){
  echo "Download OpenH264..."
  wget -c http://ciscobinary.openh264.org/libopenh264-1.4.0-linux64.so.bz2
  bzip2 -d libopenh264-1.4.0-linux64.so.bz2
  mv libopenh264-1.4.0-linux64.so libopenh264.so
  echo "Download libopenh264.so success."
}

download_openh264
