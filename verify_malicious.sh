#!/bin/bash

# ./verify_malicious.sh [file] [safe_dir]

# verify the file if contains words "corrupted, dangerous, risk, attack, malware malicious" or non ASCII characters and move the file to an safe folder if found

if test $# -ne 2
then
    echo 'No arguments were passed.'
    exit 1;
else
    if [ ! -f $1 ];
    then
        echo "First argument is not a file"
        exit 1;
    fi

    if [ ! -d $2 ];
    then
        echo "Second argument is not a directory"
        exit 1;
    fi

    results=$(cat $1 | grep -P 'corrupted|dangerous|risk|attack|malware|malicious|[[:^ascii:]]' | wc -l)

    # move file to safe folder if found any results
    if test $results -ne 0
    then
        mv "$1" "$2"
    fi
fi
