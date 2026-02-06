idf_component_register(
    # SRCS: Seznam vseh .c datotek za kompajliranje
    SRCS "display_manager.c"
    
    # INCLUDE_DIRS: Direktoriji z .h datotekami (PUBLIC - vidni vsem)
    INCLUDE_DIRS "include"
    
    # REQUIRES: Komponente, ki jih ta komponenta potrebuje
    # - esp_timer: Za periodične klice LVGL (lv_timer_handler)
    # - lvgl: LVGL knjižnica (iz managed_components)
    # - esp-box-3: ESP-BOX-3 BSP (iz managed_components)
    REQUIRES 
        esp_timer
        lvgl
        esp-box-3
)

# Opomba o REQUIRES vs PRIV_REQUIRES:
# - REQUIRES: Javne odvisnosti (expose-ane skozi naš header)
# - PRIV_REQUIRES: Privatne odvisnosti (uporabljene samo interno)
# 
# Ker naš .h vsebuje "lvgl.h", mora biti lvgl v REQUIRES.