/**
 * @file lan_manager_bridge.c
 *
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

#include <stdio.h>
#include <string.h>
#include "lanmgr_log.h"
#include "commonutil.h"
#include "lan_manager_interface.h"
#include "lan_manager_bridge.h"
#include "lan_manager_dml.h"
#include "lanmgr_communication_apis.h"
#include "lan_managerdb.h" /* Persistence API header */

/*
 * PersistLanConfig: Writes current LanConfig into persistent datastore via
 * individual SetLanConfig* APIs. Failures are logged but non-fatal so that
 * partial persistence does not block remaining sections.
 */
static void PersistLanConfig(const LanConfig *cfg)
{
    if(!cfg) return;
    const char *alias = cfg->bridgeInfo.alias;
    LM_Status st;

    st = SetLanConfigBridgeInfo(alias, &cfg->bridgeInfo);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigBridgeInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigInterfaceCountInfo(alias, &cfg->numOfIfaces);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigInterfaceCountInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigInterfaceInfo(alias, cfg->ifaces);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigInterfaceInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigDhcpInfo(alias, &cfg->dhcpConfig);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigDhcpInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigIPConfigInfo(alias, &cfg->ipConfig);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigIPConfigInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigDhcpv6ConfigInfo(alias, &cfg->dhcpv6Config);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigDhcpv6ConfigInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigFirewallConfigInfo(alias, &cfg->firewallConfig);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigFirewallConfigInfo failed (%d) for %s\n", st, alias));

    st = SetLanConfigSecurityConfigInfo(alias, &cfg->securityConfig);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigSecurityConfigInfo failed (%d) for %s\n", st, alias));

#ifdef ENABLE_IGD_DB_PERSISTENCE
    st = SetLanConfigIGDEnableConfigInfo(alias, &cfg->IGD_Enable);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigIGDEnableConfigInfo failed (%d) for %s\n", st, alias));
#endif

    st = SetLanConfigStatusConfigInfo(alias, &cfg->status);
    if(st != LM_SUCCESS)
        LanManagerError(("PersistLanConfig: SetLanConfigStatusConfigInfo failed (%d) for %s\n", st, alias));
}

/**
 * @brief Network settings map - database location for each configuration parameter
 * 
 * This table maps each network setting to its location in the database.
 * It tells the system where to find all the configuration values needed to set up networks.
 * 
 * The first entry is the main home network, the second is for home security devices,
 * and additional networks can be added through the custom bridge entries.
 */
LanConfigSource lan_config_source_table[] = {
    PRIVATE_LAN_BRIDGE_ENTRY,
    HOME_SECURITY_BRIDGE_ENTRY,
#ifdef CUSTOM_BRIDGE_TABLE_ENTRIES
    CUSTOM_BRIDGE_TABLE_ENTRIES,
#endif
    /** Sentinel node - marks the end of the configuration table */
    LAN_CONFIG_SOURCE_SENTINEL
};


/**
 * @brief Loads all network settings from the database
 * 
 * This function reads all network settings from the database based on the
 * mapping in lan_config_source_table and puts them into memory (g_bridges array)
 * where the router can use them to create and manage networks.
 * 
 * It handles errors gracefully and sets safe defaults if any settings can't be read.
 * 
 * @return void - The function populates the global g_bridges array
 */

static void PopulateBridgeInfo(int i, LanConfig *cfg, const LanConfigSource *src, int l2net_idx, char *param_buf) {
    cfg->bridgeInfo.userBridgeCategory = (enum UserBridgeCategory)i;
    bool result;
    // Read bridge name
    if (src->bridgeInfo.bridgeName.param) {
        snprintf(param_buf, 256, src->bridgeInfo.bridgeName.param, l2net_idx);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for bridgeName: param_buf=%s, src=%d\n", param_buf, src->bridgeInfo.bridgeName.src));
        result = GetValueFromDb(param_buf, cfg->bridgeInfo.bridgeName, PARAM_STRING, src->bridgeInfo.bridgeName.src);
        if (!result || cfg->bridgeInfo.bridgeName[0] == '\0') {
            LanManagerError(("Failed to get bridge name for bridge %d, skipping bridge. result=%d\n", i, (int)result));
            return;
        } else {
            LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got bridge name for bridge %d: '%s'\n", i, cfg->bridgeInfo.bridgeName));
        }
    }
    // Read networkBridgeType
    if (src->bridgeInfo.networkBridgeType.param) {
        snprintf(param_buf, 256, src->bridgeInfo.networkBridgeType.param, l2net_idx);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for networkBridgeType: param_buf=%s, src=%d\n", param_buf, src->bridgeInfo.networkBridgeType.src));
        result = GetValueFromDb(param_buf, &cfg->bridgeInfo.networkBridgeType, PARAM_STRING, src->bridgeInfo.networkBridgeType.src);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got networkBridgeType for bridge %d: '%d'\n", i, cfg->bridgeInfo.networkBridgeType));
    }
    // Read alias
    if (src->bridgeInfo.alias.param) {
        snprintf(param_buf, 256, src->bridgeInfo.alias.param, l2net_idx);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for alias: param_buf=%s, src=%d\n", param_buf, src->bridgeInfo.alias.src));
        result = GetValueFromDb(param_buf, cfg->bridgeInfo.alias, PARAM_STRING, src->bridgeInfo.alias.src);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got alias for bridge %d: '%s'\n", i, cfg->bridgeInfo.alias));
    }
    // Initialize numOfIfaces to 0; increment as we populate interfaces
    cfg->numOfIfaces = 0;
    // Read status
    if (src->bridgeInfo.status.param) {
        snprintf(param_buf, 256, src->bridgeInfo.status.param, l2net_idx);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for status: param_buf=%s, src=%d\n", param_buf, src->bridgeInfo.status.src));
        result = GetValueFromDb(param_buf, &cfg->status, PARAM_INT, src->bridgeInfo.status.src);
        LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got status for bridge %d: %d\n", i, cfg->status));
    }
    // Read network interfaces settings
    for (int j = 0; j < MAX_IFACE_COUNT; ++j) {
        // Read Interfaces
        if (src->ifaces[j].Interfaces.param) {
            snprintf(param_buf, 256, src->ifaces[j].Interfaces.param, l2net_idx);
            LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for Interfaces: param_buf=%s, src=%d, iface=%d\n", param_buf, src->ifaces[j].Interfaces.src, j));
            result = GetValueFromDb(param_buf, cfg->ifaces[j].Interfaces, PARAM_STRING, src->ifaces[j].Interfaces.src);
            if (!result) {
                LanManagerError(("Failed to get interfaces for bridge %d, iface %d. result=%d\n", i, j, (int)result));
            } else {
                LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got interfaces for bridge %d, iface %d: '%s'\n", i, j, cfg->ifaces[j].Interfaces));
                cfg->numOfIfaces++;
            }
        }
        // Read InfType
        if (src->ifaces[j].InfType.param) {
            snprintf(param_buf, 256, src->ifaces[j].InfType.param, l2net_idx);
            LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for InfType: param_buf=%s, src=%d, iface=%d\n", param_buf, src->ifaces[j].InfType.src, j));
            result = GetValueFromDb(param_buf, &cfg->ifaces[j].InfType, PARAM_INT, src->ifaces[j].InfType.src);
            if (!result) {
                LanManagerError(("Failed to get interface type for bridge %d, iface %d. result=%d\n", i, j, (int)result));
            } else {
                LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got interface type for bridge %d, iface %d: %d\n", i, j, cfg->ifaces[j].InfType));
            }
        }
        // Read vlanId
        if (src->ifaces[j].vlanId.param) {
            snprintf(param_buf, 256, src->ifaces[j].vlanId.param, l2net_idx);
            LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Calling GetValueFromDb for vlanId: param_buf=%s, src=%d, iface=%d\n", param_buf, src->ifaces[j].vlanId.src, j));
            result = GetValueFromDb(param_buf, &cfg->ifaces[j].vlanId, PARAM_INT, src->ifaces[j].vlanId.src);
            LanManagerInfo(("[DEBUG][PopulateBridgeInfo] Got vlanId for bridge %d, iface %d: %d\n", i, j, cfg->ifaces[j].vlanId));
        }
    }
}

static void PopulateIpConfig(int i, LanConfig *cfg, const LanConfigSource *src, int l3net_idx, char *param_buf) {
    bool result;
    // Read Ip_Enable
    if (src->ipConfig.Ip_Enable.param) {
        snprintf(param_buf, 256, src->ipConfig.Ip_Enable.param, l3net_idx);
        LanManagerInfo(("[DEBUG][PopulateIpConfig] Calling GetValueFromDb for Ip_Enable: param_buf=%s, src=%d\n", param_buf, src->ipConfig.Ip_Enable.src));
        result = GetValueFromDb(param_buf, &cfg->ipConfig.Ip_Enable, PARAM_BOOLEAN, src->ipConfig.Ip_Enable.src);
        if (!result) {
            LanManagerError(("Failed to get IP enable status for bridge %d. result=%d\n", i, (int)result));
            cfg->ipConfig.Ip_Enable = false;
        } else {
            LanManagerInfo(("[DEBUG][PopulateIpConfig] Got Ip_Enable for bridge %d: %d\n", i, cfg->ipConfig.Ip_Enable));
        }
    }
    // Read Ipv4Address
    if (src->ipConfig.Ipv4Address.param) {
        snprintf(param_buf, 256, src->ipConfig.Ipv4Address.param, l3net_idx);
        LanManagerInfo(("[DEBUG][PopulateIpConfig] Calling GetValueFromDb for Ipv4Address: param_buf=%s, src=%d\n", param_buf, src->ipConfig.Ipv4Address.src));
        result = GetValueFromDb(param_buf, cfg->ipConfig.Ipv4Address, PARAM_STRING, src->ipConfig.Ipv4Address.src);
        if (!result) {
            LanManagerError(("Failed to get IPv4 address for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateIpConfig] Got Ipv4Address for bridge %d: '%s'\n", i, cfg->ipConfig.Ipv4Address));
        }
    }
    // Read IpSubNet
    if (src->ipConfig.IpSubNet.param) {
        snprintf(param_buf, 256, src->ipConfig.IpSubNet.param, l3net_idx);
        LanManagerInfo(("[DEBUG][PopulateIpConfig] Calling GetValueFromDb for IpSubNet: param_buf=%s, src=%d\n", param_buf, src->ipConfig.IpSubNet.src));
        result = GetValueFromDb(param_buf, cfg->ipConfig.IpSubNet, PARAM_STRING, src->ipConfig.IpSubNet.src);
        if (!result) {
            LanManagerError(("Failed to get subnet mask for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateIpConfig] Got IpSubNet for bridge %d: '%s'\n", i, cfg->ipConfig.IpSubNet));
        }
    }
    // Read Ipv6Address
    if (src->ipConfig.Ipv6Address.param) {
        snprintf(param_buf, 256, src->ipConfig.Ipv6Address.param, l3net_idx);
        LanManagerInfo(("[DEBUG][PopulateIpConfig] Calling GetValueFromDb for Ipv6Address: param_buf=%s, src=%d\n", param_buf, src->ipConfig.Ipv6Address.src));
        result = GetValueFromDb(param_buf, cfg->ipConfig.Ipv6Address, PARAM_STRING, src->ipConfig.Ipv6Address.src);
        LanManagerInfo(("[DEBUG][PopulateIpConfig] Got Ipv6Address for bridge %d: '%s'\n", i, cfg->ipConfig.Ipv6Address));
    }
}

static void PopulateDhcpConfig(int i, LanConfig *cfg, const LanConfigSource *src, int dhcpv4_idx, char *param_buf) {
    bool result;
    // Read Dhcpv4_Enable
    if (src->dhcpConfig.Dhcpv4_Enable.param) {
        snprintf(param_buf, 256, src->dhcpConfig.Dhcpv4_Enable.param, dhcpv4_idx);
        LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Calling GetValueFromDb for Dhcpv4_Enable: param_buf=%s, src=%d\n", param_buf, src->dhcpConfig.Dhcpv4_Enable.src));
        result = GetValueFromDb(param_buf, &cfg->dhcpConfig.Dhcpv4_Enable, PARAM_BOOLEAN, src->dhcpConfig.Dhcpv4_Enable.src);
        if (!result) {
            LanManagerError(("Failed to get DHCP enable status for bridge %d. result=%d\n", i, (int)result));
            cfg->dhcpConfig.Dhcpv4_Enable = false;
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Got Dhcpv4_Enable for bridge %d: %d\n", i, cfg->dhcpConfig.Dhcpv4_Enable));
        }
    }
    // Read Dhcpv4_Start_Addr
    if (src->dhcpConfig.Dhcpv4_Start_Addr.param) {
        snprintf(param_buf, 256, src->dhcpConfig.Dhcpv4_Start_Addr.param, dhcpv4_idx);
        LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Calling GetValueFromDb for Dhcpv4_Start_Addr: param_buf=%s, src=%d\n", param_buf, src->dhcpConfig.Dhcpv4_Start_Addr.src));
        result = GetValueFromDb(param_buf, cfg->dhcpConfig.Dhcpv4_Start_Addr, PARAM_STRING, src->dhcpConfig.Dhcpv4_Start_Addr.src);
        if (!result) {
            LanManagerError(("Failed to get DHCP start address for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Got Dhcpv4_Start_Addr for bridge %d: '%s'\n", i, cfg->dhcpConfig.Dhcpv4_Start_Addr));
        }
    }
    // Read Dhcpv4_End_Addr
    if (src->dhcpConfig.Dhcpv4_End_Addr.param) {
        snprintf(param_buf, 256, src->dhcpConfig.Dhcpv4_End_Addr.param, dhcpv4_idx);
        LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Calling GetValueFromDb for Dhcpv4_End_Addr: param_buf=%s, src=%d\n", param_buf, src->dhcpConfig.Dhcpv4_End_Addr.src));
        result = GetValueFromDb(param_buf, cfg->dhcpConfig.Dhcpv4_End_Addr, PARAM_STRING, src->dhcpConfig.Dhcpv4_End_Addr.src);
        if (!result) {
            LanManagerError(("Failed to get DHCP end address for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Got Dhcpv4_End_Addr for bridge %d: '%s'\n", i, cfg->dhcpConfig.Dhcpv4_End_Addr));
        }
    }
    // Read Dhcpv4_Lease_Time
    if (src->dhcpConfig.Dhcpv4_Lease_Time.param) {
        snprintf(param_buf, 256, src->dhcpConfig.Dhcpv4_Lease_Time.param, dhcpv4_idx);
        LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Calling GetValueFromDb for Dhcpv4_Lease_Time: param_buf=%s, src=%d\n", param_buf, src->dhcpConfig.Dhcpv4_Lease_Time.src));
        result = GetValueFromDb(param_buf, &cfg->dhcpConfig.Dhcpv4_Lease_Time, PARAM_INT, src->dhcpConfig.Dhcpv4_Lease_Time.src);
        if (!result) {
            LanManagerError(("Failed to get DHCP lease time for bridge %d. result=%d\n", i, (int)result));
            cfg->dhcpConfig.Dhcpv4_Lease_Time = 86400;
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpConfig] Got Dhcpv4_Lease_Time for bridge %d: %d\n", i, cfg->dhcpConfig.Dhcpv4_Lease_Time));
        }
    }
}

static void PopulateDhcpv6Config(int i, LanConfig *cfg, const LanConfigSource *src, char *param_buf) {
    bool result;
    // Read Ipv6Prefix
    if (src->dhcpv6Config.Ipv6Prefix.param) {
        LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Calling GetValueFromDb for Ipv6Prefix: param=%s, src=%d\n", src->dhcpv6Config.Ipv6Prefix.param, src->dhcpv6Config.Ipv6Prefix.src));
        result = GetValueFromDb((char*)src->dhcpv6Config.Ipv6Prefix.param, cfg->dhcpv6Config.Ipv6Prefix, PARAM_STRING, src->dhcpv6Config.Ipv6Prefix.src);
        if (!result) {
            LanManagerError(("Failed to get IPv6 prefix for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Got Ipv6Prefix for bridge %d: '%s'\n", i, cfg->dhcpv6Config.Ipv6Prefix));
        }
    }
    // Read StateFull
    if (src->dhcpv6Config.StateFull.param) {
        LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Calling GetValueFromDb for StateFull: param=%s, src=%d\n", src->dhcpv6Config.StateFull.param, src->dhcpv6Config.StateFull.src));
        result = GetValueFromDb((char*)src->dhcpv6Config.StateFull.param, &cfg->dhcpv6Config.StateFull, PARAM_BOOLEAN, src->dhcpv6Config.StateFull.src);
        if (!result) {
            LanManagerError(("Failed to get IPv6 stateful configuration for bridge %d. result=%d\n", i, (int)result));
            cfg->dhcpv6Config.StateFull = false;
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Got StateFull for bridge %d: %d\n", i, cfg->dhcpv6Config.StateFull));
        }
    }
    // Read StateLess
    if (src->dhcpv6Config.StateLess.param) {
        LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Calling GetValueFromDb for StateLess: param=%s, src=%d\n", src->dhcpv6Config.StateLess.param, src->dhcpv6Config.StateLess.src));
        result = GetValueFromDb((char*)src->dhcpv6Config.StateLess.param, &cfg->dhcpv6Config.StateLess, PARAM_BOOLEAN, src->dhcpv6Config.StateLess.src);
        if (!result) {
            LanManagerError(("Failed to get IPv6 stateless configuration for bridge %d. result=%d\n", i, (int)result));
            cfg->dhcpv6Config.StateLess = false;
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Got StateLess for bridge %d: %d\n", i, cfg->dhcpv6Config.StateLess));
        }
    }
    // Read Dhcpv6_Start_Addr
    if (src->dhcpv6Config.Dhcpv6_Start_Addr.param) {
        LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Calling GetValueFromDb for Dhcpv6_Start_Addr: param=%s, src=%d\n", src->dhcpv6Config.Dhcpv6_Start_Addr.param, src->dhcpv6Config.Dhcpv6_Start_Addr.src));
        result = GetValueFromDb((char*)src->dhcpv6Config.Dhcpv6_Start_Addr.param, cfg->dhcpv6Config.Dhcpv6_Start_Addr, PARAM_STRING, src->dhcpv6Config.Dhcpv6_Start_Addr.src);
        if (!result) {
            LanManagerError(("Failed to get DHCPv6 start address for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Got Dhcpv6_Start_Addr for bridge %d: '%s'\n", i, cfg->dhcpv6Config.Dhcpv6_Start_Addr));
        }
    }
    // Read Dhcpv6_End_Addr
    if (src->dhcpv6Config.Dhcpv6_End_Addr.param) {
        LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Calling GetValueFromDb for Dhcpv6_End_Addr: param=%s, src=%d\n", src->dhcpv6Config.Dhcpv6_End_Addr.param, src->dhcpv6Config.Dhcpv6_End_Addr.src));
        result = GetValueFromDb((char*)src->dhcpv6Config.Dhcpv6_End_Addr.param, cfg->dhcpv6Config.Dhcpv6_End_Addr, PARAM_STRING, src->dhcpv6Config.Dhcpv6_End_Addr.src);
        if (!result) {
            LanManagerError(("Failed to get DHCPv6 end address for bridge %d. result=%d\n", i, (int)result));
        } else {
            LanManagerInfo(("[DEBUG][PopulateDhcpv6Config] Got Dhcpv6_End_Addr for bridge %d: '%s'\n", i, cfg->dhcpv6Config.Dhcpv6_End_Addr));
        }
    }
}

static void PopulateFirewallConfig(int i, LanConfig *cfg, const LanConfigSource *src, char *param_buf) {
    bool result;
    // Read Firewall_Level
    if (src->firewallConfig.Firewall_Level.param) {
        LanManagerInfo(("[DEBUG][PopulateFirewallConfig] Calling GetValueFromDb for Firewall_Level: param=%s, src=%d\n", src->firewallConfig.Firewall_Level.param, src->firewallConfig.Firewall_Level.src));
        result = GetValueFromDb((char*)src->firewallConfig.Firewall_Level.param, &cfg->firewallConfig.Firewall_Level, PARAM_INT, src->firewallConfig.Firewall_Level.src);
        if (!result) {
            LanManagerError(("Failed to get firewall level for bridge %d. result=%d\n", i, (int)result));
            cfg->firewallConfig.Firewall_Level = 2;
        } else {
            LanManagerInfo(("[DEBUG][PopulateFirewallConfig] Got Firewall_Level for bridge %d: %d\n", i, cfg->firewallConfig.Firewall_Level));
        }
    }
    // Read Firewall_Enable
    if (src->firewallConfig.Firewall_Enable.param) {
        LanManagerInfo(("[DEBUG][PopulateFirewallConfig] Calling GetValueFromDb for Firewall_Enable: param=%s, src=%d\n", src->firewallConfig.Firewall_Enable.param, src->firewallConfig.Firewall_Enable.src));
        result = GetValueFromDb((char*)src->firewallConfig.Firewall_Enable.param, &cfg->firewallConfig.Firewall_Enable, PARAM_BOOLEAN, src->firewallConfig.Firewall_Enable.src);
        if (!result) {
            LanManagerError(("Failed to get firewall enable status for bridge %d. result=%d\n", i, (int)result));
            cfg->firewallConfig.Firewall_Enable = true;
        } else {
            LanManagerInfo(("[DEBUG][PopulateFirewallConfig] Got Firewall_Enable for bridge %d: %d\n", i, cfg->firewallConfig.Firewall_Enable));
        }
    }
}

static void PopulateSecurityConfig(int i, LanConfig *cfg, const LanConfigSource *src, char *param_buf) {
    bool result;
    // Read VPN_Security_Enable
    if (src->securityConfig.VPN_Security_Enable.param) {
        LanManagerInfo(("[DEBUG][PopulateSecurityConfig] Calling GetValueFromDb for VPN_Security_Enable: param=%s, src=%d\n", src->securityConfig.VPN_Security_Enable.param, src->securityConfig.VPN_Security_Enable.src));
        result = GetValueFromDb((char*)src->securityConfig.VPN_Security_Enable.param, &cfg->securityConfig.VPN_Security_Enable, PARAM_BOOLEAN, src->securityConfig.VPN_Security_Enable.src);
        if (!result) {
            LanManagerError(("Failed to get VPN security enable status for bridge %d. result=%d\n", i, (int)result));
            cfg->securityConfig.VPN_Security_Enable = false;
        } else {
            LanManagerInfo(("[DEBUG][PopulateSecurityConfig] Got VPN_Security_Enable for bridge %d: %d\n", i, cfg->securityConfig.VPN_Security_Enable));
        }
    }
}

static void PopulateIgdConfig(int i, LanConfig *cfg, const LanConfigSource *src, char *param_buf) {
    bool result;
    // Read IGD_Enable
    if (src->igdConfig.IGD_Enable.param) {
        LanManagerInfo(("[DEBUG][PopulateIgdConfig] Calling GetValueFromDb for IGD_Enable: param=%s, src=%d\n", src->igdConfig.IGD_Enable.param, src->igdConfig.IGD_Enable.src));
        result = GetValueFromDb((char*)src->igdConfig.IGD_Enable.param, &cfg->IGD_Enable, PARAM_BOOLEAN, src->igdConfig.IGD_Enable.src);
        if (!result) {
            LanManagerError(("Failed to get IGD enable status for bridge %d. result=%d\n", i, (int)result));
            cfg->IGD_Enable = false;
        } else {
            LanManagerInfo(("[DEBUG][PopulateIgdConfig] Got IGD_Enable for bridge %d: %d\n", i, cfg->IGD_Enable));
        }
    }
}

void PopulateAllBridges()
{
    LanManagerInfo(("[DEBUG][PopulateAllBridges] Entered\n"));

    // Iterate over all bridge configuration entries
    for (int i = 0; lan_config_source_table[i].bridgeInfo.bridgeName.param != NULL; i++)
    {
        if(i >= MAX_TABLE_ROWS)
        {
            LanManagerError(("[PopulateAllBridges] Exceeded max table rows (%d), stopping.\n", MAX_TABLE_ROWS));
            break;
        }

        LanManagerInfo(("[DEBUG][PopulateAllBridges] Processing bridge index %d\n", i));
        LanConfig *cfg = &gDM.lanConfigs[i];
        const LanConfigSource *src = &lan_config_source_table[i];
        char param_buf[256];
        bool result = false;

        /* Initialize the instance */
        cfg->instNum = 0; /* Let rbus assign */
        snprintf(cfg->alias, sizeof(cfg->alias), "cpe-lan-%d", i);
        
        // Read Layer 2 network index and bridge info
        int l2net_idx = -1;
        char l2net_buf[8] = {0};
        if (src->bridgeInfo.l2net_index.param && src->bridgeInfo.l2net_index.param[0])
        {
            LanManagerInfo(("[DEBUG][PopulateAllBridges] Calling GetValueFromDb for l2net_index: param=%s, src=%d\n", src->bridgeInfo.l2net_index.param, src->bridgeInfo.l2net_index.src));
            result = GetValueFromDb((char*)src->bridgeInfo.l2net_index.param, l2net_buf, PARAM_STRING, src->bridgeInfo.l2net_index.src);
            if (!result || l2net_buf[0] == '\0')
            {
                LanManagerError(("Failed to get l2net index for bridge %d, skipping bridge. result=%d\n", i, (int)result));
                g_count--; /* Decrement because this instance failed to populate */
                continue;
            }
            else
            {
                l2net_idx = atoi(l2net_buf);
                LanManagerInfo(("[DEBUG][PopulateAllBridges] Got l2net_idx for bridge %d: %d (raw='%s')\n", i, l2net_idx, l2net_buf));
                PopulateBridgeInfo(i, cfg, src, l2net_idx, param_buf);
            }
        }

        // Read Layer 3 network index and IP config
        int l3net_idx = -1;
        char l3net_buf[8] = {0};
        if (src->ipConfig.l3net_index.param && src->ipConfig.l3net_index.param[0])
        {
            LanManagerInfo(("[DEBUG][PopulateAllBridges] Calling GetValueFromDb for l3net_index: param=%s, src=%d\n", src->ipConfig.l3net_index.param, src->ipConfig.l3net_index.src));
            result = GetValueFromDb((char*)src->ipConfig.l3net_index.param, l3net_buf, PARAM_STRING, src->ipConfig.l3net_index.src);
            if (!result || l3net_buf[0] == '\0')
            {
                LanManagerError(("Failed to get l3net index for bridge %d, skipping IP config. result=%d\n", i, (int)result));
            }
            else
            {
                l3net_idx = atoi(l3net_buf);
                LanManagerInfo(("[DEBUG][PopulateAllBridges] Got l3net_idx for bridge %d: %d (raw='%s')\n", i, l3net_idx, l3net_buf));
                PopulateIpConfig(i, cfg, src, l3net_idx, param_buf);
            }
        }

        // Read DHCPv4 index and config
        int dhcpv4_idx = -1;
        char dhcpv4_idx_buf[8] = {0};
        if (src->dhcpConfig.dhcpv4_index.param && src->dhcpConfig.dhcpv4_index.param[0])
        {
            LanManagerInfo(("[DEBUG][PopulateAllBridges] Calling GetValueFromDb for dhcpv4_index: param=%s, src=%d\n", src->dhcpConfig.dhcpv4_index.param, src->dhcpConfig.dhcpv4_index.src));
            result = GetValueFromDb((char*)src->dhcpConfig.dhcpv4_index.param, dhcpv4_idx_buf, PARAM_STRING, src->dhcpConfig.dhcpv4_index.src);
            if (!result || dhcpv4_idx_buf[0] == '\0')
            {
                LanManagerError(("Failed to get dhcpv4 index for bridge %d, skipping DHCP config. result=%d\n", i, (int)result));
            }
            else
            {
                dhcpv4_idx = atoi(dhcpv4_idx_buf);
                LanManagerInfo(("[DEBUG][PopulateAllBridges] dhcpv4_idx for bridge %d: %d\n", i, dhcpv4_idx));
                PopulateDhcpConfig(i, cfg, src, dhcpv4_idx, param_buf);
            }
        }

        // Read DHCPv6 config
        PopulateDhcpv6Config(i, cfg, src, param_buf);

        // Read firewall config
        PopulateFirewallConfig(i, cfg, src, param_buf);

        // Read security config
        PopulateSecurityConfig(i, cfg, src, param_buf);

        // Read IGD config
        PopulateIgdConfig(i, cfg, src, param_buf);

        /* Now that the config is populated, register it with rbus */
        uint32_t assignedInstNum = 0;
        rbusError_t ret = rbusTable_addRow(rbus_handle, "Device.LanManager.LanConfig.", cfg->alias, &assignedInstNum);
        if(ret == RBUS_ERROR_SUCCESS)
        {
            LanManagerInfo(("[PopulateAllBridges] Successfully added row for %s, instance number %u\n", cfg->alias, assignedInstNum));

            // Add the interfaces for this bridge to the Iface table
            for (int j = 0; j < cfg->numOfIfaces; ++j)
            {
                if (cfg->ifaces[j].Interfaces[0] != '\0')
                {
                    char ifaceTableName[256];
                    snprintf(ifaceTableName, sizeof(ifaceTableName), "Device.LanManager.LanConfig.%u.Iface.", assignedInstNum);

                    uint32_t assignedIfaceInstNum = 0;
                    rbusError_t iface_ret = rbusTable_addRow(rbus_handle, ifaceTableName, NULL, &assignedIfaceInstNum);
                    if(iface_ret == RBUS_ERROR_SUCCESS)
                    {
                        LanManagerInfo(("[PopulateAllBridges] Successfully added Iface row for %s, instance number %u\n", ifaceTableName, assignedIfaceInstNum));
                    }
                    else
                    {
                        LanManagerError(("[PopulateAllBridges] rbusTable_addRow failed for Iface table %s with error %d\n", ifaceTableName, iface_ret));
                    }
                }
            }
        }
        else
        {
            LanManagerError(("[PopulateAllBridges] rbusTable_addRow failed for alias %s with error %d\n", cfg->alias, ret));
        }

        /* Persist populated configuration regardless of rbus add outcome */
        PersistLanConfig(cfg);
    }
}
