#ifndef LANMGR_COMMUNICATION_APIS_H
#define LANMGR_COMMUNICATION_APIS_H

#include "rbus.h"

extern rbusHandle_t rbus_handle;

int lanManagerBusInit(void);
void lanManagerBusClose(void);

/**
 * @brief Subscribe to Device.DHCP.Server.StateReady events
 * 
 * Registers an RBUS event subscription to monitor when the DHCP server
 * state changes to ready. This can be used to trigger bridge configuration
 * updates or other initialization tasks that depend on DHCP server availability.
 * 
 * @return int 0 on success, -1 on failure
 */
int subscribeToDhcpServerStateReady(void);

/**
 * @brief Unsubscribe from Device.DHCP.Server.StateReady events
 * 
 * Removes the RBUS event subscription for Device.DHCP.Server.StateReady.
 * Should be called during cleanup to properly release resources.
 * 
 * @return int 0 on success, -1 on failure
 */
int unsubscribeFromDhcpServerStateReady(void);

#endif // LANMGR_COMMUNICATION_APIS_H
