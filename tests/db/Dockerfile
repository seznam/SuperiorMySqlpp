FROM mysql:5.7.22
MAINTAINER Seznam.cz a.s.

WORKDIR /
ENV MYSQL_ROOT_PASSWORD password
ADD test_database.sql /docker-entrypoint-initdb.d/

# Tests require us to restart running mysql instance. To this end, we will
# install and configure supervisord.
RUN \
apt-get update && apt-get install -y \
    patch \
    socat \
    supervisor \
&& rm -rf /var/lib/apt/lists/*

# docker-entrypoint.sh from mysql:latest needs to be patched in order to start
# supervisord instead of original mysqld
ADD docker-entrypoint.sh.patch /docker-entrypoint.sh.patch
RUN patch /usr/local/bin/docker-entrypoint.sh /docker-entrypoint.sh.patch

# Since docker-entrypoint.sh drops privileges, we need to point supervisord
# to a directory with required privileges. In this case, /tmp will suffice.
ADD supervisord.conf /etc/supervisor/supervisord.conf

# Enable mysqld socket (for testing purposes)
RUN echo "socket = /var/run/mysqld/mysqld.sock" >> /etc/mysql/conf.d/mysql.cnf

# Tunnel 3307 port to previously defined socket
ADD mysql-socket-tunnel-3307 /usr/sbin/mysql-socket-tunnel-3307
EXPOSE 3307

# Ensure that we are using correct entrypoint due to wild development of
# mysql:latest
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["mysqld"]
