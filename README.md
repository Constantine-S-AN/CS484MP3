# Building

You can either use the scripts to build MP3, or you can manually build like so:

```
mkdir build && cd build 			\\create a build directory & move inside it
/projects/eng/shared/cs484/run_singularity.sh 	\\run the course Singularity container
cmake .. 					\\run cmake with the mp3 root directory to create the makefile
make 						\\run make to build mp3
exit 						\\exit the Singularity container
```

This will create programs named
	* `build/bin/part1`
	* `build/bin/part2`

# Running the programs

## Testing & benchmarking

We have provided the `scripts mp3/scripts/regression_testing.slurm` and `mp3/scripts/batch_script.slurm` to test & benchmark.
Run them by submitting them to Slurm via sbatch like so:

```
sbatch scripts/regression_testing.slurm
sbatch scripts/batch_script.slurm
```

The file `mp3/scripts/regression_testing.bash` is a complimentary file to `regression_testing.slurm`, and it should be run on its own. If it is run on its own, then it likely won't work.

MP3 is more compute heavy than previous MP assignments, and as such should not be run on the login node. **Please submit to sbatch to test and benchmark MP3 on the compute nodes! ** Furthermore, running `mpirun` on the login node will not work and will instead throw an error.

You can manually run the tests we provide by executing the program
`mpirun -n 4 build/bin/run_tests`
(If you have problems with that, try `mpirun --launcher fork -n 4 BLD/bin/run_tests`)

Try the "--help" option to see various options.


## Custom tests

See the documentation at [Google Test GitHub Project](https://github.com/google/googletest) .


# Visualization and File Format

	https://ovito.org/manual/usage.import.html#usage.import.formats
