cmake_minimum_required(VERSION 3.15)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_generate_library()

sdk_library_add_sources(
    encoder.c
    max7219.c
    ds3231.c
)

sdk_set_main_file(main.c)

project(clock_wb02
    VERSION 1.0
    DESCRIPTION "Clock WB02"
    LANGUAGES C
)
