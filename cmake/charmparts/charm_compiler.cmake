
set(charmc_compiler_names ${charmc_compiler_names} charmc)

include(FindPackageHandleStandardArgs)

#CHARM Compiler
find_program(CHARM_COMPILER
	NAMES ${charmc_compiler_names}
	HINTS ${CHARM_PATH} ${CHARM_HOME} ENV CHARM_PATH ENV CHARM_HOME
	PATHS ${possible_charm_installations}
	PATH_SUFFIXES bin
	DOC "Charm++ compiler wrapper"
)
mark_as_advanced(CHARM_COMPILER)

#Get the version
if(CHARM_COMPILER)
	find_package(MPI)

	execute_process(COMMAND ${CHARM_COMPILER} -V
				OUTPUT_VARIABLE charmc_version
				ERROR_QUIET
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
	if(charmc_version MATCHES "^Charm\\+\\+ Version [0-9.]+")
		string(REGEX REPLACE "Charm\\+\\+ Version ([0-9.]+).*" "\\1" CHARM_VERSION_STRING "${charmc_version}")
	endif()
	unset(charmc_version)
endif(CHARM_COMPILER)

#Get all options linking, etc.
if(CHARM_COMPILER)
	execute_process(COMMAND ${CHARM_COMPILER} -print-building-blocks
				OUTPUT_VARIABLE charmc_all_variables
				ERROR_QUIET
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)


	string(REPLACE "\n" ";" charmc_all_variables_list ${charmc_all_variables})
	#TODO: loop and find all such variables, not hardcoded
	##if(charmc_all_variables MATCHES "CHARM_CC_FLAGS='.*'")
	##	message(FATAL_ERROR "Found charm CC flags " ${charmc_all_variables})
	##	##string(REGEX REPLACE "Charm\\+\\+ Version ([0-9.]+).*" "\\1" CHARM_VERSION_STRING "${charmc_version}")
	##endif()
	foreach(one_charm_variable_line ${charmc_all_variables_list})
		string(REGEX REPLACE "^(.*)='(.*)'$" "\\1" ONE_CHARM_VAR_NAME ${one_charm_variable_line})
		string(REGEX REPLACE "^(.*)='(.*)'$" "\\2" ONE_CHARM_VAR_VALUE ${one_charm_variable_line})
		#message("BALAHA\n" ${ONE_CHARM_VAR_NAME} " IS EQUAL TO " ${ONE_CHARM_VAR_VALUE})
		set(${ONE_CHARM_VAR_NAME} ${ONE_CHARM_VAR_VALUE})
	endforeach()
	unset(charmc_all_variables_list)
	unset(charmc_all_variables)
	set(CHARM_CXX_INCLUDE_PATH ${CHARMINC})
	#link_directories(${target_name} ${CHARMLIB} ${CHARMLIBSO}) #Don't like that this will make everyting have that globally.
endif(CHARM_COMPILER)
