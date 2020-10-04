if (DEFINED ENV{ASAMPL_DIR})
    set(ASAMPL_DIR $ENV{ASAMPL_DIR})
else()
    ExternalProject_Add(
        asampl
        GIT_REPOSITORY https://github.com/Asampl-development-team/Asampl
        GIT_TAG interpreter-handler
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND "")

    ExternalProject_Get_Property(asampl SOURCE_DIR)
    set(ASAMPL_DIR ${SOURCE_DIR}/Asampl)
endif()

add_library(Asampl INTERFACE IMPORTED)
add_dependencies(Asampl asampl)

include_directories(${ASAMPL_DIR}/include)
