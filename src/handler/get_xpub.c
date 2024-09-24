#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include "cx.h"

#include "get_xpub.h"

#include "../globals.h"
#include "../types.h"
#include "../io.h"
#include "../sw.h"
#include "../common/buffer.h"
#include "../ui/display.h"
#include "../helper/send_response.h"
#include "hathor.h"

void derive_xpub() {
    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};

    uint8_t chain_code[32];
    uint8_t hash_160[20];
    uint32_t error = CX_OK;

    uint32_t parent_path[MAX_BIP32_PATH] = {0};

    // derive for bip32 path
    CX_CHECK(derive_private_key(&private_key,
                                chain_code,
                                G_context.bip32_path.path,
                                G_context.bip32_path.length));
    CX_CHECK(init_public_key(&private_key, &public_key));

    const size_t pubk_buflen =
        sizeof(G_context.pk_info.raw_public_key) / sizeof(G_context.pk_info.raw_public_key[0]);
    if (pubk_buflen < public_key.W_len) {
        error = CX_INTERNAL_ERROR;
        goto end;
    }

    memmove(G_context.pk_info.raw_public_key, public_key.W, public_key.W_len);
    // pk_info.chain_code is a static buffer with size 32
    memmove(G_context.pk_info.chain_code, chain_code, CHAINCODE_LEN);

    // derive parent
    for (int i = 0; i < G_context.bip32_path.length - 1; i++) {
        parent_path[i] = G_context.bip32_path.path[i];
    }
    CX_CHECK(
        derive_private_key(&private_key, chain_code, parent_path, G_context.bip32_path.length - 1));
    CX_CHECK(init_public_key(&private_key, &public_key));

    // fingerprint is first 4 bytes of hash160
    CX_CHECK(compress_public_key(public_key.W, sizeof(public_key.W) / sizeof(public_key.W[0])));
    CX_CHECK(hash160(public_key.W, 33, hash_160, 20));
    // pk_info.fingerprint is a static buffer with size 4
    memmove(G_context.pk_info.fingerprint, hash_160, FINGERPRINT_LEN);

end:
    explicit_bzero(&private_key, sizeof(private_key));
    explicit_bzero(&public_key, sizeof(public_key));
    explicit_bzero(&chain_code, sizeof(public_key));
    explicit_bzero(&hash_160, sizeof(hash_160));

    if (error != CX_OK) {
        THROW(SW_INTERNAL_ERROR);
    }
}

int handler_get_xpub(buffer_t *cdata) {
    explicit_bzero(&G_context, sizeof(G_context));
    G_context.req_type = CONFIRM_XPUB;
    G_context.state = STATE_NONE;

    if (!buffer_read_bip32_path(cdata, &G_context.bip32_path)) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    derive_xpub();

    return ui_display_xpub_confirm();
}
