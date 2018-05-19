#!/bin/bash

PROJECT_NAME=${1}
DST_DIR=${2}

# Absolute path to this script
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in
BASE_DIR=$(dirname "$SCRIPT")
PROJECT_DIR=$(echo ${PROJECT_NAME} | tr '[A-Z]' '[a-z]')
echo "creating project '${PROJECT_NAME}' in ${2}"

cp -r ${BASE_DIR}/empty_sample ${DST_DIR}/${PROJECT_DIR}

for i in ${DST_DIR}/${PROJECT_DIR}/*.h* ${DST_DIR}/${PROJECT_DIR}/*.c*;
do of=${i/EmptySample/${PROJECT_NAME}};
sed_str=s/EmptySample/${PROJECT_NAME}/g
sed -i "${sed_str}" ${i};
mv ${i} ${of};
done
