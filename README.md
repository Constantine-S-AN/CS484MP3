# Building

You can use the provided scripts to build MP3.

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
