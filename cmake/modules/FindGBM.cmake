cmake_minimum_required(
    VERSION
        3.7
)

# Be compatible even if a newer CMake version is available
cmake_policy(
    VERSION
        3.7...3.12
)

find_package(PkgConfig)

if(${PKG_CONFIG_FOUND})

    # Just check if the gbm.pc exist, and create the PkgConfig::gbm target
    # No version requirement (yet)
    pkg_check_modules(GBM REQUIRED IMPORTED_TARGET gbm)

    include(FindPackageHandleStandardArgs)

    if(NOT BUILD_SHARED_LIBS)
        set(LIBRARY_TYPE "_STATIC")
    endif()

    # Sets the FOUND variable to TRUE if all required variables are present and set
    find_package_handle_standard_args(
        gbm
        REQUIRED_VARS
            GBM${LIBRARY_TYPE}_INCLUDE_DIRS
            GBM${LIBRARY_TYPE}_CFLAGS
            #GBM${LIBRARY_TYPE}_CFLAGS_OTHER
            GBM${LIBRARY_TYPE}_LDFLAGS
            #GBM${LIBRARY_TYPE}_LDFLAGS_OTHER
            GBM${LIBRARY_TYPE}_LIBRARIES
            GBM${LIBRARY_TYPE}_LIBRARY_DIRS
        VERSION_VAR
            GBM_VERSION
    )

    if(NOT GBM_FOUND)
        message(FATAL_ERROR "Some required variable(s) is (are) not found / set! Does gbm.pc exist? Does it contain the compiler and linker search path to compile against the librarys, are library names provided, and are compiler flags set?")
    endif()

    # Just being cautious; only continue if the target does not already exist
    if(TARGET gbm::gbm)
        message(FATAL_ERROR "The gbm::gbm target has already be created somewhere.")
    endif()

    if(NOT DEFINED TARGET_NAME)
        set(TARGET_NAME gbm::gbm)
    else()
        message(FATAL_ERROR, "Unable to define the target variable TARGET_NAME. It probably, exists.")
    endif()

    # Alternative to PkgConfig::gbm; this is not going to be build
    # Define the 'imported' library gbm
    if(BUILD_SHARED_LIBS)
        add_library(
            ${TARGET_NAME}
            SHARED
            IMPORTED
        )
    else()
        message(AUTHOR_WARNING "static libs")
        add_library(
            ${TARGET_NAME}
            STATIC
            IMPORTED
        )
    endif()

    # Pre-processor definitions to compile against the library headers
    if(CMAKE_VERSION VERSION_LESS 3.11)
        set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS ""
        )
    else()
        target_compile_definitions(
            ${TARGET_NAME}
            INTERFACE
                ""
        )
    endif()

    # Include directives to compile against the library headers
    if(CMAKE_VERSION VERSION_LESS 3.11)
        set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${GBM${LIBRARY_TYPE}_INCLUDE_DIRS}"
        )
    else()
        target_include_directories(
            ${TARGET_NAME}
            INTERFACE
                "${GBM${LIBRARY_TYPE}_INCLUDE_DIRS}"
        )
    endif()

    # Compiler options required to compile against the library headers
    if(CMAKE_VERSION VERSION_LESS 3.11)
        set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${GBM${LIBRARY_TYPE}_CFLAGS_OTHER}"
        )
    else()
        target_compile_options(
            ${TARGET_NAME}
            INTERFACE
                "${GBM${LIBRARY_TYPE}_CFLAGS_OTHER}"
        )
    endif()

    # Linker search paths
    if(NOT CMAKE_VERSION VERSION_LESS_EQUAL 3.13)
        target_link_directories(
            ${TARGET_NAME}
            INTERFACE
                "${GBM${LIBRARY_TYPE}_LIBRARY_DIRS}"
        )
    else()
        set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
                INTERFACE_LINK_DIRECTORIES "${GBM${LIBRARY_TYPE}_LIBRARY_DIRS}"
        )
    endif()

    # Linker or archiver options
    if(BUILD_SHARED_LIBS)
        if(NOT CMAKE_VERSION VERSION_LESS_EQUAL 3.13)
            # Linker flags / options
            target_link_options(
                ${TARGET_NAME}
                INTERFACE
                    "${GBM${LIBRARY_TYPE}_LDFLAGS_OTHER}"
            )
        else()
            set_target_properties(
                ${TARGET_NAME}
                PROPERTIES
                    INTERFACE_LINK_OPTIONS "${GBM${LIBRARY_TYPE}_LDFLAGS_OTHER}"
            )
        endif()
    else()
        # Set archiver flagss
        set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
                STATIC_LIBRARY_OPTIONS "${GBM${LIBRARY_TYPE}_LDFLAGS_OTHER}"
        )
    endif()

    # Specify libraries
    if(CMAKE_VERSION VERSION_LESS 3.11)
        set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
                INTERFACE_LINK_LIBRARIES "${GBM${LIBRARY_TYPE}_LIBRARIES}"
        )
    else()
        target_link_libraries(
            ${TARGET_NAME}
            INTERFACE
                "${GBM${LIBRARY_TYPE}_LIBRARIES}"
        )
    endif()

    # Unset all (global) variables to void leaking
    unset(TARGET_NAME)

    if(NOT BUILD_SHARED_LIBS)
        unset(LIBRARY_TYPE)
    endif()

    # A lot more remaining!

else()

    message(FATAL_ERROR "Unable to locate PkgConfig")

endif()
