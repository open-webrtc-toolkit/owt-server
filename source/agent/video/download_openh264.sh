# OpenH264 Library Download Script

download_openh264(){
  echo "Download OpenH264..."
  wget -c http://ciscobinary.openh264.org/libopenh264-1.4.0-linux64.so.bz2
  bzip2 -d libopenh264-1.4.0-linux64.so.bz2
  echo "Download libopenh264-1.4.0-linux64.so success."
}

download_openh264
