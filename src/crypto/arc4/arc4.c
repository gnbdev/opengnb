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

#include "arc4.h"

//参考来自这里的信息
//https://en.wikipedia.org/wiki/RC4

void arc4_init(struct arc4_sbox *sbox, unsigned char *key, unsigned int len) {

    register unsigned int x = 0;
    register unsigned int y = 0;

    sbox->x = 0;
    sbox->y = 0;

    for ( x=0; x<256; x++ ) {
        sbox->data[x] = x;
    }

    for ( x=0; x<256; x++ ) {

        y = ( y + sbox->data[x] + key[x%len]) % 256;
        
        sbox->data[x] ^= sbox->data[y];
        sbox->data[y] ^= sbox->data[x];
        sbox->data[x] ^= sbox->data[y];

    }

}

void arc4_crypt(struct arc4_sbox *sbox, unsigned char*data, unsigned int len) {

    register unsigned int  idx;
    register unsigned char k;

    register unsigned int  x;
    register unsigned int  y;

    x = sbox->x;
    y = sbox->y;

    for( idx=0; idx<len; idx++ ){

        x = (x+1) % 256;
        y = (y + sbox->data[x]) % 256;
        
        sbox->data[x] ^= sbox->data[y];
        sbox->data[y] ^= sbox->data[x];
        sbox->data[x] ^= sbox->data[y];
        
        k = sbox->data[ ( sbox->data[x] + sbox->data[y] ) % 256];

        data[idx] ^= k;

    }

    sbox->x = x;
    sbox->y = y;

}
