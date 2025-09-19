set(requires driver qmsd_utils)

idf_component_register(
	INCLUDE_DIRS include
    EMBED_FILES "asr_ota_file/asr_by_gen.bin"
)

add_prebuilt_library(lib${COMPONENT_NAME}_${IDF_TARGET} lib${COMPONENT_NAME}_${IDF_TARGET}.a REQUIRES ${requires})
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME}_${IDF_TARGET})

add_custom_target(generate-asr
    COMMAND ${PYTHON} ${COMPONENT_PATH}/tools/asr_module_gen.py
    DEPENDS ${COMPONENT_PATH}/asr_ota_file/ci1302_firmware.bin
    VERBATIM
)
add_dependencies(${COMPONENT_LIB} generate-asr)

