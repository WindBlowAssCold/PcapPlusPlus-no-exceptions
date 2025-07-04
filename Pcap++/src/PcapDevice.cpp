#include "PcapDevice.h"
#include "PcapFilter.h"
#include "Logger.h"
#include "pcap.h"

namespace pcpp
{
	namespace internal
	{
		PcapHandle::PcapHandle(pcap_t* pcapDescriptor) noexcept : m_PcapDescriptor(pcapDescriptor)
		{}

		PcapHandle::PcapHandle(PcapHandle&& other) noexcept : m_PcapDescriptor(other.m_PcapDescriptor)
		{
			other.m_PcapDescriptor = nullptr;
		}

		PcapHandle& PcapHandle::operator=(PcapHandle&& other) noexcept
		{
			if (this != &other)
			{
				reset(other.m_PcapDescriptor);
				other.m_PcapDescriptor = nullptr;
			}
			return *this;
		}

		PcapHandle& PcapHandle::operator=(std::nullptr_t) noexcept
		{
			reset();
			return *this;
		}

		PcapHandle::~PcapHandle()
		{
			reset();
		}

		pcap_t* PcapHandle::release() noexcept
		{
			auto result = m_PcapDescriptor;
			m_PcapDescriptor = nullptr;
			return result;
		}

		void PcapHandle::reset(pcap_t* pcapDescriptor) noexcept
		{
			pcap_t* oldDescriptor = m_PcapDescriptor;
			m_PcapDescriptor = pcapDescriptor;
			if (oldDescriptor != nullptr)
			{
				pcap_close(oldDescriptor);
			}
		}

		char const* PcapHandle::getLastError() const noexcept
		{
			if (!isValid())
			{
				static char const* const noHandleError = "No pcap handle";
				return noHandleError;
			}

			return pcap_geterr(m_PcapDescriptor);
		}

		bool PcapHandle::setFilter(std::string const& filter)
		{
			if (!isValid())
			{
				PCPP_LOG_ERROR("Cannot set filter to invalid handle");
				return false;
			}

			bpf_program prog;
			PCPP_LOG_DEBUG("Compiling the filter '" << filter << "'");
			if (pcap_compile(m_PcapDescriptor, &prog, filter.c_str(), 1, 0) < 0)
			{
				// Print out appropriate text, followed by the error message
				// generated by the packet capture library.
				PCPP_LOG_ERROR("Error compiling filter. Error message is: " << getLastError());
				return false;
			}

			PCPP_LOG_DEBUG("Setting the compiled filter");
			if (pcap_setfilter(m_PcapDescriptor, &prog) < 0)
			{
				// Print out error. The format will be the prefix string,
				// created above, followed by the error message that the packet
				// capture library generates.
				PCPP_LOG_ERROR("Error setting a compiled filter. Error message is: " << getLastError());
				pcap_freecode(&prog);
				return false;
			}

			PCPP_LOG_DEBUG("Filter set successfully");
			pcap_freecode(&prog);
			return true;
		}

		bool PcapHandle::clearFilter()
		{
			return setFilter("");
		}

		bool PcapHandle::getStatistics(PcapStats& stats) const
		{
			if (!isValid())
			{
				PCPP_LOG_ERROR("Cannot get stats from invalid handle");
				return false;
			}

			pcap_stat pcapStats;
			if (pcap_stats(m_PcapDescriptor, &pcapStats) < 0)
			{
				PCPP_LOG_ERROR("Error getting stats. Error message is: " << getLastError());
				return false;
			}

			stats.packetsRecv = pcapStats.ps_recv;
			stats.packetsDrop = pcapStats.ps_drop;
			stats.packetsDropByInterface = pcapStats.ps_ifdrop;
			return true;
		}
	}  // namespace internal

	IPcapStatisticsProvider::PcapStats IPcapStatisticsProvider::getStatistics() const
	{
		PcapStats stats;
		getStatistics(stats);
		return stats;
	}

	bool IPcapDevice::setFilter(std::string filterAsString)
	{
		PCPP_LOG_DEBUG("Filter to be set: '" << filterAsString << "'");
		if (!m_DeviceOpened)
		{
			PCPP_LOG_ERROR("Device not Opened!! cannot set filter");
			return false;
		}

		return m_PcapDescriptor.setFilter(filterAsString);
	}

	bool IPcapDevice::clearFilter()
	{
		return m_PcapDescriptor.clearFilter();
	}

	bool IPcapDevice::matchPacketWithFilter(GeneralFilter& filter, RawPacket* rawPacket)
	{
		return filter.matchPacketWithFilter(rawPacket);
	}

	std::string IPcapDevice::getPcapLibVersionInfo()
	{
		return std::string(pcap_lib_version());
	}
}  // namespace pcpp