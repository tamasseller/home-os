/*
 * TcpPacket.h
 *
 *      Author: tamas.seller
 */

#ifndef TCPPACKET_H_
#define TCPPACKET_H_

namespace TcpPacket {
    struct SpecialFields {
        template<size_t off>
        struct FlagsAndOffset {
            static constexpr auto offset = off;
            static constexpr auto length = 2;

            struct Composite {
                static constexpr uint16_t finMask = 0x0001;
                static constexpr uint16_t synMask = 0x0002;
                static constexpr uint16_t rstMask = 0x0004;
                static constexpr uint16_t ackMask = 0x0010;

                uint16_t data;

                inline bool hasFin() {
                    return data & finMask;
                }

                inline bool hasSyn() {
                    return data & synMask;
                }

                inline bool hasRst() {
                    return data & rstMask;
                }

                inline bool hasAck() {
                    return data & ackMask;
                }

                inline void setFin(bool x) {
                    data = static_cast<uint16_t>(x ? (data | finMask) : (data & ~finMask));
                }

                inline void setSyn(bool x) {
                    data = static_cast<uint16_t>(x ? (data | synMask) : (data & ~synMask));
                }

                inline void setRst(bool x) {
                    data = static_cast<uint16_t>(x ? (data | rstMask) : (data & ~rstMask));
                }

                inline void setAck(bool x) {
                    data = static_cast<uint16_t>(x ? (data | ackMask) : (data & ~ackMask));
                }

                inline void clear() {
                    data = 0;
                }

                inline uint16_t getDataOffset() {
                    return static_cast<uint16_t>((data & 0xf000) >> 10);
                }

                inline uint16_t setDataOffset(uint16_t offset) {
                    return data = static_cast<uint16_t>((data & ~0xf000) | ((offset & ~3) << 10));
                }
            } data;

            template<class Stream>
            inline bool read(Stream& s) {
                return s.read16net(data.data);
            }

            template<class Stream>
            inline bool write(Stream& s) {
                return s.write16net(data.data);
            }
        };
    };

    struct SourcePort: Field16<0> {};
    struct DestinationPort: Field16<2> {};
    struct SequenceNumber: Field32<4> {};
    struct AcknowledgementNumber: Field32<8> {};
    struct Flags: SpecialFields::FlagsAndOffset<12> {};
    struct WindowSize: Field16<14> {};
    struct Checksum: Field16raw<16> {};
    struct UrgentPointer: Field16<18> {};

    typedef StructuredAccessor<SourcePort, DestinationPort, SequenceNumber,
    		AcknowledgementNumber, Flags, WindowSize, Checksum, UrgentPointer> FullHeaderAccessor;
}

#endif /* TCPPACKET_H_ */
