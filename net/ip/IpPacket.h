/*
 * IpPacket.h
 *
 *  Created on: 2018.01.25.
 *      Author: tooma
 */

#ifndef IPPACKET_H_
#define IPPACKET_H_

#include "core/AddressIp4.h"

namespace IpPacket {
	struct SpecialFields {
		template<size_t off>
		struct IpAddressField {
			AddressIp4 data;
			static constexpr auto offset = off;
			static constexpr auto length = sizeof(AddressIp4::addr);

			template<class Stream>
			inline bool read(Stream& s) {
				return s.read32net(data.addr);
			}
		};

		template<size_t off>
		struct HeaderInfo {
			struct Composite {
				uint16_t data;

				inline bool versionIs4() {
					return (data & 0x4000) == 0x4000;
				}

				inline uint16_t getHeaderLength() {
					return static_cast<uint16_t>((data & 0x0f00) >> 6);
				}
			} data;

			static constexpr auto offset = off;
			static constexpr auto length = 2;

			template<class Stream>
			inline bool read(Stream& s) {
				if(!s.read16net(data.data) || !data.versionIs4() || (data.getHeaderLength() < 20))
					return false;

				s.start(static_cast<uint16_t>(((data.data & 0x0f00) >> 6) - 2));
				s.patchNet(data.data);
				return true;
			}
		};

		template<size_t off>
		struct FragmentInfo {
			struct Composite {
				static constexpr uint16_t moreFragmentsMask = 0x2000;
				static constexpr uint16_t dontFragmentMask = 0x4000;
				static constexpr uint16_t offsetMask = 0x1fff;

				uint16_t data;

				inline bool isFragmented() {
					return data & (offsetMask | moreFragmentsMask);
				}

				inline bool isDontFragmentSet() {
					return data & dontFragmentMask;
				}

				inline bool isLastFragment() {
					return !(data & moreFragmentsMask);
				}

				inline uint16_t getOffset() {
					return data & offsetMask;
				}
			} data;

			static constexpr auto offset = off;
			static constexpr auto length = 2;

			template<class Stream>
			inline bool read(Stream& s) {
				return s.read16net(data.data);
			}
		};
	};

	struct Meta: SpecialFields::HeaderInfo<0> {};
	struct Length: Field16<2> {};
	struct Identification: Field16<4> {};
	struct Fragmentation: SpecialFields::FragmentInfo<6> {};
	struct Ttl: Field8<8> {};
	struct Protocol: Field8<9> {};
	struct Checksum: Field16<10> {};
	struct SourceAddress: SpecialFields::IpAddressField<12> {};
	struct DestinationAddress: SpecialFields::IpAddressField<16> {};
}

#endif /* IPPACKET_H_ */
