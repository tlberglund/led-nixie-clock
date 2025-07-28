#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#include "pico/stdlib.h"

#define TCPIP_THREAD_PRIO           2
#define TCPIP_THREAD_STACKSIZE      4096
#define DEFAULT_THREAD_STACKSIZE    2048
#define DEFAULT_RAW_RECVMBOX_SIZE   8
#define TCPIP_MBOX_SIZE             8
#define LWIP_TIMEVAL_PRIVATE        0

#define LWIP_SOCKET                 1

#define MEM_LIBC_MALLOC             0
#define MEMP_MEM_MALLOC             0

#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETCONN                1
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_DHCP                   1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define DHCP_DOES_ARP_CHECK         1
#define LWIP_DHCP_DOES_ACD_CHECK    0
#define LWIP_ALTCP                  1
#define LWIP_ALTCP_TLS              1
#define LWIP_ALTCP_TLS_MBEDTLS      1
#define LWIP_ALTCP_PROXY_CONNECT    1
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1
#define LWIP_NETIF_API              1
#define LWIP_CALLBACK_API           1

// We are not storing a root CA chain in flash, so LMAO MitM attacks
#define ALTCP_MBEDTLS_AUTHMODE      MBEDTLS_SSL_VERIFY_OPTIONAL

#define SYS_LIGHTWEIGHT_PROT        1


// #ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
// #endif

// Define the LWIP debug flags
#define LWIP_DBG_ON      0x80U
#define LWIP_DBG_OFF     0x00U
#define LWIP_DBG_TRACE   0x40U
#define LWIP_DBG_STATE   0x20U
#define LWIP_DBG_FRESH   0x10U

// A global "all debug types on" flag; not quite sure how this is used yet
#define LWIP_DBG_TYPES_ON (LWIP_DBG_ON | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_FRESH)


#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF
#define SNTP_DEBUG                  LWIP_DBG_OFF
#define HTTPD_DEBUG                 LWIP_DBG_OFF

#define DEFAULT_TCP_RECVMBOX_SIZE 128


#define SNTP_SUPPORT      1
#define SNTP_SERVER_DNS   1
void sntpSetTimeSec(uint32_t sec);
#define SNTP_SET_SYSTEM_TIME(sec) sntpSetTimeSec(sec)
//MEMP_NUM_SYS_TIMEOUTS Needs to be one larger than default for SNTP
#define MEMP_NUM_SYS_TIMEOUT            (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 1)

// SNTP updates automatically using a timer managed internall by the LWIP stack
#define SNTP_UPDATE_DELAY 60*60*1000


#define LWIP_HTTPD                      1
#define HTTPD_USE_CUSTOM_FSDATA         0
#define LWIP_HTTPD_CGI                  1
#define LWIP_HTTPD_SSI                  0
#define LWIP_HTTPD_MAX_TAG_NAME_LEN     8
#define LWIP_HTTPD_MAX_TAG_INSERT_LEN   192
#define LWIP_HTTPD_DYNAMIC_HEADERS      1
#define LWIP_HTTPD_SUPPORT_POST         1
#define LWIP_HTTPD_POST_MANUAL_WND      1


#define CYW43_VERBOSE_DEBUG 1
#define CYW43_DEBUG
#define CYW43_INFO
#define PICO_CYW43_ARCH_DEBUG_ENABLED 1

#endif