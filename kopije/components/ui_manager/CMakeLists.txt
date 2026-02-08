idf_component_register(
    SRCS "ui_manager.c" 
    
    INCLUDE_DIRS "include" 
    REQUIRES 
        esp_timer
        lvgl
        esp-box-3
        display_manager

)

