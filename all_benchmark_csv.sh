#!/bin/sh

set -e

FOLDER='./test'

# min(file_sizes) >= (max(threads) * max(queue_depth) * max(fs_block_size))
declare -a file_size_list=("2147483648" "1073741824" "536870912")
declare -a num_threads_list=("1" "2" "4")

# queue_depth zero means psync I/O
declare -a queue_depth_list=("0" "1" "2" "4" "8" "16")

declare -a fs_block_sizes=("8388608" "4194304" "1048576" "262144" "65536")


for file_size in "${file_size_list[@]}"
do
    for num_threads in "${num_threads_list[@]}"
    do
        for queue_depth in "${queue_depth_list[@]}"
        do
            for fs_block_size in "${fs_block_sizes[@]}"
            do
                ASYNCHRONOUS="1"

                if [ "$queue_depth" = "0" ]; then
                    ASYNCHRONOUS="0"
                fi

                # every iteration we do either two psync benchmarks or four async ones (libaio and io_uring)

                if [ "$ASYNCHRONOUS" = "1" ]; then
                    #!!! async libaio without direct
                    TIMESTAMP=$(date +%s%3N)

                    SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
                    DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

                    head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE

                    ./fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $num_threads --queue_depth $queue_depth --fs_block_size $fs_block_size --legacy --csv

                    if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
                        # benchmark finished
                    else
                        echo "something went wrong, files are different!"
                        echo "source file size:"
                        wc -c <  $SOURCE_FILE
                        echo "destination file size:"
                        wc -c <  $DESTINATION_FILE
                        exit 1
                    fi

                    #!!! async libaio with direct
                    TIMESTAMP=$(date +%s%3N)

                    SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
                    DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

                    head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE

                    ./fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $num_threads --queue_depth $queue_depth --fs_block_size $fs_block_size --legacy --direct --csv

                    if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
                        # benchmark finished
                    else
                        echo "something went wrong, files are different!"
                        echo "source file size:"
                        wc -c <  $SOURCE_FILE
                        echo "destination file size:"
                        wc -c <  $DESTINATION_FILE
                        exit 1
                    fi

                    #!!! async io_uring without direct
                    TIMESTAMP=$(date +%s%3N)

                    SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
                    DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

                    head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE

                    ./fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $num_threads --queue_depth $queue_depth --fs_block_size $fs_block_size --csv

                    if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
                        # benchmark finished
                    else
                        echo "something went wrong, files are different!"
                        echo "source file size:"
                        wc -c <  $SOURCE_FILE
                        echo "destination file size:"
                        wc -c <  $DESTINATION_FILE
                        exit 1
                    fi

                    #!!! async io_uring with direct
                    TIMESTAMP=$(date +%s%3N)

                    SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
                    DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

                    head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE

                    ./fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $num_threads --queue_depth $queue_depth --fs_block_size $fs_block_size --direct --csv

                    if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
                        # benchmark finished
                    else
                        echo "something went wrong, files are different!"
                        echo "source file size:"
                        wc -c <  $SOURCE_FILE
                        echo "destination file size:"
                        wc -c <  $DESTINATION_FILE
                        exit 1
                    fi
                else
                    #!!! sync without direct
                    TIMESTAMP=$(date +%s%3N)

                    SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
                    DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

                    head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE

                    ./fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $num_threads --queue_depth 0 --fs_block_size $fs_block_size --csv

                    if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
                        # benchmark finished
                    else
                        echo "something went wrong, files are different!"
                        echo "source file size:"
                        wc -c <  $SOURCE_FILE
                        echo "destination file size:"
                        wc -c <  $DESTINATION_FILE
                        exit 1
                    fi

                    #!!! sync with direct
                    TIMESTAMP=$(date +%s%3N)

                    SOURCE_FILE="$FOLDER/fcp_source_$TIMESTAMP"
                    DESTINATION_FILE="$FOLDER/fcp_destination_$TIMESTAMP"

                    head -c $FILE_SIZE < /dev/urandom > $SOURCE_FILE

                    ./fcp --input $SOURCE_FILE --output $DESTINATION_FILE --threads $num_threads --queue_depth 0 --fs_block_size $fs_block_size --direct --csv

                    if cmp --silent -- "$SOURCE_FILE" "$DESTINATION_FILE"; then
                        # benchmark finished
                    else
                        echo "something went wrong, files are different!"
                        echo "source file size:"
                        wc -c <  $SOURCE_FILE
                        echo "destination file size:"
                        wc -c <  $DESTINATION_FILE
                        exit 1
                    fi
                fi
            done
        done
    done
done






