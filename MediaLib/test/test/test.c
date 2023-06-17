/*
 * test.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>	
#include <string.h>
#endif

#ifndef IN_MULTICAST
#define IN_MULTICAST(a) ((((uint32_t)(a)) & 0xf0000000) == 0xe0000000)
#endif

#define M_IGMP_HOST_MEMBERSHIP_QUERY        0x11
#define M_IGMP_HOST_MEMBERSHIP_REPORT       0x12
#define M_IGMP_DVMRP                        0x13
#define M_IGMP_PIM                          0x14
#define M_IGMP_TRACE                        0x15
#define M_IGMPV2_HOST_MEMBERSHIP_REPORT     0x16
#define M_IGMP_HOST_LEAVE_MESSAGE           0x17
#define M_IGMPV3_HOST_MEMBERSHIP_REPORT     0x22
#define M_IGMP_MTRACE_RESP                  0x1e
#define M_IGMP_MTRACE                       0x1f
#define M_IGMP_DELAYING_MEMBER              0x01
#define M_IGMP_IDLE_MEMBER                  0x02
#define M_IGMP_LAZY_MEMBER                  0x03
#define M_IGMP_SLEEPING_MEMBER              0x04
#define M_IGMP_AWAKENING_MEMBER             0x05

#define M_IGMP_MINLEN                       8
#define M_IGMP_MAX_HOST_REPORT_DELAY        10
#define M_IGMP_TIMER_SCALE                  10
#define M_IGMP_AGE_THRESHOLD                400
#define M_IGMP_ALL_HOSTS                    htonl(0xE0000001L)
#define M_IGMP_ALL_ROUTER                   htonl(0xE0000002L)
#define M_IGMPV3_ALL_MCR                    htonl(0xE0000016L)
#define M_IGMP_LOCAL_GROUP                  htonl(0xE0000000L)
#define M_IGMP_LOCAL_GROUP_MASK             htonl(0xFFFFFF00L)

volatile int _run = 1;

static void m_usleep(unsigned long microseconds)
{
#ifdef _WIN32
    Sleep(microseconds / 1000);
#else
    usleep(microseconds);
#endif
}

static long long m_time_us(void)
{
#ifndef _WIN32
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
#else
    FILETIME ft;
    long long t;
    GetSystemTimeAsFileTime(&ft);
    t = (long long)ft.dwHighDateTime << 32 | ft.dwLowDateTime;
    return t / 10 - 11644473600000000; /* Jan 1, 1601 */
#endif
}

static unsigned short calc_checksum(unsigned short *addr, int len)
{
    int nleft = len;
    unsigned short *w = addr;
    unsigned short answer = 0;
    int sum = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                 /* add carry */
    answer = ~sum;                      /* truncate to 16 bits */

    return answer;
}

static void send_igmp(int fd, unsigned int grp, int join)
{
    struct sockaddr_in saddr_dst;
    char buffer[M_IGMP_MINLEN];
    unsigned short checksum;

    grp = htonl(grp);
    memset(buffer, 0, M_IGMP_MINLEN);
    buffer[0] = join ? M_IGMPV2_HOST_MEMBERSHIP_REPORT : M_IGMP_HOST_LEAVE_MESSAGE;
    buffer[1] = 0x00;
    memcpy(&buffer[4], &grp, 4);
    checksum = calc_checksum((unsigned short*)buffer, M_IGMP_MINLEN);
    memcpy(&buffer[2], &checksum, 2);
    
    memset(&saddr_dst, 0, sizeof(saddr_dst));
    saddr_dst.sin_family = AF_INET;
    saddr_dst.sin_addr.s_addr = join ? grp : inet_addr("224.0.0.2");
    if (sendto(fd, buffer, M_IGMP_MINLEN, 0, (struct sockaddr*)&saddr_dst, sizeof(saddr_dst)) < 0)
        printf("Failed to send IGMP packet");
}

static void sig_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        _run = 0;
        break;
    }
}

int main(int argc, char** argv)
{
    int fd;
    int i_val = 1;
    unsigned char c_val;
    unsigned int mcast_ip;
    long long last_time = 0;
    /*fd_set read_fds;
    struct timeval tv;
    char buffer[256];*/

    if (argc <= 1)
    {
        printf("Please input multicast IP address!!!\n");
        return 0;
    }
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

#ifdef _WIN32
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
    
    mcast_ip = ntohl(inet_addr(argv[1]));

    if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0)
    {
        printf("Failed to open IGMP socket!!!\n");
        return 0;
    }

    printf("Starting IGMP reporting...\n");
#ifdef MRT_INIT
    if (setsockopt(fd, IPPROTO_IP, MRT_INIT, (void*)&i_val, sizeof(i_val)))
    {
        close(fd);
        return 0;
    }
#endif

    /* Include Header */
    i_val = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (char*)&i_val, sizeof(i_val)) < 0)
        printf("setsockopt IP_HDRINCL %d\n", i_val);

    /* Set TTL */
    /*c_val = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&c_val, sizeof(c_val)) < 0)
        printf("setsockopt IP_MULTICAST_TTL %u\n", c_val);*/

    /* Set Loop */
    c_val = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&c_val, sizeof(c_val)) < 0)
        printf("setsockopt IP_MULTICAST_LOOP %d\n", c_val);
    
    while (_run)
    {
        /*tv.tv_sec = 0;
        tv.tv_usec = 500000LL;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        i_val = select(fd + 1, &read_fds, NULL, NULL, &tv);
        if (i_val > 0)
        {
            i_val = recvfrom(fd, buffer, 256, 0, NULL, NULL);
            if (i_val < 0)
                continue;
        }*/

        if (IN_MULTICAST(mcast_ip)) // Added by K.C.Y
        {
            if (last_time == 0 || m_time_us() - last_time >= 30000000LL) // 30 sec
            {
                send_igmp(fd, mcast_ip, 1);
                last_time = m_time_us();
            }
        }
        m_usleep(10000LL);
    }

    printf("Stopping IGMP reporting...\n");
    if (IN_MULTICAST(mcast_ip))
    {
        send_igmp(fd, mcast_ip, 0);
        m_usleep(100000LL);
    }
    
#ifdef _WIN32
    closesocket(fd);
    WSACleanup();
#else
    close(fd);
#endif

    return 0;
}
