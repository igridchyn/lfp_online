FROM ubuntu:20.04

WORKDIR /usr/src/lfp_online

COPY . .

RUN apt-get -y update && apt-get -y upgrade

RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install make g++ libboost-all-dev libarmadillo-dev libsdl2-dev libsdl2-ttf-dev libmlpack-dev libann-dev libopencv-dev libdc1394-22-dev

RUN cd lfp_online/Debug && make lfp_online
RUN ln -s /usr/src/lfp_online/lfp_online/Debug/lfp_online /usr/bin/lfp_online

#CMD ["pwd"]
#ENTRYPOINT ["tail", "-f", "/dev/null"]
