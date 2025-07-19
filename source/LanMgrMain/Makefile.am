##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2017 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
bin_PROGRAMS = lan_manager

AM_CPPFLAGS =   -I$(top_srcdir)/source/include \
                -I$(top_srcdir)/source/LanMgrMain \
                -I=${includedir} \
                -I=${includedir}/ccsp
AM_CFLAGS = -D_ANSC_LINUX -D_ANSC_USER -D_ANSC_LITTLE_ENDIAN_ -DFEATURE_SUPPORT_RDKLOG $(DBUS_CFLAGS)
AM_LDFLAGS = -lsysevent -lsyscfg -lsecure_wrapper -ltelemetry_msgsender
AM_LDFLAGS = $(DBUS_LIBS)
lan_manager_LDFLAGS = -lsysevent -lsyscfg
lan_manager_SOURCES = lanmgr_log.c lan_manager.c

