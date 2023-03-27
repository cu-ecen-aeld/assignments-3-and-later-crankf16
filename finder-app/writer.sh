#!/bin/bash
echo "'$1'"
echo "'$2'"
echo "'$(dirname $1)'"

if [ $# = 0 ]; then
	echo "Warning!!  Function requires 2 parameters.  [writer.sh writefile writestr]"
	exit 1
fi

if [ $# = '1' ]; then
	echo "Warning!!  Function requires 1 more parameter. [writer.sh writefile writestr]"
	exit 1
fi

mkdir -p $(dirname $1)
touch $1

if [ $? != 0 ]; then
	echo "Warning!!  Unable to create and/or write to '$1'"
	exit 1
fi

echo "$2" > $1
echo "File '$1' was created with content '$2'"

exit 0

