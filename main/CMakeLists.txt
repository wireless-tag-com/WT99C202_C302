set(requires ci1302_protocol esp_https_ota app_update qmsd_network qmsd_button storage_nvs audio_player aiha_server WT99C202_C302)

set(src_dir . network  chat_notify chat battery)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/debug)
    list(APPEND src_dir debug)
endif()

idf_component_register(
    SRC_DIRS ${src_dir}
    INCLUDE_DIRS ${src_dir}
    REQUIRES ${requires}
    EMBED_FILES "asr_ota_file/asr_by_gen.bin"
)

add_custom_target(generate-asr
    COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/asr_ota_file/tools/asr_module_gen.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/asr_ota_file/ci1302_firmware.bin
    VERBATIM
)
add_dependencies(${COMPONENT_LIB} generate-asr)

littlefs_create_partition_image(res tone_res FLASH_IN_PROJECT)
