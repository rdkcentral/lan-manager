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
#include <stdbool.h>
#include <stdarg.h>
#include "rdk_debug.h"

/**
 * @brief Enables or disables logs of different severity levels.
 */
#define DEBUG_INI_NAME  "/etc/debug.ini"

#define ARGS_EXTRACT(msg ...) msg

#define  LANMANAGER_LOG(level, msg)  \
    RDK_LOG(level,"LOG.RDK.LANMANAGER", ARGS_EXTRACT msg);

#define LanManagerInfo(msg) LANMANAGER_LOG(RDK_LOG_INFO, msg)
#define LanManagerWarn(msg) LANMANAGER_LOG(RDK_LOG_WARN, msg)
#define LanManagerError(msg) LANMANAGER_LOG(RDK_LOG_ERROR, msg)
#define LanManagerDebug(msg) LANMANAGER_LOG(RDK_LOG_DEBUG, msg)
#define LanManagerFatal(msg) LANMANAGER_LOG(RDK_LOG_FATAL, msg)
#define LanManagerNotice(msg) LANMANAGER_LOG(RDK_LOG_NOTICE, msg)

bool LanManagerLogInit();
bool LanManagerLogDeinit();
