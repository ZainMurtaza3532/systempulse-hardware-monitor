#include "monitors/NetworkMonitor.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <vector>
#include <string>
#include <string_view>
#include <iostream>

#ifdef _WIN32
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>

#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif
#endif

namespace SystemPulse::Monitors {

    namespace {

        // Case-insensitive substring search helper
        bool ContainsIgnoreCase(std::string_view source, std::string_view target) {
            if (target.empty()) return true;
            if (source.size() < target.size()) return false;
            auto it = std::search(
                source.begin(), source.end(),
                target.begin(), target.end(),
                [](char a, char b) {
                    return std::tolower(static_cast<unsigned char>(a)) ==
                           std::tolower(static_cast<unsigned char>(b));
                }
            );
            return it != source.end();
        }

        // Checks whether an adapter should be excluded based on virtual miniport / filter driver keywords
        bool ShouldIgnoreAdapter(std::string_view name, std::string_view description) {
            // Explicit filter tokens requested:
            // "WAN Miniport", "QoS", "WFP", "Kernel Debugger", "TAP"
            static constexpr std::string_view kFilterKeywords[] = {
                "WAN Miniport",
                "QoS",
                "WFP",
                "Kernel Debugger",
                "TAP",
                // Additional duplicate filter drivers & virtual multiplexers
                "Packet Scheduler",
                "LightWeight Filter",
                "Native WiFi Filter",
                "Teredo",
                "Microsoft Wi-Fi Direct Virtual"
            };

            for (const auto& keyword : kFilterKeywords) {
                if (ContainsIgnoreCase(name, keyword) || ContainsIgnoreCase(description, keyword)) {
                    return true;
                }
            }

            return false;
        }

        // Determine friendly adapter category
        std::string DetermineAdapterType(std::string_view name, std::string_view desc, uint32_t ifType) {
            if (ContainsIgnoreCase(name, "Wi-Fi") || ContainsIgnoreCase(desc, "Wi-Fi") ||
                ContainsIgnoreCase(desc, "Wireless") || ContainsIgnoreCase(desc, "802.11") ||
                ifType == 71 /* IF_TYPE_IEEE80211 */) {
                return "Wi-Fi";
            }
            if (ContainsIgnoreCase(name, "vEthernet") || ContainsIgnoreCase(desc, "Hyper-V") ||
                ContainsIgnoreCase(desc, "Virtual Ethernet")) {
                return "vEthernet";
            }
            return "Ethernet";
        }

    } // anonymous namespace

    NetworkMonitor::NetworkMonitor()
        : m_lastUpdateTime(std::chrono::steady_clock::now()) {}

    void NetworkMonitor::Initialize() {
        QueryNetwork();
    }

    void NetworkMonitor::Update() {
        QueryNetwork();
    }

    void NetworkMonitor::QueryNetwork() {
        auto now = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(now - m_lastUpdateTime).count();
        if (elapsedSeconds <= 0.0) elapsedSeconds = 1.0;

#ifdef _WIN32
        // Use IP_ADAPTER_ADDRESSES via GetAdaptersAddresses for comprehensive physical adapter info
        ULONG bufferSize = 15000;
        std::vector<BYTE> buffer(bufferSize);
        PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

        ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
        ULONG ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, pAddresses, &bufferSize);

        if (ret == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(bufferSize);
            pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
            ret = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, pAddresses, &bufferSize);
        }

        if (ret == NO_ERROR && pAddresses != nullptr) {
            m_telemetry.adapters.clear();
            std::unordered_set<std::string> seenMacs;
            std::unordered_set<std::string> seenNames;

            uint64_t currentTotalIn = 0;
            uint64_t currentTotalOut = 0;

            for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next) {
                // Ignore Software Loopback (Type 24) and Tunnels (Type 131)
                if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK || pCurr->IfType == IF_TYPE_TUNNEL) {
                    continue;
                }

                // Convert Friendly Name (e.g. "Wi-Fi", "Ethernet")
                char nameBuf[256] = { 0 };
                WideCharToMultiByte(CP_UTF8, 0, pCurr->FriendlyName, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
                std::string adapterName = nameBuf;

                // Convert Hardware Description (e.g. "Intel(R) Wi-Fi 6 AX201 160MHz")
                char descBuf[256] = { 0 };
                WideCharToMultiByte(CP_UTF8, 0, pCurr->Description, -1, descBuf, sizeof(descBuf), nullptr, nullptr);
                std::string adapterDesc = descBuf;

                // 1. Filter out virtual miniports and duplicate filter drivers
                if (ShouldIgnoreAdapter(adapterName, adapterDesc)) {
                    continue;
                }

                // 2. Only consider operational / active network interfaces
                bool isUp = (pCurr->OperStatus == IfOperStatusUp);
                if (!isUp) {
                    continue;
                }

                // 3. Format MAC Address
                std::string macAddress;
                if (pCurr->PhysicalAddressLength == 6) {
                    char macBuf[24];
                    snprintf(macBuf, sizeof(macBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                        pCurr->PhysicalAddress[0], pCurr->PhysicalAddress[1],
                        pCurr->PhysicalAddress[2], pCurr->PhysicalAddress[3],
                        pCurr->PhysicalAddress[4], pCurr->PhysicalAddress[5]);
                    macAddress = macBuf;
                }

                // Filter out all-zero MAC addresses unless it's a valid primary vEthernet bridge
                bool isVirtualBridge = ContainsIgnoreCase(adapterName, "vEthernet");
                if (!isVirtualBridge && (macAddress.empty() || macAddress == "00:00:00:00:00:00")) {
                    continue;
                }

                // 4. Deduplicate by MAC address or adapter name
                if (!macAddress.empty() && seenMacs.find(macAddress) != seenMacs.end()) {
                    continue; // Skip duplicate MAC (e.g., duplicate filter driver binding)
                }
                if (seenNames.find(adapterName) != seenNames.end()) {
                    continue; // Skip duplicate name
                }

                if (!macAddress.empty()) {
                    seenMacs.insert(macAddress);
                }
                seenNames.insert(adapterName);

                // 5. Query IPv4 Address
                std::string ipAddress = "N/A";
                for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
                    if (pUnicast->Address.lpSockaddr && pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                        char ipBuffer[INET_ADDRSTRLEN] = { 0 };
                        auto* sa_in = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                        inet_ntop(AF_INET, &(sa_in->sin_addr), ipBuffer, sizeof(ipBuffer));
                        ipAddress = ipBuffer;
                        break;
                    }
                }

                // 6. Query Precise Traffic Bytes via GetIfEntry2
                NetworkAdapter adapter;
                adapter.name = adapterName;
                adapter.description = adapterDesc;
                adapter.macAddress = macAddress.empty() ? "Virtual Bridge" : macAddress;
                adapter.ipAddress = ipAddress;
                adapter.type = DetermineAdapterType(adapterName, adapterDesc, pCurr->IfType);
                adapter.isOperational = true;
                adapter.linkSpeedBps = pCurr->ReceiveLinkSpeed;

                MIB_IF_ROW2 ifRow;
                ZeroMemory(&ifRow, sizeof(MIB_IF_ROW2));
                ifRow.InterfaceLuid = pCurr->Luid;

                if (GetIfEntry2(&ifRow) == NO_ERROR) {
                    adapter.bytesReceived = ifRow.InOctets;
                    adapter.bytesSent = ifRow.OutOctets;
                    if (ifRow.ReceiveLinkSpeed > 0) {
                        adapter.linkSpeedBps = ifRow.ReceiveLinkSpeed;
                    }
                }

                currentTotalIn += adapter.bytesReceived;
                currentTotalOut += adapter.bytesSent;

                m_telemetry.adapters.push_back(std::move(adapter));
            }

            m_telemetry.totalBytesReceived = currentTotalIn;
            m_telemetry.totalBytesSent = currentTotalOut;

            if (m_hasInitialReading) {
                if (currentTotalIn >= m_lastInOctets) {
                    m_telemetry.currentDownloadSpeed = static_cast<uint64_t>(
                        static_cast<double>(currentTotalIn - m_lastInOctets) / elapsedSeconds
                    );
                }
                if (currentTotalOut >= m_lastOutOctets) {
                    m_telemetry.currentUploadSpeed = static_cast<uint64_t>(
                        static_cast<double>(currentTotalOut - m_lastOutOctets) / elapsedSeconds
                    );
                }
            } else {
                m_hasInitialReading = true;
            }

            m_lastInOctets = currentTotalIn;
            m_lastOutOctets = currentTotalOut;
        }
#else
        // Mock / Linux fallback
        m_telemetry.adapters.clear();

        NetworkAdapter eth0;
        eth0.name = "eth0";
        eth0.description = "Primary Physical Ethernet Controller";
        eth0.macAddress = "E4:54:E8:A1:B2:C3";
        eth0.ipAddress = "192.168.1.105";
        eth0.type = "Ethernet";
        eth0.isOperational = true;
        eth0.bytesReceived = m_lastInOctets + (rand() % 400000 + 100000);
        eth0.bytesSent = m_lastOutOctets + (rand() % 150000 + 40000);
        eth0.linkSpeedBps = 1000000000ULL;

        m_telemetry.adapters.push_back(eth0);

        m_telemetry.totalBytesReceived = eth0.bytesReceived;
        m_telemetry.totalBytesSent = eth0.bytesSent;
        m_telemetry.currentDownloadSpeed = (eth0.bytesReceived - m_lastInOctets);
        m_telemetry.currentUploadSpeed = (eth0.bytesSent - m_lastOutOctets);
        m_lastInOctets = eth0.bytesReceived;
        m_lastOutOctets = eth0.bytesSent;
#endif

        m_lastUpdateTime = now;

        // Convert speeds to KB/s for chart visualization
        float dlKb = static_cast<float>(m_telemetry.currentDownloadSpeed) / 1024.0f;
        float ulKb = static_cast<float>(m_telemetry.currentUploadSpeed) / 1024.0f;

        m_telemetry.downloadHistory.Push(dlKb);
        m_telemetry.uploadHistory.Push(ulKb);
    }

} // namespace SystemPulse::Monitors
