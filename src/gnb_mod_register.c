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

/*
改名为 pf_register
*/


#include "gnb.h"
#include "gnb_pf.h"


extern gnb_pf_t gnb_pf_dump;
extern gnb_pf_t gnb_pf_route;
extern gnb_pf_t gnb_pf_crypto_xor;
extern gnb_pf_t gnb_pf_crypto_arc4;
extern gnb_pf_t gnb_pf_zip;


gnb_pf_t *gnb_pf_mods[] = {
    &gnb_pf_dump,
    &gnb_pf_route,
    &gnb_pf_crypto_xor,
    &gnb_pf_crypto_arc4,
    &gnb_pf_zip,
    0
};

gnb_pf_t* gnb_find_pf_mod_by_name(const char *name){

    int num =  sizeof(gnb_pf_mods)/sizeof(gnb_pf_t *);

    int i;

    for ( i=0; i<num; i++ ) {

        if (NULL==gnb_pf_mods[i]) {
            break;
        }

        if ( 0 == strncmp(gnb_pf_mods[i]->name,name,128) ) {
            return gnb_pf_mods[i];
        }

    }

    return NULL;

}
