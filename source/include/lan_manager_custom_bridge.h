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

#ifndef LAN_MANAGER_CUSTOM_BRIDGE_H
#define LAN_MANAGER_CUSTOM_BRIDGE_H

/**
 * @file lan_manager_custom_bridge.h
 * @brief Placeholder for custom bridge configuration macros
 *
 * This header is a stub for custom bridge macros. It is intentionally left without
 * macro definitions. All code that uses these macros must wrap their usage in #ifdef
 * checks to ensure safe compilation when the macros are not defined.
 *
 * To extend the LAN Manager with custom bridges, your Yocto meta-layer should
 * replace this file with one that defines the necessary macros:
 *   - LANMGR_CUSTOM_BRIDGE_TYPE_MACROS: for custom BridgeType enum entries
 *   - CUSTOM_BRIDGE_TABLE_ENTRIES: for custom LanConfigSource array entries
 *
 * Do not define the macros in this file. Only define them in your meta-layer override.
 */

#endif /* LAN_MANAGER_CUSTOM_BRIDGE_H */

