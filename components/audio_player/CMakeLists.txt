set(requires esp_audio_codec audio_hal audio_stream qmsd_audio qmsd_utils ci1302_protocol)

idf_component_register(
	INCLUDE_DIRS include
)

add_prebuilt_library(lib${COMPONENT_NAME}_${IDF_TARGET} lib${COMPONENT_NAME}_${IDF_TARGET}.a REQUIRES ${requires})
target_link_libraries(${COMPONENT_LIB} INTERFACE lib${COMPONENT_NAME}_${IDF_TARGET})
