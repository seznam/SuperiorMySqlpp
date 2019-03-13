FROM debian:jessie
MAINTAINER Seznam.cz a.s.

ENV CONCURRENCY 32

RUN apt-get update && apt-get install --assume-yes \
    apt-transport-https \
    devscripts \
    dpkg-dev \
    equivs \
    make \
&& echo "deb [arch=amd64] https://download.docker.com/linux/debian jessie stable" >> /etc/apt/sources.list.d/docker.list \
&& curl -fsSL https://download.docker.com/linux/debian/gpg | apt-key add - >/dev/null 2>&1 \
&& apt-get update

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]
