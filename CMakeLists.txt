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
set(SERVER_GLUE ${GENERATED_DIR}/demo-server-glue.h)
set(CLIENT_GLUE ${GENERATED_DIR}/demo-client-glue.h)

file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
    OUTPUT ${SERVER_GLUE} ${CLIENT_GLUE}
    COMMAND ${SDBUSCPP_XML2CPP} ${DBUS_XML}
            --adaptor=${SERVER_GLUE}
            --proxy=${CLIENT_GLUE}
    DEPENDS ${DBUS_XML}
    COMMENT "Generating sdbus-c++ bindings from ${DBUS_XML}"
    VERBATIM
)

add_custom_target(dbus_glue DEPENDS ${SERVER_GLUE} ${CLIENT_GLUE})

add_executable(demo-server src/server.cpp ${SERVER_GLUE})
target_include_directories(demo-server PRIVATE ${GENERATED_DIR})
target_link_libraries(demo-server PRIVATE ${SDBUSCPP_TARGET})
add_dependencies(demo-server dbus_glue)

add_executable(demo-client src/client.cpp ${CLIENT_GLUE})
target_include_directories(demo-client PRIVATE ${GENERATED_DIR})
target_link_libraries(demo-client PRIVATE ${SDBUSCPP_TARGET})
add_dependencies(demo-client dbus_glue)
