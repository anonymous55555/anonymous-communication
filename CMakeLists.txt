cmake_minimum_required(VERSION 3.9)
project(network_sgx_example)

set(CMAKE_CXX_STANDARD 17)

set(SGX_SDK /opt/intel/sgxsdk)
set(SGX_MODE SIM)
set(SGX_ARCH x64)
set(SGX_DEBUG ON CACHE BOOL "")
set(SGX_COMMON_CFLAGS "-m64")
set(SGX_LIBRARY_PATH ${SGX_SDK}/lib64)
set(SGX_ENCLAVE_SIGNER ${SGX_SDK}/bin/x64/sgx_sign)
set(SGX_EDGER8R ${SGX_SDK}/bin/x64/sgx_edger8r)

if(${SGX_DEBUG})
    set(SGX_COMMON_CFLAGS "${SGX_COMMON_CFLAGS} -O0 -g")
else(${SGX_DEBUG})
    set(SGX_COMMON_CFLAGS "${SGX_COMMON_CFLAGS} -O2")
endif(${SGX_DEBUG})

set(SGX_TRTS_LIB sgx_trts_sim)
set(SGX_SERVICE_LIB sgx_tservice_sim)
set(SGX_Crypto_Library_Name sgx_tcrypto)
set(SGX_URTS_LIB sgx_urts_sim)
set(SGX_UAE_SERVICE sgx_uae_service_sim)

link_directories(${SGX_LIBRARY_PATH})


### BUILD ENCLAVE ###
set(Enclave_Compile_Flags "${SGX_COMMON_CFLAGS} -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections -fstack-protector-strong")

set(Common_Enclave_Link_Flags "${SGX_COMMON_CFLAGS} \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L${SGX_LIBRARY_PATH} \
    -Wl,--whole-archive -l${SGX_TRTS_LIB} -Wl,--no-whole-archive \
    -Wl,--start-group -lsgx_tstdc -lsgx_tcxx -l${SGX_Crypto_Library_Name} -l${SGX_SERVICE_LIB} -Wl,--end-group \
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
    -Wl,--defsym,__ImageBase=0 -Wl,--gc-sections")

### ZEROMQ DEPENDENCIES ###
find_package(cppzmq)
if (cppzmq_FOUND)
    include_directories(${cppzmq_INCLUDE_DIR})
endif ()

include_directories(${SGX_SDK}/include)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(client_interface)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(shared_struct_test test/shared_structs_test.cpp)
target_include_directories(shared_struct_test PRIVATE ${BOOST_INCLUDE_DIR})
target_link_libraries(shared_struct_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(peer_test test/client_test.cpp client/trusted/overlay_structure_scheme.cpp)
target_include_directories(peer_test PRIVATE ${BOOST_INCLUDE_DIR})
target_link_libraries(peer_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

target_compile_definitions(peer_test PRIVATE OUTSIDE_ENCLAVE)
