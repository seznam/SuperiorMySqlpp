FROM docker.dev/dbuilder:debian-wheezy-stable-build
MAINTAINER Seznam.cz a.s.

ENV CONCURRENCY=32

RUN \
apt-get update && apt-get install -y \
    devscripts \
    dpkg-dev \
    equivs \
    make

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]
