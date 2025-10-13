/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**************************************************************************

    module: cosa_dhcpv6_ipv6_utils.c

        For COSA Data Model Library Development.

    -------------------------------------------------------------------

    description:

        This file implements IPv6 utility functions for DHCPv6.

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    revision:

        10/12/2025    initial revision - extracted from cosa_dhcpv6_apis.c
 

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "secure_wrapper.h"
#include "safec_lib_common.h"
#include "syscfg/syscfg.h"
#include "cosa_common_util.h"
#include "cosa_dhcpv6_ipv6_utils.h"

#include <sysevent/sysevent.h>
#include <ccsp_message_bus.h>
#include <ccsp_base_api.h>
#include <ccsp_psm_helper.h>

#ifdef CORE_NET_LIB
#include <libnet.h>
#endif

// Define macros for IPv6 functionality
#define ULA_ROUTE_SET "/tmp/.ula_route_set"
#define DEF_ULA_PREF_LEN 64
#define POS_PREFIX_DELEGATION 7

// Define macros for InterfaceEventHandler_thrd
#define COSA_DML_DHCPV6C_PREF_SYSEVENT_NAME "tr_dhcp6c_received_option_17_17"
#define SE_VERSION 1
#define TUPLE_FLAG_EVENT 1
#define CCSP_SUCCESS 100

// Define WiFi management support
#if defined (WIFI_MANAGE_SUPPORTED)
#define MANAGE_WIFI_BRIDGE_INDEX "dmsb.wifiagent.managewifibridge.index"
#endif

// External system variables  
extern void* g_pDslhDmlAgent;
extern ANSC_HANDLE bus_handle;
extern char g_Subsystem[32];

// Define macros for buffer lengths
#define BUFF_LEN_64 64
#define BUFF_LEN_8 8

// Global variables for sysevent functionality (made static for this file)
static int sysevent_fd_1 = -1;
static token_t sysevent_token_1;
static pthread_t InfEvtHandle_tid;

// Include platform-specific headers for device mode check
#ifdef RDKB_EXTENDER_ENABLED
extern int Get_Device_Mode(void);
#define DEVICE_MODE_ROUTER 1
#endif

static void _get_shell_output(FILE *fp, char *buf, size_t len)
{
    if (len > 0)
    {
        buf[0] = 0;
    }

    if (fp == NULL)
    {
        return;
    }

    buf = fgets(buf, len, fp);

    v_secure_pclose(fp); 

    if ((len > 0) && (buf != NULL))
    {
        len = strlen(buf);

        if ((len > 0) && (buf[len - 1] == '\n'))
        {
            buf[len - 1] = 0;
        }
    }
}

#ifdef RDKB_EXTENDER_ENABLED

/**
 * @brief Assigns an IPv6 address to a network interface
 * 
 * @param ifname Interface name
 * @param ipv6Addr IPv6 address to assign
 */
static void AssignIpv6Addr(char* ifname, char* ipv6Addr)
{
    if (!ifname || !ipv6Addr) return;
    
#ifdef CORE_NET_LIB
    addr_add_va_arg("-6 %s dev %s", ipv6Addr, ifname);
#else
    v_secure_system("ip -6 addr add %s dev %s", ipv6Addr, ifname);
#endif /* CORE_NET_LIB */
}
#endif

#ifdef RDKB_EXTENDER_ENABLED
/**
 * @brief Sets an IPv6 default route
 * 
 * @param ifname Interface name
 * @param route_addr Route address
 * @param metric_val Metric value (0 for default)
 */
static void SetV6Route(char* ifname, char* route_addr, int metric_val)
{
    if (!ifname || !route_addr) return;
    
    if (0 == metric_val)
#ifdef CORE_NET_LIB
        route_add_va_arg("-6 default via %s dev %s ", route_addr, ifname);
#else
        v_secure_system("ip -6 route add default via %s dev %s ", route_addr, ifname);
#endif /* CORE_NET_LIB */
    else
#ifdef CORE_NET_LIB
        route_add_va_arg("-6 default via %s dev %s metric %d", route_addr, ifname, metric_val);
#else
        v_secure_system("ip -6 route add default via %s dev %s metric %d", route_addr, ifname, metric_val);
#endif /* CORE_NET_LIB */
}
#endif

int openCommonSyseventConnection();
int commonSyseventClose();
int commonSyseventSet(char* key, char* value);
int commonSyseventGet(char* key, char* value, int valLen);

#if 0
int commonSyseventFd = -1;
token_t commonSyseventToken;

int openCommonSyseventConnection() {
    if (commonSyseventFd == -1) {
        commonSyseventFd = s_sysevent_connect(&commonSyseventToken);
    }
    return 0;
}

int commonSyseventClose() {
    int retval;

    if(commonSyseventFd == -1) {
        return 0;
    }

    retval = sysevent_close(commonSyseventFd, commonSyseventToken);
    commonSyseventFd = -1;
    return retval;
}

int commonSyseventSet(char* key, char* value){
    if(commonSyseventFd == -1) {
        openCommonSyseventConnection();
    }
    return sysevent_set(commonSyseventFd, commonSyseventToken, key, value, 0);
}

int commonSyseventGet(char* key, char* value, int valLen){
    if(commonSyseventFd == -1) {
        openCommonSyseventConnection();
    }
    return sysevent_get(commonSyseventFd, commonSyseventToken, key, value, valLen);
}
#endif

static int CalcIPv6Prefix(char *GlobalPref, char *pref, int index)
{
    unsigned char buf[sizeof(struct in6_addr)];
    int domain, s;
    errno_t rc = -1;
    char str[INET6_ADDRSTRLEN];
    
    domain = AF_INET6;
    s = inet_pton(domain, GlobalPref, buf);
    if (s <= 0) {
        if (s == 0)
            fprintf(stderr, "Not in presentation format");
        else
            perror("inet_pton");
        return 0;
    }
    
    buf[POS_PREFIX_DELEGATION] = buf[POS_PREFIX_DELEGATION] + index;
    
    if (inet_ntop(domain, buf, str, INET6_ADDRSTRLEN) == NULL) {
        perror("inet_ntop");
        return 0;
    }
    
    printf("%s\n", str);
    rc = STRCPY_S_NOCLOBBER(pref, 100, str);
    if(rc != EOK)
    {
        ERR_CHK(rc);
        return 0;
    }
    return 1;
}

static int GenIPv6Prefix(const char *ifName, const char *GlobalPref, char *pref, int len)
{
    int index = 0;
    char cmd[100] = {0};
    char out[100] = {0};
    errno_t rc = -1;
    static int interface_num = 4; // Reserving first 4 /64s for dhcp configurations
    
    if(ifName == NULL)
        return 0;

    rc = sprintf_s(cmd, sizeof(cmd), "%s_ipv6_index", ifName);
    if(rc < EOK)
    {
        ERR_CHK(rc);
        return 0;
    }
    commonSyseventGet(cmd, out, sizeof(out));
    if(strlen(out) != 0)
    {
        index = atoi(out);
    }

    rc = STRCPY_S_NOCLOBBER(pref, 100, GlobalPref);
    if(rc != EOK)
    {
        ERR_CHK(rc);
        return 0;
    }
    
    if(index == 0)
    {
        if(CalcIPv6Prefix((char*)GlobalPref, pref, interface_num) == 0)
            return 0;
        rc = sprintf_s(cmd, sizeof(cmd), "%s_ipv6_index", ifName);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
        rc = sprintf_s(out, sizeof(out), "%d", interface_num);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return 0;
        }
        commonSyseventSet(cmd, out);
        interface_num++;
    }
    else
    {
        if(CalcIPv6Prefix((char*)GlobalPref, pref, index) == 0)
            return 0;
    }
    
    /* CID 71710 Fix*/
    strncat(pref, "/64", len - strlen(pref) - 1);
    printf("%s: pref %s\n", __func__, pref);
    return 1;
}

#ifdef RDKB_EXTENDER_ENABLED
/**
 * @brief Enables ULA (Unique Local Address) IPv6 on an interface
 * 
 * @param ifname Interface name
 */
static void enable_Ula_IPv6(char* ifname)
{
    if (!ifname) return;
    
    char *token_pref = NULL;
    char buf[128] = {0}, cmd[128] = {0};
    char pref_rx[16];
    char ipv6_addr[128] = {0};
    
    memset(buf, 0, sizeof(buf));
    commonSyseventGet("ula_ipv6_enabled", buf, sizeof(buf));
    if (1 == atoi(buf))
    {
        memset(cmd, 0, sizeof(cmd));
        memset(buf, 0, sizeof(buf));
        int pref_len = DEF_ULA_PREF_LEN;
        memset(pref_rx, 0, sizeof(pref_rx));
        commonSyseventGet("backup_wan_prefix_v6_len", pref_rx, sizeof(pref_rx));

        if (strlen(pref_rx) != 0)
            pref_len = atoi(pref_rx);

        snprintf(cmd, sizeof(cmd), "%s_ipaddr_v6_ula", ifname);
        commonSyseventGet(cmd, buf, sizeof(buf));
        if (buf[0] != '\0' && strlen(buf) != 0)
        {
            SetV6Route(ifname, buf, 0);
            token_pref = strtok(buf, "/");
            memset(ipv6_addr, 0, sizeof(ipv6_addr));
            snprintf(ipv6_addr, sizeof(ipv6_addr), "%s:1/%d", token_pref, pref_len);
            AssignIpv6Addr(ifname, ipv6_addr);
        }
    }
}
#endif

/**
 * @brief Enables IPv6 on a network interface
 * 
 * @param if_name Interface name
 */
static void enable_IPv6(char* if_name)
{
    FILE *fp = NULL;
    char tbuff[100], ipv6_addr[128] = {0}, cmd[128] = {0};
    errno_t rc = -1;

    if (!if_name) return;

    fp = v_secure_popen("r", "sysctl net.ipv6.conf.%s.autoconf", if_name);
    _get_shell_output(fp, tbuff, sizeof(tbuff));

    if (tbuff[strlen(tbuff)-1] == '0')
    {
        v_secure_system("sysctl -w net.ipv6.conf.%s.autoconf=1", if_name);
#ifdef CORE_NET_LIB
        interface_down(if_name);
        interface_up(if_name);
#else
        v_secure_system("ifconfig %s down;ifconfig %s up", if_name, if_name);
#endif /* CORE_NET_LIB */
    }
    
    rc = sprintf_s(cmd, sizeof(cmd), "%s_ipaddr_v6", if_name);
    if (rc < EOK)
    {
        // Handle error silently for now
        return;
    }
    
    commonSyseventGet(cmd, ipv6_addr, sizeof(ipv6_addr));
#ifdef CORE_NET_LIB
    route_add_va_arg("-6 %s dev %s", ipv6_addr, if_name);
#else
    v_secure_system("ip -6 route add %s dev %s", ipv6_addr, if_name);
#endif /* CORE_NET_LIB */

#ifdef _COSA_INTEL_XB3_ARM_
#ifdef CORE_NET_LIB
    route_add_va_arg("-6 %s dev %s table erouter", ipv6_addr, if_name);
#else
    v_secure_system("ip -6 route add %s dev %s table erouter", ipv6_addr, if_name);
#endif /* CORE_NET_LIB */
#endif

#ifdef CORE_NET_LIB
    rule_add_va_arg("-6 iif %s lookup erouter", if_name);
#else
    v_secure_system("ip -6 rule add iif %s lookup erouter", if_name);
#endif /* CORE_NET_LIB */

#ifdef RDKB_EXTENDER_ENABLED
    if (DEVICE_MODE_ROUTER == Get_Device_Mode() && access(ULA_ROUTE_SET, R_OK) == 0)
    {
        enable_Ula_IPv6(if_name);
    }
#endif
}

/**
 * @brief Removes an interface from the IPv6_Interface configuration list
 * 
 * @param Inf_name Interface name to remove
 * @return int 0 on success
 */
static int remove_interface(char* Inf_name)
{
    char *token = NULL;
    char *pt;
    char OutBuff[128], buf[128];
    
    if (!Inf_name) return -1;
    
    memset(OutBuff, 0, sizeof(OutBuff));
    
    syscfg_get(NULL, "IPv6_Interface", buf, sizeof(buf));
    // interface is present in the list, we need to remove interface to disable IPv6 PD
    pt = buf;
    while((token = strtok_r(pt, ",", &pt))) {
        if(strncmp(Inf_name, token, strlen(Inf_name)))
        {       
            /* CID 173703 Fix*/
            strncat(OutBuff, token, sizeof(OutBuff) - strlen(OutBuff) - 1);
            strncat(OutBuff, ",", sizeof(OutBuff) - strlen(OutBuff) - 1);
        }
    }
    syscfg_set_commit(NULL, "IPv6_Interface", OutBuff);
    return 0;
}

/**
 * @brief Appends an interface to the IPv6_Interface configuration list
 * 
 * @param Inf_name Interface name to append
 * @return int 0 on success
 */
static int append_interface(char* Inf_name)
{
    /* CID 174287 fix */
    char OutBuff[129], buf[128];
    
    if (!Inf_name) return -1;
    
    memset(OutBuff, 0, sizeof(OutBuff));
    
    syscfg_get(NULL, "IPv6_Interface", buf, sizeof(buf));
    
    strncpy(OutBuff, buf, (sizeof(OutBuff) - 1));
    /*CID 173701 fix*/
    strncat(OutBuff, Inf_name, sizeof(OutBuff) - strlen(OutBuff) - 1);
    strncat(OutBuff, ",", sizeof(OutBuff) - strlen(OutBuff) - 1);
    syscfg_set_commit(NULL, "IPv6_Interface", OutBuff);
    return 0;
}

#ifdef RDKB_EXTENDER_ENABLED
/**
 * @brief Gets device mode from system configuration
 * 
 * @return int Device mode value
 */
static int Get_Device_Mode()
{
    int deviceMode = 0;
    char buf[8] = {0};
    memset(buf, 0, sizeof(buf));
    if (0 == syscfg_get(NULL, "Device_Mode", buf, sizeof(buf)))
    {
        if (buf[0] != '\0' && strlen(buf) != 0)
            deviceMode = atoi(buf);
    }
    return deviceMode;
}
#endif

/**
 * @brief Extracts prefix information from a prefix string
 * 
 * @param prefix Input prefix string
 * @param value Output value buffer
 * @param val_len Value buffer length
 * @param prefix_len Output prefix length
 * @return int 0 on success, -1 on error
 */
static int getprefixinfo(const char *prefix, char *value, unsigned int val_len, unsigned int *prefix_len)
{
    /* CID 173700 Dereference after null check fix */
    if (prefix_len == NULL || !prefix || !value) {
        return -1;
    }
  
    int i;

    i = strlen(prefix);

    while((prefix[i-1] != '/') && (i > 0)) i--;

    if(i == 0) {
        return -1;
    }

    *prefix_len = atoi(&prefix[i]);

    /* CID 56443 fix*/
    snprintf(value, val_len, "%.*s", i-1, prefix);

    return 0;
}

/**
 * @brief Generates and updates IPv6 prefix into sysevent
 * 
 * @param pInfName Interface name
 * @return int 0 on success, -1 on error
 */
static int GenAndUpdateIpv6PrefixIntoSysevent(char *pInfName)
{
    char out1[128] = {0};
    char cmd[256];
    char ipv6_prefix[64] = {0};
    char prefixvalue[INET6_ADDRSTRLEN] = {0};
    int  len = 0;
    errno_t rc = -1;

    if (!pInfName)
        return -1;
        
    commonSyseventGet(COSA_DML_DHCPV6C_PREF_SYSEVENT_NAME, ipv6_prefix, sizeof(ipv6_prefix));

    if (getprefixinfo(ipv6_prefix, prefixvalue, sizeof(prefixvalue), (unsigned int*)&len) != 0) 
    {
        return -1;
    }
    
    if(GenIPv6Prefix(pInfName, prefixvalue, out1, sizeof(out1)))
    {
        rc = sprintf_s(cmd, sizeof(cmd), "%s_ipaddr_v6", pInfName);
        if(rc < EOK)
        {
            ERR_CHK(rc);
            return -1;
        }
        commonSyseventSet(cmd, out1);
    }
    return 0;
}

/**
 * @brief Handles MoCA IPv6 configuration based on status
 * 
 * @param status MoCA interface status
 * @return int 0 on success, -1 on error
 */
static int handle_MocaIpv6(char *status)
{
    FILE *fp = NULL;
    char *Inf_name = NULL;
    int retPsmGet, retPsmGet1 = CCSP_SUCCESS;
    char *str = NULL;
    int HomeIsolationEnable = 0;
    char tbuff[100];
    char ipv6If[128] = {0}; 
    char mbuf[128] = {0};
    int restart_zebra = 0;
  
    if (!status)
        return -1;

    //checking Homeisolation is enabled and ipv6_moca_bridge is true
    retPsmGet1 = PSM_Get_Record_Value2(bus_handle, g_Subsystem, "dmsb.l2net.HomeNetworkIsolation", NULL, &str);
    if(retPsmGet1 == CCSP_SUCCESS) {
        HomeIsolationEnable = _ansc_atoi(str);
    }
    else
    {
        return -1;
    }
    
    syscfg_get(NULL, "ipv6_moca_bridge", mbuf, sizeof(mbuf));
    syscfg_get(NULL, "IPv6_Interface", ipv6If, sizeof(ipv6If));

    ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(str);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, "dmsb.l2net.9.Name", NULL, &Inf_name);
    if(retPsmGet != CCSP_SUCCESS )
    {
        return -1;
    }
    if(!retPsmGet)
    {
        retPsmGet = CCSP_SUCCESS;
        if(NULL == Inf_name){
            Inf_name = (char *)AnscAllocateMemory( (strlen("brlan10") + 1) );
            strncpy(Inf_name, "brlan10", strlen("brlan10") +1);
        }
    }
    if(strcmp((const char*)status, "ready") == 0)
    {
        /*CID: 173691  - Array Compared against null - Fix */
        if (mbuf[0] != '\0' && ( ipv6If[0] != '\0' ) && ( Inf_name != NULL ))
        {
            if( (strcmp(mbuf, "true") == 0) && (HomeIsolationEnable == 1))
            {
                if (!strstr(ipv6If, Inf_name)) {
                    append_interface(Inf_name);
                    GenAndUpdateIpv6PrefixIntoSysevent(Inf_name);
                    restart_zebra = 1;
                }
            }
            else if ( (strcmp(mbuf, "false") == 0) || (HomeIsolationEnable == 0))
            {
                if (strstr(ipv6If, Inf_name)){
                    remove_interface(Inf_name);
                    restart_zebra = 1;
                }
            }
        }
        if (retPsmGet == CCSP_SUCCESS)
        {                      
            memset(tbuff, 0, sizeof(tbuff));
            fp = v_secure_popen("r", "sysctl net.ipv6.conf.%s.autoconf", Inf_name);
            _get_shell_output(fp, tbuff, sizeof(tbuff));
            if(tbuff[strlen(tbuff)-1] == '0')
            {
                enable_IPv6(Inf_name);
            }

            /* CID 180991 fix */
            if(NULL != Inf_name){
                ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(Inf_name);
                Inf_name = NULL;
            }
        }

    }
    if(strcmp((const char*)status, "stopped") == 0)
    {
        /* CID 173698 Explicit null dereferenced fix */
        if ( (strcmp(mbuf, "false") == 0) || (HomeIsolationEnable == 0) )
        {
            if ( Inf_name && strstr(ipv6If, Inf_name) ) {
                remove_interface(Inf_name);
                restart_zebra = 1;
            }
        }
    }
    if (restart_zebra)
    {
        v_secure_system("sysevent set zebra-restart");
    }
    if(NULL != Inf_name){
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(Inf_name);
        Inf_name = NULL;
    }
    return 0;
}

/**
 * @brief Interface event handler thread for managing IPv6 on various interfaces
 * 
 * @param data Thread data (unused)
 * @return void* NULL on completion
 */
void *InterfaceEventHandler_thrd(void *data)
{
    UNREFERENCED_PARAMETER(data);
    async_id_t interface_asyncid;
    async_id_t interface_XHS_asyncid;
    async_id_t interface_POD_asyncid;
    async_id_t interface_MoCA_asyncid;
#if defined (WIFI_MANAGE_SUPPORTED)
    async_id_t interface_WiFi_asyncid;
    char aMultiNetStatus[BUFF_LEN_64] = {0};
    char index[BUFF_LEN_8] = {0};
#endif /*WIFI_MANAGE_SUPPORTED*/
    
    sysevent_fd_1 = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "Interface_evt_handler", &sysevent_token_1);

    sysevent_set_options(sysevent_fd_1, sysevent_token_1, "multinet_6-status", TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd_1, sysevent_token_1, "multinet_6-status",  &interface_asyncid);
    sysevent_set_options(sysevent_fd_1, sysevent_token_1, "multinet_2-status", TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd_1, sysevent_token_1, "multinet_2-status",  &interface_XHS_asyncid);
    sysevent_set_options(sysevent_fd_1, sysevent_token_1, "multinet_10-status", TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd_1, sysevent_token_1, "multinet_10-status",  &interface_POD_asyncid);
    sysevent_set_options(sysevent_fd_1, sysevent_token_1, "multinet_9-status", TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd_1, sysevent_token_1, "multinet_9-status",  &interface_MoCA_asyncid);

#if defined (WIFI_MANAGE_SUPPORTED)
    psmGet(MANAGE_WIFI_BRIDGE_INDEX, index, BUFF_LEN_8);
    if ('\0' != index[0])
    {
        snprintf (aMultiNetStatus, BUFF_LEN_64, "multinet_%s-status", index);
        sysevent_set_options(sysevent_fd_1, sysevent_token_1, aMultiNetStatus, TUPLE_FLAG_EVENT);
        sysevent_setnotification(sysevent_fd_1, sysevent_token_1, aMultiNetStatus,  &interface_WiFi_asyncid);
    }
#endif /*WIFI_MANAGE_SUPPORTED*/

    FILE *fp = NULL;
    char *Inf_name = NULL;
    int retPsmGet = CCSP_SUCCESS;
    char tbuff[100];
    int err;
    char name[25] = {0}, val[42] = {0}, buf[128], cmd[128];
    errno_t rc = -1;
    
    rc = strcpy_s(cmd, sizeof(cmd), "multinet_9-status");
    ERR_CHK(rc);

    buf[0] = 0;
    commonSyseventGet(cmd, buf, sizeof(buf));
            
    handle_MocaIpv6(buf);

    rc = strcpy_s(cmd, sizeof(cmd), "multinet_10-status");
    ERR_CHK(rc);

    buf[0] = 0;
    commonSyseventGet(cmd, buf, sizeof(buf));
            
    if(strcmp((const char*)buf, "ready") == 0)
    {
        retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, "dmsb.l2net.10.Name", NULL, &Inf_name);
        if(!retPsmGet)
        {
            retPsmGet = CCSP_SUCCESS;
            Inf_name = "br403";        
        }
        if (retPsmGet == CCSP_SUCCESS)
        {                      
            memset(tbuff, 0, sizeof(tbuff));
            fp = v_secure_popen("r", "sysctl net.ipv6.conf.%s.autoconf", Inf_name);
            _get_shell_output(fp, tbuff, sizeof(tbuff));
            if(tbuff[strlen(tbuff)-1] == '0')
            {
                enable_IPv6(Inf_name);
            }

            ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(Inf_name);
            Inf_name = NULL;
        }
    
    }

    rc = strcpy_s(cmd, sizeof(cmd), "multinet_2-status");
    ERR_CHK(rc);

    buf[0] = 0;
    commonSyseventGet(cmd, buf, sizeof(buf));

    if(strcmp((const char*)buf, "ready") == 0)
    {
        retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, "dmsb.l2net.2.Port.1.Name", NULL, &Inf_name);
        if (retPsmGet == CCSP_SUCCESS)
        {
            fp = v_secure_popen("r", "sysctl net.ipv6.conf.%s.autoconf", Inf_name);
            _get_shell_output(fp, tbuff, sizeof(tbuff));
            if(tbuff[strlen(tbuff)-1] == '0')
            {
                enable_IPv6(Inf_name);
            }
            ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(Inf_name);
            Inf_name = NULL;
        }

    }

    rc = strcpy_s(cmd, sizeof(cmd), "multinet_6-status");
    ERR_CHK(rc);

    buf[0] = 0;
    commonSyseventGet(cmd, buf, sizeof(buf));

    if(strcmp((const char*)buf, "ready") == 0)
    {
        fp = v_secure_popen("r", "sysctl net.ipv6.conf.br106.autoconf");
        _get_shell_output(fp, tbuff, sizeof(tbuff));
        if(tbuff[strlen(tbuff)-1] == '0')
        {
            enable_IPv6("br106");
        }

    }

    while(1)
    {
        async_id_t getnotification_asyncid;
        memset(name, 0, sizeof(name));
        memset(val, 0, sizeof(val));
        memset(cmd, 0, sizeof(cmd));
        memset(buf, 0, sizeof(buf));

        int namelen = sizeof(name);
        int vallen  = sizeof(val);
        err = sysevent_getnotification(sysevent_fd_1, sysevent_token_1, name, &namelen,  val, &vallen, &getnotification_asyncid);

        if (err)
        {
            if ( 0 != v_secure_system("pidof syseventd")) {
                break;
            }    
        }
        else
        {
            if(strcmp((const char*)name, "multinet_6-status") == 0)
            {
                if(strcmp((const char*)val, "ready") == 0)
                {
                    enable_IPv6("br106");
                }
            }

            if(strcmp((const char*)name, "multinet_2-status") == 0)
            {
                if(strcmp((const char*)val, "ready") == 0)
                {
                    Inf_name = NULL ;
                    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, "dmsb.l2net.2.Port.1.Name", NULL, &Inf_name);
                    if (retPsmGet == CCSP_SUCCESS)
                    {               
                        enable_IPv6(Inf_name);
                        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(Inf_name);
                    }
                }
            }
            
            if(strcmp((const char*)name, "multinet_10-status") == 0)
            {
                if(strcmp((const char*)val, "ready") == 0)
                {
                    Inf_name = NULL ;
                    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, "dmsb.l2net.10.Name", NULL, &Inf_name);
                    if(!retPsmGet)
                    {
                        retPsmGet = CCSP_SUCCESS;
                        Inf_name = "br403";        
                    }
                    if (retPsmGet == CCSP_SUCCESS)
                    {               
                        enable_IPv6(Inf_name);
                        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(Inf_name);
                    }
                }

            }    

            if(strcmp((const char*)name, "multinet_9-status") == 0)
            {
                handle_MocaIpv6(val);

            }
#if defined (WIFI_MANAGE_SUPPORTED)
            if((0 == strcmp((const char*)name, aMultiNetStatus)) && (0 == strcmp((const char*)val, "ready")))
            {
                char aParamName[BUFF_LEN_64] = {0};
                char aParamVal[BUFF_LEN_64] = {0};
                char aBridgeName[BUFF_LEN_64] = {0};
                snprintf(aParamName, BUFF_LEN_64, "dmsb.l2net.%s.Name", index);
                psmGet(aParamName, aParamVal, BUFF_LEN_64);
                if ('\0' != aParamVal[0])
                {   
                    /*CID 66870*/
                    strncpy(aBridgeName, aParamVal, sizeof(aBridgeName)-1);
                }
                else
                {
                    /* CID 66870 - Calling risky Function Fix */
                    strncpy(aBridgeName, "brlan15", sizeof(aBridgeName) - 1);
                }
                snprintf(aParamName, BUFF_LEN_64, "dmsb.l3net.%s.IPv6Enable", index);
                psmGet(aParamName, aParamVal, BUFF_LEN_64);
                if (('\0' != aParamVal[0]) && (!strncmp(aParamVal, "true", 4)))
                {
                    if (!GenAndUpdateIpv6PrefixIntoSysevent(aBridgeName))
                    {
                        enable_IPv6(aBridgeName);
                    }
                }
            }
#endif /*WIFI_MANAGE_SUPPORTED*/
        }
    }
    return NULL;
}

void process_ipv6_subprefix(const char *v6Tpref, int pref_len)
{
    if (pref_len < 64)
    {
        char out1[100];
        char *token = NULL, *pt;
        char interface_name[32] = {0};
        char out[128] = {0};
        char cmd[100];
        FILE *fp = NULL;
        errno_t rc = -1;

        memset(out1, 0, sizeof(out1));
        fp = v_secure_popen("r", "syscfg get IPv6subPrefix");
        _get_shell_output(fp, out, sizeof(out));
        
        if (!strcmp(out, "true"))
        {
            static int first = 0;

            fp = v_secure_popen("r", "syscfg get IPv6_Interface");
            _get_shell_output(fp, out, sizeof(out));
            pt = out;

            while ((token = strtok_r(pt, ",", &pt)))
            {
                if (GenIPv6Prefix(token, v6Tpref, out1, sizeof(out1)))
                {
                    memset(cmd, 0, sizeof(cmd));
                    memset(interface_name, 0, sizeof(interface_name));

#ifdef _COSA_INTEL_XB3_ARM_
                    char LnFIfName[32] = {0}, LnFBrName[32] = {0};
                    syscfg_get(NULL, "iot_ifname", LnFIfName, sizeof(LnFIfName));
                    if ((LnFIfName[0] != '\0') && (strlen(LnFIfName) != 0))
                    {
                        if (strcmp((const char*)token, LnFIfName) == 0)
                        {
                            syscfg_get(NULL, "iot_brname", LnFBrName, sizeof(LnFBrName));
                            if ((LnFBrName[0] != '\0') && (strlen(LnFBrName) != 0))
                            {
                                strncpy(interface_name, LnFBrName, sizeof(interface_name) - 1);
                            }
                            else
                            {
                                strncpy(interface_name, token, sizeof(interface_name) - 1);
                            }
                        }
                        else
                        {
                            strncpy(interface_name, token, sizeof(interface_name) - 1);
                        }
                    }
                    else
                    {
                        strncpy(interface_name, token, sizeof(interface_name) - 1);
                    }
#else
                    strncpy(interface_name, token, sizeof(interface_name) - 1);
#endif
                    rc = sprintf_s(cmd, sizeof(cmd), "%s_ipaddr_v6", interface_name);
                    if (rc < EOK)
                    {
                        ERR_CHK(rc);
                    }
                    commonSyseventSet(cmd, out1);

                    enable_IPv6(interface_name);
                    memset(out1, 0, sizeof(out1));
                }
            }
            memset(out, 0, sizeof(out));
            if (first == 0)
            {
                first = 1;
                pthread_create(&InfEvtHandle_tid, NULL, InterfaceEventHandler_thrd, NULL);
            }
        }
    }
}
