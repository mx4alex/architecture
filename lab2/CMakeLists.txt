cmake_minimum_required(VERSION 3.12...3.31)
project(budget_service CXX)

find_package(userver COMPONENTS core REQUIRED)

include(FetchContent)
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.2
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JWT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jwt-cpp)

userver_setup_environment()

add_library(${PROJECT_NAME}_objs OBJECT
    src/models/user.hpp
    src/models/budget.hpp
    src/storage/user/user_storage.hpp
    src/storage/user/user_storage.cpp
    src/storage/budget/budget_storage.hpp
    src/storage/budget/budget_storage.cpp
    src/utils/jwt.hpp
    src/utils/jwt.cpp
    src/handlers/auth/auth_handler.hpp
    src/handlers/auth/auth_handler.cpp
    src/handlers/user/user_handler.hpp
    src/handlers/user/user_handler.cpp
    src/handlers/budget/budget_handler.hpp
    src/handlers/budget/budget_handler.cpp
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core jwt-cpp::jwt-cpp)
target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

userver_testsuite_add_simple()

include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
    message(STATUS "Set install prefix: $ENV{PREFIX}")
    file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
    set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${PROJECT_NAME})
install(FILES ${CONFIGS_FILES} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME} COMPONENT ${PROJECT_NAME})
