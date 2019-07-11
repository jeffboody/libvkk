cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall)

# Submodule library
add_library(vkk
            STATIC

            # Source
            vkk_engine.c)

# Linking
target_link_libraries(vkk

                      # Submodule libraries
                      cc

                      # NDK libraries
                      log)
