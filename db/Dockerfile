FROM mysql:latest
MAINTAINER Tomas Nozicka, Seznam.cz

WORKDIR /
ENV MYSQL_ROOT_PASSWORD password

RUN apt-get update
RUN apt-get install -y supervisor

RUN sed -i -e 's/exec "$@"/exec \/usr\/bin\/supervisord -n/' /entrypoint.sh

ADD test_database.sql /docker-entrypoint-initdb.d/
ADD supervisor.conf /etc/supervisor/conf.d/mysqld.conf
