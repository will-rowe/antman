#!/usr/bin/env bash
FAILED_TEST=0

for test in _test*; do
    printf " - running unit test (${test})\n"
    #./${test} > /dev/null 2>&1
    ./${test}
    if [ $? != 0 ]; then
        FAILED_TEST=1
        printf "\n...failed\n\n"
    else
        printf "\n...passed\n\n"
    fi
done

if [ $FAILED_TEST == 1 ]; then
    exit 1
fi
exit 0