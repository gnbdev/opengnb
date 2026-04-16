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

#ifndef XOR_H
#define XOR_H

#include <stddef.h>

void xor_crypto(unsigned char *crypto_key, unsigned char *data, unsigned int len);
void xor_crypto_copy(unsigned char *crypto_key, unsigned char *dest, unsigned char *src, unsigned int len);

void gnb_xor_expand_key(const unsigned char *key64, unsigned char *expanded, size_t expanded_size);
void gnb_xor_crypto_fast(const unsigned char *key_expanded, size_t key_expanded_size,
                         unsigned char *data, size_t len);

#endif
