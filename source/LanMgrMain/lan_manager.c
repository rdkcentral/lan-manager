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
/**************************************************************************/
/*      INCLUDES:                                                         */
/**************************************************************************/
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "lanmgr_log.h"
#if !defined(_PLATFORM_RASPBERRYPI_)
#include <sys/types.h>
#endif
#include <unistd.h>
#include <sysevent/sysevent.h>
#include <syscfg/syscfg.h>
#include <pthread.h>
#include <time.h>
#include <telemetry_busmessage_sender.h>
#include "safec_lib_common.h"
#include "secure_wrapper.h"
#include "lan_manager_bridge.h"
#include "lanmgr_communication_apis.h"
#include "lan_manager_dml.h"
#include "lan_managerds.h"

//Added for lxcserver thread function
#if defined(_PLATFORM_RASPBERRYPI_)
#define PORT 8081
#endif
#ifdef MULTILAN_FEATURE
#define BRG_INST_SIZE 5
#define BUF_SIZE 256
#endif

#define BRMODE_ROUTER 0
#define LOGGING_INTERVAL_SECS    ( 60 * 60 )
/**************************************************************************/
/*! \fn void *LNM_sysevent_threadfunc(void *data)
 **************************************************************************
 *  \brief Function to process sysevent event
 *  \return 0
**************************************************************************/
typedef enum
{
    DOCESAFE_ENABLE_DISABLE_extIf
} DOCSIS_Esafe_Db_extIf_e;

typedef enum {
    BRING_LAN,
    PNM_STATUS,
    PRIMARY_LAN_13NET,
    LAN_STATUS,
    LNM_THREAD_ERROR
} eLnmThreadType;

typedef struct
{
    char         *msgStr;
    eLnmThreadType mType;
} LnmThread_MsgItem;

LnmThread_MsgItem lnmthreadMsgArr[] = {
    {"bring-lan",                                  BRING_LAN},
    {"pnm-status",                                 PNM_STATUS},
    {"primary_lan_l3net",                          PRIMARY_LAN_13NET},
    {"lan-status",                                 LAN_STATUS}
    };

/**************************************************************************/
/*      LOCAL DECLARATIONS:                                               */
/**************************************************************************/
//static void check_lan_wan_ready();
static int Lan_Manager_Init();
static void LAN_start();
/**************************************************************************/
/*      LOCAL VARIABLES:                                                  */
/**************************************************************************/
static int sysevent_fd;
static token_t sysevent_token;
static pthread_t sysevent_tid;
static int pnm_inited = 0;
static int netids_inited = 0;
static int hotspot_started = 0;
static int lan_telnet_started = 0;
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
static int ciscoconnect_started = 0;
#endif
static unsigned int factory_mode = 0;
//static int bridgeModeInBootup = 0;
static DOCSIS_Esafe_Db_extIf_e eRouterMode = DOCESAFE_ENABLE_DISABLE_extIf;
static int bridge_mode = BRMODE_ROUTER;
char *pComponentName = "LOG.RDK.LANMANAGER";

/**************************************************************************/
/*      LOCAL FUNCTIONS:                                                  */
/**************************************************************************/

eLnmThreadType Get_LnmThreadType(char * name)
{
    errno_t rc       = -1;
    int     ind      = -1;
    eLnmThreadType ret = LNM_THREAD_ERROR;
    if (name != NULL && name[0] != '\0')
    {
        int i;
        for (i = 0; i < LNM_THREAD_ERROR; i++) {
            rc = strcmp_s(lnmthreadMsgArr[i].msgStr,strlen(lnmthreadMsgArr[i].msgStr),name,&ind);
            ERR_CHK(rc);
            if((ind==0) && (rc == EOK))
            {
                ret = lnmthreadMsgArr[i].mType;
                break;
            }
        }
    }
    return ret;
}

static int LNM_SysCfgGetInt(const char *name)
{
   LanManagerInfo(("LNM_SysCfgGetInt\n"));
   char out_value[20];
   int outbufsz = sizeof(out_value);
        LanManagerInfo((" %s : name = %s \n", __FUNCTION__, name));
   if (!syscfg_get(NULL, name, out_value, outbufsz))
   {
        LanManagerInfo((" value = %s \n", out_value));
      return atoi(out_value);
   }
   else
   {
        LanManagerError((" syscfg get failed \n"));
      return -1;
   }
}

static void LAN_start(void)
{
    LanManagerInfo((" Entry %s \n", __FUNCTION__));
    // LAN Start May Be Delayed so refresh modes.
    LanManagerInfo(("The Previous EROUTERMODE=%d\n",eRouterMode));
    LanManagerInfo(("The Previous BRIDGE MODE=%d\n",bridge_mode));
    bridge_mode = LNM_SysCfgGetInt("bridge_mode");
    eRouterMode = LNM_SysCfgGetInt("last_erouter_mode");
    LanManagerInfo(("The Refreshed EROUTERMODE=%d\n",eRouterMode));
    LanManagerInfo(("The Refreshed BRIDGE MODE=%d\n",bridge_mode));
    if (bridge_mode == 0 && eRouterMode != 0) // mipieper - add erouter check for pseudo bridge. Can remove if bridge_mode is forced in response to erouter_mode.
    {
        LanManagerInfo(("Utopia starting lan...\n"));
        LanManagerInfo((" Setting lan-start event \n"));           
        sysevent_set(sysevent_fd, sysevent_token, "lan-start", "", 0);
        
        
    } else {
        // TODO: fix this
        LanManagerInfo(("Utopia starting bridge...\n"));
        LanManagerInfo((" Setting bridge-start event \n"));         
        sysevent_set(sysevent_fd, sysevent_token, "bridge-start", "", 0);
    }
    
#ifdef DSLITE_FEATURE_SUPPORT
    {
        char buf[2];
        if ((syscfg_get(NULL, "4_to_6_enabled", buf, sizeof(buf)) == 0) && (strcmp(buf, "1") == 0))
        {
            LanManagerInfo(("Setting dslite_enabled event\n"));
            sysevent_set(sysevent_fd, sysevent_token, "dslite_enabled", "1", 0);
        }
    }
#endif
    //ADD MORE LAN NETWORKS HERE
    LanManagerInfo((" Setting dhcp_server-resync event \n"));     
    sysevent_set(sysevent_fd, sysevent_token, "dhcp_server-resync", "", 0);
   
   return;
}


static void *LNM_sysevent_threadfunc(void *data)
{
    async_id_t primary_lan_l3net_asyncid;
    async_id_t lan_status_asyncid;
    //async_id_t bridge_status_asyncid;
    async_id_t pnm_asyncid;
    time_t time_now = { 0 }, time_before = { 0 };
    LanManagerInfo((" Entry %s \n", __FUNCTION__));
    sysevent_setnotification(sysevent_fd, sysevent_token, "primary_lan_l3net",  &primary_lan_l3net_asyncid);
    sysevent_setnotification(sysevent_fd, sysevent_token, "lan-status",  &lan_status_asyncid);
    //sysevent_setnotification(sysevent_fd, sysevent_token, "bridge-status",  &bridge_status_asyncid);
    #if !defined(INTEL_PUMA7) && !defined(_COSA_BCM_MIPS_) && !defined(_COSA_BCM_ARM_) && !defined(_COSA_QCA_ARM_)
    sysevent_setnotification(sysevent_fd, sysevent_token, "bring-lan",  &pnm_asyncid);
    #else
    sysevent_setnotification(sysevent_fd, sysevent_token, "pnm-status",  &pnm_asyncid);
    #endif
    LanManagerInfo((" Set notifications done \n"));
    for (;;)
    {
    #ifdef MULTILAN_FEATURE
        char name[64], val[64], buf[BUF_SIZE];
    #else
        char name[64], val[64];
    #ifdef CONFIG_CISCO_HOME_SECURITY
        char  buffer[10];
    #endif
    #endif
        int namelen = sizeof(name);
        int vallen  = sizeof(val);
        int err;
        async_id_t getnotification_asyncid;
        errno_t rc = -1;
        int ind = -1;
    #ifdef MULTILAN_FEATURE
        errno_t rc1 = -1;
        int ind1 = -1;
        char brlan0_inst[BRG_INST_SIZE] = {0};
        char brlan1_inst[BRG_INST_SIZE] = {0};
        char* l3net_inst = NULL;
    #endif
    
     LanManagerInfo(("get notification\n"));
     err = sysevent_getnotification(sysevent_fd, sysevent_token, name, &namelen,  val, &vallen, &getnotification_asyncid);
     if (err)
     {
            /*
             * Log should come for every 1hour
             * - time_now = getting current time
             * - difference between time now and previous time is greater than
             -       *    3600 seconds
             * - time_before = getting current time as for next iteration
             *    checking
             */
            LanManagerInfo(("err received\n"));
            time(&time_now);
            if(LOGGING_INTERVAL_SECS <= ((unsigned int)difftime(time_now, time_before)))
            {
                printf("%s-ERR: %d\n", __func__, err);
                time(&time_before);
            }
            sleep(10);
     }
     else
     {
            LanManagerInfo((" %s : name = %s, val = %s \n", __FUNCTION__, name, val ));
            eLnmThreadType ret_value;
            ret_value = Get_LnmThreadType(name);
            /*if (ret_value == WEBUI_FLAG_RESET)
            {
                LanManagerInfo(("webui flag reset\n"));
                webui_started = 0;
            }*/
#if !defined(INTEL_PUMA7) && !defined(_COSA_BCM_MIPS_) && !defined(_COSA_BCM_ARM_) && !defined(_COSA_QCA_ARM_)
            if (ret_value == BRING_LAN)
#else
            if (ret_value == PNM_STATUS)
#endif
            {
                LanManagerInfo((" bring-lan/pnm-status received \n"));
                pnm_inited = 1;
                if (netids_inited)
                {
                        LanManagerInfo((" Starting lan for PNM_status \n"));
                        LAN_start();
                }
            }
            else if (ret_value == PRIMARY_LAN_13NET)
            {
                  LanManagerInfo((" primary_lan_l3net received \n"));
                  if (pnm_inited)
                  {
#if defined (_PROPOSED_BUG_FIX_)
                      LanManagerInfo(("***STARTING LAN***\n"));
#endif
                      LanManagerInfo((" Starting lan for Primary lan 13net\n"));
                      LAN_start();
                  }
                  netids_inited = 1;
            }
            else if ( ret_value == LAN_STATUS )
            {
#if defined (_PROPOSED_BUG_FIX_)
                  LanManagerInfo(("***LAN STATUS/BRIDGE STATUS RECIEVED****\n"));
                  LanManagerInfo(("THE EVENT =%s VALUE=%s\n",name,val));
#endif
                  LanManagerInfo(("***LAN STATUS/BRIDGE STATUS RECIEVED****\n"));
                  rc = strcmp_s("started", strlen("started"),val, &ind);
                  ERR_CHK(rc);
                  if ((ind == 0) && (rc == EOK))
                  {
#ifdef CONFIG_CISCO_HOME_SECURITY
                      //Piggy back off the webui start event to signal XHS startup
                      sysevent_get(sysevent_fd, sysevent_token, "homesecurity_lan_l3net", buffer, sizeof(buffer));
                      if (buffer[0] != '\0') sysevent_set(sysevent_fd, sysevent_token, "ipv4-up", buffer, 0);
#endif	  

#if defined(RDK_ONEWIFI) && (defined(_XB6_PRODUCT_REQ_) || defined(_WNXL11BWL_PRODUCT_REQ_))
                      LanManagerInfo(("CALL VLAN UTIL TO SET UP LNF\n"));
#if defined(_RDKB_GLOBAL_PRODUCT_REQ_)
                      char lnfEnabled[8] = {0};
                      syscfg_get(NULL, "lost_and_found_enable", lnfEnabled, sizeof(lnfEnabled));
                      if(strncmp(lnfEnabled, "false", 5) != 0)
                      {
                           sysevent_set(sysevent_fd, sysevent_token, "lnf-setup","6", 0);
	              }
#else
                      sysevent_set(sysevent_fd, sysevent_token, "lnf-setup","6", 0);
#endif
#endif
 
                    
#ifdef MULTILAN_FEATURE
                       LanManagerInfo((" sysevent get for primary lan,homesecurity lanand l3net\n"));
                       sysevent_get(sysevent_fd, sysevent_token, "primary_lan_l3net", brlan0_inst, sizeof(brlan0_inst));
                       sysevent_get(sysevent_fd, sysevent_token, "homesecurity_lan_l3net", brlan1_inst, sizeof(brlan1_inst));
                       /*Get the active bridge instances and bring up the bridges */
                       sysevent_get(sysevent_fd, sysevent_token, "l3net_instances", buf, sizeof(buf));
                       l3net_inst = strtok(buf, " ");
                       while(l3net_inst != NULL)
                       {
                          rc = strcmp_s(l3net_inst, strlen(l3net_inst),brlan0_inst, &ind);
                          ERR_CHK(rc);
                          rc1 = strcmp_s(l3net_inst, strlen(l3net_inst),brlan1_inst, &ind1);
                          ERR_CHK(rc);
                          /*brlan0 and brlan1 are already up. We should not call their instances again*/
                          if(!(((ind == 0) && (rc == EOK)) || ((ind1 == 0) && (rc1 == EOK))))
                          {
                              sysevent_set(sysevent_fd, sysevent_token, "ipv4-up", l3net_inst, 0);
                          }
                          l3net_inst = strtok(NULL, " ");
                      }
#endif
                      if (!hotspot_started)
                      {
                          LanManagerInfo(("hotspot not started\n"));
#if defined(INTEL_PUMA7) || defined(_COSA_BCM_MIPS_) || defined(_COSA_BCM_ARM_) ||  defined(_COSA_INTEL_XB3_ARM_) || defined(_COSA_QCA_ARM_)
                          printf("Not Calling hotspot-start for XB3,XB6 and CBR it will be done in \
                                  cosa_start_rem.sh,hotspot.service and xfinity_hotspot_bridge_setup.sh respectively\n");
#else
                          sysevent_set(sysevent_fd, sysevent_token, "hotspot-start", "", 0);
                          hotspot_started = 1 ;
#endif
                      }
                      if (factory_mode && lan_telnet_started == 0)
                      {
                          LanManagerInfo(("factorymode\n"));
                          v_secure_system("/usr/sbin/telnetd -l /usr/sbin/cli -i brlan0");
                          lan_telnet_started=1;
                      }
#ifdef CONFIG_CISCO_FEATURE_CISCOCONNECT
                      if (!ciscoconnect_started)
                      {
                          sysevent_set(sysevent_fd, sysevent_token, "ciscoconnect-restart", "", 0);
                          ciscoconnect_started = 1 ;
                      }
#endif
                  }
              }
            }
        }
    return 0;
}

static int Lan_Manager_Init()
{
    LanManagerInfo((" starting lan manager init \n"));
    lan_manager_register_dml();
    sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "lan_manager", &sysevent_token);
    LanManagerInfo((" sysevent_fd %d \n", sysevent_fd));
     if (sysevent_fd >= 0)
    {
        LanManagerInfo((" Creating Thread  LNM_sysevent_threadfunc \n"));
        pthread_create(&sysevent_tid, NULL, LNM_sysevent_threadfunc, NULL);
    }
    return 0;
}

pid_t findProcessId(char* processName)
{
    FILE *f = NULL;
    pid_t pid = -1;
    char request[256], response[256];
    snprintf(request, sizeof(request), "ps  | grep %s", processName);
    if ((f = popen(request, "r")) != NULL)
    {
        fgets(response, (255), f);
        pclose(f);
    }
    snprintf(request,sizeof(request), "pidof %s", processName);
    if ((f = popen(request, "r")) != NULL)
    {
        fgets(response, (255), f);
        pid = atoi(response);
        pclose(f);
    }
    return pid;
}

#if defined(_ANSC_LINUX)
static void daemonize(void) {
	switch (fork()) {
	case 0:
		break;
	case -1:
		// Error
		LanManagerError(("Error daemonizing (fork)! %s - %d\n", __FUNCTION__, __LINE__ ));
		exit(0);
		break;
	default:
		_exit(0);
	}
	if (setsid() < 	0) {
		LanManagerError(("Error daemonizing (fork)! %s - %d\n", __FUNCTION__, __LINE__ ));
		exit(0);
	}
}
#endif

/**************************************************************************/
/*! \fn int main(int argc, char *argv)
 **************************************************************************
 *  \brief Init and run the Provisioning process
 *  \param[in] argc
 *  \param[in] argv
 *  \return Currently, never exits
 **************************************************************************/
int main(int argc, char *argv[])
{
    printf("Started lan_manager\n");
#if defined(_ANSC_LINUX)
    daemonize();
#endif
    if (lanManagerBusInit() != 0) {
        LanManagerError(("Failed to initialize RBUS. Exiting.\n"));
        return 1;
    }
    t2_init("lanmanager");
    LanManagerLogInit();
    Lan_Manager_Init();
    LanConfigDataStoreInit();
    PopulateAllBridges();
    LanManagerInfo(("wait in loop \n"));
    while (1)
    {
        sleep(1);
    }
    LanConfigDataStoreCleanup();
    lanManagerBusClose();
    if( findProcessId(argv[0]) > 0 )
    {
        printf("Already running\n");
        LanManagerInfo((" lan_manager already running. Returning...\n"));
        return 1;
    }
    return 0;
}
