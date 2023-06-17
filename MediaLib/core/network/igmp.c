/*****************************************************************************
 * igmp.c:
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>

#include <vlc_network.h>

#define IGMP_PACKET_LEN        8

static uint16_t calc_checksum(uint16_t *addr, int len)
{
    int nleft = len;
    uint16_t *w = addr;
    uint16_t answer = 0;
    int32_t sum = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(uint8_t *)(&answer) = *(uint8_t *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                 /* add carry */
    answer = ~sum;                      /* truncate to 16 bits */

    return answer;
}

int net_IGMPOpen(void)
{
    int fd;
    int i_val = 1;
    unsigned char c_val;

    if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0)
    {
        msg_Err(NULL, "Failed to open IGMP socket!!!");
        return -1;
    }

#ifdef MRT_INIT
    if (setsockopt(fd, IPPROTO_IP, MRT_INIT, (void*)&i_val, sizeof(i_val)))
    {
        close(fd);
        return -1;
    }
#endif

    /* Include Header */
    i_val = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (char*)&i_val, sizeof(i_val)) < 0)
        msg_Warn(NULL, "setsockopt IP_HDRINCL %d", i_val);

    /* Set TTL */
    c_val = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&c_val, sizeof(c_val)) < 0)
        msg_Warn(NULL, "setsockopt IP_MULTICAST_TTL %u", c_val);

    /* Set Loop */
    c_val = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&c_val, sizeof(c_val)) < 0)
        msg_Warn(NULL, "setsockopt IP_MULTICAST_LOOP %d", c_val);

    return fd;
}

void net_IGMPClose(int fd)
{
#ifdef MRT_DONE
    if (setsockopt(fd, IPPROTO_IP, MRT_DONE, NULL, 0))
        msg_Err(NULL, "error MRT_DONE");
#endif
    close(fd);
}

void net_IGMPSend(int fd, uint32_t grp)
{
    struct sockaddr_in saddr_dst;
    //int setloop = 0, setigmpsource = 0;
    char buffer[IGMP_PACKET_LEN];
    uint16_t checksum;

    grp = htonl(grp);
    memset(buffer, 0, IGMP_PACKET_LEN);
    buffer[0] = 0x16; // VERSION, TYPE
    buffer[1] = 0x00;
    memcpy(&buffer[4], &grp, 4);
    checksum = calc_checksum((uint16_t*)buffer, IGMP_PACKET_LEN);
    memcpy(&buffer[2], &checksum, 2);

    /*if (IN_MULTICAST(ntohl(grp)))
    {
        k_set_if(src);
        setigmpsource = 1;
        if (type != IGMP_DVMRP || dst == allhosts_group) {
            setloop = 1;
            k_set_loop(true);
        }
    }*/

    memset(&saddr_dst, 0, sizeof(saddr_dst));
    saddr_dst.sin_family = AF_INET;
    saddr_dst.sin_addr.s_addr = grp;
    if (sendto(fd, buffer, IGMP_PACKET_LEN, 0, (struct sockaddr*)&saddr_dst, sizeof(saddr_dst)) < 0)
    {
        if (errno == ENETDOWN)
            msg_Err(NULL, "sender VIF was down.");
        else
            msg_Info(NULL, "sendto error");
    }

    //if (setigmpsource)
    //{
    //    if (setloop)
    //        k_set_loop(false);
    //    // Restore original...
    //    k_set_if(INADDR_ANY);
    //}
}