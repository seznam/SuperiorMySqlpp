#!/bin/sh
#
# Script build container with testing DB for tests.
# Test suite is run against such container when it's ready.
#
# Usage:
#     runtest.sh [DB_BASE_IMAGE] [DB_BASE_IMAGE_VERSION]
#
# Parameters:
#     DB_BASE_IMAGE          - Base docker image with MySQL server (or compatible); Valid options are 'mysql' or 'mariadb' at the moment, the default is 'mysql'.
#     DB_BASE_IMAGE_VERSION  - Image version of base docker image to be used; Default is 'latest'.
#

DB_BASE_IMAGE=${1:-mysql}
DB_BASE_IMAGE_VERSION=${2:-latest}

set -e
PREFIX="`hostname`-`id -u`-`echo $$`-"
IMAGE_NAME="${PREFIX}superiormysqlpp-test-${DB_BASE_IMAGE}"
CONTAINER_NAME="${PREFIX}superiormysqlpp-testdb-${DB_BASE_IMAGE}"

# Use this approach with build arguments when Docker version >= 17.05 is used
#docker build --pull -t ${IMAGE_NAME} --build-arg=DB_IMAGE=${DB_BASE_IMAGE} --build-arg=DB_IMAGE_VERSION=${DB_BASE_IMAGE_VERSION} ../db
docker build --pull -t ${IMAGE_NAME} -f ../db/Dockerfile.${DB_BASE_IMAGE} ../db
docker rm -f ${CONTAINER_NAME} 2>&1 >/dev/null || true
docker run -d -P --name ${CONTAINER_NAME} ${IMAGE_NAME} #1>/dev/null
DB_HOST=`docker inspect --format='{{.NetworkSettings.IPAddress}}' ${CONTAINER_NAME}`
set +e

./tester-${DB_BASE_IMAGE} ${DB_HOST} 3306 ${CONTAINER_NAME} --reporter=spec
RESULT=$?

docker rm -f ${CONTAINER_NAME}
exit ${RESULT}
