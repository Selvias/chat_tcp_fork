set(server_target_name server)
set(client_target_name client)

add_executable(${server_target_name} server/sm_server.cpp)
add_executable(${client_target_name} client/client.cpp)

target_include_directories(
    ${server_target_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    ${client_target_name}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${server_target_name} rt)