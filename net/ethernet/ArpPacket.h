/*
 * ArpPacket.h
 *
 *  Created on: 2018.01.24.
 *      Author: tooma
 */

#ifndef ARPPACKET_H_
#define ARPPACKET_H_

#include "core/AddressEthernet.h"
#include "core/AddressIp4.h"
#include "ip/IpPacket.h"

namespace ArpPacket {
	struct SpecialFields {
		template<size_t off>
		struct MacField {
			AddressEthernet data;
			static constexpr auto offset = off;
			static constexpr auto length = sizeof(AddressEthernet::bytes);

			template<class Stream>
			inline bool read(Stream& s) {
				return s.copyOut(reinterpret_cast<char*>(data.bytes), sizeof(data.bytes));
			}
		};

		template<size_t off>
		struct OperationField: Field16<off> {
			enum class Type: uint16_t {
				Request = 1,
				Reply = 2
			} data;

			template<class Stream>
			inline bool read(Stream& s) {
				uint16_t temp;
				if(!s.read16net(temp))
					return false;

				data = static_cast<Type>(temp);
				return true;
			}
		};
	};

	struct HType: Field16<0> {};
	struct PType: Field16<2> {};
	struct HLen: Field8<4> {};
	struct PLen: Field8<5> {};
	struct Operation: SpecialFields::OperationField<6> {};
	struct SenderMac: SpecialFields::MacField<8> {};
	struct SenderIp: IpPacket::SpecialFields::IpAddressField<14> {};
	struct TargetMac: SpecialFields::MacField<18> {};
	struct TargetIp: IpPacket::SpecialFields::IpAddressField<24> {};
	struct End: EndMarker<28> {};
}

#endif /* ARPPACKET_H_ */
