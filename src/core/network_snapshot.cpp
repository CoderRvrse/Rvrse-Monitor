#include "network_snapshot.h"

#include <algorithm>
#include <cstring>
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

    struct Tcp6Table
    {
        using NativeType = MIB_TCP6TABLE_OWNER_PID;
        using RowType = MIB_TCP6ROW_OWNER_PID;

        static DWORD Query(NativeType *table, PULONG size, BOOL sorted, ULONG)
        {
            return GetExtendedTcpTable(table,
                                       size,
                                       sorted,
                                       AF_INET6,
                                       TCP_TABLE_OWNER_PID_ALL,
                                       0);
        }
    };

    struct Udp6Table
    {
        using NativeType = MIB_UDP6TABLE_OWNER_PID;
        using RowType = MIB_UDP6ROW_OWNER_PID;

        static DWORD Query(NativeType *table, PULONG size, BOOL sorted, ULONG)
        {
            return GetExtendedUdpTable(table,
                                       size,
                                       sorted,
                                       AF_INET6,
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

        // IPv4
        DWORD tcpStatus = NO_ERROR;
        auto tcpRows = QueryTable<Tcp4Table>(AF_INET, tcpStatus);
        DWORD udpStatus = NO_ERROR;
        auto udpRows = QueryTable<Udp4Table>(AF_INET, udpStatus);

        // IPv6
        DWORD tcp6Status = NO_ERROR;
        auto tcp6Rows = QueryTable<Tcp6Table>(AF_INET6, tcp6Status);
        DWORD udp6Status = NO_ERROR;
        auto udp6Rows = QueryTable<Udp6Table>(AF_INET6, udp6Status);

        snapshot.accessDenied_ = (tcpStatus == ERROR_ACCESS_DENIED) || (udpStatus == ERROR_ACCESS_DENIED) ||
                                 (tcp6Status == ERROR_ACCESS_DENIED) || (udp6Status == ERROR_ACCESS_DENIED);

        snapshot.captureFailed_ = (!snapshot.accessDenied_) &&
                                  ((tcpStatus != NO_ERROR) || (udpStatus != NO_ERROR) ||
                                   (tcp6Status != NO_ERROR) || (udp6Status != NO_ERROR));

        if (snapshot.accessDenied_ || snapshot.captureFailed_)
        {
            return snapshot;
        }

        snapshot.connections_.reserve(tcpRows.size() + udpRows.size() + tcp6Rows.size() + udp6Rows.size());

        // IPv4 TCP
        for (const auto &row : tcpRows)
        {
            ConnectionEntry entry{};
            entry.protocol = TransportProtocol::Tcp;
            entry.addressFamily = AddressFamily::IPv4;
            entry.localAddress = row.dwLocalAddr;
            entry.localPort = ConvertPort(row.dwLocalPort);
            entry.remoteAddress = row.dwRemoteAddr;
            entry.remotePort = ConvertPort(row.dwRemotePort);
            entry.state = static_cast<std::uint8_t>(row.dwState);
            entry.owningProcessId = row.dwOwningPid;
            snapshot.connections_.push_back(entry);
        }

        // IPv4 UDP
        for (const auto &row : udpRows)
        {
            ConnectionEntry entry{};
            entry.protocol = TransportProtocol::Udp;
            entry.addressFamily = AddressFamily::IPv4;
            entry.localAddress = row.dwLocalAddr;
            entry.localPort = ConvertPort(row.dwLocalPort);
            entry.owningProcessId = row.dwOwningPid;
            snapshot.connections_.push_back(entry);
        }

        // IPv6 TCP
        for (const auto &row : tcp6Rows)
        {
            ConnectionEntry entry{};
            entry.protocol = TransportProtocol::Tcp;
            entry.addressFamily = AddressFamily::IPv6;
            std::memcpy(entry.localAddress6, &row.ucLocalAddr[0], 16);
            entry.localPort = ConvertPort(row.dwLocalPort);
            std::memcpy(entry.remoteAddress6, &row.ucRemoteAddr[0], 16);
            entry.remotePort = ConvertPort(row.dwRemotePort);
            entry.state = static_cast<std::uint8_t>(row.dwState);
            entry.owningProcessId = row.dwOwningPid;
            snapshot.connections_.push_back(entry);
        }

        // IPv6 UDP
        for (const auto &row : udp6Rows)
        {
            ConnectionEntry entry{};
            entry.protocol = TransportProtocol::Udp;
            entry.addressFamily = AddressFamily::IPv6;
            std::memcpy(entry.localAddress6, &row.ucLocalAddr[0], 16);
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
                      if (lhs.addressFamily != rhs.addressFamily)
                      {
                          return static_cast<int>(lhs.addressFamily) < static_cast<int>(rhs.addressFamily);
                      }
                      if (lhs.localPort != rhs.localPort)
                      {
                          return lhs.localPort < rhs.localPort;
                      }
                      return lhs.remotePort < rhs.remotePort;
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
