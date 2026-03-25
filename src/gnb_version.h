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

#ifndef GNB_VERSION_H
#define GNB_VERSION_H

#define GNB_VERSION_STRING    "GNB version Dev 1.6.4  protocol version 1.6.0"
#define GNB_COPYRIGHT_STRING  "Copyright (C) 2019 gnbdev<gnbdev@qq.com>"
#define GNB_URL_STRING        "https://github.com/opengnb/opengnb"

#ifndef GNB_SKIP_BUILD_TIME
#define GNB_BUILD_STRING  "Build Time ["__DATE__","__TIME__"]"
#else
#define GNB_BUILD_STRING  "Build Time [Hidden]"
#endif

#endif
