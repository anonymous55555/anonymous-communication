project(peer)

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
        trusted/client_enclave.cpp
        ../include/shared_structs.h ../include/shared_functions.h trusted/overlay_structure_scheme.cpp trusted/overlay_structure_scheme.h ../include/config.h trusted/structures.h trusted/helpers.h trusted/distributed_agreement_scheme.cpp trusted/distributed_agreement_scheme.h trusted/routing_scheme.cpp trusted/routing_scheme.h ../include/serialization.h ../include/cryptlib.h trusted/structures/aad_tuple.h ../include/cryptlib.cpp)

set(Enclave_Link_flags ${Common_Enclave_Link_Flags} -Wl,--version-script=${PROJECT_SOURCE_DIR}/settings/enclave.lds)

add_library(enclave_client SHARED ${ENCLAVE_SOURCE_FILES})

set_target_properties(enclave_client PROPERTIES COMPILE_FLAGS "${Enclave_Compile_Flags}")
target_include_directories(enclave_client PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/trusted
        ${SGX_SDK}/include ${SGX_SDK}/include/tlibc ${SGX_SDK}/include/libcxx)
set_target_properties(enclave_client PROPERTIES LINK_FLAGS "")
set_target_properties(enclave_client PROPERTIES LINK_FLAGS_DEBUG "")
target_link_libraries(enclave_client ${Enclave_Link_flags})
set_target_properties(enclave_client PROPERTIES PREFIX "")

add_custom_command(TARGET enclave_client POST_BUILD
        COMMAND ${SGX_ENCLAVE_SIGNER} sign
        -key enclave_private.pem
        -enclave ${PROJECT_BINARY_DIR}/enclave_client.so
        -out ${PROJECT_BINARY_DIR}/enclave_client.signed.so
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
        ../include/errors.h untrusted/network/network_manager.cpp untrusted/network/network_manager.h untrusted/client.cpp untrusted/client.h ../include/config.h)

add_executable(peer ${APP_SOURCE_FILES})

set_target_properties(peer PROPERTIES COMPILE_FLAGS "${APP_COMPILE_FLAGS}")
target_include_directories(peer PRIVATE
        ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/untrusted)

set_target_properties(peer PROPERTIES LINK_FLAGS "${SGX_COMMON_CFLAGS}")
target_link_libraries(peer ${SGX_URTS_LIB} pthread ${SGX_UAE_SERVICE} ${ZeroMQ_LIBRARY} ${cppzmq_LIBRARY})
add_dependencies(peer enclave_client)
