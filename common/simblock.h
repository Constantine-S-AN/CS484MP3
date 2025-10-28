#ifndef SIMBLOCK_H
#define SIMBLOCK_H

#include "gassim2d.h"
#include "gassim2d_util.h"

#include "gas_constants.h"

#include <cstdint>

/* phys_vector_t comes from gassim2d, it is struct { double x; double y; } */

/**
* @typedef
* A bitmask to represent the direction for ghosts and migrants.
* See the definitions in SimulationBlock for specific values.
*/
typedef uint8_t sim_direction_t;

#define DIR_EQ(X,Y) (X==Y)
#define DIR_HAS(X,Y) (X==(X&Y))

class SimulationGrid {
public:
	struct grid_position {int i; int j;};

	SimulationGrid(double xmin, double ymin, double xmax, double ymax, int n_cpus);
	SimulationGrid(phys_vector_t min, phys_vector_t max, int n_cpus);
	SimulationGrid(bounding_box_t bounds, int n_cpus);
	SimulationGrid(){n_cpus=0;n_cols=0;n_rows=0;};

	~SimulationGrid(){};

	const grid_position get_grid_position(const int which_cpu) const;
	const bounding_box_t bb_for_cpu(const int which_cpu) const;
	const bounding_box_t bb_for_position(const int i, const int j) const;
	const bounding_box_t bb_for_grid_position(const grid_position gp) const;

	const int cpu_for_position(const int i, const int j) const;
	const int cpu_for_grid_position(const grid_position gp) const;

	bounding_box_t bounds; //Could differ from that of the simulation, for example, when wrapping.
	bool wrap_x = false;
	bool wrap_y = false;
	phys_vector_t sizes;
	int n_cpus;
	int n_cols;//row size
	int n_rows;//col size
};

class SimulationBlock {
public:
	SimulationBlock() : the_grid(SimulationGrid()), block_rank(-1), global_max_particles(0) {}
	SimulationBlock(gas_simulation sim, SimulationGrid _grid, int _block_rank, int maximum_total_particles);
	//SimulationBlock(SimulationGrid _grid, int maximum_total_particles);
	virtual ~SimulationBlock();
	void initialize_bounds_and_edges();//called by constructor and migration code.

	void simulate_tick();//Students must not override.


	/* Students implement these. */
	virtual int init_communication(){return 0;};
	virtual int finalize_communication(){return 0;};
	virtual int exchange_particles(){return 0;};
	virtual int communicate_ghosts(){return 0;};

	/* We would not normally make these public. */
	gas_simulation my_sim = { .mass = MASS_AR_AG, .epsilon = EPSILON_AR_AGNM2PERNS2, .sigma = SIGMA_AR_NM, .cutoff = 3.0, .v_max = 1000.0 };
	SimulationGrid the_grid;
	int block_rank;
	bounding_box_t bounds; //Definitley differes from the global bounds

	unsigned int N_particles = 0;
	unsigned int N_ghosts = 0;

	phys_particle_t *all_particles = nullptr;
	phys_particle_t *all_ghosts = nullptr;
	phys_vector_t *forces = nullptr;

	uint64_t elapsed_ticks = 0;
	long double T_elapsed_ns = 0.0;//Elapsed time in nanoseconds
	double tick_delta_t_ns = 0.0000001;//timestep is given in nanoseconds, chosen by fiddling.

	unsigned int exchange_frequency = 7;

	//Utility
	inline void add_particle(phys_particle_t p);
	inline void remove_particle(unsigned int i);

	void set_max_particles(unsigned int new_max_particles);
	inline unsigned int get_max_particles(){return global_max_particles;}
	void dealloc_ephemeral_storage();

	//Things for migration and ghost communication

	bool always_ghost_NS;
	bool always_ghost_EW;
	bool north_edge;
	bool south_edge;
	bool east_edge;
	bool west_edge;
	inline sim_direction_t get_edge_mask() const {return edge_mask; }
	inline sim_direction_t check_ghost_direction(const phys_particle_t part) const;
	inline sim_direction_t check_migrant_direction(const phys_particle_t part) const;
	//Directions
	static const sim_direction_t DIR_SELF = 0;
	static const sim_direction_t DIR_N = 1;//least significant bit means north
	static const sim_direction_t DIR_S = 2;//next lest for south
	static const sim_direction_t DIR_E = 4;//next least for E
	static const sim_direction_t DIR_W = 8;//next least for W
	static const sim_direction_t DIR_NW = DIR_N | DIR_W;
	static const sim_direction_t DIR_SE = DIR_S | DIR_E;
	static const sim_direction_t DIR_NE = DIR_N | DIR_E;
	static const sim_direction_t DIR_SW = DIR_S | DIR_W;
private:
	unsigned int global_max_particles;
	bounding_box_t ghost_bb;
	sim_direction_t edge_mask = DIR_N | DIR_S | DIR_E | DIR_W; //Gets changed in constructor
};

/**
 *	Remove a particle from the simulation.
 *	This function replaces the particle at i and replaces it with the one at the end of the particle array, decrementing the storage size.
 *	NOTE: If you are processing and removing multiple particles, you should work from higher indices to lower indices.
 *
 */
inline void SimulationBlock::remove_particle(unsigned int i){
	//assert(i >= 0 && i < N_particles);
	if(!(i >= 0 && i < N_particles)){ throw std::runtime_error("Tried to remove non-existent particle."); }
	if(i < N_particles){
		all_particles[i] = all_particles[N_particles - 1];//might be self assign. That's fine.
		N_particles--;
	}
}

inline void SimulationBlock::add_particle(phys_particle_t p){
	if( N_particles >= global_max_particles){ throw std::runtime_error("Failed to add particle, insufficient space allocated."); }

	all_particles[N_particles] = p;
	N_particles++;
}

/**
*	\brief Find directions to ghost a particle.
*
*	@param[in] part A particle owned by this SimulationBlock
*	@return A bitmask representing combination of directions.
*/
inline sim_direction_t SimulationBlock::check_ghost_direction(const phys_particle_t part) const {
	register int ghost_direction = 0;
	if( always_ghost_NS || part.p.y < ghost_bb.min.y ){ ghost_direction |= DIR_N; }
	if( always_ghost_NS || part.p.y > ghost_bb.max.y ){ ghost_direction |= DIR_S; }
	if( always_ghost_EW || part.p.x < ghost_bb.min.x ){ ghost_direction |= DIR_W; }
	if( always_ghost_EW || part.p.x > ghost_bb.max.x ){ ghost_direction |= DIR_E; }

	ghost_direction &= edge_mask;
	return ghost_direction;
}

/**
*	\brief Find migration direction for OOB particle.
*
*	@param[in] part A particle owned by this SimulationBlock
*	@return A bitmask representing ONE direction.
*/
inline sim_direction_t SimulationBlock::check_migrant_direction(const phys_particle_t part) const {
	register int migrate_direction = 0;

	//For migration, directions _are_ mutualy exclusive.
	if( part.p.y < bounds.min.y ) { migrate_direction |= DIR_N; }
	else if ( part.p.y > bounds.max.y){ migrate_direction |= DIR_S; }

	if( part.p.x < bounds.min.x ) { migrate_direction |= DIR_W; }
	else if (part.p.x > bounds.max.x){ migrate_direction |= DIR_E; }

	migrate_direction &= edge_mask;
	return migrate_direction;
}



#endif /* SIMBLOCK_H */
