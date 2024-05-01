#!/bin/bash

# ./verify_malicious.sh [file]

# verify the file if is too long on single lines or contains words "corrupted, dangerous, risk, attack, malware malicious" or non ASCII characters and print filename or "SAFE" 

if test $# -ne 1
then
    echo 'No arguments were passed.'
    exit 1;
else
    if [ ! -f $1 ];
    then
        echo "Argument is not a file"
        exit 1;
    fi

    # grep on dangerous words
    grep_result=$(cat $1 | grep -P 'corrupted|dangerous|risk|attack|malware|malicious|[[:^ascii:]]' | wc -l)

    if [ "$grep_result" -ne 0 ]; then
        echo `basename $1`
        exit 0
    fi 

    # Count lines, words, and characters in the file
    lines=$(wc -l < "$1")
    words=$(wc -w < "$1")
    characters=$(wc -m < "$1")

    # test if lines are less than 3, words greater than 1000, chars greater than 2000 -> print filename
    if [ "$lines" -lt 3 ] && [ "$words" -gt 1000 ] && [ "$characters" -gt 2000 ]; then
        echo `basename $1`
    else
        echo "SAFE"
    fi
fi
