#!/bin/sh

if [ $# != 1 ]; then
  echo >&2 "Expected testing app file name!\nAborting."
  exit 1
fi

set -e
PREFIX="`hostname`-`id --user`-`echo $$`-"
IMAGE_NAME="${PREFIX}superiormysqlpp-test-mysql"
CONTAINER_NAME="${PREFIX}superiormysqlpp-testdb"
docker build --pull --tag=${IMAGE_NAME} db
docker rm --force ${CONTAINER_NAME} >/dev/null 2>&1 || true
docker run --detach --publish-all --name ${CONTAINER_NAME} ${IMAGE_NAME}
MYSQL_HOST=`docker inspect --format='{{.NetworkSettings.IPAddress}}' ${CONTAINER_NAME}`
set +e

LOCAL_TMP_SOCKET="/tmp/${CONTAINER_NAME}.sock"

# Create local socket forwarded to docker's 3307 port where is the traffic forwarded to docker mysql socket
socat "UNIX-LISTEN:$LOCAL_TMP_SOCKET,fork,reuseaddr,unlink-early,mode=777" \
    TCP:$MYSQL_HOST:3307 &

$1 ${MYSQL_HOST} 3306 ${CONTAINER_NAME} ${LOCAL_TMP_SOCKET} --reporter=spec
RESULT=$?

## TODO Add option to run test in manual mode.
## It's needed to run docker in interactive mode to get `read` command working, just add `--interactive` option to docker run in main project's makefile.
#echo
#echo "To connect to test DB:
#echo "  mysql --user=root --password=password --host=$MYSQL_HOST --port=3306"
#echo "To manually test socket forwarding through 3307, try:"
#echo "  mysql --user=root --password=password --host=$MYSQL_HOST --port=3307"
#echo "  mysql --user=root --password=password --socket=$LOCAL_TMP_SOCKET"
#echo "To run some test case again:"
#echo "  ./tester ${MYSQL_HOST} 3306 ${CONTAINER_NAME} ${LOCAL_TMP_SOCKET} --reporter=spec --only=<test-name-substring>"
#echo
#read -p "Press Enter to continue .. (this will kill running docker & socat forwarding)" _
#echo

# Remove local socket forwarding (so kill last job put to background)
kill $!

echo "Removing temporary created docker container ..."
docker rm --force ${CONTAINER_NAME}

if [ ! "$?" -eq "0" ]; then
    echo >&2 "Removing docker container '${CONTAINER_NAME}' failed"
fi

exit ${RESULT}
