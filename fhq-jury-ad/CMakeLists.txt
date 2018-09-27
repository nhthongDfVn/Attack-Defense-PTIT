cmake_minimum_required(VERSION 3.0)

project(fhq-jury-ad)

add_definitions(-DJURY_AD_VERSION="0.0.2")
add_definitions(-DJURY_AD_APP_NAME="fhq-jury-ad")

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${fhq-jury-ad_SOURCE_DIR})
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set (JURY_AD_LIBRARIES "")
set (JURY_AD_INCLUDE_DIRS "")
set (JURY_AD_SOURCES "")

# Sources

# include header dirs
list (APPEND JURY_AD_INCLUDE_DIRS "src")
list (APPEND JURY_AD_INCLUDE_DIRS "src/models")
list (APPEND JURY_AD_INCLUDE_DIRS "src/utils")
list (APPEND JURY_AD_INCLUDE_DIRS "src/interfaces")
list (APPEND JURY_AD_INCLUDE_DIRS "src/storage")
list (APPEND JURY_AD_INCLUDE_DIRS "src/scoreboard")
list (APPEND JURY_AD_INCLUDE_DIRS "src/3rd_party/nlohmann")
list (APPEND JURY_AD_INCLUDE_DIRS "src/3rd_party/INIReader")
list (APPEND JURY_AD_INCLUDE_DIRS "src/3rd_party/HowardHinnant/date")

# nlohmann::json
list (APPEND JURY_AD_SOURCES "src/3rd_party/nlohmann/json.hpp")


# HowardHinnant/date
list (APPEND JURY_AD_SOURCES "src/3rd_party/HowardHinnant/date/date.h")

# INIReader
list (APPEND JURY_AD_SOURCES "src/3rd_party/INIReader/ini.c")
list (APPEND JURY_AD_SOURCES "src/3rd_party/INIReader/INIReader.cpp")
list (APPEND JURY_AD_SOURCES "src/3rd_party/INIReader/ini.h")
list (APPEND JURY_AD_SOURCES "src/3rd_party/INIReader/INIReader.h")

# interfaces
list (APPEND JURY_AD_SOURCES "src/interfaces/istorage.h")
list (APPEND JURY_AD_SOURCES "src/interfaces/ilighthttprequest.h")
list (APPEND JURY_AD_SOURCES "src/interfaces/ilighthttphandler.h")

# scoreboard
list (APPEND JURY_AD_SOURCES "src/scoreboard/http_handler.h")
list (APPEND JURY_AD_SOURCES "src/scoreboard/http_handler.cpp")

# models
list (APPEND JURY_AD_SOURCES "src/models/model_flag.h")
list (APPEND JURY_AD_SOURCES "src/models/model_flag.cpp")
list (APPEND JURY_AD_SOURCES "src/models/model_scoreboard.h")
list (APPEND JURY_AD_SOURCES "src/models/model_scoreboard.cpp")
list (APPEND JURY_AD_SOURCES "src/models/model_service_conf.h")
list (APPEND JURY_AD_SOURCES "src/models/model_service_conf.cpp")
list (APPEND JURY_AD_SOURCES "src/models/model_service_status.h")
list (APPEND JURY_AD_SOURCES "src/models/model_service_status.cpp")
list (APPEND JURY_AD_SOURCES "src/models/model_team_conf.h")
list (APPEND JURY_AD_SOURCES "src/models/model_team_conf.cpp")
list (APPEND JURY_AD_SOURCES "src/models/model_team_status.h")
list (APPEND JURY_AD_SOURCES "src/models/model_team_status.cpp")

# utils
list (APPEND JURY_AD_SOURCES "src/utils/colormod.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_dorunchecker.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_dorunchecker.cpp")
list (APPEND JURY_AD_SOURCES "src/utils/utils_help_parse_args.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_help_parse_args.cpp")
list (APPEND JURY_AD_SOURCES "src/utils/utils_light_http_request.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_light_http_request.cpp")
list (APPEND JURY_AD_SOURCES "src/utils/utils_light_http_server.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_light_http_server.cpp")
list (APPEND JURY_AD_SOURCES "src/utils/utils_logger.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_logger.cpp")
list (APPEND JURY_AD_SOURCES "src/utils/utils_search_lazy_conf.h")
list (APPEND JURY_AD_SOURCES "src/utils/utils_search_lazy_conf.cpp")

# ram storage
list (APPEND JURY_AD_SOURCES "src/storage/ram_storage.h")
list (APPEND JURY_AD_SOURCES "src/storage/ram_storage.cpp")

# main
list (APPEND JURY_AD_SOURCES "src/main.cpp")
list (APPEND JURY_AD_SOURCES "src/config.h")
list (APPEND JURY_AD_SOURCES "src/config.cpp")
list (APPEND JURY_AD_SOURCES "src/service_checker_thread.h")
list (APPEND JURY_AD_SOURCES "src/service_checker_thread.cpp")


############################
##### mysql storage
list (APPEND JURY_AD_SOURCES "src/storage/mysql_storage.h")
list (APPEND JURY_AD_SOURCES "src/storage/mysql_storage.cpp")

# Find and make sure the system have the header file
find_path(MYSQL_HEADER mysql/mysql.h)
if(MYSQL_HEADER STREQUAL "MYSQL_HEADER-NOTFOUND")
    message(FATAL_ERROR "Could not find the mysql/mysql.h header file: apt install libmysqlclient-dev")
endif()

include(FindPkgConfig)
pkg_check_modules(LIBMYSQLCLIENT REQUIRED mysqlclient)

foreach(FLAG ${LIBMYSQLCLIENT_CFLAGS_OTHER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
endforeach()

list (APPEND JURY_AD_INCLUDE_DIRS ${LIBMYSQLCLIENT_LIBRARY_DIRS})
list (APPEND JURY_AD_LIBRARIES ${LIBMYSQLCLIENT_LIBRARIES})

#####
############################

# ZLIB
find_package( ZLIB REQUIRED )
if ( ZLIB_FOUND )
    list (APPEND JURY_AD_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS})
    include_directories( ${ZLIB_INCLUDE_DIRS} )
    list (APPEND JURY_AD_LIBRARIES ${ZLIB_LIBRARIES})
endif( ZLIB_FOUND )

# LIBPNG
find_package(PNG REQUIRED)
if (NOT PNG_FOUND)
    message(FATAL_ERROR "You don't seem to have libpng development libraries installed (apt isntall libpng-dev)")
else ()
    list (APPEND JURY_AD_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
    list (APPEND JURY_AD_LIBRARIES ${PNG_LIBRARY})
endif ()

# CURL
FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  list (APPEND JURY_AD_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
  list (APPEND JURY_AD_LIBRARIES ${CURL_LIBRARIES})
ELSE(CURL_FOUND)
  MESSAGE(FATAL_ERROR "Could not find the CURL library and development files.")
ENDIF(CURL_FOUND)

include_directories(${JURY_AD_INCLUDE_DIRS})

add_executable (fhq-jury-ad ${JURY_AD_SOURCES})

target_link_libraries(fhq-jury-ad -lpthread ${JURY_AD_LIBRARIES} )

install(
    TARGETS
        fhq-jury-ad
    RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/bin
)