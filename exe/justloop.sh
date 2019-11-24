SLEEP_FOR=${SLEEP_FOR:-7}
echo "Starting at $(date)" >> justloop.out;
while [ ! -f justloop.stop ]; do date >> justloop.out; sleep ${SLEEP_FOR}; done;
echo "Finished at $(date)" >> justloop.out;
