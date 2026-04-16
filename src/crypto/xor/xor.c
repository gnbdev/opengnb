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

void gnb_xor_crypto_fast(const unsigned char *key_expanded, unsigned char *data, size_t len) {
    size_t i = 0;

    while (i < len && (((uintptr_t)(data + i)) & 7u)) {
        data[i] ^= key_expanded[i];
        i++;
    }

#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ >= 8)
    for (; i + 8 <= len; i += 8) {
        uint64_t d = *(const uint64_t *)(data + i);
        uint64_t k = *(const uint64_t *)(key_expanded + i);
        *(uint64_t *)(data + i) = d ^ k;
    }
#else
    for (; i + 4 <= len; i += 4) {
        uint32_t d = *(const uint32_t *)(data + i);
        uint32_t k = *(const uint32_t *)(key_expanded + i);
        *(uint32_t *)(data + i) = d ^ k;
    }
#endif

    for (; i < len; i++) {
        data[i] ^= key_expanded[i];
    }
}
