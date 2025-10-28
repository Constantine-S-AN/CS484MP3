
//#include <gassim2d.h>

inline void operator|(PUP::er &p, phys_vector_t &pvect){
		p|pvect.x;
		p|pvect.y;
}

inline void operator|(PUP::er &p, phys_particle_t &part){
		p|part.p;
		p|part.v;
}

inline void operator|(PUP::er &p, bounding_box_t &bb){
		p|bb.min;
		p|bb.max;
}

inline void operator|(PUP::er &p, int_vector_t &ivt){
		p|ivt.x;
		p|ivt.y;
}

inline void operator|(PUP::er &p, gas_simulation &gs){
		p|gs.mass;
		p|gs.epsilon;
		p|gs.sigma;
		p|gs.cutoff;
		p|gs.v_max;
		p|gs.bounds;
		p|gs.enforce_bounds;
}
