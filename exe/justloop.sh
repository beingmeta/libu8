#!/bin/sh
OUTFILE=${1:-justloop.out}
SLEEP_FOR=${SLEEP_FOR:-7}
STARTED=$(date)
echo "LOG: Starting at ${STARTED} pid=$$"
echo "Starting at ${STARTED} pid=$$" >> ${OUTFILE};
while [ ! -f justloop.stop ]; 
  do date >> ${OUTFILE}; sleep ${SLEEP_FOR}; 
done;
FINISHED=$(date)
echo "Finished at ${FINISHED} pid=$$" >> ${OUTFILE};
echo "LOG: Finished at ${FINISHED} pid=$$"

