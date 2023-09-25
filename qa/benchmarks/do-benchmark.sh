#!/bin/bash
set -e

SERVER=$1
HOST=$2
PORT=$3

URLS="http://$HOST:$PORT/ \
      http://$HOST:$PORT/qa/files/small.txt \
      http://$HOST:$PORT/qa/files/far_side_dog_ok.jpg"

for URL in $URLS; do
    echo $URL
    if [[ $URL == */ ]]; then
        FILENAME=""
    else
        FILENAME=$(basename "$URL" | cut -d '.' -f 1)
    fi
    NCLIENTS="1 10 100"
    for C in $NCLIENTS; do
        echo -n "$C "
        hey -n 100000 -c $C $URL > $SERVER--$FILENAME--$C.txt
    done
    echo
done