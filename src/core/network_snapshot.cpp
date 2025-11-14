#include "network_snapshot.h"

#include <algorithm>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

namespace
{
    template <typename TableType>
    std::vector<typename TableType::RowType> QueryTable(ULONG family, DWORD &statusOut)
    {
        std::vector<typename TableType::RowType> rows;
        ULONG bufferSize = 0;
        statusOut = TableType::Query(nullptr, &bufferSize, TRUE, family);

        if (statusOut == ERROR_ACCESS_DENIED)
        {
            return rows;
        }

        if (statusOut != ERROR_INSUFFICIENT_BUFFER)
        {
            return rows;
        }

        std::vector<std::uint8_t> buffer(bufferSize);
        auto *table = reinterpret_cast<typename TableType::NativeType *>(buffer.data());

        statusOut = TableType::Query(table, &bufferSize, TRUE, family);
        if (statusOut != NO_ERROR)
        {
            return rows;
        }

        rows.reserve(table->dwNumEntries);
        for (ULONG i = 0; i < table->dwNumEntries; ++i)
        {
            rows.push_back(table->table[i]);
        }

        statusOut = NO_ERROR;
        return rows;
    }

    struct Tcp4Table
    {
        using NativeType = MIB_TCPTABLE_OWNER_PID;
        using RowType = MIB_TCPROW_OWNER_PID;

        static DWORD Query(NativeType *table, PULONG size, BOOL sorted, ULONG)
        {
            return GetExtendedTcpTable(table,
                                       size,
                                       sorted,
                                       AF_INET,
                                       TCP_TABLE_OWNER_PID_ALL,
                                       0);
        }
    };

    struct Udp4Table
    {
        using NativeType = MIB_UDPTABLE_OWNER_PID;
        using RowType = MIB_UDPROW_OWNER_PID;

        static DWORD Query(NativeType *table, PULONG size, BOOL sorted, ULONG)
        {
            return GetExtendedUdpTable(table,
                                       size,
                                       sorted,
                                       AF_INET,
                                       UDP_TABLE_OWNER_PID,
                                       0);
        }
    };

    std::uint16_t ConvertPort(DWORD value)
    {
        return static_cast<std::uint16_t>(ntohs(static_cast<std::uint16_t>(value)));
    }
}

namespace rvrse::core
{
    NetworkSnapshot NetworkSnapshot::Capture()
    {
        NetworkSnapshot snapshot;

        DWORD tcpStatus = NO_ERROR;
        auto tcpRows = QueryTable<Tcp4Table>(AF_INET, tcpStatus);
        DWORD udpStatus = NO_ERROR;
        auto udpRows = QueryTable<Udp4Table>(AF_INET, udpStatus);

        snapshot.accessDenied_ = (tcpStatus == ERROR_ACCESS_DENIED) || (udpStatus == ERROR_ACCESS_DENIED);
        snapshot.captureFailed_ = (!snapshot.accessDenied_) &&
                                  ((tcpStatus != NO_ERROR) || (udpStatus != NO_ERROR));

        if (snapshot.accessDenied_ || snapshot.captureFailed_)
        {
            return snapshot;
        }

        snapshot.connections_.reserve(tcpRows.size() + udpRows.size());

        for (const auto &row : tcpRows)
        {
            ConnectionEntry entry{};
            entry.protocol = TransportProtocol::Tcp;
            entry.localAddress = row.dwLocalAddr;
            entry.localPort = ConvertPort(row.dwLocalPort);
            entry.remoteAddress = row.dwRemoteAddr;
            entry.remotePort = ConvertPort(row.dwRemotePort);
            entry.state = static_cast<std::uint8_t>(row.dwState);
            entry.owningProcessId = row.dwOwningPid;
            snapshot.connections_.push_back(entry);
        }

        for (const auto &row : udpRows)
        {
            ConnectionEntry entry{};
            entry.protocol = TransportProtocol::Udp;
            entry.localAddress = row.dwLocalAddr;
            entry.localPort = ConvertPort(row.dwLocalPort);
            entry.owningProcessId = row.dwOwningPid;
            snapshot.connections_.push_back(entry);
        }

        std::sort(snapshot.connections_.begin(), snapshot.connections_.end(),
                  [](const ConnectionEntry &lhs, const ConnectionEntry &rhs)
                  {
                      if (lhs.owningProcessId != rhs.owningProcessId)
                      {
                          return lhs.owningProcessId < rhs.owningProcessId;
                      }
                      if (lhs.protocol != rhs.protocol)
                      {
                          return lhs.protocol < rhs.protocol;
                      }
                      if (lhs.localAddress != rhs.localAddress)
                      {
                          return lhs.localAddress < rhs.localAddress;
                      }
                      return lhs.localPort < rhs.localPort;
                  });

        return snapshot;
    }

    std::vector<ConnectionEntry> NetworkSnapshot::ConnectionsForProcess(std::uint32_t processId) const
    {
        std::vector<ConnectionEntry> results;
        for (const auto &connection : connections_)
        {
            if (connection.owningProcessId == processId)
            {
                results.push_back(connection);
            }
        }
        return results;
    }

    std::size_t NetworkSnapshot::ConnectionCountForProcess(std::uint32_t processId) const
    {
        std::size_t count = 0;
        for (const auto &connection : connections_)
        {
            if (connection.owningProcessId == processId)
            {
                ++count;
            }
        }
        return count;
    }
}
