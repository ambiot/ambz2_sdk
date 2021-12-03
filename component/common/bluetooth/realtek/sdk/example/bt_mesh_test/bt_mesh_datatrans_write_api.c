#include "osdep_service.h"
#include "bt_mesh_user_api.h"

#if (defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE || \
     defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE)
#include "bt_mesh_device_api.h"
#endif

#if (defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER || \
     defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE)
#include "bt_mesh_provisioner_api.h"
#endif

#include "bt_mesh_datatrans_write_api.h"
#include "datatrans_model.h"
#include "mesh_node.h"

#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST

void bt_mesh_cmd_datatrans_write_api(uint16_t mesh_addr, uint8_t * data_array, uint16_t data_len)
{
    PUSER_ITEM puserItem = NULL;
    /* _datatrans_write */
    puserItem = bt_mesh_alloc_hdl(USER_API_ASYNCH);

    if (!puserItem) {
        printf("bt_mesh_cmd_datatrans_write_api malloc fail!\r\n");
		return;
    }

    puserItem->pparseValue->dw_parameter[0] = mesh_addr;

    for (uint16_t i = 0; i < data_len; i++) {
        puserItem->pparseValue->dw_parameter[i + 1] = data_array[i];
    }

    puserItem->pparseValue->dw_parameter[data_len + 1] = 0;
    puserItem->pparseValue->dw_parameter[data_len + 2] = 0;
    puserItem->pparseValue->para_count = data_len + 3;
    /* Packet sending API defined by Datatrans Model */
    bt_mesh_set_user_cmd(GEN_MESH_CODE(_datatrans_write), puserItem->pparseValue, NULL, puserItem);
}
#endif