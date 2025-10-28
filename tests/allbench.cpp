#include <random>
#include <chrono>
#include <ctime>
#include <thread>

#include <benchmark/benchmark.h>


#include "gsparallel.hpp"
#include "gsserial.hpp"
#include "gs_utils.hpp"

#include <string>
#include <sstream>

static std::string simple_data1 = "SIZE: 1000 1000\n"
"BOUND: 0 0 499 0 100.0\n"
"BOUND: 500 0 999 0 0.0\n"
"BOUND: 0 1 0 998 0.0\n"
"BOUND: 0 999 499 999 0.0\n"
"BOUND: 500 999 999 999 100.0\n"
"BOUND: 999 1 999 998 0.0\n";

#define BENCHMARK_NUM_EPOCHS 100
#define BENCHMARK_NUM_THREADS 8
#define BENCHMARK_TILE_SIZE 32

static void BM_GS_Serial(benchmark::State& state){
	int N_iters = state.range(0);

	std::stringstream ss1;
	ss1.str(simple_data1);

	FD_Problem the_problem;
	read_boundaries_into_feprob(the_problem, ss1);

	for( auto _ : state ){
		the_problem.init();
		auto start = std::chrono::high_resolution_clock::now();
		double last_change = gauss_seidel_sequential(the_problem, N_iters);
		auto end   = std::chrono::high_resolution_clock::now();

		auto elapsed_seconds = std::chrono::duration_cast<
				std::chrono::duration<double> >(end - start);

		state.SetIterationTime(elapsed_seconds.count());
	}
	state.counters["epochs"] = N_iters;
	state.counters["tile_size"] = the_problem.get_width(); // meaningless in serial code.
	state.counters["num_threads"] = 1; // serial
}
BENCHMARK(BM_GS_Serial)->UseManualTime()->ArgNames({"epochs"})
			->Arg(BENCHMARK_NUM_EPOCHS);


static void BM_GS_Parallel(benchmark::State& state){
	int N_iters = state.range(0);
	int tile_size = state.range(1);
	int num_threads = state.range(2);

	std::stringstream ss1;
	ss1.str(simple_data1);

	FD_Problem the_problem;
	read_boundaries_into_feprob(the_problem, ss1);

	for( auto _ : state ){
		the_problem.init();
		auto start = std::chrono::high_resolution_clock::now();
		double last_change = gauss_seidel_parallel(the_problem, N_iters,
			tile_size, num_threads);
		auto end   = std::chrono::high_resolution_clock::now();

		auto elapsed_seconds = std::chrono::duration_cast<
				std::chrono::duration<double> >(end - start);

		state.SetIterationTime(elapsed_seconds.count());
	}

	state.counters["epochs"] = N_iters;
	state.counters["tile_size"] = tile_size;
	state.counters["num_threads"] = num_threads;
}
BENCHMARK(BM_GS_Parallel)->UseManualTime()->ArgNames({"epochs", "tile_size", "num_threads"})
			->RangeMultiplier(2)
			->Ranges( { {BENCHMARK_NUM_EPOCHS, BENCHMARK_NUM_EPOCHS},
							{8, BENCHMARK_TILE_SIZE},
							{1, BENCHMARK_NUM_THREADS} } );
