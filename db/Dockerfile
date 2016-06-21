FROM mysql:latest
MAINTAINER Seznam.cz a.s.

WORKDIR /
ENV MYSQL_ROOT_PASSWORD password

RUN \
apt-get update && apt-get install -y \
    supervisor \
&& rm -rf /var/lib/apt/lists/*

RUN sed -i -e 's/exec "$@"/exec \/usr\/bin\/supervisord -n/' /entrypoint.sh

ADD test_database.sql /docker-entrypoint-initdb.d/
ADD supervisor.conf /etc/supervisor/conf.d/mysqld.conf
