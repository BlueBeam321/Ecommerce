/*****************************************************************************
 * udp.c: network socket abstractions
 *****************************************************************************
 * Copyright (C) 2010-2011 M2X BV
 *
 * Authors: Jean-Paul Saman <jpsaman@videolan.org>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *****************************************************************************/

#ifndef DVBINFO_UDP_H_
#define DVBINFO_UDP_H_

int udp_open(const char *interface, const char *ipaddress, int port);
int udp_close(int fd);
ssize_t udp_read(int fd, void *buf, size_t count);

#endif

