#!/usr/bin/env bash
FAILED_TEST=0

for test in _test*; do
    #./${test} > /dev/null 2>&1
    ./${test}
    if [ $? != 0 ]; then
        FAILED_TEST=1
    fi
done

if [ $FAILED_TEST == 1 ]; then
    exit 1
fi
exit 0