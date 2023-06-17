/* RTCP code taken directly from the most recent RTP specification:
 *     draft-ietf-avt-rtp-new-11.txt
 * C header
 */

#ifndef _RTCP_FROM_SPEC_VOD_H
#define _RTCP_FROM_SPEC_VOD_H

#include <stdlib.h>


    /* The code from the spec assumes a type "event"; make this a void*: */
    typedef void* event;

#define EVENT_UNKNOWN 0
#define EVENT_REPORT 1
#define EVENT_BYE 2

    /* The code from the spec assumes a type "time_tp"; make this a double: */
    typedef double time_tp;

    /* The code from the spec assumes a type "packet"; make this a void*: */
    typedef void* packet;

#define PACKET_UNKNOWN_TYPE 0
#define PACKET_RTP 1
#define PACKET_RTCP_REPORT 2
#define PACKET_BYE 3

    /* The code from the spec calls drand48(), but we have drand30() instead */
#define drand48 drand30

    /* EXPORTS: */
    void OnExpire(event, int, int, double, int, double*, int*, time_tp, time_tp*, int*);
    void OnReceive(packet, event, int*, int*, int*, double*, double*, double, double);

    /* IMPORTS: */
    void Schedule(double, event);
    void Reschedule(double, event);
    void SendRTCPReport(event);
    void SendBYEPacket(event);
    int TypeOfEvent(event);
    int SentPacketSize(event);
    int PacketType(packet);
    int ReceivedPacketSize(packet);
    int NewMember(packet);
    int NewSender(packet);
    void AddMember(packet);
    void AddSender(packet);
    void RemoveMember(packet);
    void RemoveSender(packet);
    double drand30(void);

#endif	// _RTCP_FROM_SPEC_VOD_H
