# Add C source files here.
add_executable(lab1
    Src/lab1.c
    Src/hal_gpio.c
    Src/stm32f0xx_hal_msp.c
    Src/stm32f0xx_it.c
)

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(lab1 PROPERTIES ADDITIONAL_CLEAN_FILES lab1.map)

# Add user defined include paths
target_include_directories(lab1 PRIVATE
    Inc
)

# Add project symbols (macros)
target_compile_definitions(lab1 PRIVATE
)

# Add linked libraries
target_link_libraries(lab1
    STM32_Drivers
)

flash_target(lab1)
