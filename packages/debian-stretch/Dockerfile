FROM debian:stretch
LABEL maintainer="Seznam.cz a.s."

ENV CONCURRENCY=32

RUN apt-get update && apt-get install -y \
    apt-transport-https \
    devscripts \
    dpkg-dev \
    equivs \
    make \
&& echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main" > /etc/apt/sources.list.d/llvm.list \
&& curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - >/dev/null 2>&1 \
&& echo "deb [arch=amd64] https://download.docker.com/linux/debian stretch stable" >> /etc/apt/sources.list.d/docker.list \
&& curl -fsSL https://download.docker.com/linux/debian/gpg | apt-key add - >/dev/null 2>&1 \
&& echo "deb http://repo.mysql.com/apt/debian/ stretch mysql-5.7" > /etc/apt/sources.list.d/mysql.list \
&& curl -fsSL https://repo.mysql.com/RPM-GPG-KEY-mysql | apt-key add - >/dev/null 2>&1 \
# OPTIONALLY use MariaDB 10.2 repository instead of MySQL 5.7 repo
#&& echo "deb [arch=amd64,i386,ppc64el] http://mirror.vpsfree.cz/mariadb/repo/10.2/debian stretch main" > /etc/apt/sources.list.d/mariadb.list \
#&& apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 0xF1656F24C74CD1D8 >/dev/null 2>&1 \
&& apt-get update

VOLUME /dbuild/sources

ADD docker-run.sh /
ENTRYPOINT ["/bin/sh", "-c", "/docker-run.sh"]
