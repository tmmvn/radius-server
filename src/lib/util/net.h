#pragma once
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/** Structures and functions for parsing raw network packets
 *
 * @file src/lib/util/net.h
 *
 * @author Arran Cudbard-Bell (a.cudbardb@freeradius.org)
 * @copyright 2014 Arran Cudbard-Bell (a.cudbardb@freeradius.org)
 */
RCSIDH(net_h, "$Id$")

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_LIBPCAP
#  include <pcap.h>
#endif

#include <freeradius-devel/build.h>
#include <freeradius-devel/missing.h>
#include <freeradius-devel/util/hash.h>
#include <freeradius-devel/util/misc.h>
#include <freeradius-devel/util/strerror.h>
#include <freeradius-devel/util/table.h>

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

/*
 *	If we don't have libpcap, we still need an enumeration of link layers.
 */
#ifndef HAVE_LIBPCAP
typedef enum {
	DLT_RAW,
	DLT_NULL,
	DLT_LOOP,
	DLT_EN10MB,
	DLT_LINUX_SLL,
	DLT_PFLOG
} fr_dlt;
#endif

/*
 *	The number of bytes in an ethernet (MAC) address.
 */
#define ETHER_ADDR_LEN 	6

/*
 *	Length of a DEC/Intel/Xerox or 802.3 Ethernet header.
 *	Note that some compilers may pad "struct ether_header" to
 *	a multiple of 4 *bytes, for example, so "sizeof (struct
 *	ether_header)" may not give the right answer.
 *
 *	6 Byte SRC, 6 Byte DST, 2 Byte Ether type, 4 Byte CVID, 4 Byte SVID
 */
#define ETHER_HDR_LEN	22
#define IP_HDR_LEN	60

/*
 *	The number of bytes in a RADIUS packet header.
 */
#define RADIUS_HEADER_LENGTH	20

/*
 *	RADIUS packet length.
 *	RFC 2865, Section 3., subsection 'length' says:
 *	" ... and maximum length is 4096."
 */
#define MAX_RADIUS_LEN	4096
#define MIN_RADIUS_LEN	20
#define RADIUS_AUTH_VECTOR_LENGTH	16


#define IP_V(ip)	(((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)       ((ip)->ip_vhl & 0x0f)

#define IP_VHL(v, hl) ((v & 0x0f) << 4) | (hl & 0x0f)

#define	I_DF		0x4000		//!< Dont fragment flag.
#define IP_MF		0x2000		//!< More fragments flag.
#define IP_OFFMASK	0x1fff		//!< Mask for fragmenting bits.

/*
 *	Structure of a DEC/Intel/Xerox or 802.3 Ethernet header.
 */
typedef struct CC_HINT(__packed__) {
	uint8_t		ether_dst[ETHER_ADDR_LEN];
	uint8_t		ether_src[ETHER_ADDR_LEN];
	uint16_t	ether_type;
} ethernet_header_t;

/*
 *	Structure of an internet header, naked of options.
 */
typedef struct CC_HINT(__packed__) {
	uint8_t		ip_vhl;		//!< Header length, version.

	uint8_t		ip_tos;		//!< Type of service.
	uint16_t	ip_len;		//!< Total length.
	uint16_t	ip_id;		//!< identification.
	uint16_t	ip_off;		//!< Fragment offset field.

	uint8_t		ip_ttl;		//!< Time To Live.
	uint8_t		ip_p;		//!< Protocol.
	uint16_t	ip_sum;		//!< Checksum.
	struct in_addr	ip_src, ip_dst;	//!< Src and Dst address
} ip_header_t;

typedef struct CC_HINT(__packed__) {
	uint32_t	ip_vtcfl;	//!< Version, traffic class, flow label.
	uint16_t	ip_len;		//!< Payload length

	uint8_t		ip_next;	//!< Next header (protocol)
	uint8_t		ip_hopl;	//!< IP Hop Limit

	struct in6_addr ip_src, ip_dst;	//!< Src and Dst address
} ip_header6_t;

/*
 *	UDP protocol header.
 *	Per RFC 768, September, 1981.
 */
typedef struct CC_HINT(__packed__) {
	uint16_t	src;		//!< Source port.
	uint16_t	dst;		//!< Destination port.
	uint16_t	len;		//!< UDP length.
	uint16_t	checksum;	//!< UDP checksum.
} udp_header_t;

extern fr_table_num_sorted_t const fr_net_ip_proto_table[];
extern size_t fr_net_ip_proto_table_len;
extern fr_table_num_sorted_t const fr_net_sock_type_table[];
extern size_t fr_net_sock_type_table_len;
extern fr_table_num_sorted_t const fr_net_af_table[];
extern size_t fr_net_af_table_len;

uint16_t	fr_udp_checksum(uint8_t const *data, uint16_t len, uint16_t checksum,
			 	struct in_addr const src_addr, struct in_addr const dst_addr);
int		fr_udp_header_check(uint8_t const *data, uint16_t remaining, ip_header_t const *ip);
uint16_t	fr_ip_header_checksum(uint8_t const *data, uint8_t ihl);

static inline void fr_put_be16(uint8_t a[static sizeof(uint16_t)], uint16_t val)
{
	a[0] = (val >> 8) & 0xff;
	a[1] = val & 0xff;
}

static inline void fr_put_be32(uint8_t a[static sizeof(uint32_t)], uint32_t val)
{
	fr_put_be16(a, (uint16_t) (val >> 16));
	fr_put_be16(a + sizeof(uint16_t), (uint16_t) val);
}

static inline void fr_put_be64(uint8_t a[static sizeof(uint64_t)], uint64_t val)
{
	fr_put_be32(a, (uint32_t) (val >> 32));
	fr_put_be32(a + sizeof(uint32_t), (uint32_t) val);
}

static inline uint16_t fr_get_be16(uint8_t const a[static sizeof(uint16_t)])
{
	return (((uint16_t) a[0]) << 8) | a[1];
}

static inline uint32_t fr_get_be32(uint8_t const a[static sizeof(uint32_t)])
{
	return ((uint32_t) fr_get_be16(a) << 16) | fr_get_be16(a + sizeof(uint16_t));
}

static inline uint64_t fr_get_be64(uint8_t const a[static sizeof(uint64_t)])
{
	return ((uint64_t) fr_get_be32(a) << 32) | fr_get_be32(a + sizeof(uint32_t));
}


/** Encode a 64bit unsigned integer as a big endian number in the fewest bytes possible
 *
 * @param[out] out	Where to write big endian encoding of num.
 *			Should be at least 8 bytes.
 * @param[in] num	Number to encode.
 * @return
 *	- The number of bytes written to out.
 */
static inline size_t htonx(uint8_t *out, uint64_t num)
{
	size_t ret;

	/*
	 *	ffsll isn't POSIX, but it's in at least
	 *	Linux, FreeBSD, OpenBSD and Solaris.
	 *
	 *	If we really care, implementing it
	 *	in missing.c is trivial.
	 *
	 *	This version however should compile down
	 *	to a single CPU instruction on supported
	 *	platforms.
	 */
	ret = ROUND_UP_DIV((size_t)ffsll((long long)num), 8);
	switch (ret) {
	case 8:
		out[7] = (num & 0xFF00000000000000) >> 56;
	/* FALL-THROUGH */
	case 7:
		out[6] = (num & 0xFF000000000000) >> 48;
	/* FALL-THROUGH */
	case 6:
		out[5] = (num & 0xFF0000000000) >> 40;
	/* FALL-THROUGH */
	case 5:
		out[4] = (num & 0xFF00000000) >> 32;
	/* FALL-THROUGH */
	case 4:
		out[3] = (num & 0xFF000000) >> 24;
	/* FALL-THROUGH */
	case 3:
		out[2] = (num & 0xFF0000) >> 16;
	/* FALL-THROUGH */
	case 2:
		out[1] = (num & 0xFF00) >> 8;
	/* FALL-THROUGH */
	case 1:
		out[0] = (num & 0xFF);
		return ret;

	case 0:
		out[0] = 0;
		return 1;
	}

	return 0;
}

/** Convert a big endian number of variable size to a unsigned 64bit integer of native endianness
 *
 * @param[in] data	Buffer containing the number.
 * @param[in] data_len	Length of number.
 * @return unsigned 64bit integer.
 */
static inline uint64_t ntohx(uint8_t const *data, size_t data_len)
{
	uint64_t ret = 0;

	switch (data_len) {
	case 8:
		ret += ((uint64_t)data[7]) << 56;
	/* FALL-THROUGH */
	case 7:
		ret += ((uint64_t)data[6]) << 48;
	/* FALL-THROUGH */
	case 6:
		ret += ((uint64_t)data[5]) << 40;
	/* FALL-THROUGH */
	case 5:
		ret += ((uint64_t)data[4]) << 32;
	/* FALL-THROUGH */
	case 4:
		ret += ((uint64_t)data[3]) << 24;
	/* FALL-THROUGH */
	case 3:
		ret += ((uint64_t)data[2]) << 16;
	/* FALL-THROUGH */
	case 2:
		ret += ((uint64_t)data[1]) << 8;
	/* FALL-THROUGH */
	case 1:
		ret += data[8];
		return ret;
	}

	return 0;
}
#ifdef __cplusplus
}
#endif
