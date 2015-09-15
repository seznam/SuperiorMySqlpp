FROM fedora:22
MAINTAINER Tomas Nozicka, Seznam.cz

RUN dnf install -y dnf-plugins-core make hostname findutils rpm-build

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]