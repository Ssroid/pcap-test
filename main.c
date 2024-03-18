#include <pcap.h>
#include <stdbool.h>
#include <stdio.h>

#define ETHER_ADDR_LEN 6
#define ETHERTYPE_IP 0x0800
#define LIBNET_LIL_ENDIAN 1

void usage() {
    printf("syntax: pcap-test <interface>\n");
    printf("sample: pcap-test wlan0\n");
}

/*
 *  Ethernet II header
 *  Static header size: 14 bytes
 */
struct libnet_ethernet_hdr
{
    u_int8_t  ether_dhost[ETHER_ADDR_LEN];/* destination ethernet address */
    u_int8_t  ether_shost[ETHER_ADDR_LEN];/* source ethernet address */
    u_int16_t ether_type;                 /* protocol */
};

/*
 *  IPv4 header
 *  Internet Protocol, version 4
 *  Static header size: 20 bytes
 */
struct libnet_ipv4_hdr
{
#if (LIBNET_LIL_ENDIAN)
    u_int8_t ip_hl:4,      /* header length */
        ip_v:4;         /* version */
#endif
#if (LIBNET_BIG_ENDIAN)
    u_int8_t ip_v:4,       /* version */
        ip_hl:4;        /* header length */
#endif
    u_int8_t ip_tos;       /* type of service */
#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY      0x10
#endif
#ifndef IPTOS_THROUGHPUT
#define IPTOS_THROUGHPUT    0x08
#endif
#ifndef IPTOS_RELIABILITY
#define IPTOS_RELIABILITY   0x04
#endif
#ifndef IPTOS_LOWCOST
#define IPTOS_LOWCOST       0x02
#endif
    u_int16_t ip_len;         /* total length */
    u_int16_t ip_id;          /* identification */
    u_int16_t ip_off;
#ifndef IP_RF
#define IP_RF 0x8000        /* reserved fragment flag */
#endif
#ifndef IP_DF
#define IP_DF 0x4000        /* dont fragment flag */
#endif
#ifndef IP_MF
#define IP_MF 0x2000        /* more fragments flag */
#endif
#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff   /* mask for fragmenting bits */
#endif
    u_int8_t ip_ttl;          /* time to live */
    u_int8_t ip_p;            /* protocol */
    u_int16_t ip_sum;         /* checksum */
    struct in_addr ip_src, ip_dst; /* source and dest address */
};

/*
 *  TCP header
 *  Transmission Control Protocol
 *  Static header size: 20 bytes
 */
struct libnet_tcp_hdr
{
    u_int16_t th_sport;       /* source port */
    u_int16_t th_dport;       /* destination port */
    u_int32_t th_seq;          /* sequence number */
    u_int32_t th_ack;          /* acknowledgement number */
#if (LIBNET_LIL_ENDIAN)
    u_int8_t th_x2:4,         /* (unused) */
        th_off:4;        /* data offset */
#endif
#if (LIBNET_BIG_ENDIAN)
    u_int8_t th_off:4,        /* data offset */
        th_x2:4;         /* (unused) */
#endif
    u_int8_t  th_flags;       /* control flags */
#ifndef TH_FIN
#define TH_FIN    0x01      /* finished send data */
#endif
#ifndef TH_SYN
#define TH_SYN    0x02      /* synchronize sequence numbers */
#endif
#ifndef TH_RST
#define TH_RST    0x04      /* reset the connection */
#endif
#ifndef TH_PUSH
#define TH_PUSH   0x08      /* push data to the app layer */
#endif
#ifndef TH_ACK
#define TH_ACK    0x10      /* acknowledge */
#endif
#ifndef TH_URG
#define TH_URG    0x20      /* urgent! */
#endif
#ifndef TH_ECE
#define TH_ECE    0x40
#endif
#ifndef TH_CWR
#define TH_CWR    0x80
#endif
    u_int16_t th_win;         /* window */
    u_int16_t th_sum;         /* checksum */
    u_int16_t th_urp;         /* urgent pointer */
};

typedef struct {
    char* dev_;
} Param;

Param param = {
    .dev_ = NULL
};

bool parse(Param* param, int argc, char* argv[]) {
    if (argc != 2) {
        usage();
        return false;
    }
    param->dev_ = argv[1];
    return true;
}

int main(int argc, char* argv[]) {
    if (!parse(&param, argc, argv))
        return -1;

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* pcap = pcap_open_live(param.dev_, BUFSIZ, 1, 1000, errbuf);
    if (pcap == NULL) {
        fprintf(stderr, "pcap_open_live(%s) return null - %s\n", param.dev_, errbuf);
        return -1;
    }

    while (true) {
        struct pcap_pkthdr* header;
        const u_char* packet;
        int res = pcap_next_ex(pcap, &header, &packet);
        if (res == 0) continue;
        if (res == PCAP_ERROR || res == PCAP_ERROR_BREAK) {
            printf("pcap_next_ex return %d(%s)\n", res, pcap_geterr(pcap));
            break;
        }

        printf("-------------------\n");
        struct libnet_ethernet_hdr *eth_hdr = packet;
        if(ntohs(eth_hdr->ether_type) != ETHERTYPE_IP) continue;
        printf("src mac = %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->ether_shost[0], eth_hdr->ether_shost[1], eth_hdr->ether_shost[2],
               eth_hdr->ether_shost[3], eth_hdr->ether_shost[4], eth_hdr->ether_shost[5]);
        printf("dst mac = %02x:%02x:%02x:%02x:%02x:%02x\n", eth_hdr->ether_dhost[0], eth_hdr->ether_dhost[1], eth_hdr->ether_dhost[2],
               eth_hdr->ether_dhost[3], eth_hdr->ether_dhost[4], eth_hdr->ether_dhost[5]);
        // printf("type = 0x%04x \n", ntohs(eth_hdr->ether_type));

        struct libnet_ipv4_hdr *ip_hdr = packet + sizeof(struct libnet_ethernet_hdr);
        if(ip_hdr->ip_p != IPPROTO_TCP) continue;

        uint32_t src_ip = ntohl(ip_hdr->ip_src.s_addr);
        uint8_t src_ip1 = (src_ip & 0xff000000) >> 24;
        uint8_t src_ip2 = (src_ip & 0x00ff0000) >> 16;
        uint8_t src_ip3 = (src_ip & 0x0000ff00) >> 8;
        uint8_t src_ip4 = (src_ip & 0x000000ff);
        uint32_t dst_ip = ntohl(ip_hdr->ip_dst.s_addr);
        uint8_t dst_ip1 = (dst_ip & 0xff000000) >> 24;
        uint8_t dst_ip2 = (dst_ip & 0x00ff0000) >> 16;
        uint8_t dst_ip3 = (dst_ip & 0x0000ff00) >> 8;
        uint8_t dst_ip4 = (dst_ip & 0x000000ff);

        printf("src ip = %d.%d.%d.%d\n", src_ip1, src_ip2, src_ip3, src_ip4);
        printf("dst ip = %d.%d.%d.%d\n", dst_ip1, dst_ip2, dst_ip3, dst_ip4);

        struct libnet_tcp_hdr *tcp_hdr = packet + sizeof(struct libnet_ethernet_hdr) + sizeof(struct libnet_ipv4_hdr);
        printf("src port = %d\n",  ntohs(tcp_hdr->th_sport));
        printf("dst port = %d\n",  ntohs(tcp_hdr->th_dport));

        int pcap_len = header->caplen;
        int until_tcp_len = 14 + ip_hdr->ip_hl * 4 + tcp_hdr->th_off * 4;
        printf("Payload(Data) = ");
        if(pcap_len <= until_tcp_len) {
            printf("Empty\n");
            continue;
        }
        for(int i = until_tcp_len; i < until_tcp_len + 10 && i < pcap_len; i++) {
            printf("0x%02x ", packet[i]);
        }
        printf("\n");
    }

    pcap_close(pcap);
}
