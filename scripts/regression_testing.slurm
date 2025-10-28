#!/bin/bash
#SBATCH --time=00:15:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name mp3-regression
#SBATCH --output=mp3-regression.%J.out
#SBATCH --error=mp3-regression.%J.err
#SBATCH --account="<YOUR ACCOUNT HERE>"
#SBATCH -p eng-instruction
#PBS -S /projects/illinois/eng/shared/shared/cs484/sing_shell.sh

#this is the file for testing & benchmarking MP3

#it calls a complimentary script 'regression_testing.bash'
#do not try to run 'regression_testing.bash' by itself! submit this file to sbatch instead

#you can either submit your job directly to slurm via the sbatch command like so:
#   sbatch ./regression_testing.slurm

#you can run it either from the mp3 root directory or from mp3/script directory

#the job output files will be mp3-benchmark.<jobid>.out & mp3-benchmark.<jobid>.err, & will be in your mp3 root directory once the job completes

#remember to ADD YOUR ACCOUNT to this script!!!


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

#moves to the directory the user was in when they ran sbatch
cd ${SLURM_SUBMIT_DIR} 

srun -n 1 /projects/illinois/eng/shared/shared/cs484/sing_exec.sh ${SCRIPT_DIR}/regression_testing.bash
