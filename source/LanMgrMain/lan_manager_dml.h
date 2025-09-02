#ifndef _LAN_MANAGER_DML_H_
#define _LAN_MANAGER_DML_H_

#include <stdint.h>

#define MAX_TABLE_ROWS 16

typedef enum
{
    TableRowIDType_InstNum,
    TableRowIDType_Alias,
    TableRowIDType_Unknown
} TableRowIDType;

typedef struct
{
    TableRowIDType type;
    uint32_t instNum;
    char* alias;
} TableRowID;

typedef struct DataModel
{
    LanConfig lanConfigs[MAX_TABLE_ROWS];
    uint32_t lanConfigInstNum;
} DataModel;

extern DataModel gDM;
extern int g_count;

int lan_manager_register_dml();
void lan_manager_unregister_dml();

#endif // _LAN_MANAGER_DML_H_
