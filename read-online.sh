#!/bin/bash 

FILES=$(echo /data/netflow/RT/*-eth*/nfcapd.current.*)
ARGS="--loop-read -o shm "

for f in $FILES; do 

	ARGS="$ARGS -r $f"
done

../libnf/bin/nfdumpp $ARGS


