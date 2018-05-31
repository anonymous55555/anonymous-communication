project(peer_interface)

add_executable(peer_interface main.cpp)

### ZEROMQ DEPENDENCIES ###
find_package(cppzmq)
if (cppzmq_FOUND)
    target_include_directories(peer_interface PRIVATE ${cppzmq_INCLUDE_DIR})
endif ()

target_link_libraries(peer_interface ${ZeroMQ_LIBRARY} ${cppzmq_LIBRARY})

