project(login_server)

set(ENCLAVES_UNTRUSTED_SOURCE_FILES ${PROJECT_SOURCE_DIR}/untrusted/enclave_u.c
        ${PROJECT_SOURCE_DIR}/untrusted/enclave_u.h)

add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/trusted/enclave_t.c ${PROJECT_SOURCE_DIR}/trusted/enclave_t.h
        COMMAND ${SGX_EDGER8R}
        --trusted ../edl/enclave.edl --search-path ../edl --search-path ${SGX_SDK}/include
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/trusted
        DEPENDS ${PROJECT_SOURCE_DIR}/edl/enclave.edl
        COMMENT "Create *_t.c/.h files for Enclave")

add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/untrusted/enclave_u.c ${PROJECT_SOURCE_DIR}/untrusted/enclave_u.h
        COMMAND ${SGX_EDGER8R}
        --untrusted ../edl/enclave.edl --search-path ../edl --search-path ${SGX_SDK}/include
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/untrusted
        DEPENDS ${PROJECT_SOURCE_DIR}/edl/enclave.edl
        COMMENT "Create *_u.c/.h files for Enclave")

set(ENCLAVE_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/trusted/enclave_t.c
        ${PROJECT_SOURCE_DIR}/trusted/enclave_t.h
        ${PROJECT_SOURCE_DIR}/untrusted/enclave_u.h
        trusted/server_enclave.cpp
        ../include/cryptlib.cpp ../include/cryptlib.h
        ../include/shared_structs.h ../include/shared_functions.h)

set(Enclave_Link_flags ${Common_Enclave_Link_Flags} -Wl,--version-script=${PROJECT_SOURCE_DIR}/settings/enclave.lds)

add_library(enclave_server SHARED ${ENCLAVE_SOURCE_FILES})

set_target_properties(enclave_server PROPERTIES COMPILE_FLAGS "${Enclave_Compile_Flags}")
target_include_directories(enclave_server PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/trusted
        ${SGX_SDK}/include ${SGX_SDK}/include/tlibc ${SGX_SDK}/include/libcxx
        include_directories(${SGX_SDK}/include/stlport))
set_target_properties(enclave_server PROPERTIES LINK_FLAGS "")
set_target_properties(enclave_server PROPERTIES LINK_FLAGS_DEBUG "")
target_link_libraries(enclave_server ${Enclave_Link_flags})
set_target_properties(enclave_server PROPERTIES PREFIX "")

add_custom_command(TARGET enclave_server POST_BUILD
        COMMAND ${SGX_ENCLAVE_SIGNER} sign
        -key enclave_private.pem
        -enclave ${PROJECT_BINARY_DIR}/enclave_server.so
        -out ${PROJECT_BINARY_DIR}/enclave_server.signed.so
        -config enclave.config.xml
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/settings
        COMMENT "SIGNED ENCLAVE")


### ZEROMQ DEPENDENCIES ###
find_package(cppzmq)
if (cppzmq_FOUND)
    include_directories(${cppzmq_INCLUDE_DIR})
endif ()


### BUILD APP ###
set(APP_SOURCE_FILES
        untrusted/main.cpp
        untrusted/enclave_u.h
        untrusted/enclave_u.c
        ../include/errors.h untrusted/network/network_manager_server.cpp untrusted/network/network_manager_server.h untrusted/server.cpp untrusted/server.h)

add_executable(login_server ${APP_SOURCE_FILES})

set_target_properties(login_server PROPERTIES COMPILE_FLAGS "${APP_COMPILE_FLAGS}")
target_include_directories(login_server PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/untrusted)

set_target_properties(login_server PROPERTIES LINK_FLAGS "${SGX_COMMON_CFLAGS}")
target_link_libraries(login_server ${SGX_URTS_LIB} pthread ${SGX_UAE_SERVICE} ${ZeroMQ_LIBRARY} ${cppzmq_LIBRARY})
add_dependencies(login_server enclave_server)
