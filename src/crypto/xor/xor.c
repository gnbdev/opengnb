/*
   Copyright (C) gnbdev

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>

void xor_crypto(unsigned char *crypto_key, unsigned char *data, unsigned int len) {
    int i;
    int j = 0;
    for ( i=0; i<len; i++ ) {
        *data = *data ^ crypto_key[j];
        data++;
        j++;
        if ( j >= 64 ) {
            j = 0;
        }
    }
}

void xor_crypto_copy(unsigned char *crypto_key, unsigned char *dest, unsigned char *src, unsigned int len) {
    int i;
    int j = 0;
    for ( i=0; i<len; i++ ) {
        *dest = *src ^ crypto_key[j];
        src++;
        dest++;
        j++;
        if ( j >= 64 ) {
            j = 0;
        }
    }
}

void gnb_xor_expand_key(const unsigned char *key64, unsigned char *expanded, size_t expanded_size) {
    size_t i;
    for (i = 0; i < expanded_size; i++) {
        expanded[i] = key64[i & 63];
    }
}

void gnb_xor_crypto_fast(const unsigned char *key_expanded, size_t key_expanded_size,
                         unsigned char *data, size_t len) {
    size_t offset = 0;

    while (offset < len) {
        size_t remaining = len - offset;
        size_t chunk = remaining < key_expanded_size ? remaining : key_expanded_size;
        size_t i = 0;

        while (i < chunk && (((uintptr_t)(data + offset + i)) & 7u)) {
            data[offset + i] ^= key_expanded[i];
            i++;
        }

#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ >= 8)
        for (; i + 8 <= chunk; i += 8) {
            uint64_t d, k;
            memcpy(&d, data + offset + i, 8);
            memcpy(&k, key_expanded + i, 8);
            d ^= k;
            memcpy(data + offset + i, &d, 8);
        }
#else
        for (; i + 4 <= chunk; i += 4) {
            uint32_t d, k;
            memcpy(&d, data + offset + i, 4);
            memcpy(&k, key_expanded + i, 4);
            d ^= k;
            memcpy(data + offset + i, &d, 4);
        }
#endif
        for (; i < chunk; i++) {
            data[offset + i] ^= key_expanded[i];
        }

        offset += chunk;
    }
}
