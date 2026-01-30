# Add C source files here.
add_executable(lab6
    Src/lab6.c
    Src/stm32f0xx_hal_msp.c
    Src/stm32f0xx_it.c
)

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(lab6 PROPERTIES ADDITIONAL_CLEAN_FILES lab6.map)

# Add user defined include paths
target_include_directories(lab6 PRIVATE
    Inc
)

# Add project symbols (macros)
target_compile_definitions(lab6 PRIVATE
)

# Add linked libraries
target_link_libraries(lab6
    STM32_Drivers
)

flash_target(lab6)
