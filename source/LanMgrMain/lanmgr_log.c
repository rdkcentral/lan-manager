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
#include <unistd.h>
#include <stdlib.h>
#include "lanmgr_log.h"


/**
 * @brief LanManagerLogInit Initialize RDK Logger
 */
bool LanManagerLogInit()
{
#ifdef FEATURE_SUPPORT_RDKLOG
    if (rdk_logger_init(DEBUG_INI_NAME) != RDK_SUCCESS)
    {
        return false;
    }
#endif

    return true;
}

bool LanManagerLogDeinit()
{
#ifdef FEATURE_SUPPORT_RDKLOG
    return (rdk_logger_deinit() == RDK_SUCCESS ? true : false);
#endif
}

