#ifndef LANMGR_COMMUNICATION_APIS_H
#define LANMGR_COMMUNICATION_APIS_H

#include "rbus.h"

extern rbusHandle_t rbus_handle;

int lanManagerBusInit(void);
void lanManagerBusClose(void);

#endif // LANMGR_COMMUNICATION_APIS_H
