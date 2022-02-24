#include <stdint.h>

int estimateFreeHeap(int allocSize);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define htons(n) (n)
#define ntohs(n) (n)
#else
#define htons(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define ntohs(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#endif

// big endian uint16_t handling
class be_uint16_t {
public:
    be_uint16_t() : be_val_(0) {
    }
    // Transparently cast from uint16_t
    be_uint16_t(const uint16_t &val) : be_val_(htons(val)) {
    }
    // Transparently cast to uint16_t
    operator uint16_t() const {
            return ntohs(be_val_);
    }
private:
    uint16_t be_val_;
} __attribute__((packed));

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define htonl(n) (n)
#define ntohl(n) (n)
#else
#define htonl(n) (((((unsigned long)(n) & 0xFF)) << 24) | (((unsigned long)(n) & 0xFF00) << 8) | (((unsigned long)(n) & 0xFF0000) >> 8) | (((unsigned long)(n) & 0xFF000000) >> 24))
#define ntohl(n) htonl(n)
#endif

// big endian uint32_t handling
class be_uint32_t {
public:
    be_uint32_t() : be_val_(0) {
    }
    // Transparently cast from uint32_t
    be_uint32_t(const uint32_t &val) : be_val_(htonl(val)) {
    }
    // Transparently cast to uint32_t
    operator uint32_t() const {
            return ntohl(be_val_);
    }
private:
    uint32_t be_val_;
} __attribute__((packed));