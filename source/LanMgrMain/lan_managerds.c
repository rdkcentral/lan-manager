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
#include "lan_managerds.h"
#include "lan_manager_dml.h"
#include "lan_manager_interface.h"
#include "lan_manager_dml.h"
#include "lanmgr_log.h"

static LanConfig *LanDB = NULL;
static int count = 0;
static int capacity = BASE_MAX_BRIDGES;

/**************************************************************
 *LanConfigDataStoreInit: Init the Bridge Info or LanDB Table
 * The purpose of the function is to allocate memory for the
 * LanDB table for BASE_MAX_BRIDGES number of entries.
 *Parameters:
 * None
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * Nil
 **************************************************************/
LM_Status LanConfigDataStoreInit() {
    gDM.lanConfigs = LanDB = (LanConfig*)malloc(capacity * sizeof(LanConfig));
    if(LanDB == NULL)
    {
        LanManagerError(("%s: Returned Failure\n", __FUNCTION__));
        return LM_FAILURE;
    }
    return LM_SUCCESS;
}

/*********************************************************************
 *LanConfigDataStoreCleanup: Clean up the Bridge Info or LanDB Table
 * The purpose of the function is to free the memory allocated for the
 * LanDB table.
 *Parameters:
 * None
 *Return:
 * None
 *Notes:
 * Nil
*********************************************************************/
void LanConfigDataStoreCleanup() {
    free(LanDB);
}

/************************************************************************************
 *LanConfigDataStoreAdd: Add entries into the LanDB Table
 * The purpose of the function is to add the LanConfig entries to the LanDB table
 * and if need be reallocate memory for the LanDB table in case the number of entries
 * exceeds BASE_MAX_BRIDGES.
 *Parameters:
 * LanInfo: Pointer to LanConfig structure to be added to LanDB table.
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and the function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
************************************************************************************/
LM_Status LanConfigDataStoreAdd(const LanConfig *LanInfo) {
    if(LanInfo==NULL)
    {
        LanManagerError(("%s: Invalid Parameters are passed\n", __FUNCTION__));
        return LM_FAILURE;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(LanDB[i].bridgeInfo.alias, LanInfo->bridgeInfo.alias) == 0) {
            /*Doing a shallow copy with the understanding that there are no
              pointers to nested structures in LanConfig*/
            LanDB[i] = *LanInfo; // Copy the struct
            return LM_SUCCESS;
        }
    }

    // Increase if needed
    if (count == capacity) {
        LanConfig *temp = (LanConfig*)realloc(LanDB, (capacity+1) * sizeof(LanConfig));
        if(temp == NULL)
        {
            LanManagerError(("%s: Realloc Failed\n", __FUNCTION__));
            return LM_FAILURE;
        }
        gDM.lanConfigs = LanDB = temp;
        capacity++; // Only increment after successful realloc
        LanManagerInfo(("%s: Realloc Success\n", __FUNCTION__));
    }
    /*Doing a shallow copy with the understanding that there are no
      pointers to nested structures in LanConfig*/
    LanDB[count] = *LanInfo;
    count++;
    return LM_SUCCESS;
}

/*************************************************************************************
 *LanConfigDataStoreGet: Get the entry from the LanDB Table
 * The purpose of the function is to return the LanConfig entry from the LanDB table
 * based on key which is ideally the alias name of the Bridge
 *Parameters:
 * result: Pointer to LanConfig structure to be returned from LanDB table.
 * key: Alias name of the bridge entry to be retrived.
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and the function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*************************************************************************************/
LM_Status LanConfigDataStoreGet(const char *key,LanConfig *result) {
    if((key==NULL)||(result==NULL))
    {
        LanManagerError(("%s: Invalid Parameters are passed\n", __FUNCTION__));
        return LM_FAILURE;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(LanDB[i].bridgeInfo.alias, key) == 0) {
            /*Doing a shallow copy with the understanding that there are no
              pointers to nested structures in LanConfig*/
            *result = LanDB[i]; // Copy the struct
            return LM_SUCCESS;
        }
    }
    return LM_FAILURE;
}

/*************************************************************************************
 *LanConfigDataStoreGetAll: Get the entire LanDB Table and entry count
 * The purpose of the function is to provide the the LanDB table and the entry count
 *Parameters:
 * fullTbl: Pointer to LanConfig structure to be return the LanDB table.
 * numEntries: Pointer to return count the entries in the LanDB table.
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and the function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*************************************************************************************/
LM_Status LanConfigDataStoreGetAll(int *numEntries, LanConfig **fullTbl) {
    if((numEntries==NULL)||(fullTbl==NULL))
    {
        LanManagerError(("%s: Invalid Parameters are passed\n", __FUNCTION__));
        return LM_FAILURE;
    }
    *fullTbl=LanDB;
    *numEntries=count;
    return LM_SUCCESS;

}

/*************************************************************************************
 *LanConfigDataStoreRemove: Remove the bridge entry from the LanDB Table
 * The purpose of the function is to remove the LanConfig entries from the LanDB table
 * and if need be reallocate memory for the LanDB table in case the number of entries
 * become equal to BASE_MAX_BRIDGES.
 *Parameters:
 * LanInfo: Pointer to LanConfig structure to be removed from LanDB table.
 *Return:
 * LM_SUCCESS
 * LM_FAILURE
 *Notes:
 * The parameters passed need to be valid and the function will not free any
 * parameter passed to it. Freeing dynamically allocated memory for parameters
 * is the responsibility of the calling function.
*************************************************************************************/
LM_Status LanConfigDataStoreRemove(const LanConfig *LanInfo) {
    if(LanInfo==NULL)
    {
        LanManagerError(("%s: Invalid Parameters are passed\n", __FUNCTION__));
        return LM_FAILURE;
    }

    for (int i = 0; i < count; i++) {
        if (strcmp(LanDB[i].bridgeInfo.alias, LanInfo->bridgeInfo.alias) == 0) {
            /*Doing a shallow copy with the understanding that there are no
              pointers to nested structures in LanConfig*/
            LanDB[i] = LanDB[count - 1]; // Replace with last entry
            count--;

            // Shrink if needed
            if (capacity > BASE_MAX_BRIDGES && count <= BASE_MAX_BRIDGES) {
                LanConfig *temp = (LanConfig*)realloc(LanDB, BASE_MAX_BRIDGES * sizeof(LanConfig));
                if (temp != NULL) {
                    gDM.lanConfigs = LanDB = temp;
                    capacity = BASE_MAX_BRIDGES;
                    LanManagerInfo(("%s: Realloc Success\n", __FUNCTION__));
                }
                else
                {
                    // If realloc fails, keep the old memory to avoid losing data
                    LanManagerError(("%s: Realloc Failed\n", __FUNCTION__));
                }
            }

            return LM_SUCCESS;
        }
    }
    return LM_FAILURE;
}

/*************************************************************************************
 *LanConfigDataStoreDump: Dump/Print the bridge entry from the LanDB Table
 * The purpose of the function is to dump the LanConfig entries from the LanDB table
 * for debug purposes
 *Parameters:
 * None
 *Return:
 * None
 *Notes:
 * Nil
*************************************************************************************/
void LanConfigDataStoreDump() {

    int travDB = 0;
    if(LanDB != NULL)
    {
        for(travDB=0;travDB < count;travDB++)
        {
            LanManagerInfo(("\r\n**********************************\r\n"));
            if(strcmp(LanDB[travDB].bridgeInfo.alias,"")!=0)
            {
                LanManagerInfo(("\r\nLanDB Entry:%15d\r\n",travDB));
                LanManagerInfo(("\r\n**********************************\r\n"));
                printf("\r\n|%15s|%17s|%27s|%26s|%24s|\r\n","Alias Name","BridgeInfo",
                        "IPConfig","DHCPConfig","DHCPv6Config");
                printf("\r\n|---------------|-----------------|---------------------------"
                        "|--------------------------|------------------------|\r\n");
                LanManagerInfo(("\r\n|%11s    | %s%10s | %s%15d | %s%12d | %s%15s |\r\n",LanDB[travDB].bridgeInfo.alias,
                        "Name:",LanDB[travDB].bridgeInfo.bridgeName,
                        "IP Status:",LanDB[travDB].ipConfig.Ip_Enable,
                        "DHCP Status:",LanDB[travDB].dhcpConfig.dhcpv4Config.Dhcpv4_Enable,
                        "Prefix:",LanDB[travDB].dhcpConfig.dhcpv6Config.Ipv6Prefix));
                LanManagerInfo(("\r\n|%15s| %s%10d | %s%18s | %s%13d | %s%12d |\r\n","",
                        "Type:",LanDB[travDB].bridgeInfo.networkBridgeType,
                        "IPv4 @:",LanDB[travDB].ipConfig.Ipv4Address,
                        "Lease Time:",LanDB[travDB].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time,
                        "StateFull:",LanDB[travDB].dhcpConfig.dhcpv6Config.StateFull));
                LanManagerInfo(("\r\n|%15s| %s%6d | %s%16s | %s%16s | %s%12d |\r\n","",
                        "Category:",LanDB[travDB].bridgeInfo.userBridgeCategory,
                        "Subnet @:",LanDB[travDB].ipConfig.IpSubNet,
                        "Start @:",LanDB[travDB].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr,
                        "StateLess:",LanDB[travDB].dhcpConfig.dhcpv6Config.StateLess));
                LanManagerInfo(("\r\n|%15s| %s%4d | %s%18s | %s%18s | %s%14s |\r\n","",
                        "STP Status:",LanDB[travDB].bridgeInfo.stpEnable,
                        "IPv6 @:",LanDB[travDB].ipConfig.Ipv6Address,
                        "End @:",LanDB[travDB].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr,
                        "Start @:",LanDB[travDB].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr));
                LanManagerInfo(("\r\n|%15s| %s%5d | %25s | %24s | %s%16s |\r\n","",
                        "IGD Status",LanDB[travDB].bridgeInfo.igdEnable,"","",
                        "End @:",LanDB[travDB].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr));
                LanManagerInfo(("\r\n|%15s| %s%5d | %25s | %24s | %s%16d |\r\n","",
                        "Life Time:",LanDB[travDB].bridgeInfo.bridgeLifeTime,
                        "","","@ Type",LanDB[travDB].dhcpConfig.dhcpv6Config.addrType));
                LanManagerInfo(("\r\n|               |-----------------|---------------------------"
                        "|--------------------------|------------------------|\r\n"));
                LanManagerInfo(("\r\n|%15s|%17s|%27s|%26s|%24s|\r\n","","FirewallConfig",
                        "SecurityConfig","",""));
                LanManagerInfo(("\r\n|               |-----------------|---------------------------"
                        "|--------------------------|------------------------|\r\n"));
                LanManagerInfo(("\r\n|%15s| %s%9d | %s%11d | %24s | %22s |\r\n","",
                        "Level:",LanDB[travDB].firewallConfig.Firewall_Level,
                        "Enable Status:",LanDB[travDB].securityConfig.VPN_Security_Enable,
                        "",""));
                LanManagerInfo(("\r\n|%15s| %s%1d | %25s | %24s | %22s |\r\n","",
                        "Enable Status:",LanDB[travDB].firewallConfig.Firewall_Enable,
                        "","",""));
                LanManagerInfo(("\r\n|---------------|-----------------|---------------------------"
                        "|--------------------------|------------------------|\r\n"));
                LanManagerInfo(("\r\n===========================================================\r\n"));
                LanManagerInfo(("\r\nNum of Interfaces:%d | Bridge Status:%d\r\n",
                        LanDB[travDB].numOfIfaces,LanDB[travDB].status));
                LanManagerInfo(("\r\n===========================================================\r\n"));
                LanManagerInfo(("\r\n------------------------------------------------------------\r\n"));
                LanManagerInfo(("\r\n| %-20s | %-6s | %-6s | %-6s |\n", "Interface Name", "Type", "VLAN", "Status"));
                LanManagerInfo(("\r\n------------------------------------------------------------\r\n"));

                for (int i = 0; i < MAX_IFACE_COUNT; i++) {
                    LanManagerInfo(("\r\n| %-20s | %-6d | %-6d | %-6d |\r\n", 
                            LanDB[travDB].interfaces[i].interfaceName,
                            LanDB[travDB].interfaces[i].InfType,
                            LanDB[travDB].interfaces[i].vlanId,
                            LanDB[travDB].interfaces[i].vlanEnable));
                }

                LanManagerInfo(("\r\n===========================================================\r\n"));
            }
        }
        LanManagerInfo(("\r\n**********************************\r\n"));
    }
    else
    {
        LanManagerInfo(("\r\nLanDB is NULL\r\n"));
    }
}
