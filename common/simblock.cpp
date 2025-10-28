#include <cassert>
#include <cstdlib>
#include <stdexcept>

#include "gassim2d.h"
#include "gassim2d_util.h"

#include "simblock.h"



SimulationGrid::SimulationGrid(double xmin, double ymin, double xmax, double ymax, int n_cpus) : SimulationGrid((bounding_box_t){.min = (phys_vector_t){.x=xmin,.y=ymin}, .max = (phys_vector_t){.x=xmax,.y=ymax}}, n_cpus) {};
SimulationGrid::SimulationGrid(phys_vector_t min, phys_vector_t max, int n_cpus) : SimulationGrid((bounding_box_t){.min = min, .max = max}, n_cpus) {}
SimulationGrid::SimulationGrid(bounding_box_t bounds, int n_cpus) : bounds(bounds), sizes((phys_vector_t){.x=bounds.max.x-bounds.min.x, .y = bounds.max.y-bounds.min.y}), n_cpus(n_cpus)
{
	n_cols = static_cast<int>(sqrt(n_cpus));
	n_rows = n_cpus / n_cols;
	//assert(world_width*world_height == world_size);//Ensure that it is square.
	if(n_rows*n_cols != n_cpus){
		throw std::runtime_error("The world is not square, which this code currently expects.");
	}//Ensure that it is square.

}

const SimulationGrid::grid_position SimulationGrid::get_grid_position(const int which_cpu) const{
	return {.i = which_cpu / n_cols, .j = which_cpu % n_cols};
}
const bounding_box_t SimulationGrid::bb_for_cpu(const int which_cpu) const{
	return this->bb_for_grid_position(this->get_grid_position(which_cpu));
}
const bounding_box_t SimulationGrid::bb_for_grid_position(const SimulationGrid::grid_position gp) const{
	return bb_for_position(gp.i, gp.j);
}
const bounding_box_t SimulationGrid::bb_for_position(const int i, const int j) const{

	const double world_spatial_width  = bounds.max.x - bounds.min.x;
	const double world_spatial_height = bounds.max.y - bounds.min.y;
	const phys_vector_t my_subregion_min = (phys_vector_t){.x=bounds.min.x + j*(world_spatial_width/n_cols),
																.y=bounds.min.y + i*(world_spatial_height/n_rows)};

	const phys_vector_t my_subregion_max = (phys_vector_t){.x=bounds.min.x + (1.0 + j)*(world_spatial_width/n_cols),
																.y=bounds.min.y + (1.0 + i)*(world_spatial_height/n_rows)};
	return (bounding_box_t){.min = my_subregion_min, .max = my_subregion_max };
}

const int SimulationGrid::cpu_for_grid_position(const SimulationGrid::grid_position gp) const{
	return this->cpu_for_position(gp.i, gp.j);
}
const int SimulationGrid::cpu_for_position(const int i, const int j) const{
	int n_i = i%n_rows;
	int n_j = j%n_cols;

	if(n_i < 0){ n_i += n_rows; }
	if(n_j < 0){ n_j += n_cols; }

	return n_i*n_cols + n_j;
}

SimulationBlock::SimulationBlock(gas_simulation sim, SimulationGrid _grid, int _block_rank, int maximum_total_particles) : my_sim(sim), the_grid(_grid), block_rank(_block_rank), bounds(sim.bounds), global_max_particles(0){
	//TODO: allocate (aligned?)
	this->all_particles = nullptr;
	this->forces = nullptr;
	this->all_ghosts = nullptr;
	
	set_max_particles(maximum_total_particles);
	initialize_bounds_and_edges();

}

void SimulationBlock::initialize_bounds_and_edges(){
	bounds = the_grid.bb_for_cpu(block_rank);//You need to keep this.
	ghost_bb = bounds;
	ghost_bb.min.x += my_sim.cutoff;
	ghost_bb.min.y += my_sim.cutoff;
	ghost_bb.max.x -= my_sim.cutoff;
	ghost_bb.max.y -= my_sim.cutoff;

	always_ghost_NS = ghost_bb.min.y >= bounds.max.y && ghost_bb.max.y <= bounds.min.y;
	always_ghost_EW = ghost_bb.min.x >= bounds.max.x && ghost_bb.max.x <= bounds.min.x;

	//If I am on one of the edges, I need to post-process my outgoing buffers.
	SimulationGrid::grid_position my_pos = the_grid.get_grid_position(block_rank);
	north_edge = my_pos.i == 0;
	south_edge = my_pos.i == the_grid.n_rows-1;
	west_edge = my_pos.j == 0;
	east_edge = my_pos.j == the_grid.n_cols-1;

	edge_mask = DIR_N | DIR_S | DIR_E | DIR_W;
	if(north_edge && my_sim.enforce_bounds.y){ edge_mask &= ~DIR_N; }
	if(south_edge && my_sim.enforce_bounds.y){ edge_mask &= ~DIR_S; }
	if(east_edge && my_sim.enforce_bounds.x){ edge_mask &= ~DIR_E; }
	if(west_edge && my_sim.enforce_bounds.x){ edge_mask &= ~DIR_W; }
}

void SimulationBlock::set_max_particles(unsigned int new_max_particles){
	if(new_max_particles < this->N_particles || new_max_particles < this->N_ghosts){
		throw std::runtime_error("Cannot shrink below current number of particals!");
	}

	if(nullptr == this->all_particles){
		this->all_particles = (phys_particle_t*)malloc(sizeof(phys_particle_t)*(size_t)new_max_particles);
	}else{
		this->all_particles = (phys_particle_t*)realloc(this->all_particles,
							sizeof(phys_particle_t)*(size_t)new_max_particles);
	}

	if(nullptr == this->forces){
		this->forces = (phys_vector_t*)malloc(sizeof(phys_vector_t)*(size_t)new_max_particles);
	}else{
		this->forces = (phys_vector_t*)realloc(this->forces,
							sizeof(phys_vector_t)*(size_t)new_max_particles);
	}

	if(nullptr == this->all_ghosts){
		this->all_ghosts = (phys_particle_t*)malloc(sizeof(phys_particle_t)*(size_t)new_max_particles);
	}else{
		this->all_ghosts = (phys_particle_t*)realloc(this->all_ghosts,
							sizeof(phys_particle_t)*(size_t)new_max_particles);
	}

	if(nullptr==this->all_particles || nullptr == this->forces || nullptr == this->all_ghosts){
		throw std::runtime_error("Simblock allocated nullptr containment.");
	}

	global_max_particles = new_max_particles;
}

void SimulationBlock::dealloc_ephemeral_storage(){
	//You should only call this if you intend to call set_max_particles again soon.
	if(nullptr != this->all_ghosts){
		this->N_ghosts = 0;
		free(this->all_ghosts);
		this->all_ghosts = nullptr;
	}

	if(nullptr != this->forces){
		free(this->forces);
		this->forces = nullptr;
	}
}

SimulationBlock::~SimulationBlock(){
	//TODO(unknown) allocate (aligned?)

	if(nullptr != this->all_particles){ free(this->all_particles); }
	if(nullptr != this->all_ghosts){ free(this->all_ghosts); }
	if(nullptr != this->forces){ free(this->forces); }
}


void SimulationBlock::simulate_tick(){
		//every k steps: migrate OOB particles to neighbors, receive OOB particles from neighbors
		if(0 == elapsed_ticks % exchange_frequency){
			//TODO(unknown): exception handling in case the students want to throw exceptions.
			this->exchange_particles();
		}
		//every step: send/receive ghosts to/from all neighbors
		this->communicate_ghosts();//TOOD: Exception handling


		find_forces_environment(&my_sim, all_particles, N_particles, forces);
		find_forces_self(&my_sim, all_particles, N_particles, forces);
		find_forces_ghost(&my_sim, all_particles, N_particles, forces, all_ghosts, N_ghosts);
		apply_relativity(&my_sim, all_particles, N_particles);
		update_states(&my_sim, tick_delta_t_ns, all_particles, N_particles, forces);

		elapsed_ticks++;
		T_elapsed_ns+=tick_delta_t_ns;
}
