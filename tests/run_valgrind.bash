#!/bin/bash
#SBATCH --time=01:20:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name mp3-experiments
#SBATCH -p cs
#PBS -S /projects/cs/cs484sp23/sing_shell.sh

export MAXIMUM_DECOMPOSITION=256
export USE_DECOMPOSITION=100
export TOTAL_CPUS=$(( ${SLURM_JOB_NUM_NODES} * ${PBS_NUM_PPN} ))
export PARTICLE_RUNSPEC="-s 48 --high-density 0.05 --low-density 0.05 --exchange 5 -N 50 -i 1"
export CHARM_BALANCER_OPTIONS="+balancer GreedyRefine +isomalloc_sync --process-migrate 10"

export VALGRIND_OPTIONS=" --leak-check=yes --leak-check=full --show-leak-kinds=all"

## If not started with PBS, figure out where we are relative to the build directory
#####Snippet from:   http://stackoverflow.com/questions/59895/
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#####end snippet
#IF SLURM_SUBMIT_DIR is not set, we are not running in PBS, choose directory relative to script.
SLURM_SUBMIT_DIR=${SLURM_SUBMIT_DIR:-${SCRIPT_DIR}/..}

#moves to the directory the user was in when they ran qsub
cd ${SLURM_SUBMIT_DIR} #assumed to be the source tree

mpirun --launcher fork -n 4 valgrind --log-file=VG.part1.out.%p ${VALGRIND_OPTIONS} ./build/bin/part1 ${PARTICLE_RUNSPEC} 2>&1 > ./writeup/valgrind_part1.txt

#there are issues between Charm++ and Valgrind, if your part1 program works, it should work under aMPI, but it may fail under valgrind.
#mpirun --launcher fork -n 4 valgrind --log-file=VG.part2.out.%p ${VALGRIND_OPTIONS} ./build/bin/part2 +vp 4 ${PARTICLE_RUNSPEC} ${CHARM_BALANCER_OPTIONS} 2>&1 > ./writeup/valgrind_part2.txt
