
if(CHARMXI_COMPILER)

function(set_charm_target target_name)
	set(options SEARCH STANDALONE NOMAIN) #Tells if we want to search for .ci files in the basic sources list
	set(oneValueArgs TRACEMODE) #TODO: actually look at charmc to figure out how to properly build traces
	set(multiValueArgs CHARM_SOURCES CHARM_MODULES )
	cmake_parse_arguments(SET_CHARM_TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	#examine all the sources and find any charm sources.
	#print_target_properties(${target_name})

	get_target_property(ALL_SOURCES_PATHS ${target_name} SOURCES)

	set(TMP_CHARM_SOURCES ) #TODO: Start with / append any charm sources provided here.
	set(TMP_NON_CHARM_SOURCES )

	foreach(one_source ${ALL_SOURCES_PATHS})
		#message("blah : " ${one_source})
		if(${one_source} MATCHES "\\.ci$")
			#message("Appending : " ${one_source})
			list(APPEND TMP_CHARM_SOURCES ${one_source})
		else()
			list(APPEND TMP_NON_CHARM_SOURCES ${one_source})
		endif()
	endforeach(one_source)
	foreach(one_charm_source ${TMP_CHARM_SOURCES})
		list(REMOVE_ITEM ALL_SOURCES_PATHS ${one_charm_source})
	endforeach(one_charm_source)

	#set_target_properties(${target_name} PROPERTIES SOURCES "${TMP_NON_CHARM_SOURCES}" SCOPE PARENT_SCOPE)
	#TODO: append to if the charm sources property already exists
	#set_target_properties(${target_name} PROPERTIES "CHARM_SOURCES" "${TMP_CHARM_SOURCES}" SCOPE PARENT_SCOPE)

	#message("all charm sources : " "${TMP_CHARM_SOURCES}")
	#message("all non-charm sources : " "${TMP_NON_CHARM_SOURCES}")

	#message("THE CHARMXI COMPILER IS " ${CHARMXI_COMPILER})

	foreach(one_charm_source ${TMP_CHARM_SOURCES})
		get_filename_component(SINGLE_CHARM_DEFAULT_OUTPUT ${one_charm_source} NAME)
		string(REGEX REPLACE "\\.ci$" "" SINGLE_CHARM_DEFAULT_OUTPUT ${SINGLE_CHARM_DEFAULT_OUTPUT})

		#TODO: We should create a directory that these generated files go into.
		#If only certain modules were asked for, we should generate those into a non-default directory.

		list(APPEND TMP_NON_CHARM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${SINGLE_CHARM_DEFAULT_OUTPUT}.decl.h")
		list(APPEND TMP_NON_CHARM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${SINGLE_CHARM_DEFAULT_OUTPUT}.def.h")
		include_directories(${target_name} ${CMAKE_CURRENT_BINARY_DIR})

		#If we use an OUTPUT type custom_command, and alter the target's sources list, we might avoid that.
		#message("one_charm_source : " ${CMAKE_CURRENT_SOURCE_DIR}/${one_charm_source} )
		set(SET_CHARM_TARGET_SINGLE_CHARM_SOURCE_FULL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${one_charm_source})
		add_custom_command(
			PRE_BUILD
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SINGLE_CHARM_DEFAULT_OUTPUT}.decl.h ${CMAKE_CURRENT_BINARY_DIR}/${SINGLE_CHARM_DEFAULT_OUTPUT}.def.h
			COMMAND ${CHARMXI_COMPILER} ${SET_CHARM_TARGET_SINGLE_CHARM_SOURCE_FULL_PATH}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			DEPENDS ${SET_CHARM_TARGET_SINGLE_CHARM_SOURCE_FULL_PATH}
			VERBATIM
		)

		#TODO: needs to be per module
		set(mod_init_src "${CMAKE_CURRENT_BINARY_DIR}/${SINGLE_CHARM_DEFAULT_OUTPUT}_modinit.C")
		list(APPEND TMP_NON_CHARM_SOURCES ${mod_init_src})
		add_custom_command(
			PRE_BUILD
			OUTPUT ${mod_init_src}
			COMMAND echo "void _registerExternalModules(char **argv) { (void)argv; } void _createTraces(char **argv) {(void)argv;}" >> ${mod_init_src}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			VERBATIM
		)


		#todo: each module needs a modInit
	endforeach()

	set_target_properties(${target_name} PROPERTIES SOURCES "${TMP_NON_CHARM_SOURCES}" SCOPE PARENT_SCOPE)
	#TODO: append to if the charm sources property already exists
	set_target_properties(${target_name} PROPERTIES "CHARM_SOURCES" "${TMP_CHARM_SOURCES}" SCOPE PARENT_SCOPE)

	#compile and linking flags
	#TODO: Charm can be built without MPI, can't it?
	#TODO: Detect the language C/CXX etc.
	#TODO: Get the last compiler/linker flags dynamically from interrogating charmc, not hardcoded as they are here "-m64 etc."
	include_directories(${target_name} ${MPI_CXX_INCLUDE_PATH})
	target_link_libraries(${target_name} ${MPI_CXX_LIBRARIES})
	set_target_properties(${target_name} PROPERTIES COMPILE_FLAGS "${CHARM_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS} -m64 -fPIC " SCOPE PARENT_SCOPE)
	set_target_properties(${target_name} PROPERTIES LINK_FLAGS "${CHARM_LDXX_FLAGS} ${MPI_CXX_LINK_FLAGS} -m64 -fPIC -rdynamic " SCOPE PARENT_SCOPE)

	include_directories(${target_name} ${CHARMINC})

endfunction()

endif(CHARMXI_COMPILER)
