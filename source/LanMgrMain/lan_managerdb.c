/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include <stdbool.h>
#include <string.h>
#include "lan_manager.h"
#include "lan_managerds.h"
#include "lm_bridge_util.h"

#define LM_TRACE printf  //To be replaced with actual logging infra

/******************************************************************************
 *SetLanConfigBridgeInfo: API to set the main bridge information
 * The purpose of the function is to set the Bridge Information in the
 * LanDB table. The function will also add details to the LanDB as a
 * new entry in case the entry is already not present.
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgInfo: Pointer of structure BridgeInfo which has the Alias, Name,
 * Type and Category
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
******************************************************************************/
LM_Status SetLanConfigBridgeInfo(const char *BrgAlias, const BridgeInfo *BrgInfo)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgInfo==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(LanDBData.bridgeInfo.alias,BrgInfo->alias);
        strcpy(LanDBData.bridgeInfo.bridgeName,BrgInfo->bridgeName);
        LanDBData.bridgeInfo.networkBridgeType=BrgInfo->networkBridgeType;
        LanDBData.bridgeInfo.userBridgeCategory=BrgInfo->userBridgeCategory;
        LanDBData.bridgeInfo.stpEnable=BrgInfo->stpEnable;
        LanDBData.bridgeInfo.igdEnable=BrgInfo->igdEnable;
        LanDBData.bridgeInfo.bridgeLifeTime=BrgInfo->bridgeLifeTime;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }

        return LM_FAILURE;
    }
    memset(&LanDBData, 0, sizeof(LanDBData));
    strcpy(LanDBData.bridgeInfo.alias,BrgInfo->alias);
    strcpy(LanDBData.bridgeInfo.bridgeName,BrgInfo->bridgeName);
    LanDBData.bridgeInfo.networkBridgeType=BrgInfo->networkBridgeType;
    LanDBData.bridgeInfo.userBridgeCategory=BrgInfo->userBridgeCategory;

    if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
    {
        return LM_SUCCESS;
    }

    return LM_FAILURE;
}

/*****************************************************************************
 *GetLanConfigBridgeInfo: API to get the main bridge information
 * The purpose of the function is to get the Bridge Information from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgInfo: Pointer of structure BridgeInfo which has the Alias, Name,
 * Type and Category
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*****************************************************************************/
LM_Status GetLanConfigBridgeInfo(const char *BrgAlias, BridgeInfo *BrgInfo)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgInfo==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(BrgInfo->alias,LanDBData.bridgeInfo.alias);
        strcpy(BrgInfo->bridgeName,LanDBData.bridgeInfo.bridgeName);
        BrgInfo->networkBridgeType=LanDBData.bridgeInfo.networkBridgeType;
        BrgInfo->userBridgeCategory=LanDBData.bridgeInfo.userBridgeCategory;
        BrgInfo->stpEnable=LanDBData.bridgeInfo.stpEnable;
        BrgInfo->igdEnable=LanDBData.bridgeInfo.igdEnable;
        BrgInfo->bridgeLifeTime=LanDBData.bridgeInfo.bridgeLifeTime;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/*****************************************************************************
 *RemoveLanConfigEntry: API to remove the Lan bridge information
 * The purpose of the function is to remove the entire Bridge entry from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
 ****************************************************************************/
LM_Status RemoveLanConfigEntry(const char *BrgAlias)
{
    LanConfig LanDBData;

    if(BrgAlias==NULL)
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(LanDBData.bridgeInfo.alias,BrgAlias);

        if(LM_SUCCESS == LanConfigDataStoreRemove(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/******************************************************************************
 *SetLanConfigInterfaceCountInfo: API to set information on Bridge Interface count
 * The purpose of the function is to set the Bridge Interface count in the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIfaceCount: Pointer to number of interfaces associated to the bridge
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
******************************************************************************/
LM_Status SetLanConfigInterfaceCountInfo(const char *BrgAlias, const int *BrgIfaceCount)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIfaceCount==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        LanDBData.numOfIfaces=*BrgIfaceCount;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/******************************************************************************
 *GetLanConfigInterfaceCountInfo: API to get information on Bridge Interface count
 * The purpose of the function is to get the Bridge Interface count from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIfaceCount: Pointer to get number of interfaces associated to the bridge
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
******************************************************************************/
LM_Status GetLanConfigInterfaceCountInfo(const char *BrgAlias, int *BrgIfaceCount)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIfaceCount==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        *BrgIfaceCount=LanDBData.numOfIfaces;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/****************************************************************************************
 *SetLanConfigInterfaceInfo: API to set information on interfaces associated with the bridge
 * The purpose of the function is to set the Bridge Interface details to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIface: Pointer to array of Iface stucture with Type, Name and Vlan of the interfaces
 *  and array size of MAX_IFACE_COUNT.
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function. This function also will not validate the
 * array size of the passed BrgIface and it is responsibility of the calling function to
 * ensure that the array of correct size is passed.
*****************************************************************************************/
LM_Status SetLanConfigInterfaceInfo(const char *BrgAlias, const Iface *BrgIface)
{

    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIface==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        memcpy(LanDBData.ifaces,BrgIface,sizeof(Iface)*MAX_IFACE_COUNT);

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/****************************************************************************************
 *GetLanConfigInterfaceInfo: API to get information on interfaces associated with the bridge
 * The purpose of the function is to get the Bridge Interface details to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIface: Pointer to array of Iface structure to return details of Type, Name and
 *  Vlan of the interfaces and array size of MAX_IFACE_COUNT
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function. This function also will not validate the
 * array size of the passed BrgIface and it is responsibility of the calling function to
 * ensure that the array of correct size is passed.
****************************************************************************************/
LM_Status GetLanConfigInterfaceInfo(const char *BrgAlias, Iface *BrgIface)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIface==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        memcpy(BrgIface,LanDBData.ifaces,sizeof(Iface)*MAX_IFACE_COUNT);

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/************************************************************************************
 *SetLanConfigDhcpInfo: API to set information on dhcp config associated with the bridge
 * The purpose of the function is to set the DHCP config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgDhcpConfig: Pointer to DHCPConfig structure with  details of bridge like Status,
 *  Address range and Lease time
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*************************************************************************************/
LM_Status SetLanConfigDhcpInfo(const char *BrgAlias, const DHCPConfig *BrgDhcpConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgDhcpConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(LanDBData.dhcpConfig.Dhcpv4_Start_Addr,BrgDhcpConfig->Dhcpv4_Start_Addr);
        strcpy(LanDBData.dhcpConfig.Dhcpv4_End_Addr,BrgDhcpConfig->Dhcpv4_End_Addr);
        LanDBData.dhcpConfig.Dhcpv4_Enable=BrgDhcpConfig->Dhcpv4_Enable;
        LanDBData.dhcpConfig.Dhcpv4_Lease_Time=BrgDhcpConfig->Dhcpv4_Lease_Time;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/************************************************************************************
 *GetLanConfigDhcpInfo: API to get information on dhcp config associated with the bridge
 * The purpose of the function is to get the DHCP config details of the bridge from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgDhcpConfig: Pointer to return DHCPConfig structure with details like Status,
 *  Address range and Lease time
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
************************************************************************************/
LM_Status GetLanConfigDhcpInfo(const char *BrgAlias, DHCPConfig *BrgDhcpConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgDhcpConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(BrgDhcpConfig->Dhcpv4_Start_Addr,LanDBData.dhcpConfig.Dhcpv4_Start_Addr);
        strcpy(BrgDhcpConfig->Dhcpv4_End_Addr,LanDBData.dhcpConfig.Dhcpv4_End_Addr);
        BrgDhcpConfig->Dhcpv4_Enable=LanDBData.dhcpConfig.Dhcpv4_Enable;
        BrgDhcpConfig->Dhcpv4_Lease_Time=LanDBData.dhcpConfig.Dhcpv4_Lease_Time;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**************************************************************************************
 *SetLanConfigIPConfigInfo: API to set information on IP config associated with the bridge
 * The purpose of the function is to set the IP config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIPConfig: Pointer to IPConfig structure with details of bridge like IPv4
 *  and IPv6 addresses and Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**************************************************************************************/
LM_Status SetLanConfigIPConfigInfo(const char *BrgAlias, const IPConfig *BrgIPConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIPConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(LanDBData.ipConfig.Ipv4Address,BrgIPConfig->Ipv4Address);
        strcpy(LanDBData.ipConfig.IpSubNet,BrgIPConfig->IpSubNet);
        LanDBData.ipConfig.Ip_Enable=BrgIPConfig->Ip_Enable;
        strcpy(LanDBData.ipConfig.Ipv6Address,BrgIPConfig->Ipv6Address);

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**************************************************************************************
 *GetLanConfigIPConfigInfo: API to get information on IP config associated with the bridge
 * The purpose of the function is to get the IP config details of the bridge from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIPConfig: Pointer to return IPConfig structure with details of bridge like IPv4
 *  and IPv6 addresses and Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**************************************************************************************/
LM_Status GetLanConfigIPConfigInfo(const char *BrgAlias, IPConfig *BrgIPConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIPConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(BrgIPConfig->Ipv4Address,LanDBData.ipConfig.Ipv4Address);
        strcpy(BrgIPConfig->IpSubNet,LanDBData.ipConfig.IpSubNet);
        BrgIPConfig->Ip_Enable=LanDBData.ipConfig.Ip_Enable;
        strcpy(BrgIPConfig->Ipv6Address,LanDBData.ipConfig.Ipv6Address);

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**********************************************************************************************
 *SetLanConfigDhcpv6ConfigInfo: API to set information on DHCPv6 config associated with the bridge
 * The purpose of the function is to set the DHCPv6 config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgDhcpv6Config: Pointer of DHCPV6Config structure with details like Address range, Prefix and State
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**********************************************************************************************/
LM_Status SetLanConfigDhcpv6ConfigInfo(const char *BrgAlias, const DHCPv6Config *BrgDhcpv6Config)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgDhcpv6Config==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(LanDBData.dhcpv6Config.Ipv6Prefix,BrgDhcpv6Config->Ipv6Prefix);
        strcpy(LanDBData.dhcpv6Config.Dhcpv6_Start_Addr,BrgDhcpv6Config->Dhcpv6_Start_Addr);
        strcpy(LanDBData.dhcpv6Config.Dhcpv6_End_Addr,BrgDhcpv6Config->Dhcpv6_End_Addr);
        LanDBData.dhcpv6Config.StateFull=BrgDhcpv6Config->StateFull;
        LanDBData.dhcpv6Config.StateLess=BrgDhcpv6Config->StateLess;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**********************************************************************************************
 *GetLanConfigDhcpv6ConfigInfo: API to get information on DHCPv6 config associated with the bridge
 * The purpose of the function is to get the DHCPv6 config details of the bridge from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgDhcpv6Config: Pointer to return DHCPv6 Config structur  with details like Address range, Prefix and State
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**********************************************************************************************/
LM_Status GetLanConfigDhcpv6ConfigInfo(const char *BrgAlias, DHCPV6Config *BrgDhcpv6Config)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgDhcpv6Config==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        strcpy(BrgDhcpv6Config->Ipv6Prefix,LanDBData.dhcpv6Config.Ipv6Prefix);
        strcpy(BrgDhcpv6Config->Dhcpv6_Start_Addr,LanDBData.dhcpv6Config.Dhcpv6_Start_Addr);
        strcpy(BrgDhcpv6Config->Dhcpv6_End_Addr,LanDBData.dhcpv6Config.Dhcpv6_End_Addr);
        BrgDhcpv6Config->StateFull=LanDBData.dhcpv6Config.StateFull;
        BrgDhcpv6Config->StateLess=LanDBData.dhcpv6Config.StateLess;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**************************************************************************************************
 *SetLanConfigFirewallConfigInfo: API to set information on Firewall config associated with the bridge
 * The purpose of the function is to set the Firewall config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgFirewallConfig: Pointer to FirewallConfig structure with details like Status and Level
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**************************************************************************************************/
LM_Status SetLanConfigFirewallConfigInfo(const char *BrgAlias, const FirewallConfig *BrgFirewallConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgFirewallConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        LanDBData.firewallConfig.Firewall_Level=BrgFirewallConfig->Firewall_Level;
        LanDBData.firewallConfig.Firewall_Enable=BrgFirewallConfig->Firewall_Enable;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**************************************************************************************************
 *GetLanConfigFirewallConfigInfo: API to get information on Firewall config associated with the bridge
 * The purpose of the function is to get the Firewall config details of the bridge from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgFirewallConfig: Pointer to return FirewallConfig structure with details like Status and Level
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**************************************************************************************************/
LM_Status GetLanConfigFirewallConfigInfo(const char *BrgAlias, FirewallConfig *BrgFirewallConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgFirewallConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        BrgFirewallConfig->Firewall_Level=LanDBData.firewallConfig.Firewall_Level;
        BrgFirewallConfig->Firewall_Enable=LanDBData.firewallConfig.Firewall_Enable;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/**************************************************************************************************
 *SetLanConfigSecurityConfigInfo: API to set information on Security config associated with the bridge
 * The purpose of the function is to set the Security config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgSecurityConfig: Pointer to SecurityConfig structure with details like Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**************************************************************************************************/
LM_Status SetLanConfigSecurityConfigInfo(const char *BrgAlias, const SecurityConfig *BrgSecurityConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgSecurityConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        LanDBData.securityConfig.VPN_Security_Enable=BrgSecurityConfig->VPN_Security_Enable;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    return LM_FAILURE;
}

/**************************************************************************************************
 *GetLanConfigSecurityConfigInfo: API to get information on Security config associated with the bridge
 * The purpose of the function is to get the Security config details of the bridge from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgSecurityConfig: Pointer to return SecurityConfig structure with details like Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
**************************************************************************************************/
LM_Status GetLanConfigSecurityConfigInfo(const char *BrgAlias, SecurityConfig *BrgSecurityConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgSecurityConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        BrgSecurityConfig->VPN_Security_Enable=LanDBData.securityConfig.VPN_Security_Enable;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}
#if 0
/*****************************************************************************************************
 *SetLanConfigIGDEnableConfigInfo: API to set information on Bridge IGD config associated with the bridge
 * The purpose of the function is to set the IGD Enable config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIGDEnableConfig: Pointer to boolean with Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*****************************************************************************************************/
LM_Status SetLanConfigIGDEnableConfigInfo(const char *BrgAlias, const bool *BrgIGDEnableConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIGDEnableConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        LanDBData.IGD_Enable=*BrgIGDEnableConfig;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/*****************************************************************************************************
 *GetLanConfigIGDEnableConfigInfo: API to get information on Bridge IGD config associated with the bridge
 * The purpose of the function is to get the IGD Enable config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgIGDEnableConfig: Pointer to boolean to return Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*****************************************************************************************************/
LM_Status GetLanConfigIGDEnableConfigInfo(const char *BrgAlias, bool *BrgIGDEnableConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgIGDEnableConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        *BrgIGDEnableConfig=LanDBData.IGD_Enable;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}
#endif
/*****************************************************************************************************
 *SetLanConfigStatusConfigInfo: API to set information on Bridge Status config associated with the bridge
 * The purpose of the function is to set the Bridge Enable config details of the bridge to the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgStatusConfig: Pointer to BridgeStatus structure with details of Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*****************************************************************************************************/
LM_Status SetLanConfigStatusConfigInfo(const char *BrgAlias, const BridgeStatus *BrgStatusConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgStatusConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        LanDBData.status=*BrgStatusConfig;

        if(LM_SUCCESS == LanConfigDataStoreAdd(&LanDBData))
        {
            return LM_SUCCESS;
        }
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/*****************************************************************************************************
 *GetLanConfigStatusConfigInfo: API to get information on Bridge Status config associated with the bridge
 * The purpose of the function is to get the Bridge Enable config details of the bridge from the
 * LanDB
 *Parameters:
 * BrgAlias: Alias name for the Bridge
 * BrgStatusConfig: Pointer to BridgeStatus structureto return details of Status
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and this function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*****************************************************************************************************/
LM_Status GetLanConfigStatusConfigInfo(const char *BrgAlias, BridgeStatus *BrgStatusConfig)
{
    LanConfig LanDBData;

    if((BrgAlias==NULL)||(BrgStatusConfig==NULL))
    {
        LM_TRACE("\r\n%s:Invalid Parameters are passed\r\n",__FUNCTION__);
        return LM_FAILURE;
    }

    if(LM_SUCCESS == LanConfigDataStoreGet(BrgAlias,&LanDBData))
    {
        *BrgStatusConfig=LanDBData.status;

        return LM_SUCCESS;
    }

    LM_TRACE("\r\n%s:Returned failure\r\n",__FUNCTION__);
    return LM_FAILURE;
}

/*****************************************************************************************************
 *LanConfigCreateBridges: API to create the Lan Bridges
 * The purpose of the function is to get the LanDB and then create the bridges using the same.
 *Parameters:
 * None
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The pointer to the actual LanDB is being used in this api and so care must be taken to not corrupt
 * the date in anyway
*****************************************************************************************************/
LM_Status LanConfigCreateBridges()
{
    LanConfig *LanDBFull = NULL;
    int numEntries = 0;

    if(LM_SUCCESS == LanConfigDataStoreGetAll(&numEntries, &LanDBFull))
    {
        for(int i=0;i<numEntries;i++)
        {
            printf("\r\n%s:Alias:%s\r\n",__FUNCTION__,LanDBFull[i].bridgeInfo.alias);
        }

        if(0 == CreateLanBridges(LanDBFull, numEntries))
        {
            return LM_SUCCESS;
        }

    }
    return LM_FAILURE;
}

/*****************************************************************************************************
 *LanConfigDeleteBridges: API to create the Lan Bridges
 * The purpose of the function is to get the LanDB and then delete the bridges using the same.
 *Parameters:
 * None
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The pointer to the actual LanDB is being used in this api and so care must be taken to not corrupt
 * the date in anyway
*****************************************************************************************************/
LM_Status LanConfigDeleteBridges()
{
    LanConfig *LanDBFull = NULL;
    int numEntries = 0;

    if(LM_SUCCESS == LanConfigDataStoreGetAll(&numEntries, &LanDBFull))
    {
        for(int i=0;i<numEntries;i++)
        {
            printf("\r\n%s:Alias:%s\r\n",__FUNCTION__,LanDBFull[i].bridgeInfo.alias);
        }

        if(0 == DeleteLanBridges(LanDBFull, numEntries))
        {
            return LM_SUCCESS;
        }

    }
    return LM_FAILURE;
}
