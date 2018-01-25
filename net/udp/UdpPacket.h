/*
 * UdpPacket.h
 *
 *      Author: tamas.seller
 */

#ifndef UDPPACKET_H_
#define UDPPACKET_H_

namespace UdpPacket {
    struct SourcePort: Field16<0> {};
    struct DestinationPort: Field16<2> {};
    struct Length: Field16<4> {};
    struct Checksum: Field16<6> {};
    struct End: EndMarker<8> {};
}

#endif /* UDPPACKET_H_ */
