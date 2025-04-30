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
