#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rvrse::core
{
    enum class TransportProtocol
    {
        Tcp,
        Udp
    };

    enum class AddressFamily
    {
        IPv4,
        IPv6
    };

    struct ConnectionEntry
    {
        TransportProtocol protocol = TransportProtocol::Tcp;
        AddressFamily addressFamily = AddressFamily::IPv4;

        // IPv4 addresses (4 bytes each)
        std::uint32_t localAddress = 0;
        std::uint32_t remoteAddress = 0;

        // IPv6 addresses (16 bytes each) - stored as network byte order
        std::uint8_t localAddress6[16] = {};
        std::uint8_t remoteAddress6[16] = {};

        std::uint16_t localPort = 0;
        std::uint16_t remotePort = 0;
        std::uint32_t owningProcessId = 0;
        std::uint8_t state = 0;
    };

    class NetworkSnapshot
    {
    public:
        NetworkSnapshot() = default;

        static NetworkSnapshot Capture();

        const std::vector<ConnectionEntry> &Connections() const { return connections_; }

        std::vector<ConnectionEntry> ConnectionsForProcess(std::uint32_t processId) const;
        std::size_t ConnectionCountForProcess(std::uint32_t processId) const;

        bool AccessDenied() const { return accessDenied_; }
        bool CaptureFailed() const { return captureFailed_; }

    private:
        std::vector<ConnectionEntry> connections_;
        bool accessDenied_ = false;
        bool captureFailed_ = false;
    };
}
