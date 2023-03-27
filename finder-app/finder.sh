#!/bin/bash

if [ "$#" = "0" ]; then
	echo "Warning!!  Function requires 2 parameters.  [finder.sh filesdir searchstr]"
	exit 1
fi

if [ $# = '1' ]; then
	echo "Warning!!  Function requires 1 more parameter. [finder.sh filesdir searchstr]"
	exit 1
fi

if [ ! -d $1 ]; then
	echo "Warning!!  No directory named '$1' was found"
	exit 1
fi

echo "The number of files are $(find $1 -type f | wc -l) and the number of matching lines are $(grep -r $2 $1 | wc -l)" 

exit 0

