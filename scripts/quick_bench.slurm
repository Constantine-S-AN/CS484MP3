#!/bin/bash
#SBATCH -J mp3-quickbench
#SBATCH -p eng-instruction
#SBATCH -A 25fa-cs484pp4-eng
#SBATCH -N 2
#SBATCH -n 36
#SBATCH --time=00:15:00
#SBATCH -o quick_bench.%j.out
#SBATCH -e quick_bench.%j.err

set -euo pipefail

cd "$SLURM_SUBMIT_DIR"

echo "Running quick MPI/AMPI timings"

echo "=== MPI n=4 ==="
/projects/illinois/eng/shared/shared/cs484/sing_exec.sh mpirun -n 4  build/bin/part1 -N 1000 -i 10

echo "=== MPI n=36 ==="
/projects/illinois/eng/shared/shared/cs484/sing_exec.sh mpirun -n 36 build/bin/part1 -N 1000 -i 10

echo "=== AMPI +vp 4 ==="
/projects/illinois/eng/shared/shared/cs484/sing_exec.sh build/bin/part2 +vp 4  -N 1000 -i 10 +balancer GreedyRefine +isomalloc_sync

echo "=== AMPI +vp 36 ==="
/projects/illinois/eng/shared/shared/cs484/sing_exec.sh build/bin/part2 +vp 36 -N 1000 -i 10 +balancer GreedyRefine +isomalloc_sync
