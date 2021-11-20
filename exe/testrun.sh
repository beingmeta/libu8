#!/bin/sh
COUNTDOWN=${1:-30}
STARTED=$(date)
echo "LOG: This is stdout for pid $$ started at ${STARTED}" >&1
echo "ERR: This is stderr for pid $$ started at ${STARTED}" >&2

echo U8_STOPFILE=${U8_STOPFILE}
while ( [ -z "${U8_STOPFILE}" ] || [ ! -f "${U8_STOPFILE}" ] ) && 
      [ ${COUNTDOWN} -gt 0 ]; do
    sleep 1;
    COUNTDOWN=$((${COUNTDOWN}-1));
done;
FINISHED=$(date)
echo "LOG: Finished at ${FINISHED} pid=$$" >&1
echo "ERR: Finished at ${FINISHED} pid=$$" >&2
