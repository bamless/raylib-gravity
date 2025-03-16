add_executable(raylib-gravity
    main.c
)

# set(EXTRA_LIBS)
# if(UNIX)
#     set(EXTRA_LIBS dl)
# endif()

target_link_libraries(raylib-gravity PRIVATE raylib)

# Enable link-time optimization if supported
if(LTO)
    set_target_properties(raylib-gravity PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
