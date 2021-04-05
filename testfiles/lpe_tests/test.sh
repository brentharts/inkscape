#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later

if [ "$#" -lt 2 ]; then
    echo "pass the path of the inkscape executable as parameter then the name of the test" $#
    exit 1
fi


LPES_TESTS_EXE=$1
exit_status=0
test=$2
testname=$(basename $test)
filedata=echo "$(cat my_file.txt)"

${LPES_TESTS_EXE} $(filedata) #2>/dev/null >/dev/null

exit $exit_status
