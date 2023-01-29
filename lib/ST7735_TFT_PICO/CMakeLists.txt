add_library(st7735 INTERFACE)

target_sources(st7735 INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/st7735/ST7735_TFT.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/st7735/ST7735_TFT_graphics.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/st7735/ST7735_TFT_Print.cpp
)

target_include_directories(st7735 INTERFACE include)

target_link_libraries(st7735 INTERFACE 
        hardware_spi
        pico_stdlib
)
