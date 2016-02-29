#!/bin/bash

START_PWD=${PWD}

for DIR in dial gauge tzclock
do
	if [ -d ${DIR} ]
	then
		cd ${DIR}
		if [ -e buildrpm.sh ]
		then
			../../project/buildrpm.expect
#			./buildrpm.sh
		fi
		cd ${START_PWD}
	fi
done

