
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        display_manager
        ui_manager
        sensor_manager
        wifi_manager
        shelly_manager
        furnace_controller
)