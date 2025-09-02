/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef LAN_MANAGER_INTERFACE_H
#define LAN_MANAGER_INTERFACE_H

#include <stdbool.h>
#include "lan_manager_custom_bridge.h" /* For custom bridge type macros */

/**
 * @file lan_manager_interface.h
 * @brief Network interface and bridge configuration structures
 * 
 * This file defines the data structures for creating and managing
 * network bridges and their configurations.
 */


/**
 * @def MAX_NAME_LEN
 * @brief Maximum length for interface and bridge names
 */
#define MAX_NAME_LEN 512

/**
 * @def MAX_IFACE_COUNT
 * @brief Maximum number of interfaces per bridge
 */
#define MAX_IFACE_COUNT 16

/**
 * @def MAX_IP_LEN
 * @brief Maximum length for IP address strings
 */
#define MAX_IP_LEN 40

/**
 * @def MAX_PREFIX_LEN
 * @brief Maximum length for IP prefix strings
 */
#define MAX_PREFIX_LEN 40

/**
 * @def ALIAS_MAX_LEN
 * @brief Maximum length for bridge alias strings
 */
#define ALIAS_MAX_LEN 64


/**
 * @enum FirewallLevel
 * @brief Firewall security levels
 */
enum FirewallLevel {
    FIREWALL_LEVEL_LOW = 0,      /**< Low security level */
    FIREWALL_LEVEL_MEDIUM,       /**< Medium security level */
    FIREWALL_LEVEL_HIGH,         /**< High security level */
    FIREWALL_LEVEL_CUSTOM        /**< Custom security level */
};


/**
 * @enum NetworkBridgeType
 * @brief Supported network bridge types
 */
enum NetworkBridgeType {
    LINUX_BRIDGE,    /**< Standard Linux bridge */
    OVS_BRIDGE       /**< Open vSwitch bridge */
};


/**
 * @enum UserBridgeCategory
 * @brief Logical categories for user bridges
 */
enum UserBridgeCategory {
    PRIVATE_LAN = 1,              /**< Private LAN bridge */
    HOME_SECURITY,                /**< Home security bridge */
    HOTSPOT_OPEN_2G,              /**< Open hotspot 2.4GHz */
    HOTSPOT_OPEN_5G,              /**< Open hotspot 5GHz */
    LOST_N_FOUND,                 /**< Lost and found network */
    HOTSPOT_SECURE_2G,            /**< Secure hotspot 2.4GHz */
    HOTSPOT_SECURE_5G,            /**< Secure hotspot 5GHz */
    HOTSPOT_SECURE_6G,            /**< Secure hotspot 6GHz */
    MOCA_ISOLATION,               /**< MoCA isolation bridge */
    MESH_BACKHAUL,                /**< Mesh backhaul bridge */
    ETH_BACKHAUL,                 /**< Ethernet backhaul bridge */
    MESH_WIFI_BACKHAUL_2G,        /**< Mesh WiFi backhaul 2.4GHz */
    MESH_WIFI_BACKHAUL_5G,        /**< Mesh WiFi backhaul 5GHz */
    CONNECTED_BUILDING,           /**< Connected building bridge */
    CONNECTED_BUILDING_2G,        /**< Connected building 2.4GHz */
    CONNECTED_BUILDING_5G,        /**< Connected building 5GHz */
    CONNECTED_BUILDING_6G         /**< Connected building 6GHz */
    // MESH_ONBOARD,
    // MESH_WIFI_ONBOARD_2G
};


/**
 * @enum INF_TYPE
 * @brief Types of network interfaces
 */
typedef enum {
    INF_ETH = 1,      /**< Ethernet interface */
    INF_WIFI = 2,     /**< WiFi interface */
    INF_VLAN = 3,     /**< VLAN interface */
    INF_GRE = 4,      /**< GRE tunnel interface */
    INF_MOCA = 5,     /**< MoCA interface */
    INF_OTHER = 6     /**< Other interface type */
} INF_TYPE;

/**
 * @struct Iface
 * @brief Structure representing a network interface configuration
 * @var Iface::InfType Type of network interface (see INF_TYPE)
 * @var Iface::Interfaces Name of the interface
 * @var Iface::vlanId VLAN ID for the interface
 */
typedef struct {
    INF_TYPE InfType;
    char Interfaces[MAX_NAME_LEN];
    int vlanId;
} Iface;

/**
 * @struct IPConfig
 * @brief Structure for IP configuration settings
 * @var IPConfig::Ip_Enable Enable/disable IP configuration
 * @var IPConfig::Ipv4Address IPv4 address string
 * @var IPConfig::IpSubNet IPv4 subnet string
 * @var IPConfig::Ipv6Address IPv6 address string
 */
typedef struct {
    bool Ip_Enable;
    char Ipv4Address[MAX_IP_LEN];
    char IpSubNet[MAX_IP_LEN];
    char Ipv6Address[MAX_IP_LEN];
} IPConfig;

/**
 * @struct DHCPConfig
 * @brief Structure for DHCPv4 configuration
 * @var DHCPConfig::Dhcpv4_Enable Enable/disable DHCPv4
 * @var DHCPConfig::Dhcpv4_Start_Addr Start address for DHCPv4 pool
 * @var DHCPConfig::Dhcpv4_End_Addr End address for DHCPv4 pool
 * @var DHCPConfig::Dhcpv4_Lease_Time Lease time in seconds
 */
typedef struct {
    bool Dhcpv4_Enable; /**< 1 = enabled, 0 = disabled */
    char Dhcpv4_Start_Addr[MAX_IP_LEN];
    char Dhcpv4_End_Addr[MAX_IP_LEN];
    int Dhcpv4_Lease_Time; /**< in seconds */
} DHCPConfig;

/**
 * @struct DHCPV6Config
 * @brief Structure for DHCPv6 configuration
 * @var DHCPV6Config::Ipv6Prefix IPv6 prefix string
 * @var DHCPV6Config::StateFull Enable/disable stateful DHCPv6
 * @var DHCPV6Config::StateLess Enable/disable stateless DHCPv6
 * @var DHCPV6Config::Dhcpv6_Start_Addr Start address for DHCPv6 pool
 * @var DHCPV6Config::Dhcpv6_End_Addr End address for DHCPv6 pool
 */
typedef struct {
    char Ipv6Prefix[MAX_PREFIX_LEN];
    bool StateFull; /**< 1 = enabled, 0 = disabled */
    bool StateLess; /**< 1 = enabled, 0 = disabled */
    char Dhcpv6_Start_Addr[MAX_IP_LEN];
    char Dhcpv6_End_Addr[MAX_IP_LEN];
} DHCPV6Config;

/**
 * @struct FirewallConfig
 * @brief Structure for firewall configuration
 * @var FirewallConfig::Firewall_Level Firewall security level (see FirewallLevel)
 * @var FirewallConfig::Firewall_Enable Enable/disable firewall
 */
typedef struct {
    int Firewall_Level; /**< Corresponds to FirewallLevel enum */
    bool Firewall_Enable; /**< 1 = enabled, 0 = disabled */
} FirewallConfig;

/**
 * @struct SecurityConfig
 * @brief Structure for VPN security configuration
 * @var SecurityConfig::VPN_Security_Enable Enable/disable VPN security
 */
typedef struct {
    bool VPN_Security_Enable;
} SecurityConfig;

/**
 * @struct BridgeInfo
 * @brief Structure containing bridge information
 * @var BridgeInfo::networkBridgeType Type of network bridge (see NetworkBridgeType)
 * @var BridgeInfo::userBridgeCategory Logical category of user bridge (see UserBridgeCategory)
 * @var BridgeInfo::alias Alias name for the bridge
 * @var BridgeInfo::bridgeName Name of the bridge
 */
typedef struct {
    enum NetworkBridgeType networkBridgeType;
    enum UserBridgeCategory userBridgeCategory;
    char alias[ALIAS_MAX_LEN];
    char bridgeName[MAX_NAME_LEN];
} BridgeInfo;

/**
 * @enum BridgeStatus
 * @brief Status codes for bridge operations
 */
typedef enum {
    BRIDGE_STATUS_NOT_STARTED = 0, /**< Bridge not started */
    BRIDGE_STATUS_SUCCESS,         /**< Bridge operation successful */
    BRIDGE_STATUS_ERROR            /**< Bridge operation error */
} BridgeStatus;

/**
 * @struct LanConfig
 * @brief Structure for LAN bridge configuration
 * @var LanConfig::bridgeInfo Information about the bridge
 * @var LanConfig::numOfIfaces Number of interfaces in the bridge
 * @var LanConfig::ifaces Array of interface configurations
 * @var LanConfig::ipConfig IP configuration settings
 * @var LanConfig::dhcpConfig DHCPv4 configuration settings
 * @var LanConfig::dhcpv6Config DHCPv6 configuration settings
 * @var LanConfig::firewallConfig Firewall configuration settings
 * @var LanConfig::securityConfig Security configuration settings
 * @var LanConfig::IGD_Enable Enable/disable IGD (Internet Gateway Device)
 * @var LanConfig::status Bridge status
 */
typedef struct {
    BridgeInfo bridgeInfo;
    int numOfIfaces;
    Iface ifaces[MAX_IFACE_COUNT];
    IPConfig ipConfig;
    DHCPConfig dhcpConfig;
    DHCPV6Config dhcpv6Config;
    FirewallConfig firewallConfig;
    SecurityConfig securityConfig;
    bool IGD_Enable;
    BridgeStatus status;
    uint32_t instNum;
    char alias[ALIAS_MAX_LEN];
} LanConfig;

#endif /* LAN_MANAGER_INTERFACE_H */
