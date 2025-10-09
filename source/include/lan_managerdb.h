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
#ifndef LMDATABASE_H
#define LMDATABASE_H

#include "lan_managerds.h"

LM_Status GetLanConfigBridgeInfo(const char *BrgAlias,BridgeInfo *BrgInfo);
LM_Status GetLanConfigInterfaceCountInfo(const char *BrgAlias, int *BrgIfaceCount);
LM_Status GetLanConfigInterfaceInfo(const char *BrgAlias, Iface *BrgIface);
LM_Status GetLanConfigDhcpInfo(const char *BrgAlias, DHCPConfig *BrgDhcpConfig);
LM_Status GetLanConfigIPConfigInfo(const char *BrgAlias, IPConfig *BrgIPConfig);
LM_Status GetLanConfigDhcpv6ConfigInfo(const char *BrgAlias, DHCPV6Config *BrgDhcpv6Config);
LM_Status GetLanConfigFirewallConfigInfo(const char *BrgAlias, FirewallConfig *BrgFirewallConfig);
LM_Status GetLanConfigSecurityConfigInfo(const char *BrgAlias, SecurityConfig *BrgSecurityConfig);
LM_Status GetLanConfigIGDEnableConfigInfo(const char *BrgAlias, bool *BrgIGDEnableConfig);
LM_Status GetLanConfigStatusConfigInfo(const char *BrgAlias, BridgeStatus *BrgStatusConfig);

LM_Status SetLanConfigBridgeInfo(const char *BrgAlias, const BridgeInfo *BrgInfo);
LM_Status SetLanConfigInterfaceCountInfo(const char *BrgAlias, const int *BrgIfaceCount);
LM_Status SetLanConfigInterfaceInfo(const char *BrgAlias, const Iface *BrgIface);
LM_Status SetLanConfigDhcpInfo(const char *BrgAlias, const DHCPConfig *BrgDhcpConfig);
LM_Status SetLanConfigIPConfigInfo(const char *BrgAlias, const IPConfig *BrgIPConfig);
LM_Status SetLanConfigDhcpv6ConfigInfo(const char *BrgAlias, const DHCPV6Config *BrgDhcpv6Config);
LM_Status SetLanConfigFirewallConfigInfo(const char *BrgAlias, const FirewallConfig *BrgFirewallConfig);
LM_Status SetLanConfigSecurityConfigInfo(const char *BrgAlias, const SecurityConfig *BrgSecurityConfig);
LM_Status SetLanConfigIGDEnableConfigInfo(const char *BrgAlias, const bool *BrgIGDEnableConfig);
LM_Status SetLanConfigStatusConfigInfo(const char *BrgAlias, const BridgeStatus *BrgStatusConfig);

LM_Status RemoveLanConfigEntry(const char *BrgAlias);

LM_Status LanConfigCreateBridges();
LM_Status LanConfigDeleteBridges();

#endif
