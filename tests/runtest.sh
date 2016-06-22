#!/bin/sh

set -e
PREFIX="`hostname`-`id -u`-`echo $$`-"
IMAGE_NAME="${PREFIX}superiormysqlpp-test-mysql"
CONTAINER_NAME="${PREFIX}superiormysqlpp-testdb"
docker build --pull -t ${IMAGE_NAME} ../db
docker rm -f ${CONTAINER_NAME} 2>&1 >/dev/null || true
docker run -d -P --name ${CONTAINER_NAME} ${IMAGE_NAME} #1>/dev/null
MYSQL_HOST=`docker inspect --format='{{.NetworkSettings.IPAddress}}' ${CONTAINER_NAME}`
set +e

./tester ${MYSQL_HOST} 3306 ${CONTAINER_NAME} --reporter=spec
RESULT=$?

docker rm -f ${CONTAINER_NAME}
exit ${RESULT}
