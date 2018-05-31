//
// Created by c1 on 05.05.18.
//

#ifndef NETWORK_SGX_EXAMPLE_CONFIG_H
#define NETWORK_SGX_EXAMPLE_CONFIG_H

#define MESSAGE_SIZE 128  // size in bytes (refers to messages injected by users)
#define PSEUDONYM_SIZE 64 // size in bytes
//#define BLOB_SIZE 1024    // size in bytes

#ifndef INCLUDING_FROM_EDL
constexpr size_t kMessageSize{MESSAGE_SIZE};
constexpr size_t kPseudonymSize{PSEUDONYM_SIZE};
//constexpr int kBlobSize{BLOB_SIZE};

#undef MESSAGE_SIZE
#undef PSEUDONYM_SIZE
#undef BLOB_SIZE

/** variable x as in the paper (see definition of beta) */
constexpr int kX{2};
/** variable epsilon as in the paper */
constexpr int kEpsilon{1};

/** variable Delta as in the paper */
constexpr int kDelta{3};
/** variable A_max as in the paper */
constexpr int kAMax{1};
/** variable k_send as in the paper */
constexpr int kSend{1};
/** variable k_recv as in the paper */
constexpr int kRecv{2};

typedef uint64_t round_t;
typedef uint64_t onid_t;

#endif

#endif //NETWORK_SGX_EXAMPLE_CONFIG_H
