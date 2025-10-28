#!/bin/bash

BUILD_DIR=$1

REGRESSION_N_RANKS=4
REGRESSION_PARAMETERS="-N 100 --high-density 0.85 --low-density 0.3 -s 48.0 -T 295.0 --seed 1337 -i 1 -e 1"

#####Snippet from:   http://stackoverflow.com/questions/59895/
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#####end snippet

testing_tmp_dir=$(mktemp -d /tmp/regress.XXXXXXXXX)

echo "TEMP Directory ${testing_tmp_dir}"

#mpirun -n ${REGRESSION_N_RANKS} ${BUILD_DIR}/bin/part1 ${REGRESSION_PARAMETERS} -o ${testing_tmp_dir}/mpioutput
${BUILD_DIR}/bin/part3 --ranks ${REGRESSION_N_RANKS} ${REGRESSION_PARAMETERS} -o ${testing_tmp_dir}/charmoutput


#grep regex to get number of particles in frame only:
#
#grep -e "^[0-9]\\+$"
#

for i in $(seq 0 3)
do
	${BUILD_DIR}/bin/xyz_sorter -i ${testing_tmp_dir}/charmoutput_${i}.xyz -o ${testing_tmp_dir}/charm_sorted_${i}.xyz
	${BUILD_DIR}/bin/xyz_sorter -i ${BUILD_DIR}/data/regression_${i}.xyz -o ${testing_tmp_dir}/reference_sorted_${i}.xyz

	diff -q ${testing_tmp_dir}/reference_sorted_${i}.xyz ${testing_tmp_dir}/charm_sorted_${i}.xyz

	if [ "$?" != "0" ]
	then
		echo "Regression FAIL."
		(>&2 echo "Regression FAIL.")
		exit 1
	fi
done

echo "Regression SUCCESS."
(>&2 echo "Regression SUCCESS.")
exit 0
