FROM docker.dev/szn-wheezy:stable-build
MAINTAINER Tomas Nozicka <tomas.nozicka@firma.seznam.cz>

RUN apt-get update 
RUN apt-get install -y make dpkg-dev devscripts equivs

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]