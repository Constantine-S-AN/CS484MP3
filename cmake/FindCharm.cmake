
include(FindPackageHandleStandardArgs)

set(charmc_compiler_names charmc)
set(charmxi_compiler_names charmxi)
set(ampi_cc_names ampicc)
set(ampi_cxx_names ampicxx)


set(possible_charm_installations ~/usr /usr/local /usr /usr/charm* /usr/local/charm* /opt/charm*)
file(GLOB possible_charm_installations ${possible_charm_installations})
#message("possible locations" ${possible_charm_installations})

include("${CMAKE_CURRENT_LIST_DIR}/charmparts/charm_compiler.cmake")

#once the compiler is found, we can find the libraries and stuff
include("${CMAKE_CURRENT_LIST_DIR}/charmparts/charm_libraries.cmake")

include("${CMAKE_CURRENT_LIST_DIR}/charmparts/charm_charmxi.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/charmparts/charm_charmxi_deprecated.cmake")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Charm
	FOUND_VAR "CHARM_FOUND"
	REQUIRED_VARS CHARM_COMPILER CHARMXI_COMPILER
	VERSION_VAR CHARM_VERSION_STRING)

#Also find AMPI
include("${CMAKE_CURRENT_LIST_DIR}/charmparts/charm_ampi.cmake")
