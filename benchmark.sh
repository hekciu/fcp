#!/bin/sh

FOLDER=$1
FILE_SIZE=$2
THREADS=$3
QUEUE_DEPTH=$4

ASYNCHRONOUS="1"

if [ "$QUEUE_DEPTH" = "0" ]; then
	ASYNCHRONOUS="0"
fi


TIMESTAMP=$(date +%s%3N)

SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

echo "creating source file in location $SOURCE_FILE"

BS=512
COUNT=$(expr $FILE_SIZE / $BS)

head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE
# dd if=<(base64 < /dev/urandom) of=$SOURCE_FILE bs=$BS count=$FILE_SIZE

echo "copying file to $DESTINATION_FILE"

if [ "$ASYNCHRONOUS" = "1" ]; then
	fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $THREADS --queue_depth $QUEUE_DEPTH
else
	fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $THREADS
fi

if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
  echo "files contents are identical"
	echo "benchmark finished"
	exit 0
else
  echo "files differ"
	echo "something went wrong, files are different!"
	exit 1
fi

