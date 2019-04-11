################################
# OWT WebRTC Conference Sample

FROM owt-server:latest

ENV CLIENT_SAMPLE_PATH=/owt-client-javascript/dist/samples/conference

WORKDIR /owt-server

RUN ./scripts/build.js -t all --check
RUN ./scripts/pack.js -t all --install-module --sample-path $CLIENT_SAMPLE_PATH .

RUN ./dist/video_agent/install_openh264.sh
