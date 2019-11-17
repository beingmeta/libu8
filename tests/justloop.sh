OUTFILE=${1:-lines}
while [ ! -f ${OUTFILE}.stop ]; do date >> ${OUTFILE}; sleep 3; done
