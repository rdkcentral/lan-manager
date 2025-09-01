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

#ifndef LAN_MANAGER_BRIDGE_H
#define LAN_MANAGER_BRIDGE_H

#include "commonutil.h"
#include "lan_manager_custom_bridge.h" /* For custom bridge macros */
#include "lan_manager_interface.h" /* For MAX_IFACE_COUNT, LanConfig, etc. */

/**
 * @brief Main home network bridge entry macro (PRIVATE_LAN)
 */
#define PRIVATE_LAN_BRIDGE_ENTRY \
    { \
        .bridgeInfo = { \
            .networkBridgeType = { "", PSM_DB }, \
            .userBridgeCategory = { "", PSM_DB }, \
            .alias = { "", PSM_DB }, \
            .bridgeName = { "dmsb.l2net.%d.Name", PSM_DB }, \
            .l2net_index = { "dmsb.MultiLAN.PrimaryLAN_l2net", PSM_DB }, \
            .numOfIfaces = { "", PSM_DB }, \
            .status = { "", PSM_DB } \
        }, \
        .ifaces = { \
            { .InfType = { "", PSM_DB }, \
              .Interfaces = { "dmsb.l2net.%d.Members.Eth", PSM_DB }, \
              .vlanId = { "", PSM_DB } }, \
            { .InfType = { "", PSM_DB }, \
              .Interfaces = { "dmsb.l2net.%d.Members.WiFi", PSM_DB }, \
              .vlanId = { "", PSM_DB } }, \
            { .InfType = { "", PSM_DB }, \
              .Interfaces = { "dmsb.l2net.%d.Members.Moca", PSM_DB }, \
              .vlanId = { "", PSM_DB } } \
        }, \
        .ipConfig = { \
            .Ip_Enable = { "dmsb.l3net.%d.IPv4Enable", PSM_DB }, \
            .Ipv4Address = { "dmsb.l3net.%d.V4Addr", PSM_DB }, \
            .IpSubNet = { "dmsb.l3net.%d.V4SubnetMask", PSM_DB }, \
            .Ipv6Address = { "", PSM_DB }, \
            .l3net_index = { "dmsb.MultiLAN.PrimaryLAN_l3net", PSM_DB } \
        }, \
        .dhcpConfig = { \
            .dhcpv4_index = { "", PSM_DB }, \
            .Dhcpv4_Enable = { "dhcp_server_enabled", SYSCFG_DB }, \
            .Dhcpv4_Start_Addr = { "dhcp_start", SYSCFG_DB }, \
            .Dhcpv4_End_Addr = { "dhcp_end", SYSCFG_DB }, \
            .Dhcpv4_Lease_Time = { "dhcp_lease_time", SYSCFG_DB } \
        }, \
        .dhcpv6Config = { \
            .Ipv6Prefix = { "", PSM_DB }, \
            .StateFull = { "", PSM_DB }, \
            .StateLess = { "", PSM_DB }, \
            .Dhcpv6_Start_Addr = { "", PSM_DB }, \
            .Dhcpv6_End_Addr = { "", PSM_DB } \
        }, \
        .firewallConfig = { \
            .Firewall_Level = { "firewall_level", SYSCFG_DB }, \
            .Firewall_Enable = { "firewall_enable", SYSCFG_DB } \
        }, \
        .securityConfig = { \
            .VPN_Security_Enable = { "", SYSCFG_DB } \
        }, \
        .igdConfig = { .IGD_Enable = { "", SYSCFG_DB } } \
    }

/**
 * @brief Home security network bridge entry macro (HOME_SECURITY)
 */
#define HOME_SECURITY_BRIDGE_ENTRY \
    { \
        .bridgeInfo = { \
            .networkBridgeType = { "", PSM_DB }, \
            .userBridgeCategory = { "", PSM_DB }, \
            .alias = { "", PSM_DB }, \
            .bridgeName = { "dmsb.l2net.%d.Name", PSM_DB }, \
            .l2net_index = { "dmsb.MultiLAN.HomeSecurity_l2net", PSM_DB }, \
            .numOfIfaces = { "", PSM_DB }, \
            .status = { "", PSM_DB } \
        }, \
        .ifaces = { \
            { .InfType = { "", PSM_DB }, \
              .Interfaces = { "dmsb.l2net.%d.Members.WiFi", PSM_DB }, \
              .vlanId = { "", PSM_DB } } \
        }, \
        .ipConfig = { \
            .Ip_Enable = { "dmsb.l3net.%d.IPv4Enable", PSM_DB }, \
            .Ipv4Address = { "dmsb.l3net.%d.V4Addr", PSM_DB }, \
            .IpSubNet = { "dmsb.l3net.%d.V4SubnetMask", PSM_DB }, \
            .Ipv6Address = { "", PSM_DB }, \
            .l3net_index = { "dmsb.MultiLAN.HomeSecurity_l3net", PSM_DB } \
        }, \
        .dhcpConfig = { \
            .dhcpv4_index = { "dmsb.MultiLAN.HomeSecurity_DHCPv4ServerPool", PSM_DB }, \
            .Dhcpv4_Enable = { "dmsb.dhcpv4.server.pool.%d.Enable", PSM_DB }, \
            .Dhcpv4_Start_Addr = { "dmsb.dhcpv4.server.pool.%d.MinAddress", PSM_DB }, \
            .Dhcpv4_End_Addr = { "dmsb.dhcpv4.server.pool.%d.MaxAddress", PSM_DB }, \
            .Dhcpv4_Lease_Time = { "dmsb.dhcpv4.server.pool.%d.LeaseTime", PSM_DB } \
        }, \
        .dhcpv6Config = { \
            .Ipv6Prefix = { "", PSM_DB }, \
            .StateFull = { "", PSM_DB }, \
            .StateLess = { "", PSM_DB }, \
            .Dhcpv6_Start_Addr = { "", PSM_DB }, \
            .Dhcpv6_End_Addr = { "", PSM_DB } \
        }, \
        .firewallConfig = { \
            .Firewall_Level = { "firewall_level", SYSCFG_DB }, \
            .Firewall_Enable = { "firewall_enable", SYSCFG_DB } \
        }, \
        .securityConfig = { \
            .VPN_Security_Enable = { "", SYSCFG_DB } \
        }, \
        .igdConfig = { .IGD_Enable = { "", SYSCFG_DB } } \
    }

/**
 * @brief Sentinel node macro for marking the end of configuration tables
 *
 * This macro represents a configuration entry with all NULL/0 values
 * to indicate the end of a configuration array.
 */
#define LAN_CONFIG_SOURCE_SENTINEL \
    { \
        .bridgeInfo = { \
            .networkBridgeType = { NULL, 0 }, \
            .userBridgeCategory = { NULL, 0 }, \
            .alias = { NULL, 0 }, \
            .bridgeName = { NULL, 0 }, \
            .l2net_index = { NULL, 0 }, \
            .numOfIfaces = { NULL, 0 }, \
            .status = { NULL, 0 } \
        }, \
        .ifaces = { \
            { .InfType = { NULL, 0 }, .Interfaces = { NULL, 0 }, .vlanId = { NULL, 0 } }, \
        }, \
        .ipConfig = { \
            .Ip_Enable = { NULL, 0 }, \
            .Ipv4Address = { NULL, 0 }, \
            .IpSubNet = { NULL, 0 }, \
            .Ipv6Address = { NULL, 0 }, \
            .l3net_index = { NULL, 0 } \
        }, \
        .dhcpConfig = { \
            .dhcpv4_index = { NULL, 0 }, \
            .Dhcpv4_Enable = { NULL, 0 }, \
            .Dhcpv4_Start_Addr = { NULL, 0 }, \
            .Dhcpv4_End_Addr = { NULL, 0 }, \
            .Dhcpv4_Lease_Time = { NULL, 0 } \
        }, \
        .dhcpv6Config = { \
            .Ipv6Prefix = { NULL, 0 }, \
            .StateFull = { NULL, 0 }, \
            .StateLess = { NULL, 0 }, \
            .Dhcpv6_Start_Addr = { NULL, 0 }, \
            .Dhcpv6_End_Addr = { NULL, 0 } \
        }, \
        .firewallConfig = { \
            .Firewall_Level = { NULL, 0 }, \
            .Firewall_Enable = { NULL, 0 } \
        }, \
        .securityConfig = { \
            .VPN_Security_Enable = { NULL, 0 } \
        }, \
        .igdConfig = { .IGD_Enable = { NULL, 0 } } \
    }

/**
 * @file lan_manager_bridge.h
 * @brief Bridge database mapping and configuration structures
 * 
 * This file defines the database mappings for network settings and provides
 * functions to load these settings into memory. It works with the structure
 * definitions in lan_manager_interface.h to create a complete configuration
 * system for network bridges.
 * 
 * The key components are:
 * 1. Database mapping structures (LanConfigSource and related types)
 * 2. The global bridge configuration array (g_bridges)
 * 3. Functions to populate bridge configurations from the database
 * 
 * The system supports extensibility through the lan_manager_custom_bridge.h
 * file, which allows adding custom bridge types and configurations without
 * modifying the core code.
 */

/**
 * @brief Database mapping structures
 *
 * The following structures define where to find each network setting in the system database.
 * They map configuration parameters to their location in either PSM_DB or SYSCFG_DB.
 * Each structure represents a different aspect of network configuration.
 */

/**
 * @brief Bridge Info Source - Where to find basic network info in the database
 */
/**
 * @brief Bridge Info Source - Where to find basic network info in the database
 *
 * l2net_index and l3net_index must be set to the PSM parameter names (and source)
 * (e.g., {"dmsb.MultiLAN.PrimaryLAN_l2net", PSM_DB}) to dynamically fetch the l2/l3 index for this bridge.
 * These fields are required for correct index lookup and must not be NULL.
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } networkBridgeType; /**< Bridge type (LINUX_BRIDGE/OVS_BRIDGE) */
    struct { const char *param; paramDbName_t src; } userBridgeCategory;  /**< Name of user bridge category setting */
    struct { const char *param; paramDbName_t src; } alias; /**< Bridge alias */
    struct { const char *param; paramDbName_t src; } bridgeName;  /**< Name of bridge name setting */
    struct { const char *param; paramDbName_t src; } l2net_index; /**< Key/src to fetch l2net index (e.g., {"dmsb.MultiLAN.PrimaryLAN_l2net", PSM_DB}) */
    struct { const char *param; paramDbName_t src; } numOfIfaces; /**< Number of interfaces */
    struct { const char *param; paramDbName_t src; } status; /**< Bridge status */
} BridgeInfoSource;

/**
 * @brief Interface Source - Where to find interface settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } InfType;     /**< Where to find IP version setting */
    struct { const char *param; paramDbName_t src; } Interfaces;  /**< Where to find interface list */
    struct { const char *param; paramDbName_t src; } vlanId;      /**< VLAN ID for the interface */
} IfaceSource;

/**
 * @brief IP Config Source - Where to find IP settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } Ip_Enable;   /**< IP enable/disable setting */
    struct { const char *param; paramDbName_t src; } Ipv4Address; /**< IP address setting */
    struct { const char *param; paramDbName_t src; } IpSubNet;    /**< Subnet mask setting */
    struct { const char *param; paramDbName_t src; } Ipv6Address; /**< IPv6 address setting */
    struct { const char *param; paramDbName_t src; } l3net_index; /**< Key/src to fetch l3net index (e.g., {"dmsb.MultiLAN.PrimaryLAN_l3net", PSM_DB}) */
} IPConfigSource;

/**
 * @brief DHCP Config Source - Where to find DHCP server settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } dhcpv4_index;         /**< Key/src to fetch DHCPv4 server pool index */
    struct { const char *param; paramDbName_t src; } Dhcpv4_Enable;        /**< DHCP enable setting */
    struct { const char *param; paramDbName_t src; } Dhcpv4_Start_Addr;    /**< First DHCP address */
    struct { const char *param; paramDbName_t src; } Dhcpv4_End_Addr;      /**< Last DHCP address */
    struct { const char *param; paramDbName_t src; } Dhcpv4_Lease_Time;    /**< How long IPs are valid */
} DHCPConfigSource;

/**
 * @brief IPv6 Config Source - Where to find IPv6 settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } Ipv6Prefix;         /**< IPv6 network prefix */
    struct { const char *param; paramDbName_t src; } StateFull;          /**< DHCPv6 enable setting */
    struct { const char *param; paramDbName_t src; } StateLess;          /**< SLAAC enable setting */
    struct { const char *param; paramDbName_t src; } Dhcpv6_Start_Addr;  /**< First DHCPv6 address */
    struct { const char *param; paramDbName_t src; } Dhcpv6_End_Addr;    /**< Last DHCPv6 address */
} DHCPV6ConfigSource;

/**
 * @brief Firewall Config Source - Where to find firewall settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } Firewall_Level;   /**< Security level setting */
    struct { const char *param; paramDbName_t src; } Firewall_Enable;  /**< Firewall on/off setting */
} FirewallConfigSource;

/**
 * @brief Security Config Source - Where to find security settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } VPN_Security_Enable; /**< VPN setting */
} SecurityConfigSource;

/**
 * @brief IGD Config Source - Where to find IGD settings in the database
 */
typedef struct {
    struct { const char *param; paramDbName_t src; } IGD_Enable; /**< IGD on/off setting */
} IGDConfigSource;

/**
 * @brief LAN Config Source - Master structure that holds all database locations
 * 
 * This defines the complete map of where to find all network settings in the database.
 * Each field points to a database parameter name and source (PSM_DB or SYSCFG_DB).
 * 
 * The lan_config_source_table array (defined in lan_manager_bridge.c) contains
 * instances of this structure, one for each bridge configuration. The array includes:
 * - Standard bridge configurations (Private LAN, Home Security)
 * - Custom bridge configurations added via the CUSTOM_BRIDGE_TABLE_ENTRIES macro
 * - A sentinel node to mark the end of the array
 * 
 * @see lan_config_source_table
 * @see CUSTOM_BRIDGE_TABLE_ENTRIES
 */
typedef struct {
    BridgeInfoSource bridgeInfo;                /**< Basic network info sources */
    IfaceSource ifaces[MAX_IFACE_COUNT];        /**< Interface setting sources */
    IPConfigSource ipConfig;                    /**< IP setting sources */
    DHCPConfigSource dhcpConfig;                /**< DHCP setting sources */
    DHCPV6ConfigSource dhcpv6Config;            /**< DHCPv6 setting sources */
    FirewallConfigSource firewallConfig;        /**< Firewall setting sources */
    SecurityConfigSource securityConfig;        /**< Security setting sources */
    IGDConfigSource igdConfig;                  /**< IGD setting sources */
} LanConfigSource;

/**
 * @brief Array of all active network bridges
 *
 * This global array stores the runtime configurations for all network bridges.
 * It is populated by the PopulateAllBridges() function, which reads settings
 * from the database. Each element in the array represents a complete bridge
 * configuration.
 * 
 * The array size matches the number of configurations defined in lan_config_source_table,
 * which includes the standard bridges (Private LAN, Home Security) as well as any
 * custom bridges defined through the CUSTOM_BRIDGE_TABLE_ENTRIES macro.
 */
extern LanConfig g_bridges[];

/**
 * @brief Loads all network settings from the database into memory
 * 
 * This function reads all network settings from the database based on the mapping
 * in lan_config_source_table and populates the g_bridges array with the current
 * network configurations. For each setting, it:
 * 
 * 1. Calls GetValueFromDb() to retrieve the value
 * 2. Checks if retrieval was successful
 * 3. Handles any errors by logging and setting appropriate defaults
 * 4. Stores the value in the corresponding field in g_bridges
 *
 * Each bridge configuration is populated with settings for:
 * - Basic bridge information
 * - Network interfaces
 * - IP configuration
 * - DHCP server settings
 * - IPv6 configuration
 * - Firewall settings
 * - Security settings
 * - IGD settings
 * 
 * @return void - The function populates the global g_bridges array
 */
void PopulateAllBridges();

#endif /* LAN_MANAGER_BRIDGE_H */
