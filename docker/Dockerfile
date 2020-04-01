################################
# OWT WebRTC server
#
# Base image Ubuntu 18.04

FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
ENV MEDIA_SDK_VER=18.4.0
ARG IMG_APP_PATH=/app_data/
ENV APP_PATH=${IMG_APP_PATH}

# Update the system
RUN apt-get update

# Install utils
RUN apt-get install -y ca-certificates git lsb-release mongodb sudo wget

# Keep proxy environment for sudo
RUN bash -c "echo \"Defaults env_keep = \\\"http_proxy https_proxy no_proxy \\\"\" >> /etc/sudoers"
RUN wget https://github.com/Intel-Media-SDK/MediaSDK/releases/download/intel-mediasdk-$MEDIA_SDK_VER/MediaStack.tar.gz -P /tmp
RUN cd /tmp && tar -zxf MediaStack.tar.gz && cd MediaStack && ./install_media.sh
RUN rm /tmp/MediaStack.tar.gz && rm -rf /tmp/MediaStack
ENV MFX_HOME=/opt/intel/mediasdk

RUN git clone https://github.com/open-webrtc-toolkit/owt-server.git
RUN mkdir -p ${APP_PATH}

WORKDIR /owt-server

# This is needed to patch licode
RUN  git config --global user.email "you@example.com" && \
  git config --global user.name "Your Name"

RUN ./scripts/installDepsUnattended.sh

# Set up ENV variables for nodejs and npm so they can be used from Dockerfiles
ENV NVM_DIR /root/.nvm
RUN bash -c 'source $NVM_DIR/nvm.sh && nvm use default && export NODE_VERSION="$(nvm current)"'
ENV NODE_PATH $NVM_DIR/$NODE_VERSION/lib/node_modules
ENV PATH $NVM_DIR/versions/node/$NODE_VERSION/bin:$PATH

RUN npm install -g grunt-cli node-gyp

# Set workpath to where the app should be installed for the next step of the multibuild
WORKDIR ${APP_PATH}
