#!/bin/bash

START_PWD=${PWD}

function doBigText
{
	TEXTCMD=$(which bigWord 2> /dev/null)
	if [ "${TEXTCMD}" != "" ] && [ "${1}" != "" ]
	then
		${TEXTCMD} -c 14 "$1"
	else
		echo $1
	fi
}

doBigText "Dial Packages"

for DIR in libdial gauge tzclock
do
	if [ -d ${DIR} ]
	then
		cd ${DIR}
		if [ -e buildpkg.sh ]
		then
			./buildpkg.sh
		else
			echo "No ${DIR}/buildpkg.sh found."
		fi
		cd ${START_PWD}
	else
		echo "No ${DIR} directory found."
	fi
done

