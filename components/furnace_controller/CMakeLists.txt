idf_component_register(
    SRCS "furnace_controller.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        shelly_manager
        sensor_manager
)