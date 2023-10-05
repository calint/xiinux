#!/bin/bash
set -e

SERVER=$1
HOST=$2
PORT=$3
NREQS=200000
#NCLIENTS="1 10 100 200 400 800 1000 1500 2000"
#NCLIENTS="1 10 100 200 300 400 500 600 700 800 900 1000"
NCLIENTS="1  10 20  30  40  50  60  70  80  90 100 \
           110 120 130 140 150 160 170 180 190 200 \
           210 220 230 240 250 260 270 280 290 300"

URLS="http://$HOST:$PORT/ \
      http://$HOST:$PORT/qa/coverage/files/small.txt \
      http://$HOST:$PORT/qa/coverage/files/far_side_dog_ok.jpg"

#URLS="http://$HOST:$PORT/qa/files/small.txt \
#      http://$HOST:$PORT/qa/files/far_side_dog_ok.jpg"

for URL in $URLS; do
    echo $URL
    if [[ $URL == */ ]]; then
        FILENAME="homepage"
    else
        FILENAME=$(basename "$URL" | cut -d '.' -f 1)
    fi
    for C in $NCLIENTS; do
        echo -n "$C "
        hey -n $NREQS -c $C $URL > $FILENAME--$C--$SERVER.rep
    done
    echo
done
