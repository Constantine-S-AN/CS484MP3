#!/bin/bash
#SBATCH --time=00:25:00
#SBATCH --nodes=1
#SBATCH --ntasks=40
#SBATCH --exclusive
#SBATCH --job-name "mp3-benchmark"
#SBATCH --output=mp3-benchmark.%J.out
#SBATCH --error=mp3-benchmark.%J.err       
#SBATCH --account="<YOUR ACCOUNT HERE>"
#SBATCH -p eng-instruction

export MAXIMUM_DECOMPOSITION=256
export USE_DECOMPOSITION=100
export TOTAL_CPUS=$(( ${SLURM_JOB_NUM_NODES} * ${SLURM_CPUS_ON_NODE} ))
export PARTICLE_RUNSPEC="-s 48 --high-density 0.3 --low-density 0.1 --exchange 5 --steps 10000 -i 500"
export CHARM_BALANCER_OPTIONS="+balancer GreedyRefine +isomalloc_sync --process-migrate 1000"

### BEGIN SNIPPET FROM https://stackoverflow.com/questions/56962129/
# check if script is started via SLURM or bash
# if with SLURM: there variable '$SLURM_JOB_ID' will exist
# `if [ -n $SLURM_JOB_ID ]` checks if $SLURM_JOB_ID is not an empty string
if [ -n $SLURM_JOB_ID ];  then
    # check the original location through scontrol and $SLURM_JOB_ID
    SCRIPT_PATH=$(scontrol show job $SLURM_JOBID | awk -F= '/Command=/{print $2}')
	SCRIPT_DIR=$(dirname ${SCRIPT_PATH})
else
    # otherwise: started with bash. Get the real location.
    echo "Not running under SLURM. Exiting." 1>&2
	exit 1
fi
### END SNIPPET

ROOT_DIR=$(dirname ${SCRIPT_DIR})

#moves to the mp3 root directory
cd ${ROOT_DIR} #assumed to be the source tree

echo "Script path: ${SCRIPT_PATH}"
echo "Script dir: ${SCRIPT_DIR}"
echo "Root dir: ${ROOT_DIR}"

#check that the script was submit from the right place.
if [  "${ROOT_DIR}" == "${PWD}" ];
then
	echo "We seem to be in the right place [MP1 root directory]."
elif [ "${SCRIPT_DIR}" == "${PWD}" ];
then 
	echo "We seem to be in the right place [MP1 script directory]."
else
	echo "Not submit from the right place! Submit from the root of your repository or from the script directory."
	exit 1
fi

set -e

#creates an out-of-tree build directory for CMake and moves to it
mkdir -p ${ROOT_DIR}/build
pushd ${ROOT_DIR}/build

#build the programs (into the build directory, IE, the current directory)
#then benchmark them. Quit early on failure.
echo "Compiling"
srun --mpi=pmi2 --ntasks 1 /projects/illinois/eng/shared/shared/cs484/sing_exec.sh cmake ${ROOT_DIR} && srun --mpi=pmi2 --ntasks 1 /projects/illinois/eng/shared/shared/cs484/sing_exec.sh make

#Google test tests
echo "Testing"
srun --mpi=pmi2 --ntasks 4 /projects/illinois/eng/shared/shared/cs484/sing_exec.sh ./bin/run_tests || ( echo "No use benchmarking an incorrect program." ; exit 1 )



OUTFILE_MPI_BENCHMARK="${ROOT_DIR}/writeup/benchmark_mpi-${SLURM_JOB_ID}.txt"
OUTFILE_AMPI_BENCHMARK="${ROOT_DIR}/writeup/benchmark_ampi-${SLURM_JOB_ID}.txt"
OUTFILE_CHARM_BENCHMARK="${ROOT_DIR}/writeup/benchmark_charm-${SLURM_JOB_ID}.txt"


echo "BEGIN _VARIES_"             >> ${OUTFILE_MPI_BENCHMARK}
echo "BEGIN ${USE_DECOMPOSITION}" >> ${OUTFILE_AMPI_BENCHMARK}
echo "BEGIN ${USE_DECOMPOSITION}" >> ${OUTFILE_CHARM_BENCHMARK}

for N_CPUS in 1 4 9 16 25 36
do

	#Individual MPI run
	echo "Doing MPI CPUS = ${N_CPUS} "
	echo "Doing MPI CPUS = ${N_CPUS} " >> ${OUTFILE_MPI_BENCHMARK}
	srun --mpi=pmi2 --cpu-bind=cores --ntasks ${N_CPUS} /projects/illinois/eng/shared/shared/cs484/sing_exec.sh ./bin/part1 ${PARTICLE_RUNSPEC} | grep "Simulation_Duration" >> ${OUTFILE_MPI_BENCHMARK}

	echo "Doing aMPI CPUS = ${N_CPUS} "
	echo "Doing aMPI CPUS = ${N_CPUS} " >> ${OUTFILE_AMPI_BENCHMARK}
	srun --mpi=pmi2 --cpu-bind=cores --ntasks ${N_CPUS} /projects/illinois/eng/shared/shared/cs484/sing_exec.sh ./bin/part2 ${PARTICLE_RUNSPEC} +vp ${USE_DECOMPOSITION} ${CHARM_BALANCER_OPTIONS} | grep "Simulation_Duration" >> ${OUTFILE_AMPI_BENCHMARK}

	echo "Doing CHARM CPUS = ${N_CPUS} "
	echo "Doing CHARM CPUS = ${N_CPUS} " >> ${OUTFILE_CHARM_BENCHMARK}
	srun --mpi=pmi2 --cpu-bind=cores --ntasks ${N_CPUS} /projects/illinois/eng/shared/shared/cs484/sing_exec.sh ./bin/part3 ${PARTICLE_RUNSPEC} --ranks ${USE_DECOMPOSITION} ${CHARM_BALANCER_OPTIONS} | grep "Simulation_Duration" >> ${OUTFILE_CHARM_BENCHMARK}

done
