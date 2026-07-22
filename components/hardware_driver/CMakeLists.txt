idf_component_register(
    SRCS
        "esp_board_init.c"
        "boards/esp32s3-inmp441/bsp_board.c"
    INCLUDE_DIRS
        "include"
        "boards/include"
    REQUIRES
        driver
        fatfs
)

component_compile_options(-w)
