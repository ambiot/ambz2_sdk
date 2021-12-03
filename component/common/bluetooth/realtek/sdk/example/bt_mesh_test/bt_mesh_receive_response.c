#include <osdep_service.h>
#include "bt_mesh_user_api.h"

#include "bt_mesh_app_user_cmd_parse.h"
#include "bt_mesh_timer_handler.h"
#include "bt_mesh_datatrans_write_api.h"
#include "bt_mesh_receive_response.h"
#include "bt_mesh_device_test.h"

#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST

int32_t common_receive_response(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    datatrans_server_write_t * pdata = pargs;
    uint16_t source_address = pdata->src;
    uint8_t * data_array = pdata->data;
    uint16_t data_length = pdata->data_len;
  
    int cmd_index = data_array[0];

    /* The integrity test for the received packet */
    if (data_length != data_array[1]) {
        printf("common_receive_response - Error Integrity Test\n");
        return 1;
    }

    switch(cmd_index) {
        case Configure_Test_Item_Response:
            printf("\n---------------------------------\n");
            printf("Node %X received the configure item\n", source_address);
            printf("Configure Test Item Response\n");
            printf("---------------------------------\n");
            break;
        case Report_Test_Result_Response:
            printf("\nReport Test Result Response\n");
            printf("These data is from %X\n", source_address);
            printf("TestItemIndex is %X\n", data_array[2]);
            printf("---------------------------------\n");
            int Node_Num = (data_length - 3) / 4;

            for(int i = 0; i < Node_Num; i++) {
                int Source_Address = 0;
                int Packet_Num = 0;

                Source_Address = (data_array[3 + 4 * i + 0] << 8) | \
                                  data_array[3 + 4 * i + 1];
                Packet_Num     = (data_array[3 + 4 * i + 2] << 8) | \
                                  data_array[3 + 4 * i + 3];

                printf("Source_Address is %X\n", Source_Address);
                printf("Packet_Num is %d\n", Packet_Num);
                printf("---------------------------------\n");
            }

            int response_cmd_length = 2;
            u8 response_cmd_array[2] = {Clear_Test_Result, 2};
            bt_mesh_cmd_datatrans_write_api(source_address, response_cmd_array, response_cmd_length);
            break;
    }

    return 0;
}
#endif