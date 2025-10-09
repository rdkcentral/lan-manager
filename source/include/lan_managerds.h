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
#ifndef LMDATASTORE_H
#define LMDATASTORE_H

#include "lan_manager_interface.h"

#define BASE_MAX_BRIDGES   2 //20

typedef enum {
LM_SUCCESS,
LM_FAILURE
}LM_Status;

LM_Status LanConfigDataStoreInit();
void LanConfigDataStoreCleanup();
LM_Status LanConfigDataStoreAdd(const LanConfig *LanInfo);
LM_Status LanConfigDataStoreGet(const char *key,LanConfig *result);
LM_Status LanConfigDataStoreRemove(const LanConfig *LanInfo);
void LanConfigDataStoreDump();
LM_Status LanConfigDataStoreGetAll(int *numEntries, LanConfig **fullTbl);

#endif

