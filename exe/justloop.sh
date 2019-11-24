#!/bin/sh
SLEEP_FOR=${SLEEP_FOR:-7}
STARTED=$(date)
echo "Starting at ${STARTED} pid=$$"
echo "Starting at ${STARTED} pid=$$" >> justloop.out;
while [ ! -f justloop.stop ]; 
  do date >> justloop.out; sleep ${SLEEP_FOR}; 
done;
FINISHED=$(date)
echo "Finished at ${FINISHED} pid=$$" >> justloop.out;
echo "Finished at ${FINISHED} pid=$$"

