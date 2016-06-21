FROM debian:jessie
MAINTAINER Seznam.cz a.s.

ENV CONCURRENCY 32

RUN \
echo "deb http://http.debian.net/debian jessie-backports main" >> /etc/apt/sources.list.d/jessie_backports.list \
&& apt-get update && apt-get install -y \
    devscripts \
    dpkg-dev \
    equivs \
    make

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]