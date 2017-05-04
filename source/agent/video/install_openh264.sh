# OpenH264 Library Install Script

this=$(dirname "$0")
this=$(cd "${this}"; pwd)

echo -e "\x1b[32mOpenH264 Video Codec provided by Cisco Systems, Inc.\x1b[0m"

download_openh264(){
  echo "Download OpenH264..."
  wget -c http://ciscobinary.openh264.org/libopenh264-1.6.0-linux64.3.so.bz2 && \
  bzip2 -d libopenh264-1.6.0-linux64.3.so.bz2 && \
  echo "Download libopenh264-1.6.0-linux64.3.so success."
}

enable_openh264() {
  [ -f ${this}/lib/dummyopenh264.so ] || mv ${this}/lib/libopenh264.so.3 ${this}/lib/dummyopenh264.so
  mv libopenh264-1.6.0-linux64.3.so ${this}/lib/libopenh264.so.3 && \
  sed -i "s/.*openh264Enabled.*/openh264Enabled = true/g" "${this}/agent.toml" && \
  echo "OpenH264 install finished."
}

download_openh264 && enable_openh264
