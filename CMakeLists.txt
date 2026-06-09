cmake_minimum_required(VERSION 3.20)

project(my_dbus_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(sdbus-c++ QUIET)
if(TARGET SDBusCpp::sdbus-c++)
    set(SDBUSCPP_TARGET SDBusCpp::sdbus-c++)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDBUSCPP REQUIRED IMPORTED_TARGET sdbus-c++)
    set(SDBUSCPP_TARGET PkgConfig::SDBUSCPP)
endif()

find_package(sdbus-c++-tools QUIET)
if(TARGET SDBusCpp::sdbus-c++-xml2cpp)
    set(SDBUSCPP_XML2CPP SDBusCpp::sdbus-c++-xml2cpp)
else()
    find_program(SDBUSCPP_XML2CPP sdbus-c++-xml2cpp REQUIRED)
endif()

set(DBUS_XML ${CMAKE_CURRENT_SOURCE_DIR}/dbus/com.example.Demo.xml)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(ADAPTOR_GLUE ${GENERATED_DIR}/demo-adaptor-glue.h)
set(PROXY_GLUE ${GENERATED_DIR}/demo-proxy-glue.h)

file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
    OUTPUT ${ADAPTOR_GLUE} ${PROXY_GLUE}
    COMMAND ${SDBUSCPP_XML2CPP} ${DBUS_XML}
            --adaptor=${ADAPTOR_GLUE}
            --proxy=${PROXY_GLUE}
    DEPENDS ${DBUS_XML}
    COMMENT "Generating sdbus-c++ bindings from ${DBUS_XML}"
    VERBATIM
)

add_custom_target(dbus_glue DEPENDS ${ADAPTOR_GLUE} ${PROXY_GLUE})

add_executable(demo-server src/server.cpp ${ADAPTOR_GLUE} ${PROXY_GLUE})
target_include_directories(demo-server PRIVATE ${GENERATED_DIR})
target_link_libraries(demo-server PRIVATE ${SDBUSCPP_TARGET})
add_dependencies(demo-server dbus_glue)

add_executable(demo-client src/client.cpp ${PROXY_GLUE} ${ADAPTOR_GLUE})
target_include_directories(demo-client PRIVATE ${GENERATED_DIR})
target_link_libraries(demo-client PRIVATE ${SDBUSCPP_TARGET})
add_dependencies(demo-client dbus_glue)
