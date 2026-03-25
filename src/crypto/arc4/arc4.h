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

#ifndef ARC4_H
#define ARC4_H

struct arc4_sbox {
	unsigned int x;
	unsigned int y;
    unsigned char data[256];
};


void arc4_init(struct arc4_sbox *sbox, unsigned char *key, unsigned int len);
void arc4_crypt(struct arc4_sbox *sbox, unsigned char*data, unsigned int len);

#endif
