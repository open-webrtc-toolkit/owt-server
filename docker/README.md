This document is used for generating OWT build and all-in-one OWT running environment images.

## Prequisites

  * Docker is setup in your local environment and related proxy is set so that Docker can access to external network (including Google because OWT will download WebRTC related code and patch from Google)

## Build OWT images

1. Run following command to generate Docker image containing OWT source code and dependencies. A dist folder will be generated with all OWT running binaries. You can run OWT service using this image directly in case you want to modify OWT source code and debug.

    ```shell
    docker build --target owt-build -t owt:build \
            --build-arg http_proxy=${HTTP_PROXY} \
            --build-arg https_proxy=${HTTPS_PROXY} \
            .
    ```
The default sample web app will be packed into the OWT package, you can modify Dockerfile to copy your own web app into the image and specify your app path in pack.js:

```
    ./scripts/pack.js -t all --install-module --no-pseudo --app-path /home/owt-client-javascript/dist/samples/conference
```
Or you can refer conference/build_server.sh to see how to use docker swarm to pack your app into image.

2. Run following command to generate OWT running environment Docker image with OWT binaries generated in step 1, not including source code.
    ```shell
    docker build --target owt-run -t owt:run \
    --build-arg http_proxy=${HTTP_PROXY} \
    --build-arg https_proxy=${HTTPS_PROXY} \
    .
    ```
Or you can simply run `build_docker_image.sh` to generate both 2 images.
We provide a `startowt.sh` to launch OWT and expose following parameters to configure:

```
    --rabbit rabbitmq server address if rabbitmq service is deployed on a different device
    --mongo mongodb server address if mongo service is deployed on a different device, like --mongo=xx.xx.xx.xx/owtdb
    --externalip external ip of host device if there are internal and external ip for host device
    --network_interface network interface for external ip of host device if there are external and internal ip for host device
```
Note that `externalip` and `network_interface` should be both set if there are external and internal ip on host device. `startowt.sh` is just a reference, you can customize Dockerfile and script to generate the Docker images for cluster deployment. For example, separate each OWT modules into different Docker images.

2. Launch OWT service

    a) Launch a Docker container with following command to simply try OWT:

    ```shell
    docker run -itd --name=owt --net=host owt:run bash
    docker exec -it owt bash
    cd /home/
    ./startowt.sh    ##default password for user owt is owt
    ```
    Then OWT service should be launched and you can connect to https://localhost:3004 to start your OWT journey.

    b) Launch OWT service with Docker swarm with script ```conference/build_server.sh```
    
    c) Customize different OWT module images and launch OWT service with Kubernetes.
