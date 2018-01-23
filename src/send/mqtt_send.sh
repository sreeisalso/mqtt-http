#!/bin/sh
EXEC=`readlink -f ${0}`
PROJECT=`dirname ${EXEC}`
cd ${PROJECT}
node index.js
