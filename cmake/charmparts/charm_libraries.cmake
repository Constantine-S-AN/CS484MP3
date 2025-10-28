
if(NOT CHARM_LIBRARIES_CREATED)
	set(CHARM_LIBRARIES_CREATED TRUE)



#TODO: tediously find all of these so we don't have to globally change the link directory.
link_directories("${CHARMLIB}" "${CHARMLIBSO}")

find_library(CKMAIN_LIBRARY ckmain HINTS "${CHARMLIB}" "${CHARMLIBSO}")
find_library(CK_LIBRARY ck HINTS "${CHARMLIB}" "${CHARMLIBSO}")
find_library(MEMORY-DEFAULT_LIBRARY memory-default HINTS "${CHARMLIB}" "${CHARMLIBSO}")
find_library(THREADS-DEFAULT_LIBRARY threads-default HINTS "${CHARMLIB}" "${CHARMLIBSO}")
#conv-machine conv-core tmgr conv-util conv-partition hwloc_embedded m memory-default threads-default ldb-rand conv-ldb ckqt dl moduleNDMeshStreamer modulecompletion

#
# This is really ugly. Somehow, we need to be an OBJECT, and have both an INTERFACE *AND* PUBLIC linking
#
cmake_policy(SET CMP0022 NEW)
add_library(charm_all OBJECT "${CMAKE_CURRENT_LIST_DIR}/charm_all.cpp")
#TODO: Populate this list automatically by interrogating charmc
target_link_libraries(charm_all INTERFACE ckmain ck memory-default threads-default conv-machine conv-core tmgr conv-util conv-partition hwloc_embedded m memory-default threads-default ldb-rand conv-ldb ckqt dl moduleNDMeshStreamer modulecompletion z m )
target_link_libraries(charm_all PUBLIC ckmain ck memory-default threads-default conv-machine conv-core tmgr conv-util conv-partition hwloc_embedded m memory-default threads-default ldb-rand conv-ldb ckqt dl moduleNDMeshStreamer modulecompletion z m )


#target_link_libraries(charm_all INTERFACE ${CKMAIN_LIBRARY} ${CK_LIBRARY} ${MEMORY-DEFAULT_LIBRARY} ${THREADS-DEFAULT_LIBRARY})
#set_target_properties(charm_all PROPERTIES LINK_FLAGS "${CHARM_LDXX_FLAGS} ${MPI_CXX_LINK_FLAGS} -m64 -fPIC -rdynamic ")


endif(NOT CHARM_LIBRARIES_CREATED)
