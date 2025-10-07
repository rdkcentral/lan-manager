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

#define DEFAULT_BRIDGE_LIFETIME -1   /**< Use either 0 or -1 for infinite bridge lifetime. */

/**
 * @def MAX_NAME_LEN
 * @brief Maximum length for interface and bridge names.
 */
#define MAX_NAME_LEN      512

/**
 * @def MAX_IFACE_COUNT
 * @brief Maximum number of interfaces per bridge.
 */
#define MAX_IFACE_COUNT   16

/**
 * @def MAX_IP_LEN
 * @brief Maximum length for IP address strings.
 */
#define MAX_IP_LEN        40

/**
 * @def MAX_PREFIX_LEN
 * @brief Maximum length for IP prefix strings.
 */
#define MAX_PREFIX_LEN    40

/**
 * @def ALIAS_MAX_LEN
 * @brief Maximum length for bridge alias strings.
 */
#define ALIAS_MAX_LEN     64

/**
 * @enum FirewallLevel
 * @brief Firewall security levels.
 */
enum FirewallLevel {
    FIREWALL_LEVEL_LOW = 0, /**< Low security level. */
    FIREWALL_LEVEL_MEDIUM,  /**< Medium security level. */
    FIREWALL_LEVEL_HIGH,    /**< High security level. */
    FIREWALL_LEVEL_CUSTOM   /**< Custom security level. */
};

/**
 * @enum NetworkBridgeType
 * @brief Supported network bridge types.
 */
typedef enum NetworkBridgeType
{
    LINUX_BRIDGE,    /**< Standard Linux bridge. */
    OVS_BRIDGE       /**< Open vSwitch bridge. */
}NetworkBridgeType;

/**
 * @enum UserBridgeCategory
 * @brief Logical categories for user bridges.
 */
typedef enum UserBridgeCategory {
    PRIVATE_LAN = 1,              /**< Private LAN bridge. */
    HOME_SECURITY,                /**< Home security bridge. */
    HOTSPOT_OPEN_2G,              /**< Open hotspot 2.4GHz. */
    HOTSPOT_OPEN_5G,              /**< Open hotspot 5GHz. */
    LOST_N_FOUND,                 /**< Lost and found network. */
    HOTSPOT_SECURE_2G,            /**< Secure hotspot 2.4GHz. */
    HOTSPOT_SECURE_5G,            /**< Secure hotspot 5GHz. */
    HOTSPOT_SECURE_6G,            /**< Secure hotspot 6GHz. */
    MOCA_ISOLATION,               /**< MoCA isolation bridge. */
    MESH_BACKHAUL,                /**< Mesh backhaul bridge. */
    ETH_BACKHAUL,                 /**< Ethernet backhaul bridge. */
    MESH_WIFI_BACKHAUL_2G,        /**< Mesh WiFi backhaul 2.4GHz. */
    MESH_WIFI_BACKHAUL_5G,        /**< Mesh WiFi backhaul 5GHz. */
    CONNECTED_BUILDING,           /**< Connected building bridge. */
    CONNECTED_BUILDING_2G,        /**< Connected building 2.4GHz. */
    CONNECTED_BUILDING_5G,        /**< Connected building 5GHz. */
    CONNECTED_BUILDING_6G         /**< Connected building 6GHz. */
   // MESH_ONBOARD,
   // MESH_WIFI_ONBOARD_2G
}UserBridgeCategory;

/**
 * @enum INF_TYPE
 * @brief Types of network interfaces.
 */
typedef enum {
    INF_ETH = 1,      /**< Ethernet interface. */
    INF_WIFI = 2,     /**< WiFi interface. */
    INF_VLAN = 3,     /**< VLAN interface. */
    INF_GRE = 4,      /**< GRE tunnel interface. */
    INF_MOCA = 5,     /**< MoCA interface. */
    INF_OTHER = 6     /**< Other interface type. */
} INF_TYPE;

/**
 * @struct Iface
 * @brief Structure representing a network interface configuration.
 */
typedef struct {
    INF_TYPE InfType;                   /**< Type of network interface (see INF_TYPE). */
    char interfaceName[MAX_NAME_LEN];   /**< Name of the interface. */
    bool vlanEnable;                    /**< Whether VLAN is enabled on this interface. */
    int vlanId;                         /**< VLAN ID for the interface. */
} Iface;

/**
 * @struct IPConfig
 * @brief Structure for IP configuration settings.
 */
typedef struct {
    bool Ip_Enable;                     /**< Enable/disable IP configuration. */
    char Ipv4Address[MAX_IP_LEN];       /**< IPv4 address string. */
    char IpSubNet[MAX_IP_LEN];          /**< IPv4 subnet string. */
    char Ipv6Address[MAX_IP_LEN];       /**< IPv6 address string. */
} IPConfig;

/**
 * @struct DHCPV4Config
 * @brief Structure for DHCPv4 configuration.
 */
typedef struct {
    bool Dhcpv4_Enable;                 /**< 1 = enabled, 0 = disabled. */
    char Dhcpv4_Start_Addr[MAX_IP_LEN]; /**< Start address for DHCPv4 pool. */
    char Dhcpv4_End_Addr[MAX_IP_LEN];   /**< End address for DHCPv4 pool. */
    int  Dhcpv4_Lease_Time;             /**< Lease time in seconds. */
} DHCPV4Config;

/**
 * @enum IPv6AddrType
 * @brief Type of IPv6 address to be assigned to clients.
 */
typedef enum {
    IPV6_ADDR_TYPE_GLOBAL, /**< Global IPv6 addresses. */
    IPV6_ADDR_TYPE_ULA     /**< Unique Local Addresses. */
} IPv6AddrType;

/**
 * @struct DHCPV6Config
 * @brief Structure for DHCPv6 configuration.
 */
typedef struct {
    char Ipv6Prefix[MAX_PREFIX_LEN];     /**< IPv6 prefix string (e.g., 2001:db8::/64 or fd00::/64). */
    bool StateFull;                      /**< 1 = stateful DHCPv6 enabled. */
    bool StateLess;                      /**< 1 = stateless DHCPv6 enabled. */
    char Dhcpv6_Start_Addr[MAX_IP_LEN];  /**< Start address for DHCPv6 pool (used only if StateFull is enabled). */
    char Dhcpv6_End_Addr[MAX_IP_LEN];    /**< End address for DHCPv6 pool (used only if StateFull is enabled). */
    IPv6AddrType addrType;               /**< Global or ULA addresses for clients. */
    void *customConfig;                  /**< Pointer for future custom configurations (e.g., PVD/FQDN). */
} DHCPV6Config;

/**
 * @struct FirewallConfig
 * @brief Structure for firewall configuration.
 */
typedef struct {
    int  Firewall_Level;    /**< Corresponds to FirewallLevel enum. */
    bool Firewall_Enable;   /**< 1 = enabled, 0 = disabled. */
} FirewallConfig;

/**
 * @struct SecurityConfig
 * @brief Structure for VPN security configuration.
 */
typedef struct {
    bool VPN_Security_Enable; /**< Enable/disable VPN security. */
} SecurityConfig;

/**
 * @struct BridgeInfo
 * @brief Structure containing bridge information.
 */
typedef struct {
    NetworkBridgeType networkBridgeType;    /**< Type of network bridge (see NetworkBridgeType). */
    UserBridgeCategory userBridgeCategory;  /**< Logical category of user bridge (see UserBridgeCategory). */
    char alias[ALIAS_MAX_LEN];              /**< Alias name for the bridge. */
    int stpEnable;                          /**< 1 = enabled, 0 = disabled. */
    int igdEnable;                          /**< 1 = enabled, 0 = disabled. */
    int bridgeLifeTime;                     /**< Lifetime of the bridge in hours (default: DEFAULT_BRIDGE_LIFETIME). */
    char bridgeName[MAX_NAME_LEN];          /**< Name of the bridge. */
} BridgeInfo;

/**
 * @enum BridgeStatus
 * @brief Status codes for bridge operations.
 */
typedef enum {
    BRIDGE_STATUS_NOT_STARTED = 0, /**< Bridge not started. */
    BRIDGE_STATUS_SUCCESS,         /**< Bridge operation successful. */
    BRIDGE_STATUS_ERROR            /**< Bridge operation error. */
} BridgeStatus;

/**
 * @struct DHCPConfig
 * @brief Wrapper for DHCPv4 and DHCPv6 configurations.
 */
typedef struct {
    DHCPV4Config dhcpv4Config; /**< DHCPv4 configuration. */
    DHCPV6Config dhcpv6Config; /**< DHCPv6 configuration. */
} DHCPConfig;

/**
 * @struct LanConfig
 * @brief Structure for LAN bridge configuration.
 */
typedef struct {
    BridgeInfo bridgeInfo;                  /**< Information about the bridge. */
    int numOfIfaces;                        /**< Number of interfaces in the bridge. */
    Iface interfaces[MAX_IFACE_COUNT];      /**< Array of interface configurations. */
    IPConfig ipConfig;                      /**< IP configuration settings. */
    DHCPConfig dhcpConfig;                  /**< DHCP configuration settings. */
    FirewallConfig firewallConfig;          /**< Firewall configuration settings. */
    SecurityConfig securityConfig;          /**< Security configuration settings. */
    BridgeStatus status;                    /**< Bridge status. */
    // Mode switch , profile based config 
} LanConfig;

/**
 * @struct LanConfigPayload
 * @brief Payload wrapper for FSM.
 */
typedef struct {
    LanConfig *lan_table;   /**< Pointer to an array of LanConfig. */
    int num_entries;        /**< Number of entries in the lan_table. */
} LanConfigPayload;

/**
 * @struct DhcpPayload
 * @brief Payload for DHCP specific configurations.
 */
typedef struct {
    BridgeInfo bridgeInfo; /**< Bridge properties. */
    DHCPConfig dhcpConfig; /**< DHCP settings. */
} DhcpPayload;

/**
 * @struct FirewallPayload
 * @brief Payload for firewall specific configurations.
 */
typedef struct {
    BridgeInfo bridgeInfo;        /**< Bridge properties. */
    FirewallConfig firewallConfig; /**< Firewall settings. */
    SecurityConfig securityConfig; /**< Security settings. */
} FirewallPayload;

/**
 * @brief Retrieves the LAN configuration from the provider.
 * @param[out] configs Pointer to an array of LanConfig structures to be filled.
 * @param[out] count Pointer to an integer that will hold the number of configurations retrieved.
 * @return true if the configuration was retrieved successfully, false otherwise.
 */
bool GetLanConfigFromProvider(LanConfig *configs, int *count);

#endif /* LAN_MANAGER_INTERFACE_H */
