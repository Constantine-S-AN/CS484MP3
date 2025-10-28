set(possible_charm_installations ~/usr /usr/local /usr /usr/charm* /usr/local/charm* /opt/charm*)
file(GLOB possible_charm_installations ${possible_charm_installations})

##### LIBRARIES for AMPI, Charm++ installation with AMPI #######


find_path(AMPI_INCLUDE_DIR "ampi.h"
	HINTS ${CHARM_HOME} ${CHARM_ROOT} ${AMPI_ROOT} ${AMPI_HOME}
    ENV CHARM_ROOT ENV CHARM_HOME ENV AMPI_HOME
    PATHS ${possible_charm_installations}
    PATH_SUFFIXES include
)

find_library(AMPI_LIBRARY "ampi-compat"
    HINTS ${CHARM_ROOT} ${AMPI_ROOT}
    $ENV{CHARM_ROOT} $ENV{AMPI_ROOT}
    PATHS ${possible_charm_installations}
    PATH_SUFFIXES lib
)

###### COMPILERS #####
set(ampi_cc_names ${ampi_cc_names} ampicc)
set(ampi_cxx_names ${ampi_cxx_names} ampicxx)

#AMPI C Compiler
find_program(AMPI_C_COMPILER
	NAMES ampicc ${ampi_cc_names}
	HINTS ${CHARM_PATH} ${CHARM_HOME} ENV CHARM_PATH ENV CHARM_HOME
	PATHS ${possible_charm_installations}
	PATH_SUFFIXES bin
	DOC "AMPI C compiler wrapper"
)
mark_as_advanced(AMPI_C_COMPILER)

#AMPI CXX Compiler
find_program(AMPI_CXX_COMPILER
	NAMES ampicxx ${ampi_cxx_names}
	HINTS ${CHARM_PATH} ${CHARM_HOME} ENV CHARM_PATH ENV CHARM_HOME
	PATHS ${possible_charm_installations}
	PATH_SUFFIXES bin
	DOC "AMPI CXX compiler wrapper"
)
mark_as_advanced(AMPI_CXX_COMPILER)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AMPI
	FOUND_VAR "AMPI_FOUND"
	REQUIRED_VARS AMPI_C_COMPILER AMPI_CXX_COMPILER AMPI_INCLUDE_DIR AMPI_LIBRARY
	VERSION_VAR CHARM_VERSION_STRING)


if(AMPI_FOUND)
    
    find_package(MPI QUIET REQUIRED)
    
    if(NOT TARGET AMPI::AMPI_CXX)
        add_library(AMPI::AMPI_CXX STATIC IMPORTED)
        
        set_target_properties(AMPI::AMPI_CXX PROPERTIES IMPORTED_LOCATION ${AMPI_LIBRARY})
        target_link_libraries(AMPI::AMPI_CXX INTERFACE ${AMPI_LIBRARY})
        target_include_directories(AMPI::AMPI_CXX INTERFACE ${AMPI_INCLUDE_DIR})
        
        
        target_link_libraries(AMPI::AMPI_CXX INTERFACE MPI::MPI_CXX)
        
        set_target_properties(AMPI::AMPI_CXX PROPERTIES COMPILE_FLAGS "${MPI_CXX_COMPILE_FLAGS} -modules CommonLBs -memory isomalloc")
        set_target_properties(AMPI::AMPI_CXX PROPERTIES LINK_FLAGS "${MPI_CXX_LINK_FLAGS} -modules CommonLBs -memory isomalloc")
        
        #set_property(TARGET AMPI::AMPI_CXX APPEND_STRING PROPERTY COMPILE_FLAGS " -modules CommonLBs -memory isomalloc")
        #set_property(TARGET AMPI::AMPI_CXX APPEND_STRING PROPERTY LINK_FLAGS " -modules CommonLBs -memory isomalloc")
        
    endif()
    
    
endif(AMPI_FOUND)

function(make_ampi_target target_name)
    if(NOT AMPI_FOUND)
        message(FATAL_ERROR "Can't make_ampi_target without AMPI compiler.")
    endif()
    
    set_target_properties(${target_name} PROPERTIES
            CMAKE_C_COMPILER "${AMPI_C_COMPILER}"
            CMAKE_CXX_COMPILER "${AMPI_CXX_COMPILER}"
            CMAKE_EXE_LINKER_FLAGS "-L${CHARMLIB}"
        )
    
    #message(FATAL_ERROR "FOOBAR!")
    #set(CMAKE_C_COMPILER "${AMPI_C_COMPILER}")
    #set(CMAKE_CXX_COMPILER "${AMPI_CXX_COMPILER}")
    #set(CMAKE_LINKER "/opt/charm-6.9.0/bin/ampicxx")
    #set(CMAKE_EXE_LINKER_FLAGS "-L${CHARMLIB}") #absolutely disgusting, but leave for now
endfunction()
