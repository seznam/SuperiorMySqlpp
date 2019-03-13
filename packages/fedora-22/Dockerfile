FROM fedora:22
MAINTAINER Seznam.cz a.s.

ENV CONCURRENCY 32

RUN \
dnf install --assumeyes \
    dnf-plugins-core \
    findutils \
    hostname \
    make \
    rpm-build

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]
