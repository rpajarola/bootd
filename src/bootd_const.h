/*
 * $Id: bootd_const.h,v 1.1 2002/07/26 11:24:55 pajarola Exp $
 */

/*
 * ethernet
 */
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif
#ifndef ETHER_TYPE_LEN
#define ETHER_TYPE_LEN 2
#endif
#ifndef ETHER_CRC_LEN
#define ETHER_CRC_LEN 4
#endif
#ifndef ETHER_HDR_LEN
#define ETHER_HDR_LEN           (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)
#endif
#ifndef ETHER_MIN_LEN
#define ETHER_MIN_LEN           64
#endif 
#ifndef ETHER_MAX_LEN
#define ETHER_MAX_LEN           1518
#endif

#ifndef HAVE_STRUCT_ETHER_HEADER
#define HAVE_STRUCT_ETHER_HEADER 1
struct  ether_header {
        u_char  ether_dhost[ETHER_ADDR_LEN];
        u_char  ether_shost[ETHER_ADDR_LEN];
        u_short ether_type;
};
#endif

#ifndef HAVE_STRUCT_ETHER_ADDR
#define HAVE_STRUCT_ETHER_ADDR 1
struct  ether_addr {
        u_char octet[ETHER_ADDR_LEN];
};
#endif

#ifndef ETHERTYPE_PUP
#define ETHERTYPE_PUP           0x0200  /* PUP protocol */
#endif
#ifndef ETHERTYPE_IP
#define ETHERTYPE_IP            0x0800  /* IP protocol */
#endif
#ifndef ETHERTYPE_ARP
#define ETHERTYPE_ARP           0x0806  /* Addr. resolution protocol */
#endif
#ifndef ETHERTYPE_REVARP
#define ETHERTYPE_REVARP        0x8035  /* reverse Addr. resolution protocol */
#endif
#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN          0x8100  /* IEEE 802.1Q VLAN tagging */
#endif
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6          0x86dd  /* IPv6 */
#endif

#ifndef ETHERTYPE_TRAIL
#define ETHERTYPE_TRAIL	0x1000
#endif
#ifndef ETHERTYPE_NTRAILER
#define ETHERTYPE_NTRAILER 16
#endif

#ifndef ETHERMTU
#define ETHERMTU        (ETHER_MAX_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#endif
#ifndef ETHERMIN
#define ETHERMIN        (ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#endif


