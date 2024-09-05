FROM ubuntu:20.04

WORKDIR /root

ENV all_proxy="http://10.156.168.146:2080"
ENV http_proxy="http://10.156.168.146:2080"
ENV https_proxy="http://10.156.168.146:2080"

RUN apt-get update && apt-get install -y libevent-dev
RUN apt-get update && \
  apt-get install -y iproute2 net-tools vim iputils-ping iptables gdb ethtool && \
  apt-get clean