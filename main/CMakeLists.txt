set(requires
    hardware_driver
    esp_timer
    nvs_flash
    )

idf_component_register(SRCS main.c main_ble.c
                       INCLUDE_DIRS "."
                       REQUIRES ${requires})
