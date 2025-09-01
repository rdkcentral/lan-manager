#include "lanmgr_communication_apis.h"
#include "lanmgr_log.h"

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
