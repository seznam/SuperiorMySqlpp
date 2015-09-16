FROM debian:jessie
MAINTAINER Tomas Nozicka, Seznam.cz

ENV CONCURRENCY 32

RUN echo "deb http://http.debian.net/debian jessie-backports main" >> /etc/apt/sources.list.d/jessie_backports.list
RUN apt-get update 
RUN apt-get install -y make dpkg-dev devscripts equivs

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]