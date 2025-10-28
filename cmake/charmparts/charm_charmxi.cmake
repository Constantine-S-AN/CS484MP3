#include(allprops)


cmake_policy(SET CMP0022 NEW)
set(charmxi_compiler_names ${charmxi_compiler_names} charmxi)

include(FindPackageHandleStandardArgs)

#CHARMXI charm module compiler
find_program(CHARMXI_COMPILER
	NAMES ${charmxi_compiler_names}
	HINTS ${CHARM_PATH} ${CHARM_HOME} ENV CHARM_PATH ENV CHARM_HOME
	PATHS ${possible_charm_installations}
	PATH_SUFFIXES bin
	DOC "Charm++ module compiler"
)
mark_as_advanced(CHARMXI_COMPILER)


if(CHARMXI_COMPILER)
	define_property(TARGET PROPERTY "CHARM_SOURCES"
		BRIEF_DOCS "Sources for charmxi"
		FULL_DOCS  "List of source files that the charm module compiler should interpret."
	)

	function(create_modinit_src modinit_src_varname )
		set(options SEARCH STANDALONE NOMAIN) #Tells if we want to search for .ci files in the basic sources list
		set(oneValueArgs TRACEMODE) #TODO: actually look at charmc to figure out how to properly build traces
		set(multiValueArgs CHARM_SOURCES CHARM_MODULES LINK_MODULES )
		cmake_parse_arguments(CREATE_MODINIT_SRC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

		set(my_mi_sourcecode "")
		foreach(one_dependency_module ${CREATE_MODINIT_SRC_LINK_MODULES})
			set(my_mi_sourcecode "${my_mi_sourcecode} extern void _register${one_dependency_module}(void); ")
		endforeach()


		set(my_mi_sourcecode "${my_mi_sourcecode}  void _registerExternalModules(char **argv) {  (void)argv; ")
		foreach(one_dependency_module ${CREATE_MODINIT_SRC_LINK_MODULES})
			set(my_mi_sourcecode "${my_mi_sourcecode} _register${one_dependency_module}(); ")
		endforeach()
		set(my_mi_sourcecode "${my_mi_sourcecode} } " )

		set(my_mi_sourcecode "${my_mi_sourcecode}  void _createTraces(char **argv) { (void)argv; } ")
		set(${modinit_src_varname} "${my_mi_sourcecode}" PARENT_SCOPE)
	endfunction()


	function(add_charm_module module_name)
		set(options SEARCH STANDALONE NOMAIN) #Tells if we want to search for .ci files in the basic sources list
		set(oneValueArgs TRACEMODE) #TODO: actually look at charmc to figure out how to properly build traces
		set(multiValueArgs CHARM_SOURCES CHARM_MODULES LINK_MODULES )
		cmake_parse_arguments(ADD_CHARM_MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

		add_library(${module_name} INTERFACE)

		#message("add_charm_module ARGC " ${ARGC})
		#message("add_charm_module ARGV " ${ARGV})
		#message("add_charm_module ARGN " ${ARGN})
		#message("add_charm_module unparsed " ${ADD_CHARM_MODULE_UNPARSED_ARGUMENTS})

		#the director for the generated files
		set(MODULE_GENPATH "${CMAKE_CURRENT_BINARY_DIR}/${module_name}_charmxi.dir")
		add_custom_target(${module_name}_builddir)
		add_custom_command(TARGET ${module_name}_builddir PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${MODULE_GENPATH}")
		add_dependencies(${module_name} INTERFACE ${module_name}_builddir)
		target_include_directories(${module_name} INTERFACE ${MODULE_GENPATH})

		foreach(one_charm_source ${ADD_CHARM_MODULE_UNPARSED_ARGUMENTS})
			get_filename_component(SINGLE_CHARM_DEFAULT_OUTPUT ${one_charm_source} NAME)
			string(REGEX REPLACE "\\.ci$" "" SINGLE_CHARM_DEFAULT_OUTPUT ${SINGLE_CHARM_DEFAULT_OUTPUT})

			#If only certain modules were asked for, we should generate those into a non-default directory.

			target_sources(${module_name} INTERFACE "${MODULE_GENPATH}/${SINGLE_CHARM_DEFAULT_OUTPUT}.decl.h")
			target_sources(${module_name} INTERFACE "${MODULE_GENPATH}/${SINGLE_CHARM_DEFAULT_OUTPUT}.def.h")
			set(SET_CHARM_TARGET_SINGLE_CHARM_SOURCE_FULL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${one_charm_source})
			add_custom_command(
				PRE_BUILD
				OUTPUT "${MODULE_GENPATH}/${SINGLE_CHARM_DEFAULT_OUTPUT}.decl.h" "${MODULE_GENPATH}/${SINGLE_CHARM_DEFAULT_OUTPUT}.def.h"
				#COMMAND ${CMAKE_COMMAND} -E make_directory "${MODULE_GENPATH}"
				COMMAND ${CHARMXI_COMPILER} ${SET_CHARM_TARGET_SINGLE_CHARM_SOURCE_FULL_PATH}
				WORKING_DIRECTORY ${MODULE_GENPATH}
				DEPENDS "${SET_CHARM_TARGET_SINGLE_CHARM_SOURCE_FULL_PATH}"
				VERBATIM
			)
			#add_custom_target()

			#create_modinit_src(tmp_modinit_sourcecode LINK_MODULES ${ADD_CHARM_MODULE_LINK_MODULES})

			set(mod_init_src "${MODULE_GENPATH}/${SINGLE_CHARM_DEFAULT_OUTPUT}_modinit.C")
			target_sources(${module_name} INTERFACE ${mod_init_src})
			add_custom_command(
				PRE_BUILD
				OUTPUT "${mod_init_src}"
				#COMMAND ${CMAKE_COMMAND} -E make_directory "${MODULE_GENPATH}"
				COMMAND echo "${tmp_modinit_sourcecode}" >> ${mod_init_src}
				WORKING_DIRECTORY ${MODULE_GENPATH}
				VERBATIM
			)
		endforeach()

		foreach(one_linked_module ${ADD_CHARM_MODULE_LINK_MODULES})
			#TODO: Need to process modules dependencies.
			target_link_libraries(${module_name} INTERFACE "module${one_linked_module}")
		endforeach()

		target_include_directories(${module_name} INTERFACE ${MPI_CXX_INCLUDE_PATH})
		target_link_libraries(${module_name} INTERFACE ${MPI_CXX_LIBRARIES})
		target_include_directories(${module_name} INTERFACE ${CHARM_CXX_INCLUDE_PATH})

		#set_target_properties(${module_name} PROPERTIES INTERFACE_COMPILE_FLAGS "${CHARM_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS} -m64 -fPIC ")
		#set_target_properties(${module_name}_linkage PROPERTIES INTERFACE_LINK_FLAGS "${CHARM_LDXX_FLAGS} ${MPI_CXX_LINK_FLAGS} -m64 -fPIC -rdynamic ")

	endfunction()


endif(CHARMXI_COMPILER)
