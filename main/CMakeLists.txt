
# Main komponenta
#
# "main" je posebna komponenta - entry point aplikacije.
# ESP-IDF avtomatsko išče app_main() funkcijo tukaj.

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    
    # REQUIRES: Main komponenta potrebuje display_manager in sensor_manager
    # Ko dodamo display_manager, dobimo avtomatsko tudi:
    # - lvgl (ker je display_manager REQUIRES lvgl)
    # - esp-box-3 (transitively)
    # - esp_timer (transitively)
    REQUIRES 
    display_manager
    sensor_manager
    ui_manager
    wifi_manager
    shelly_manager
)