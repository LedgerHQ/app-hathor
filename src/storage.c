#include "storage.h"
#include <string.h>

#include "os.h"

int get_secret(uint8_t secret[static SECRET_LEN]) {
    if (secret == NULL) {
        return 1;
    }
    for (uint8_t i = 0; i < SECRET_LEN; i++) {
        if (*(N_storage.secret + i) != 0) {
            memmove(secret, (const uint8_t*) N_storage.secret, SECRET_LEN);
            return 0;
        }
    }
    // first access, generate and write to storage
    generate_secret();
    memmove(secret, (const uint8_t*) N_storage.secret, SECRET_LEN);
    return 0;
}

void generate_secret() {
    uint8_t new_secret[SECRET_LEN];
    cx_rng_no_throw(new_secret, SECRET_LEN);
    nvm_write((void*) &N_storage.secret, &new_secret, SECRET_LEN);
}
