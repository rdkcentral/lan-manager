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

    module: cosa_dhcpv6_ipv6_utils.h

        For COSA Data Model Library Development.

    -------------------------------------------------------------------

    description:

        This header file contains IPv6 utility functions for DHCPv6.

    -------------------------------------------------------------------

    environment:

        platform independent

    -------------------------------------------------------------------

    revision:

        10/12/2025    initial revision - extracted from cosa_dhcpv6_apis.c
 

**************************************************************************/

#ifndef _COSA_DHCPV6_IPV6_H
#define _COSA_DHCPV6_IPV6_H

/**
 * @brief Process IPv6 sub-prefix when prefix length is less than 64
 * 
 * This function handles IPv6 sub-prefix processing for interfaces when the
 * delegated prefix length is less than 64 bits. It configures IPv6 addresses
 * on multiple LAN interfaces based on system configuration.
 * 
 * @param v6Tpref The IPv6 prefix string (e.g., "2001:db8::")
 * @param pref_len The prefix length in bits
 */
void process_ipv6_subprefix(const char *v6Tpref, int pref_len);

#endif /* _COSA_DHCPV6_IPV6_H */