cmake_minimum_required( VERSION 3.14 )

project( UnixSocket LANGUAGES CXX )

###########
### Library
###########
message( "${CYN}Configuring library : ${PROJECT_NAME}${NOMR}" )
file( GLOB CXX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp )
add_library(
    ${PROJECT_NAME}
STATIC
    ${CXX_FILES}
)
target_include_directories(
    ${PROJECT_NAME}
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(
    ${PROJECT_NAME}
LINK_PRIVATE
    Tools
    ${Boost_LIBRARIES}
)

########
### Test
########
message( "${MAG}Configuring test : ${PROJECT_NAME}Test.out${NORM}" )
include_directories( ${Boost_INCLUDE_DIRS} )
message( "${BLU}Boost headers : ${Boost_INCLUDE_DIRS}${NORM}" )
message( "${BLU}Located boost compiled libs : ${Boost_LIBRARIES}${NORM}" )

file( GLOB CXX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp )
add_executable(
    ${PROJECT_NAME}Test.out
    ${CXX_FILES}
)
target_link_libraries(
    ${PROJECT_NAME}Test.out
LINK_PRIVATE
    ${PROJECT_NAME}
    Tools
    ${Boost_LIBRARIES}
)