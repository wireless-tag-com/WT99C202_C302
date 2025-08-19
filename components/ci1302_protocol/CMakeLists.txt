set(requires driver qmsd_utils)

idf_component_register(
	INCLUDE_DIRS .
)

add_prebuilt_library(lib${COMPONENT_NAME}_${IDF_TARGET} lib${COMPONENT_NAME}_${IDF_TARGET}.a REQUIRES ${requires})
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME}_${IDF_TARGET})
