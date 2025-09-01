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
    char* p = (char*)name + strlen(name) - 1;
    rowID->type = TableRowIDType_Unknown;
    rowID->alias = NULL;
    rowID->instNum = 0;

    while(p > name + start)
    {
        if(*p == '.')
        {
            p++;
            break;
        }
        p--;
    }

    if(p > name + start)
    {
        int i = 0;
        char* end = p;
        while(*end)
        {
            if(i > 0 && *end == '.')
                break;
            end++;
            i++;
        }
        if(*end == '.')
            *(end-1) = 0;

        if(p[0] == '[')
        {
            char* end;
            rowID->type = TableRowIDType_Alias;
            rowID->alias = p+1;
            end = strchr(p, ']');
            if(end)
                *end = 0;
        }
        else
        {
            rowID->type = TableRowIDType_InstNum;
            rowID->instNum = atoi(p);
        }
    }
}

bool compareTableRowID(TableRowID const* rowID, uint32_t instNum, char const* alias)
{
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
    return strcmp(p, partial) == 0;
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
        // Assuming instance number is stored in a way accessible for comparison
        // For simplicity, let's use the index as instance number for now
        if(lanConfig->bridgeInfo.bridgeName[0] != '\0' && compareTableRowID(&rowID, i + 1, lanConfig->bridgeInfo.alias))
            return lanConfig;
    }

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

    LanManagerInfo(("%s: called. tableName=%s aliasName=%s\n", __FUNCTION__, tableName, aliasName));

    if(g_count >= MAX_TABLE_ROWS)
    {
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
            printDataModel();
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableRemoveRowHandlerLanConfig(rbusHandle_t handle, char const* rowName)
{
    LanConfig* lanConfig;
    (void)handle;

    LanManagerInfo(("%s: called. rowName=%s\n", __FUNCTION__, rowName));

    lanConfig = findLanConfig(rowName);

    if(lanConfig)
    {
        memset(lanConfig, 0, sizeof(LanConfig));
        --g_count;
        printDataModel();
        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableAddRowHandlerIface(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    LanConfig* lanConfig;
    (void)handle;
    (void)aliasName;

    LanManagerInfo(("%s: called. tableName=%s\n", __FUNCTION__, tableName));

    lanConfig = findLanConfig(tableName);

    if(lanConfig)
    {
        if(lanConfig->numOfIfaces < MAX_IFACE_COUNT)
        {
            Iface* iface = &lanConfig->ifaces[lanConfig->numOfIfaces];
            memset(iface, 0, sizeof(Iface));
            *instNum = lanConfig->numOfIfaces + 1;
            lanConfig->numOfIfaces++;
            printDataModel();
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableRemoveRowHandlerIface(rbusHandle_t handle, char const* rowName)
{
    LanConfig* lanConfig;
    Iface* iface;
    (void)handle;

    LanManagerInfo(("%s: called. rowName=%s\n", __FUNCTION__, rowName));

    iface = findIface(rowName, &lanConfig);

    if(iface && lanConfig)
    {
        // This is a simplification. A real implementation would need to shift elements.
        memset(iface, 0, sizeof(Iface));
        //lanConfig->numOfIfaces--;
        printDataModel();
        return RBUS_ERROR_SUCCESS;
    }

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
        return RBUS_ERROR_BUS_ERROR;

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
        rbusValue_Release(value);
        return RBUS_ERROR_BUS_ERROR;
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandlerLanConfig(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    LanConfig* lanConfig;
    char const* name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);

    (void)handle;
    (void)opts;

    LanManagerDebug(("%s: called. property=%s\n", __FUNCTION__, name));

    lanConfig = findLanConfig(name);

    if(!lanConfig)
        return RBUS_ERROR_BUS_ERROR;

    if(propertyNameEquals(name, "Alias"))
    {
        strncpy(lanConfig->bridgeInfo.alias, rbusValue_GetString(value, NULL), ALIAS_MAX_LEN-1);
    }
    else if(propertyNameEquals(name, "BridgeName"))
    {
        strncpy(lanConfig->bridgeInfo.bridgeName, rbusValue_GetString(value, NULL), MAX_NAME_LEN-1);
    }
    else if(propertyNameEquals(name, "NetworkBridgeType"))
    {
        lanConfig->bridgeInfo.networkBridgeType = rbusValue_GetInt32(value);
    }
    else if(propertyNameEquals(name, "UserBridgeCategory"))
    {
        lanConfig->bridgeInfo.userBridgeCategory = rbusValue_GetInt32(value);
    }
    else if(propertyNameEquals(name, "IP_Enable"))
    {
        lanConfig->ipConfig.Ip_Enable = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "Ipv4Address"))
    {
        strncpy(lanConfig->ipConfig.Ipv4Address, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "IpSubNet"))
    {
        strncpy(lanConfig->ipConfig.IpSubNet, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Ipv6Address"))
    {
        strncpy(lanConfig->ipConfig.Ipv6Address, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Enable"))
    {
        lanConfig->dhcpConfig.Dhcpv4_Enable = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Start_Addr"))
    {
        strncpy(lanConfig->dhcpConfig.Dhcpv4_Start_Addr, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv4_End_Addr"))
    {
        strncpy(lanConfig->dhcpConfig.Dhcpv4_End_Addr, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv4_Lease_Time"))
    {
        lanConfig->dhcpConfig.Dhcpv4_Lease_Time = rbusValue_GetInt32(value);
    }
    else if(propertyNameEquals(name, "Ipv6Prefix"))
    {
        strncpy(lanConfig->dhcpv6Config.Ipv6Prefix, rbusValue_GetString(value, NULL), MAX_PREFIX_LEN-1);
    }
    else if(propertyNameEquals(name, "StateFull"))
    {
        lanConfig->dhcpv6Config.StateFull = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "StateLess"))
    {
        lanConfig->dhcpv6Config.StateLess = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "Dhcpv6_Start_Addr"))
    {
        strncpy(lanConfig->dhcpv6Config.Dhcpv6_Start_Addr, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Dhcpv6_End_Addr"))
    {
        strncpy(lanConfig->dhcpv6Config.Dhcpv6_End_Addr, rbusValue_GetString(value, NULL), MAX_IP_LEN-1);
    }
    else if(propertyNameEquals(name, "Firewall_Level"))
    {
        lanConfig->firewallConfig.Firewall_Level = rbusValue_GetInt32(value);
    }
    else if(propertyNameEquals(name, "Firewall_Enable"))
    {
        lanConfig->firewallConfig.Firewall_Enable = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "VPN_Security_Enable"))
    {
        lanConfig->securityConfig.VPN_Security_Enable = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "IGD_Enable"))
    {
        lanConfig->IGD_Enable = rbusValue_GetBoolean(value);
    }
    else if(propertyNameEquals(name, "Status"))
    {
        lanConfig->status = rbusValue_GetInt32(value);
    }
    // Add more properties here
    else
    {
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
        rbusValue_Release(value);
        return RBUS_ERROR_BUS_ERROR;
    }

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

    (void)handle;
    (void)opts;

    LanManagerDebug(("%s: called. property=%s\n", __FUNCTION__, name));

    iface = findIface(name, &lanConfig);

    if(!iface)
        return RBUS_ERROR_BUS_ERROR;

    if(propertyNameEquals(name, "Interface"))
    {
        strncpy(iface->Interfaces, rbusValue_GetString(value, NULL), MAX_NAME_LEN-1);
    }
    else if(propertyNameEquals(name, "VlanId"))
    {
        iface->vlanId = rbusValue_GetInt32(value);
    }
    else if(propertyNameEquals(name, "InfType"))
    {
        iface->InfType = rbusValue_GetInt32(value);
    }
    else
    {
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
    LanManagerInfo((
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName));

    return RBUS_ERROR_SUCCESS;
}

#define NUM_DATA_ELEMENTS 28
static rbusDataElement_t dataElements[NUM_DATA_ELEMENTS] = {
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
    {"Device.LanManager.LanConfig.{i}.Iface.{i}.InfType", RBUS_ELEMENT_TYPE_PROPERTY, {getHandlerIface, setHandlerIface, NULL, NULL, NULL, NULL}}
};

extern rbusHandle_t rbus_handle;

int lan_manager_register_dml()
{
    LanManagerInfo(("%s: Enter \n", __FUNCTION__));
    int rc = RBUS_ERROR_SUCCESS;

    memset(&gDM, 0, sizeof(DataModel));

    rc = rbus_regDataElements(rbus_handle, NUM_DATA_ELEMENTS, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        LanManagerError(("%s: rbus_regDataElements failed: %d\n", __FUNCTION__, rc));
    }
    return rc;
}

void lan_manager_unregister_dml()
{
    LanManagerInfo(("%s: Enter \n", __FUNCTION__));
    rbus_unregDataElements(rbus_handle, NUM_DATA_ELEMENTS, dataElements);
}
