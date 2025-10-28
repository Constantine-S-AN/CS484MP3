#!/bin/bash

#DO NOT RUN THIS SCRIPT ON ITS OWN!!!

#this is a complimentary script to 'regression_testing.slurm'

#instead submit 'regression_testing.slurm' to Slurm like so:
#	sbatch regression_testing.slurm


SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_PATH=$(dirname ${SCRIPT_PATH})

#check that the script was submit from the right place.
if [  "${ROOT_PATH}" == "${PWD}" ];
then
	echo "We seem to be in the right place [MP1 root directory]."
elif [ "${SCRIPT_PATH}" == "${PWD}" ];
then 
	echo "We seem to be in the right place [MP1 script directory]."
else
	echo "Not submit from the right place! Submit from the root of your repository or from the script directory."
	exit 1
fi

#moves to the mp3 root directory
cd ${ROOT_PATH} #assumed to be the source tree

#creates an out-of-tree build directory for CMake and moves to it
mkdir -p ${ROOT_PATH}/build
pushd ${ROOT_PATH}/build

#build the programs (into the build directory, IE, the current directory)
#then benchmark them. Quit early on failure.
echo "Compiling (if necessary)"
cmake ${ROOT_PATH} && make part3 xyz_sorter || exit 1
popd

bash ${ROOT_PATH}/tests/p3regress.bash ${ROOT_PATH}/build > ${ROOT_PATH}/writeup/regression.txt
exit $?
