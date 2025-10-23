#include "lanmgr_communication_apis.h"
#include "lanmgr_log.h"
#include "lan_manager_bridge.h"

rbusHandle_t rbus_handle;

int lanManagerBusInit(void)
{
    rbusError_t rc = rbus_open(&rbus_handle, "lan_manager");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        LanManagerError(("lanManagerBusInit: rbus_open failed: %d\n", rc));
        rbus_handle = NULL;
        return -1;
    }
    LanManagerInfo(("lanManagerBusInit: rbus_open succeeded, rbus_handle=%p\n", rbus_handle));
    return 0;
}

void lanManagerBusClose(void)
{
    if (rbus_handle)
    {
        rbus_close(rbus_handle);
        rbus_handle = NULL;
        LanManagerInfo(("lanManagerBusClose: rbus closed\n"));
    }
    else
    {
        LanManagerError(("lanManagerBusClose: rbus_handle is already NULL\n"));
    }
}

/**
 * @brief RBUS event subscription handler for Device.DHCP.Server.StateReady
 * 
 * This handler is called when the DHCP server state changes to ready.
 * It can be used to trigger bridge configuration updates or other
 * initialization tasks that depend on DHCP server availability.
 */
static void dhcpServerStateReadyHandler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;
    
    if(!event)
    {
        LanManagerError(("dhcpServerStateReadyHandler: NULL event received\n"));
        return;
    }
    
    LanManagerInfo(("dhcpServerStateReadyHandler: Received DHCP server state ready event\n"));
    LanManagerInfo(("  Event name: %s\n", event->name ? event->name : "NULL"));
    LanManagerInfo(("  Event type: %d\n", event->type));
    
    if(event->data)
    {
        rbusValue_t value = rbusObject_GetValue(event->data, "value");
        if(value)
        {
            rbusValueType_t valueType = rbusValue_GetType(value);
            LanManagerInfo(("  Event value type: %d\n", valueType));
            
            if(valueType == RBUS_BOOLEAN)
            {
                bool stateReady = rbusValue_GetBoolean(value);
                LanManagerInfo(("  DHCP Server StateReady: %s\n", stateReady ? "true" : "false"));
                
                if(stateReady)
                {
                    LanManagerInfo(("DHCP server is now ready - triggering bridge configuration update\n"));
                    // place holder to send dhcp data
                }
                else
                {
                    LanManagerInfo(("DHCP server is not ready\n"));
                }
            }
            else
            {
                LanManagerError(("dhcpServerStateReadyHandler: Unexpected value type %d for StateReady\n", valueType));
            }
        }
        else
        {
            LanManagerError(("dhcpServerStateReadyHandler: Could not extract value from event data\n"));
        }
    }
    else
    {
        LanManagerError(("dhcpServerStateReadyHandler: NULL event data\n"));
    }
}

/**
 * @brief Subscribe to Device.DHCP.Server.StateReady events
 * 
 * @return int 0 on success, -1 on failure
 */
int subscribeToDhcpServerStateReady(void)
{
    if(!rbus_handle)
    {
        LanManagerError(("subscribeToDhcpServerStateReady: rbus_handle is NULL\n"));
        return -1;
    }
    
    rbusError_t rc = rbusEvent_Subscribe(
        rbus_handle,
        "Device.DHCP.Server.StateReady",
        dhcpServerStateReadyHandler,
        NULL,
        0
    );
    
    if(rc != RBUS_ERROR_SUCCESS)
    {
        LanManagerError(("subscribeToDhcpServerStateReady: rbusEvent_Subscribe failed: %d\n", rc));
        return -1;
    }
    
    LanManagerInfo(("subscribeToDhcpServerStateReady: Successfully subscribed to Device.DHCP.Server.StateReady\n"));
    return 0;
}

/**
 * @brief Unsubscribe from Device.DHCP.Server.StateReady events
 * 
 * @return int 0 on success, -1 on failure
 */
int unsubscribeFromDhcpServerStateReady(void)
{
    if(!rbus_handle)
    {
        LanManagerError(("unsubscribeFromDhcpServerStateReady: rbus_handle is NULL\n"));
        return -1;
    }
    
    rbusError_t rc = rbusEvent_Unsubscribe(rbus_handle, "Device.DHCP.Server.StateReady");
    
    if(rc != RBUS_ERROR_SUCCESS)
    {
        LanManagerError(("unsubscribeFromDhcpServerStateReady: rbusEvent_Unsubscribe failed: %d\n", rc));
        return -1;
    }
    
    LanManagerInfo(("unsubscribeFromDhcpServerStateReady: Successfully unsubscribed from Device.DHCP.Server.StateReady\n"));
    return 0;
}
