# OpenH264 Library Uninstall Script

this=$(dirname "$0")
this=$(cd "${this}"; pwd)

echo -e "\x1b[32mOpenH264 Video Codec provided by Cisco Systems, Inc.\x1b[0m"

uninstall_openh264() {
  [ -f ${this}/lib/dummyopenh264.so ] && mv ${this}/lib/dummyopenh264.so ${this}/lib/libopenh264.so.3
  sed -i "s/.*openh264Enabled.*/openh264Enabled = false/g" "${this}/agent.toml"
  echo "OpenH264 uninstalled finished."
}

uninstall_openh264