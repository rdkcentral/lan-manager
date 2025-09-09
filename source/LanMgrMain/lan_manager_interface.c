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


#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <rbus.h>
#include "lan_manager_interface.h"

bool GetLanConfigFromProvider(LanConfig *configs, int *count)
{
    rbusHandle_t handle;
    rbusError_t rc;
    rbusObject_t inParams = NULL, outParams = NULL;

    rc = rbus_open(&handle, "LanConfigConsumer");
    if(rc != RBUS_ERROR_SUCCESS)
        return false;

    rc = rbusMethod_Invoke(handle, "Device.LanManager.LanConfigCopy", inParams, &outParams);
    if(rc == RBUS_ERROR_SUCCESS && outParams)
    {
        rbusValue_t value = rbusObject_GetValue(outParams, "value");
        if(value)
        {
            int len = 0;
            const uint8_t* bytes = rbusValue_GetBytes(value, &len);
            if(len >= sizeof(int))
            {
                int n = 0;
                memcpy(&n, bytes, sizeof(int));
                if(len == sizeof(int) + n * sizeof(LanConfig))
                {
                    if(configs && count)
                    {
                        memcpy(configs, bytes + sizeof(int), n * sizeof(LanConfig));
                        *count = n;
                        rbusObject_Release(outParams);
                        rbus_close(handle);
                        return true;
                    }
                }
            }
        }
        rbusObject_Release(outParams);
    }
    rbus_close(handle);
    return false;
}