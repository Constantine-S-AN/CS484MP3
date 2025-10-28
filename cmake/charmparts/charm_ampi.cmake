
set(ampi_cc_names ${ampi_cc_names} ampicc)
set(ampi_cxx_names ${ampi_cxx_names} ampicxx)

include(FindPackageHandleStandardArgs)

#AMPI C Compiler
find_program(AMPI_C_COMPILER
	NAMES ${ampi_cc_names}
	HINTS ${CHARM_PATH} ${CHARM_HOME} ENV CHARM_PATH ENV CHARM_HOME
	PATHS ${possible_charm_installations}
	PATH_SUFFIXES bin
	DOC "AMPI C compiler wrapper"
)
mark_as_advanced(AMPI_C_COMPILER)

#AMPI CXX Compiler
find_program(AMPI_CXX_COMPILER
	NAMES ${ampi_cxx_names}
	HINTS ${CHARM_PATH} ${CHARM_HOME} ENV CHARM_PATH ENV CHARM_HOME
	PATHS ${possible_charm_installations}
	PATH_SUFFIXES bin
	DOC "AMPI CXX compiler wrapper"
)
mark_as_advanced(AMPI_CXX_COMPILER)

find_package_handle_standard_args(AMPI
	FOUND_VAR "AMPI_FOUND"
	REQUIRED_VARS AMPI_C_COMPILER AMPI_CXX_COMPILER
	VERSION_VAR CHARM_VERSION_STRING)
