#define LOG_MODULE PacketLogModuleAsn1Codec

#include "Asn1Codec.h"
#include "GeneralUtils.h"
#include "EndianPortable.h"
#include <unordered_map>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>
#include <limits>
#include <cstring>

#if defined(_WIN32)
#	undef max
#endif

namespace pcpp
{
	const std::unordered_map<Asn1TagClass, std::string, EnumClassHash<Asn1TagClass>> Asn1TagClassToString{
		{ Asn1TagClass::Universal,       "Universal"       },
		{ Asn1TagClass::ContextSpecific, "ContextSpecific" },
		{ Asn1TagClass::Application,     "Application"     },
		{ Asn1TagClass::Private,         "Private"         }
	};

	std::string toString(Asn1TagClass tagClass)
	{
		if (Asn1TagClassToString.find(tagClass) != Asn1TagClassToString.end())
		{
			return Asn1TagClassToString.at(tagClass);
		}

		return "Unknown";
	}

	const std::unordered_map<Asn1UniversalTagType, std::string, EnumClassHash<Asn1UniversalTagType>>
	    Asn1UniversalTagTypeToString{
		    { Asn1UniversalTagType::EndOfContent,                "EndOfContent"                },
		    { Asn1UniversalTagType::Boolean,                     "Boolean"                     },
		    { Asn1UniversalTagType::Integer,                     "Integer"                     },
		    { Asn1UniversalTagType::BitString,                   "BitString"                   },
		    { Asn1UniversalTagType::OctetString,                 "OctetString"                 },
		    { Asn1UniversalTagType::Null,                        "Null"                        },
		    { Asn1UniversalTagType::ObjectIdentifier,            "ObjectIdentifier"            },
		    { Asn1UniversalTagType::ObjectDescriptor,            "ObjectDescriptor"            },
		    { Asn1UniversalTagType::External,                    "External"                    },
		    { Asn1UniversalTagType::Real,                        "Real"                        },
		    { Asn1UniversalTagType::Enumerated,                  "Enumerated"                  },
		    { Asn1UniversalTagType::EmbeddedPDV,                 "EmbeddedPDV"                 },
		    { Asn1UniversalTagType::UTF8String,                  "UTF8String"                  },
		    { Asn1UniversalTagType::RelativeObjectIdentifier,    "RelativeObjectIdentifier"    },
		    { Asn1UniversalTagType::Time,                        "Time"                        },
		    { Asn1UniversalTagType::Reserved,                    "Reserved"                    },
		    { Asn1UniversalTagType::Sequence,                    "Sequence"                    },
		    { Asn1UniversalTagType::Set,                         "Set"                         },
		    { Asn1UniversalTagType::NumericString,               "NumericString"               },
		    { Asn1UniversalTagType::PrintableString,             "PrintableString"             },
		    { Asn1UniversalTagType::T61String,                   "T61String"                   },
		    { Asn1UniversalTagType::VideotexString,              "VideotexString"              },
		    { Asn1UniversalTagType::IA5String,                   "IA5String"                   },
		    { Asn1UniversalTagType::UTCTime,                     "UTCTime"                     },
		    { Asn1UniversalTagType::GeneralizedTime,             "GeneralizedTime"             },
		    { Asn1UniversalTagType::GraphicString,               "GraphicString"               },
		    { Asn1UniversalTagType::VisibleString,               "VisibleString"               },
		    { Asn1UniversalTagType::GeneralString,               "GeneralString"               },
		    { Asn1UniversalTagType::UniversalString,             "UniversalString"             },
		    { Asn1UniversalTagType::CharacterString,             "CharacterString"             },
		    { Asn1UniversalTagType::BMPString,                   "BMPString"                   },
		    { Asn1UniversalTagType::Date,                        "Date"                        },
		    { Asn1UniversalTagType::TimeOfDay,                   "TimeOfDay"                   },
		    { Asn1UniversalTagType::DateTime,                    "DateTime"                    },
		    { Asn1UniversalTagType::Duration,                    "Duration"                    },
		    { Asn1UniversalTagType::ObjectIdentifierIRI,         "ObjectIdentifierIRI"         },
		    { Asn1UniversalTagType::RelativeObjectIdentifierIRI, "RelativeObjectIdentifierIRI" },
		    { Asn1UniversalTagType::NotApplicable,               "Unknown"                     }
    };

	std::string toString(Asn1UniversalTagType tagType)
	{
		if (Asn1UniversalTagTypeToString.find(tagType) != Asn1UniversalTagTypeToString.end())
		{
			return Asn1UniversalTagTypeToString.at(tagType);
		}

		return "Unknown";
	}

	std::unique_ptr<Asn1Record> Asn1Record::decode(const uint8_t* data, size_t dataLen, bool lazy)
	{
		auto record = decodeInternal(data, dataLen, lazy);
		return std::unique_ptr<Asn1Record>(record);
	}

	uint8_t Asn1Record::encodeTag()
	{
		uint8_t tagByte;

		switch (m_TagClass)
		{
		case Asn1TagClass::Private:
		{
			tagByte = 0xc0;
			break;
		}
		case Asn1TagClass::ContextSpecific:
		{
			tagByte = 0x80;
			break;
		}
		case Asn1TagClass::Application:
		{
			tagByte = 0x40;
			break;
		}
		default:
		{
			tagByte = 0;
			break;
		}
		}

		if (m_IsConstructed)
		{
			tagByte |= 0x20;
		}

		auto tagType = m_TagType & 0x1f;
		tagByte |= tagType;

		return tagByte;
	}

	std::vector<uint8_t> Asn1Record::encodeLength() const
	{
		std::vector<uint8_t> result;

		if (m_ValueLength < 128)
		{
			result.push_back(static_cast<uint8_t>(m_ValueLength));
			return result;
		}

		auto tempValueLength = m_ValueLength;
		do
		{
			uint8_t byte = tempValueLength & 0xff;
			result.push_back(byte);  // Inserts the bytes in reverse order
			tempValueLength >>= 8;
		} while (tempValueLength != 0);

		uint8_t firstByte = 0x80 | static_cast<uint8_t>(result.size());
		result.push_back(firstByte);

		// Reverses the bytes to get forward ordering
		std::reverse(result.begin(), result.end());

		return result;
	}

	std::vector<uint8_t> Asn1Record::encode()
	{
		std::vector<uint8_t> result;

		result.push_back(encodeTag());

		auto lengthBytes = encodeLength();
		result.insert(result.end(), lengthBytes.begin(), lengthBytes.end());

		auto encodedValue = encodeValue();
		result.insert(result.end(), encodedValue.begin(), encodedValue.end());

		return result;
	}

	Asn1Record* Asn1Record::decodeInternal(const uint8_t* data, size_t dataLen, bool lazy)
	{
		uint8_t tagLen;
		auto decodedRecord = decodeTagAndCreateRecord(data, dataLen, tagLen);

		uint8_t lengthLen;
		// try
		//{
		lengthLen = decodedRecord->decodeLength(data + tagLen, dataLen - tagLen);
		//}
		// catch (...)
		//{
		//	delete decodedRecord;
		//	throw;
		//}

		decodedRecord->m_TotalLength = tagLen + lengthLen + decodedRecord->m_ValueLength;
		if (decodedRecord->m_TotalLength < decodedRecord->m_ValueLength ||  // check for overflow
		    decodedRecord->m_TotalLength > dataLen)
		{
			delete decodedRecord;
			return nullptr;
			// throw std::invalid_argument("Cannot decode ASN.1 record, data doesn't contain the entire record");
		}

		if (!lazy)
		{
			// try
			//{
			decodedRecord->decodeValue(const_cast<uint8_t*>(data) + tagLen + lengthLen, lazy);
			//}
			// catch (...)
			//{
			//	delete decodedRecord;
			//	throw;
			//}
		}
		else
		{
			decodedRecord->m_EncodedValue = const_cast<uint8_t*>(data) + tagLen + lengthLen;
		}

		return decodedRecord;
	}

	Asn1UniversalTagType Asn1Record::getUniversalTagType() const
	{
		if (m_TagClass == Asn1TagClass::Universal)
		{
			return static_cast<Asn1UniversalTagType>(m_TagType);
		}

		return Asn1UniversalTagType::NotApplicable;
	}

	Asn1Record* Asn1Record::decodeTagAndCreateRecord(const uint8_t* data, size_t dataLen, uint8_t& tagLen)
	{
		if (dataLen < 1)
		{
			return nullptr;
			// throw std::invalid_argument("Cannot decode ASN.1 record tag");
		}

		tagLen = 1;

		Asn1TagClass tagClass = Asn1TagClass::Universal;

		// Check first 2 bits
		auto tagClassBits = data[0] & 0xc0;
		if (tagClassBits == 0)
		{
			tagClass = Asn1TagClass::Universal;
		}
		else if ((tagClassBits & 0xc0) == 0xc0)
		{
			tagClass = Asn1TagClass::Private;
		}
		else if ((tagClassBits & 0x80) == 0x80)
		{
			tagClass = Asn1TagClass::ContextSpecific;
		}
		else if ((tagClassBits & 0x40) == 0x40)
		{
			tagClass = Asn1TagClass::Application;
		}

		// Check bit 6
		auto tagTypeBits = data[0] & 0x20;
		bool isConstructed = (tagTypeBits != 0);

		// Check last 5 bits
		auto tagType = data[0] & 0x1f;
		if (tagType == 0x1f)
		{
			if (dataLen < 2)
			{
				return nullptr;
				// throw std::invalid_argument("Cannot decode ASN.1 record tag");
			}

			if ((data[1] & 0x80) != 0)
			{
				return nullptr;
				// throw std::invalid_argument("ASN.1 tags with value larger than 127 are not supported");
			}

			tagType = data[1] & 0x7f;
			tagLen = 2;
		}

		Asn1Record* newRecord;

		if (isConstructed)
		{
			if (tagClass == Asn1TagClass::Universal)
			{
				switch (static_cast<Asn1UniversalTagType>(tagType))
				{
				case Asn1UniversalTagType::Sequence:
				{
					newRecord = new Asn1SequenceRecord();
					break;
				}
				case Asn1UniversalTagType::Set:
				{
					newRecord = new Asn1SetRecord();
					break;
				}
				default:
				{
					newRecord = new Asn1ConstructedRecord();
				}
				}
			}
			else
			{
				newRecord = new Asn1ConstructedRecord();
			}
		}
		else
		{
			if (tagClass == Asn1TagClass::Universal)
			{
				auto asn1UniversalTagType = static_cast<Asn1UniversalTagType>(tagType);
				switch (asn1UniversalTagType)
				{
				case Asn1UniversalTagType::Integer:
				{
					newRecord = new Asn1IntegerRecord();
					break;
				}
				case Asn1UniversalTagType::Enumerated:
				{
					newRecord = new Asn1EnumeratedRecord();
					break;
				}
				case Asn1UniversalTagType::OctetString:
				{
					newRecord = new Asn1OctetStringRecord();
					break;
				}
				case Asn1UniversalTagType::Boolean:
				{
					newRecord = new Asn1BooleanRecord();
					break;
				}
				case Asn1UniversalTagType::Null:
				{
					newRecord = new Asn1NullRecord();
					break;
				}
				default:
				{
					newRecord = new Asn1GenericRecord();
				}
				}
			}
			else
			{
				newRecord = new Asn1GenericRecord();
			}
		}

		newRecord->m_TagClass = tagClass;
		newRecord->m_IsConstructed = isConstructed;
		newRecord->m_TagType = tagType;

		return newRecord;
	}

	uint8_t Asn1Record::decodeLength(const uint8_t* data, size_t dataLen)
	{
		if (dataLen < 1)
		{
			// [SUSPICIOUS-FIX]
			return 0;
			// throw std::invalid_argument("Cannot decode ASN.1 record length");
		}

		// Check 8th bit
		auto lengthForm = data[0] & 0x80;

		// Check if the tag is using more than one byte
		// 8th bit at 0 means the length only uses one byte
		// 8th bit at 1 means the length uses more than one byte. The number of bytes is encoded in the other 7 bits
		if (lengthForm == 0)
		{
			m_ValueLength = data[0];
			return 1;
		}

		uint8_t actualLengthBytes = data[0] & 0x7F;
		const uint8_t* actualLengthData = data + 1;

		if (dataLen < static_cast<size_t>(actualLengthBytes) + 1)
		{
			// [SUSPICIOUS-FIX]
			return 0;
			// throw std::invalid_argument("Cannot decode ASN.1 record length");
		}

		for (int i = 0; i < actualLengthBytes; i++)
		{
			size_t partialValueLength = m_ValueLength << 8;
			if (partialValueLength < m_ValueLength)  // check for overflow
			{
				// [SUSPICIOUS-FIX]
				return 0;
				// throw std::invalid_argument("Cannot decode ASN.1 record length");
			}

			m_ValueLength = partialValueLength | actualLengthData[i];
		}

		return 1 + actualLengthBytes;
	}

	void Asn1Record::decodeValueIfNeeded()
	{
		if (m_EncodedValue != nullptr)
		{
			decodeValue(m_EncodedValue, true);
			m_EncodedValue = nullptr;
		}
	}

	std::string Asn1Record::toString()
	{
		auto lines = toStringList();

		auto commaSeparated = [](std::string str1, std::string str2) {
			return std::move(str1) + '\n' + std::move(str2);
		};

		return std::accumulate(std::next(lines.begin()), lines.end(), lines[0], commaSeparated);
	}

	std::vector<std::string> Asn1Record::toStringList()
	{
		std::ostringstream stream;

		auto universalType = getUniversalTagType();
		if (universalType == Asn1UniversalTagType::NotApplicable)
		{
			stream << pcpp::toString(m_TagClass) << " (" << static_cast<int>(m_TagType) << ")";
		}
		else
		{
			stream << pcpp::toString(universalType);
		}

		if (m_IsConstructed)
		{
			stream << " (constructed)";
		}

		stream << ", Length: " << m_TotalLength - m_ValueLength << "+" << m_ValueLength;

		return { stream.str() };
	}

	Asn1GenericRecord::Asn1GenericRecord(Asn1TagClass tagClass, bool isConstructed, uint8_t tagType,
	                                     const uint8_t* value, size_t valueLen)
	{
		init(tagClass, isConstructed, tagType, value, valueLen);
	}

	Asn1GenericRecord::Asn1GenericRecord(Asn1TagClass tagClass, bool isConstructed, uint8_t tagType,
	                                     const std::string& value)
	{
		init(tagClass, isConstructed, tagType, reinterpret_cast<const uint8_t*>(value.c_str()), value.size());
	}

	Asn1GenericRecord::~Asn1GenericRecord()
	{
		delete m_Value;
	}

	void Asn1GenericRecord::decodeValue(uint8_t* data, bool lazy)
	{
		delete m_Value;

		m_Value = new uint8_t[m_ValueLength];
		memcpy(m_Value, data, m_ValueLength);
	}

	std::vector<uint8_t> Asn1GenericRecord::encodeValue() const
	{
		return { m_Value, m_Value + m_ValueLength };
	}

	void Asn1GenericRecord::init(Asn1TagClass tagClass, bool isConstructed, uint8_t tagType, const uint8_t* value,
	                             size_t valueLen)
	{
		m_TagType = tagType;
		m_TagClass = tagClass;
		m_IsConstructed = isConstructed;
		m_Value = new uint8_t[valueLen];
		memcpy(m_Value, value, valueLen);
		m_ValueLength = valueLen;
		m_TotalLength = m_ValueLength + 2;
	}

	Asn1ConstructedRecord::Asn1ConstructedRecord(Asn1TagClass tagClass, uint8_t tagType,
	                                             const std::vector<Asn1Record*>& subRecords)
	{
		init(tagClass, tagType, subRecords.begin(), subRecords.end());
	}

	Asn1ConstructedRecord::Asn1ConstructedRecord(Asn1TagClass tagClass, uint8_t tagType,
	                                             const PointerVector<Asn1Record>& subRecords)
	{
		init(tagClass, tagType, subRecords.begin(), subRecords.end());
	}

	void Asn1ConstructedRecord::decodeValue(uint8_t* data, bool lazy)
	{
		if (!(data || m_ValueLength))
		{
			return;
		}

		auto value = data;
		auto valueLen = m_ValueLength;

		while (valueLen > 0)
		{
			auto subRecord = Asn1Record::decodeInternal(value, valueLen, lazy);
			value += subRecord->getTotalLength();
			valueLen -= subRecord->getTotalLength();

			m_SubRecords.pushBack(subRecord);
		}
	}

	std::vector<uint8_t> Asn1ConstructedRecord::encodeValue() const
	{
		std::vector<uint8_t> result;
		result.reserve(m_ValueLength);

		for (auto record : m_SubRecords)
		{
			auto encodedRecord = record->encode();
			result.insert(result.end(), std::make_move_iterator(encodedRecord.begin()),
			              std::make_move_iterator(encodedRecord.end()));
		}
		return result;
	}

	std::vector<std::string> Asn1ConstructedRecord::toStringList()
	{
		decodeValueIfNeeded();
		std::vector<std::string> result = { Asn1Record::toStringList().front() };
		for (auto subRecord : m_SubRecords)
		{
			for (const auto& line : subRecord->toStringList())
			{
				result.push_back("  " + line);
			}
		}
		return result;
	}

	Asn1SequenceRecord::Asn1SequenceRecord(const std::vector<Asn1Record*>& subRecords)
	    : Asn1ConstructedRecord(Asn1TagClass::Universal, static_cast<uint8_t>(Asn1UniversalTagType::Sequence),
	                            subRecords)
	{}

	Asn1SequenceRecord::Asn1SequenceRecord(const PointerVector<Asn1Record>& subRecords)
	    : Asn1ConstructedRecord(Asn1TagClass::Universal, static_cast<uint8_t>(Asn1UniversalTagType::Sequence),
	                            subRecords)
	{}

	Asn1SetRecord::Asn1SetRecord(const std::vector<Asn1Record*>& subRecords)
	    : Asn1ConstructedRecord(Asn1TagClass::Universal, static_cast<uint8_t>(Asn1UniversalTagType::Set), subRecords)
	{}

	Asn1SetRecord::Asn1SetRecord(const PointerVector<Asn1Record>& subRecords)
	    : Asn1ConstructedRecord(Asn1TagClass::Universal, static_cast<uint8_t>(Asn1UniversalTagType::Set), subRecords)
	{}

	Asn1PrimitiveRecord::Asn1PrimitiveRecord(Asn1UniversalTagType tagType) : Asn1Record()
	{
		m_TagType = static_cast<uint8_t>(tagType);
		m_TagClass = Asn1TagClass::Universal;
		m_IsConstructed = false;
	}

	Asn1IntegerRecord::BigInt::BigInt(const std::string& value)
	{
		m_Value = initFromString(value);
	}

	Asn1IntegerRecord::BigInt::BigInt(const BigInt& other)
	{
		m_Value = other.m_Value;
	}

	std::string Asn1IntegerRecord::BigInt::initFromString(const std::string& value)
	{
		std::string valueStr = value;

		// Optional 0x or 0X prefix
		if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
		{
			valueStr = value.substr(2);
		}

		if (valueStr.empty())
		{
			return nullptr;
			// throw std::invalid_argument("Value is not a valid hex stream");
		}

		for (const char i : valueStr)
		{
			if (!std::isxdigit(i))
			{
				return nullptr;
				// throw std::invalid_argument("Value is not a valid hex stream");
			}
		}

		return valueStr;
	}

	Asn1IntegerRecord::BigInt& Asn1IntegerRecord::BigInt::operator=(const std::string& value)
	{
		m_Value = initFromString(value);
		return *this;
	}

	size_t Asn1IntegerRecord::BigInt::size() const
	{
		return m_Value.size() / 2;
	}

	std::string Asn1IntegerRecord::BigInt::toString() const
	{
		return m_Value;
	}

	std::vector<uint8_t> Asn1IntegerRecord::BigInt::toBytes() const
	{
		std::string value = m_Value;
		if (m_Value.size() % 2 != 0)
		{
			value.insert(0, 1, '0');
		}

		std::vector<uint8_t> result;
		for (std::size_t i = 0; i < value.size(); i += 2)
		{
			std::string byteStr = value.substr(i, 2);
			auto byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
			result.push_back(byte);
		}

		return result;
	}

	Asn1IntegerRecord::Asn1IntegerRecord(uint64_t value) : Asn1PrimitiveRecord(Asn1UniversalTagType::Integer)
	{
		m_Value = value;

		std::size_t length = 0;
		while (value != 0)
		{
			++length;
			value >>= 8;
		}
		m_ValueLength = length == 0 ? 1 : length;

		m_TotalLength = m_ValueLength + 2;
	}

	Asn1IntegerRecord::Asn1IntegerRecord(const std::string& value) : Asn1PrimitiveRecord(Asn1UniversalTagType::Integer)
	{
		m_Value = value;
		m_ValueLength = m_Value.size();
		m_TotalLength = m_ValueLength + 2;
	}

	void Asn1IntegerRecord::decodeValue(uint8_t* data, bool lazy)
	{
		m_Value = pcpp::byteArrayToHexString(data, m_ValueLength);
	}

	std::vector<uint8_t> Asn1IntegerRecord::encodeValue() const
	{
		return m_Value.toBytes();
	}

	std::vector<std::string> Asn1IntegerRecord::toStringList()
	{
		auto valueAsString =
		    m_Value.canFit<uint64_t>() ? std::to_string(getIntValue<uint64_t>()) : "0x" + getValueAsString();
		return std::vector<std::string>({ Asn1Record::toStringList().front() + ", Value: " + valueAsString });
	}

	Asn1EnumeratedRecord::Asn1EnumeratedRecord(uint32_t value) : Asn1IntegerRecord(value)
	{
		m_TagType = static_cast<uint8_t>(Asn1UniversalTagType::Enumerated);
	}

	Asn1OctetStringRecord::Asn1OctetStringRecord(const std::string& value)
	    : Asn1PrimitiveRecord(Asn1UniversalTagType::OctetString)
	{
		m_Value = value;
		m_ValueLength = value.size();
		m_TotalLength = m_ValueLength + 2;
		m_IsPrintable = true;
	}

	Asn1OctetStringRecord::Asn1OctetStringRecord(const uint8_t* value, size_t valueLength)
	    : Asn1PrimitiveRecord(Asn1UniversalTagType::OctetString)
	{
		m_Value = byteArrayToHexString(value, valueLength);
		m_ValueLength = valueLength;
		m_TotalLength = m_ValueLength + 2;
		m_IsPrintable = false;
	}

	void Asn1OctetStringRecord::decodeValue(uint8_t* data, bool lazy)
	{
		auto value = reinterpret_cast<char*>(data);

		m_IsPrintable = std::all_of(value, value + m_ValueLength, [](char c) { return isprint(0xff & c); });

		if (m_IsPrintable)
		{
			m_Value = std::string(value, m_ValueLength);
		}
		else
		{
			m_Value = byteArrayToHexString(data, m_ValueLength);
		}
	}

	std::vector<uint8_t> Asn1OctetStringRecord::encodeValue() const
	{
		if (m_IsPrintable)
		{
			return { m_Value.begin(), m_Value.end() };
		}

		// converting the hex stream to a byte array.
		// The byte array size is half the size of the string
		// i.e "1a2b" (length == 4)  becomes {0x1a, 0x2b} (length == 2)
		auto rawValueSize = static_cast<size_t>(m_Value.size() / 2);
		std::vector<uint8_t> rawValue;
		rawValue.resize(rawValueSize);
		hexStringToByteArray(m_Value, rawValue.data(), rawValueSize);
		return rawValue;
	}

	std::vector<std::string> Asn1OctetStringRecord::toStringList()
	{
		return { Asn1Record::toStringList().front() + ", Value: " + getValue() };
	}

	Asn1BooleanRecord::Asn1BooleanRecord(bool value) : Asn1PrimitiveRecord(Asn1UniversalTagType::Boolean)
	{
		m_Value = value;
		m_ValueLength = 1;
		m_TotalLength = 3;
	}

	void Asn1BooleanRecord::decodeValue(uint8_t* data, bool lazy)
	{
		m_Value = data[0] != 0;
	}

	std::vector<uint8_t> Asn1BooleanRecord::encodeValue() const
	{
		uint8_t byte = (m_Value ? 0xff : 0x00);
		return { byte };
	}

	std::vector<std::string> Asn1BooleanRecord::toStringList()
	{
		return { Asn1Record::toStringList().front() + ", Value: " + (getValue() ? "true" : "false") };
	}

	Asn1NullRecord::Asn1NullRecord() : Asn1PrimitiveRecord(Asn1UniversalTagType::Null)
	{
		m_ValueLength = 0;
		m_TotalLength = 2;
	}
}  // namespace pcpp