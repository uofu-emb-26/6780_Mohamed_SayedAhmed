# Add C source files here.
add_executable(lab5
    Src/lab5.c
    Src/stm32f0xx_hal_msp.c
    Src/stm32f0xx_it.c
)

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(lab5 PROPERTIES ADDITIONAL_CLEAN_FILES lab5.map)

# Add user defined include paths
target_include_directories(lab5 PRIVATE
    Inc
)

# Add project symbols (macros)
target_compile_definitions(lab5 PRIVATE
)

# Add linked libraries
target_link_libraries(lab5
    STM32_Drivers
)

flash_target(lab5)
