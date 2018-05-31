//
// Created by c1 on 15.01.18.
//

#ifndef NETWORK_SGX_EXAMPLE_SERVER_ERRORS_H
#define NETWORK_SGX_EXAMPLE_SERVER_ERRORS_H

#include "sgx_urts.h"
#include <cstring>
#include <cstdio>
#include <cstdarg>

/*
 * h_printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void h_printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
        {
                SGX_ERROR_UNEXPECTED,
                "Unexpected error occurred.",
                nullptr
        },
        {
                SGX_ERROR_INVALID_PARAMETER,
                "Invalid parameter.",
                nullptr
        },
        {
                SGX_ERROR_OUT_OF_MEMORY,
                "Out of memory.",
                nullptr
        },
        {
                SGX_ERROR_ENCLAVE_LOST,
                "Power transition occurred.",
                "Please refer to the sample \"PowerTransition\" for details."
        },
        {
                SGX_ERROR_INVALID_ENCLAVE,
                "Invalid enclave image.",
                nullptr
        },
        {
                SGX_ERROR_INVALID_ENCLAVE_ID,
                "Invalid enclave identification.",
                nullptr
        },
        {
                SGX_ERROR_INVALID_SIGNATURE,
                "Invalid enclave signature.",
                nullptr
        },
        {
                SGX_ERROR_OUT_OF_EPC,
                "Out of EPC memory.",
                nullptr
        },
        {
                SGX_ERROR_NO_DEVICE,
                "Invalid SGX device.",
                "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
        },
        {
                SGX_ERROR_MEMORY_MAP_CONFLICT,
                "Memory map conflicted.",
                nullptr
        },
        {
                SGX_ERROR_INVALID_METADATA,
                "Invalid enclave metadata.",
                nullptr
        },
        {
                SGX_ERROR_DEVICE_BUSY,
                "SGX device was busy.",
                nullptr
        },
        {
                SGX_ERROR_INVALID_VERSION,
                "Enclave version was invalid.",
                nullptr
        },
        {
                SGX_ERROR_INVALID_ATTRIBUTE,
                "Enclave was not authorized.",
                nullptr
        },
        {
                SGX_ERROR_ENCLAVE_FILE_ACCESS,
                "Can't open enclave file.",
                nullptr
        },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
    unsigned long idx = 0;
    unsigned long ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if (ret == sgx_errlist[idx].err) {
            if (NULL != sgx_errlist[idx].sug)
                h_printf("Info: %s\n", sgx_errlist[idx].sug);
            h_printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
        h_printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n",
               ret);
}


#endif //NETWORK_SGX_EXAMPLE_SERVER_ERRORS_H
