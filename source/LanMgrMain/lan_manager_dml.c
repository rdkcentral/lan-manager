/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <rbus.h>
#include "lan_manager_interface.h"
#include "lan_manager_dml.h"
#include "lanmgr_log.h"

int g_count = 0;
DataModel gDM;

void getTableRowID(char const* name, int start, TableRowID* rowID)
{
    char const* p = name;
    char const* token_start;
    int i;
    int component = start; /* Renaming for clarity inside the function */

    LanManagerDebug(("%s: called with name=%s component=%d\n", __FUNCTION__, name, component));

    rowID->type = TableRowIDType_Unknown;
    rowID->alias[0] = '\0';
    rowID->instNum = 0;

    // In rbus, the path passed to a handler for a table item looks like:
    // Device.LanManager.LanConfig.1.
    // or Device.LanManager.LanConfig.[alias].
    // The 'start' parameter tells us how many components to skip.
    // For Device.LanManager.LanConfig.{i}.Alias, 'start' would be 3 for findLanConfig
    // which means we are interested in the 4th component.
    for(i = 0; i < component; ++i)
    {
        p = strchr(p, '.');
        if(!p)
        {
            LanManagerError(("%s: could not find component %d in %s\n", __FUNCTION__, component, name));
            return;
        }
        p++; /*skip the dot*/
    }

    token_start = p;
    p = strchr(token_start, '.');

    char buffer[128];
    int len;

    if(p)
        len = p - token_start;
    else
        len = strlen(token_start);

    if(len >= sizeof(buffer))
    {
        LanManagerError(("%s: instance identifier too long: %s\n", __FUNCTION__, token_start));
        return;
    }

    strncpy(buffer, token_start, len);
    buffer[len] = 0;

    LanManagerDebug(("%s: parsing instance identifier '%s'\n", __FUNCTION__, buffer));

    if(buffer[0] == '[')
    {
        static char alias_buffer[ALIAS_MAX_LEN];
        char* alias_end;
        rowID->type = TableRowIDType_Alias;
        strncpy(alias_buffer, buffer+1, sizeof(alias_buffer)-1);
        alias_buffer[sizeof(alias_buffer)-1] = 0;
        alias_end = strchr(alias_buffer, ']');
        if(alias_end)
            *alias_end = 0;
        strncpy(rowID->alias, alias_buffer, sizeof(rowID->alias) -1);
        rowID->alias[sizeof(rowID->alias)-1] = '\0';
        LanManagerDebug(("%s: parsed alias=%s\n", __FUNCTION__, rowID->alias));
    }
    else
    {
        rowID->type = TableRowIDType_InstNum;
        rowID->instNum = atoi(buffer);
        rowID->alias[0] = '\0';
        LanManagerDebug(("%s: parsed instNum=%d\n", __FUNCTION__, rowID->instNum));
    }
}

bool compareTableRowID(TableRowID const* rowID, uint32_t instNum, char const* alias)
{
    LanManagerDebug(("%s: comparing rowID (type=%d, instNum=%d, alias=%s) with instNum=%d alias=%s\n",
        __FUNCTION__, rowID->type, rowID->instNum, rowID->alias ? rowID->alias : "NULL", instNum, alias ? alias : "NULL"));
    if(rowID->type == TableRowIDType_InstNum && rowID->instNum == instNum)
        return true;
    if(rowID->type == TableRowIDType_Alias && alias && strcmp(rowID->alias, alias) == 0)
        return true;
    return false;
}

bool propertyNameEquals(char const* full, char const* partial)
{
    char const* p = strrchr(full, '.');
    if(!p)
        p = full;
    else
        p++;
    bool result = strcmp(p, partial) == 0;
    LanManagerDebug(("%s: comparing '%s' and '%s', result: %s\n", __FUNCTION__, p, partial, result ? "match" : "no match"));
    return result;
}

void printDataModel()
{
    int i;
    LanManagerDebug(("%s: begin datamodel\n", __FUNCTION__));
    for(i = 0; i < MAX_TABLE_ROWS; ++i)
    {
        LanConfig* lanConfig = &gDM.lanConfigs[i];
        if(lanConfig->bridgeInfo.bridgeName[0] != '\0') // Assuming bridgeName is set when in use
        {
            int j;
            LanManagerDebug(("%s: %d: instNum=%s, alias=%s, bridgeName=%s\n", __FUNCTION__, i, lanConfig->bridgeInfo.bridgeName, lanConfig->bridgeInfo.alias, lanConfig->bridgeInfo.bridgeName));

            for(j = 0; j < lanConfig->numOfIfaces; ++j)
            {
                Iface* iface = &lanConfig->ifaces[j];
                LanManagerDebug(("%s: \t%d: InfType=%d, Interface=%s, vlanId=%d\n", __FUNCTION__, j, iface->InfType, iface->Interfaces, iface->vlanId));
            }
        }
    }
    LanManagerDebug(("%s: end datamodel\n", __FUNCTION__));
}

LanConfig* findLanConfig(char const* rowName)
{
    TableRowID rowID;
    int i;

    getTableRowID(rowName, 3, &rowID);

    for(i = 0; i < MAX_TABLE_ROWS; ++i)
    {
        LanConfig* lanConfig = &gDM.lanConfigs[i];
        LanManagerDebug(("%s: checking index %d, instNum %d, alias %s\n", __FUNCTION__, i, i+1, lanConfig->bridgeInfo.alias));
        // Assuming instance number is stored in a way accessible for comparison
        // For simplicity, let's use the index as instance number for now
        if(lanConfig->bridgeInfo.bridgeName[0] != '\0' && compareTableRowID(&rowID, i + 1, lanConfig->bridgeInfo.alias))
            return lanConfig;
    }

    LanManagerError(("%s: Failed to find LanConfig for %s\n", __FUNCTION__, rowName));
    return NULL;
}

Iface* findIface(char const* rowName, LanConfig** lanConfig)
{
    TableRowID rowID;
    int i;

    *lanConfig = findLanConfig(rowName);

    if(!*lanConfig)
    {
        return NULL;
    }

    getTableRowID(rowName, 5, &rowID);

    for(i = 0; i < (*lanConfig)->numOfIfaces; ++i)
    {
        Iface* iface = &(*lanConfig)->ifaces[i];

        // Assuming instance number for ifaces is the index
        if(compareTableRowID(&rowID, i + 1, NULL)) // Assuming ifaces don't have aliases
            return iface;
    }

    LanManagerError(("%s: findIface failed for: %s\n", __FUNCTION__, rowName));
    return NULL;
}

rbusError_t tableAddRowHandlerLanConfig(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    int i;
    (void)handle;

    LanManagerDebug(("%s: called. tableName=%s aliasName=%s\n", __FUNCTION__, tableName, aliasName));

    if(g_count >= MAX_TABLE_ROWS)
    {
        LanManagerError(("%s: Maximum number of rows reached.\n", __FUNCTION__));
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }
    for(i = 0; i < MAX_TABLE_ROWS; ++i)
    {
        LanConfig* lanConfig = &gDM.lanConfigs[i];

        if(lanConfig->bridgeInfo.bridgeName[0] == '\0')
        {
            memset(lanConfig, 0, sizeof(LanConfig));

            gDM.lanConfigInstNum++;
            snprintf(lanConfig->bridgeInfo.bridgeName, MAX_NAME_LEN, "br%d", gDM.lanConfigInstNum);
            if(aliasName)
                strncpy(lanConfig->bridgeInfo.alias, aliasName, ALIAS_MAX_LEN);

            *instNum = gDM.lanConfigInstNum;
            g_count++;
            LanManagerDebug(("%s: Added new row with instNum %d at index %d\n", __FUNCTION__, *instNum, i));
            printDataModel();
            return RBUS_ERROR_SUCCESS;
        }
    }

    LanManagerError(("%s: Failed to find an empty slot for a new row.\n", __FUNCTION__));
    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableRemoveRowHandlerLanConfig(rbusHandle_t handle, char const* rowName)
{
    LanConfig* lanConfig;
    (void)handle;

    LanManagerDebug(("%s: called. rowName=%s\n", __FUNCTION__, rowName));

    lanConfig = findLanConfig(rowName);

    if(lanConfig)
    {
        LanManagerDebug(("%s: Removing row %s\n", __FUNCTION__, rowName));
        memset(lanConfig, 0, sizeof(LanConfig));
        --g_count;
        printDataModel();
        return RBUS_ERROR_SUCCESS;
    }

    LanManagerError(("%s: Failed to find row %s to remove.\n", __FUNCTION__, rowName));
    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableAddRowHandlerIface(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    LanConfig* lanConfig;
    (void)handle;
    (void)aliasName;

    LanManagerDebug(("%s: called. tableName=%s\n", __FUNCTION__, tableName));

    lanConfig = findLanConfig(tableName);

    if(lanConfig)
    {
        if(lanConfig->numOfIfaces < MAX_IFACE_COUNT)
        {
            Iface* iface = &lanConfig->ifaces[lanConfig->numOfIfaces];
            memset(iface, 0, sizeof(Iface));
            *instNum = lanConfig->numOfIfaces + 1;
            lanConfig->numOfIfaces++;
            LanManagerDebug(("%s: Added new Iface row with instNum %d to LanConfig %s\n", __FUNCTION__, *instNum, tableName));
            printDataModel();
            return RBUS_ERROR_SUCCESS;
        }
        else
        {
            LanManagerError(("%s: Maximum number of Ifaces reached for %s.\n", __FUNCTION__, tableName));
        }
    }
    else
    {
        LanManagerError(("%s: Could not find parent LanConfig for %s.\n", __FUNCTION__, tableName));
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableRemoveRowHandlerIface(rbusHandle_t handle, char const* rowName)
{
    LanConfig* lanConfig;
    Iface* iface;
    (void)handle;

    LanManagerDebug(("%s: called. rowName=%s\n", __FUNCTION__, rowName));

    iface = findIface(rowName, &lanConfig);

    if(iface && lanConfig)
    {
        // This is a simplification. A real implementation would need to shift elements.
        LanManagerDebug(("%s: Removing Iface row %s\n", __FUNCTION__, rowName));
        memset(iface, 0, sizeof(Iface));
        //lanConfig->numOfIfaces--;
        printDataModel();
        return RBUS_ERROR_SUCCESS;
    }

    LanManagerError(("%s: Failed to find Iface row %s to remove.\n", __FUNCTION__, rowName));
    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t getHandlerLanConfig(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    rbusValue_t value;
    LanConfig* lanConfig;
    char const* name = rbusProperty_GetName(property);

    (void)handle;
    (void)opts;

    LanManagerDebug(("%s: called. property=%s\n", __FUNCTION__, name));

    lanConfig = findLanConfig(name);

    if(!lanConfig)
    {
        LanManagerError(("%s: findLanConfig failed for %s\n", __FUNCTION__, name));
        return RBUS_ERROR_BUS_ERROR;
    }

    rbusValue_Init(&value);

    if(propertyNameEquals(name, "Alias"))
    {
        rbusValue_SetString(value, lanConfig->bridgeInfo.alias);
    }
    else if(propertyNameEquals(name, "BridgeName"))
    {
        rbusValue_SetString(value, lanConfig->bridgeInfo.bridgeName);
    }
    else if(propertyNameEquals(name, "NetworkBridgeType"))
    {
        rbusValue_SetInt32(value, lanConfig->bridgeInfo.networkBridgeType);
    }
    else if(propertyNameEquals(name, "UserBridgeCategory"))
    {
        rbusValue_SetInt32(value, lanConfig->bridgeInfo.userBridgeCategory);
    }
    else if(propertyNameEquals(name, "NumOfIfaces"))
    {
        rbusValue_SetInt32(value, lanConfig->numOfIfaces);
    }
    else if(propertyNameEquals(name, "IP_Enable"))
    {
        rbusValue_SetBoolean(value, lanConfig->ipConfig.Ip_Enable);
    }
    else if(propertyNameEquals(name, "Ipv4Address"))
    {
        rbusValue_SetString(value, lanConfig->ipConfig.Ipv4Address);
    }
    else if(propertyNameEquals(name, "IpSubNet"))
    {
        rbusValue_SetString(value, lanConfig->ipConfig.IpSubNet);
    }
    else if(propertyNameEquals(name, "Ipv6Address"))
    {
        rbusValue_SetString(value, lanConfig->ipConfig.Ipv6Address);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Enable"))
    {
        rbusValue_SetBoolean(value, lanConfig->dhcpConfig.Dhcpv4_Enable);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Start_Addr"))
    {
        rbusValue_SetString(value, lanConfig->dhcpConfig.Dhcpv4_Start_Addr);
    }
    else if(propertyNameEquals(name, "Dhcpv4_End_Addr"))
    {
        rbusValue_SetString(value, lanConfig->dhcpConfig.Dhcpv4_End_Addr);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Lease_Time"))
    {
        rbusValue_SetInt32(value, lanConfig->dhcpConfig.Dhcpv4_Lease_Time);
    }
    else if(propertyNameEquals(name, "Ipv6Prefix"))
    {
        rbusValue_SetString(value, lanConfig->dhcpv6Config.Ipv6Prefix);
    }
    else if(propertyNameEquals(name, "StateFull"))
    {
        rbusValue_SetBoolean(value, lanConfig->dhcpv6Config.StateFull);
    }
    else if(propertyNameEquals(name, "StateLess"))
    {
        rbusValue_SetBoolean(value, lanConfig->dhcpv6Config.StateLess);
    }
    else if(propertyNameEquals(name, "Dhcpv6_Start_Addr"))
    {
        rbusValue_SetString(value, lanConfig->dhcpv6Config.Dhcpv6_Start_Addr);
    }
    else if(propertyNameEquals(name, "Dhcpv6_End_Addr"))
    {
        rbusValue_SetString(value, lanConfig->dhcpv6Config.Dhcpv6_End_Addr);
    }
    else if(propertyNameEquals(name, "Firewall_Level"))
    {
        rbusValue_SetInt32(value, lanConfig->firewallConfig.Firewall_Level);
    }
    else if(propertyNameEquals(name, "Firewall_Enable"))
    {
        rbusValue_SetBoolean(value, lanConfig->firewallConfig.Firewall_Enable);
    }
    else if(propertyNameEquals(name, "VPN_Security_Enable"))
    {
        rbusValue_SetBoolean(value, lanConfig->securityConfig.VPN_Security_Enable);
    }
    else if(propertyNameEquals(name, "IGD_Enable"))
    {
        rbusValue_SetBoolean(value, lanConfig->IGD_Enable);
    }
    else if(propertyNameEquals(name, "Status"))
    {
        rbusValue_SetInt32(value, lanConfig->status);
    }
    // Add more properties here based on LanConfig struct
    else
    {
        LanManagerError(("%s: property %s not supported\n", __FUNCTION__, name));
        rbusValue_Release(value);
        return RBUS_ERROR_BUS_ERROR;
    }

    char dbg_val[256] = {0};
    rbusValue_ToString(value, dbg_val, sizeof(dbg_val));
    LanManagerDebug(("%s: for property %s, returning value '%s'\n", __FUNCTION__, name, dbg_val));
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandlerLanConfig(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    LanConfig* lanConfig;
    char const* name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);
    rbusValueType_t type = rbusValue_GetType(value);

    (void)handle;
    (void)opts;

    LanManagerDebug(("%s: called. property=%s type=%d\n", __FUNCTION__, name, type));

    lanConfig = findLanConfig(name);

    if(!lanConfig)
    {
        LanManagerError(("%s: findLanConfig failed for %s\n", __FUNCTION__, name));
        return RBUS_ERROR_BUS_ERROR;
    }

    if(propertyNameEquals(name, "Alias"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Alias to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->bridgeInfo.alias, str, ALIAS_MAX_LEN-1);
    }
    else if(propertyNameEquals(name, "BridgeName"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting BridgeName to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->bridgeInfo.bridgeName, str, MAX_NAME_LEN-1);
    }
    else if(propertyNameEquals(name, "NetworkBridgeType"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting NetworkBridgeType to '%d'\n", __FUNCTION__, i));
        lanConfig->bridgeInfo.networkBridgeType = i;
    }
    else if(propertyNameEquals(name, "UserBridgeCategory"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting UserBridgeCategory to '%d'\n", __FUNCTION__, i));
        lanConfig->bridgeInfo.userBridgeCategory = i;
    }
    else if(propertyNameEquals(name, "IP_Enable"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting IP_Enable to '%d'\n", __FUNCTION__, b));
        lanConfig->ipConfig.Ip_Enable = b;
    }
    else if(propertyNameEquals(name, "Ipv4Address"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Ipv4Address to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->ipConfig.Ipv4Address, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "IpSubNet"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting IpSubNet to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->ipConfig.IpSubNet, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Ipv6Address"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Ipv6Address to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->ipConfig.Ipv6Address, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Enable"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting Dhcpv4_Enable to '%d'\n", __FUNCTION__, b));
        lanConfig->dhcpConfig.Dhcpv4_Enable = b;
    }
    else if(propertyNameEquals(name, "Dhcpv4_Start_Addr"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Dhcpv4_Start_Addr to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->dhcpConfig.Dhcpv4_Start_Addr, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv4_End_Addr"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Dhcpv4_End_Addr to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->dhcpConfig.Dhcpv4_End_Addr, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Lease_Time"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting Dhcpv4_Lease_Time to '%d'\n", __FUNCTION__, i));
        lanConfig->dhcpConfig.Dhcpv4_Lease_Time = i;
    }
    else if(propertyNameEquals(name, "Ipv6Prefix"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Ipv6Prefix to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->dhcpv6Config.Ipv6Prefix, str, MAX_PREFIX_LEN-1);
    }
    else if(propertyNameEquals(name, "StateFull"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting StateFull to '%d'\n", __FUNCTION__, b));
        lanConfig->dhcpv6Config.StateFull = b;
    }
    else if(propertyNameEquals(name, "StateLess"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting StateLess to '%d'\n", __FUNCTION__, b));
        lanConfig->dhcpv6Config.StateLess = b;
    }
    else if(propertyNameEquals(name, "Dhcpv6_Start_Addr"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Dhcpv6_Start_Addr to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->dhcpv6Config.Dhcpv6_Start_Addr, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv6_End_Addr"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Dhcpv6_End_Addr to '%s'\n", __FUNCTION__, str));
        strncpy(lanConfig->dhcpv6Config.Dhcpv6_End_Addr, str, MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Firewall_Level"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting Firewall_Level to '%d'\n", __FUNCTION__, i));
        lanConfig->firewallConfig.Firewall_Level = i;
    }
    else if(propertyNameEquals(name, "Firewall_Enable"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting Firewall_Enable to '%d'\n", __FUNCTION__, b));
        lanConfig->firewallConfig.Firewall_Enable = b;
    }
    else if(propertyNameEquals(name, "VPN_Security_Enable"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting VPN_Security_Enable to '%d'\n", __FUNCTION__, b));
        lanConfig->securityConfig.VPN_Security_Enable = b;
    }
    else if(propertyNameEquals(name, "IGD_Enable"))
    {
        bool b = rbusValue_GetBoolean(value);
        LanManagerDebug(("%s: setting IGD_Enable to '%d'\n", __FUNCTION__, b));
        lanConfig->IGD_Enable = b;
    }
    else if(propertyNameEquals(name, "Status"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting Status to '%d'\n", __FUNCTION__, i));
        lanConfig->status = i;
    }
    // Add more properties here
    else
    {
        LanManagerError(("%s: property %s not supported\n", __FUNCTION__, name));
        return RBUS_ERROR_BUS_ERROR;
    }
    printDataModel();
    return RBUS_ERROR_SUCCESS;
}

rbusError_t getHandlerIface(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    LanConfig* lanConfig;
    Iface* iface;
    rbusValue_t value;
    char const* name = rbusProperty_GetName(property);

    (void)handle;
    (void)opts;

    LanManagerDebug(("%s: called. property=%s\n", __FUNCTION__, name));

    iface = findIface(name, &lanConfig);

    if(!iface)
        return RBUS_ERROR_BUS_ERROR;

    rbusValue_Init(&value);

    if(propertyNameEquals(name, "Interface"))
    {
        rbusValue_SetString(value, iface->Interfaces);
    }
    else if(propertyNameEquals(name, "VlanId"))
    {
        rbusValue_SetInt32(value, iface->vlanId);
    }
    else if(propertyNameEquals(name, "InfType"))
    {
        rbusValue_SetInt32(value, iface->InfType);
    }
    else
    {
        LanManagerError(("%s: property %s not supported\n", __FUNCTION__, name));
        rbusValue_Release(value);
        return RBUS_ERROR_BUS_ERROR;
    }

    char dbg_val[256] = {0};
    rbusValue_ToString(value, dbg_val, sizeof(dbg_val));
    LanManagerDebug(("%s: for property %s, returning value '%s'\n", __FUNCTION__, name, dbg_val));
    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandlerIface(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    LanConfig* lanConfig;
    Iface* iface;
    char const* name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);
    rbusValueType_t type = rbusValue_GetType(value);

    (void)handle;
    (void)opts;

    LanManagerDebug(("%s: called. property=%s type=%d\n", __FUNCTION__, name, type));

    iface = findIface(name, &lanConfig);

    if(!iface)
    {
        LanManagerError(("%s: findIface failed for %s\n", __FUNCTION__, name));
        return RBUS_ERROR_BUS_ERROR;
    }

    if(propertyNameEquals(name, "Interface"))
    {
        const char* str = rbusValue_GetString(value, NULL);
        LanManagerDebug(("%s: setting Interface to '%s'\n", __FUNCTION__, str));
        strncpy(iface->Interfaces, str, MAX_NAME_LEN-1);
    }
    else if(propertyNameEquals(name, "VlanId"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting VlanId to '%d'\n", __FUNCTION__, i));
        iface->vlanId = i;
    }
    else if(propertyNameEquals(name, "InfType"))
    {
        int32_t i = rbusValue_GetInt32(value);
        LanManagerDebug(("%s: setting InfType to '%d'\n", __FUNCTION__, i));
        iface->InfType = i;
    }
    else
    {
        LanManagerError(("%s: property %s not supported\n", __FUNCTION__, name));
        return RBUS_ERROR_BUS_ERROR;
    }
    printDataModel();
    return RBUS_ERROR_SUCCESS;
}

rbusError_t eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)interval;
    (void)autoPublish;
    LanManagerDebug((
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName));

    return RBUS_ERROR_SUCCESS;
}

rbusError_t getLanConfigHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle; (void)methodName; (void)inParams; (void)asyncHandle;
    size_t total_size = sizeof(int) + g_count * sizeof(LanConfig);
    uint8_t* buffer = malloc(total_size);
    memcpy(buffer, &g_count, sizeof(int));
    memcpy(buffer + sizeof(int), gDM.lanConfigs, g_count * sizeof(LanConfig));
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetBytes(value, buffer, total_size);
    rbusObject_SetValue(outParams, "value", value);
    rbusValue_Release(value);
    free(buffer);
    return RBUS_ERROR_SUCCESS;
}

static rbusDataElement_t dataElements[] = {
    {"Device.LanManager.LanConfig.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandlerLanConfig, tableRemoveRowHandlerLanConfig, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.BridgeName", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.NetworkBridgeType", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.UserBridgeCategory", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.NumOfIfaces", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, NULL, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.IP_Enable", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Ipv4Address", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.IpSubNet", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Ipv6Address", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Dhcpv4_Enable", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Dhcpv4_Start_Addr", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Dhcpv4_End_Addr", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Dhcpv4_Lease_Time", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Ipv6Prefix", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.StateFull", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.StateLess", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Dhcpv6_Start_Addr", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Dhcpv6_End_Addr", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Firewall_Level", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Firewall_Enable", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.VPN_Security_Enable", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.IGD_Enable", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},
    {"Device.LanManager.LanConfig.{i}.Status", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerLanConfig, setHandlerLanConfig, NULL, NULL, eventSubHandler, NULL}},

    {"Device.LanManager.LanConfig.{i}.Iface.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandlerIface,  tableRemoveRowHandlerIface, NULL, NULL}},
    {"Device.LanManager.LanConfig.{i}.Iface.{i}.Interface", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerIface, setHandlerIface, NULL, NULL, NULL, NULL}},
    {"Device.LanManager.LanConfig.{i}.Iface.{i}.VlanId", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerIface, setHandlerIface, NULL, NULL, NULL, NULL}},
    {"Device.LanManager.LanConfig.{i}.Iface.{i}.InfType", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerIface, setHandlerIface, NULL, NULL, NULL, NULL}},
    {"Device.LanManager.LanConfigCopy()", RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, getLanConfigHandler}}
};

extern rbusHandle_t rbus_handle;

int lan_manager_register_dml()
{
    LanManagerDebug(("%s: Enter \n", __FUNCTION__));
    int rc = RBUS_ERROR_SUCCESS;

    memset(&gDM, 0, sizeof(DataModel));

    rc = rbus_regDataElements(rbus_handle, sizeof(dataElements)/sizeof(rbusDataElement_t), dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        LanManagerError(("%s: rbus_regDataElements failed: %d\n", __FUNCTION__, rc));
    }
    return rc;
}

void lan_manager_unregister_dml()
{
    LanManagerDebug(("%s: Enter \n", __FUNCTION__));
    rbus_unregDataElements(rbus_handle, sizeof(dataElements)/sizeof(rbusDataElement_t), dataElements);
}
