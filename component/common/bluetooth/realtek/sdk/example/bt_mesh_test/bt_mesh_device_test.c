#include "stdio.h"
#include <stdbool.h>
#include <osdep_service.h>
#include "bt_mesh_user_api.h"

#include "bt_mesh_malloc_free.h"
#include "bt_mesh_device_test.h"
#include "bt_mesh_app_user_cmd_parse.h"
#include "bt_mesh_timer_handler.h"
#include "bt_mesh_test_result.h"
#include "bt_mesh_receive_response.h"
#include "bt_mesh_datatrans_write_api.h"
#include "datatrans_app.h"

struct BT_MESH_TEST_PARAMETER Bt_Mesh_Test_Parameter;
uint16_t Current_TestItemIndex = 0;

static int judge_test_item_index (uint8_t * data_array);

void init_bt_mesh_test_parameter (void)
{
    Bt_Mesh_Test_Parameter.TestItemIndex  = 0;
    Bt_Mesh_Test_Parameter.DstAddr        = 0;
    Bt_Mesh_Test_Parameter.SendPacketNum  = 0;
    Bt_Mesh_Test_Parameter.PacketInterval = 0;
    Bt_Mesh_Test_Parameter.DelayTime      = 0;
}

#if defined(CONFIG_BT_MESH_TEST) && CONFIG_BT_MESH_TEST

int32_t cmd_data_receive (const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    datatrans_server_write_t * pdata = pargs;
    uint16_t dst = pdata->src;
    uint8_t * data_array = pdata->data;
    uint16_t data_len = pdata->data_len;

    static bool test_start_flag = false;
    int cmd_index = data_array[0];

    /* The integrity test for the received packet */
    if (data_len != data_array[1]) {
        printf("cmd_data_receive - Error Integrity Test");
        return 1;
    }

    switch(cmd_index) {
        case Configure_Test_Item_Request:
            config_test_item_request_post(data_array);
            send_config_test_item_response(dst);
            test_start_flag = true;
            break;
        case Start_Test:
			destroy_list();
            if (test_start_flag != true) {
                printf("The test is being started!\n");
                return 1;
            }

            start_test_post();
            test_start_flag = false;
            break;
        case Report_Test_Result_Request:
            report_test_result_request_post(dst, data_array);
            break;
        case Test_Data_Packet:
            // printf("I receive the packet, the length is %d\n", data_len);
            
            Current_TestItemIndex = judge_test_item_index(data_array);
            if(Current_TestItemIndex == 0){
                printf("Packet is from %x, Index is %d\r\n", dst, 0);
            }
            else{
                printf("Packet is from %x, Index is %d, Seq num is %d\r\n", dst, Current_TestItemIndex, (data_array[3] << 8) | data_array[4]);
            }

            if (data_array[2] == 0x1) { /* Need Reply */
               int response_cmd_length = 20;
               u8 response_cmd_array[20] = {Test_Data_Packet_Response, 20, 0};
               bt_mesh_cmd_datatrans_write_api(dst, response_cmd_array, response_cmd_length);
            }

            test_data_packet_post(dst, data_array);
            break;
        case Test_Data_Packet_Response:
            printf("DstAddr response succeed!\n");
            break;
        case Clear_Test_Result:
            printf("Clear test result succeed!\n");
            destroy_list();
            break;
        default:
            printf("Receive the error cmd\n");
            break;
    }

    return 0;
}
#endif

void config_test_item_request_post (uint8_t * data_array)
{
    Bt_Mesh_Test_Parameter.TestItemIndex  = data_array[2];
    Bt_Mesh_Test_Parameter.DstAddr        = (data_array[3] << 8) | data_array[4];
    Bt_Mesh_Test_Parameter.SendPacketNum  = (data_array[5] << 8) | data_array[6];
    Bt_Mesh_Test_Parameter.PacketInterval = (data_array[7] << 8) | data_array[8];
    Bt_Mesh_Test_Parameter.DelayTime      = (data_array[9] << 8) | data_array[10];

    printf("\n-----------Config Test Item Request--------------\n");
    printf("TestItemIndex is %d\n", Bt_Mesh_Test_Parameter.TestItemIndex);
    printf("DstAddr is %X\n", Bt_Mesh_Test_Parameter.DstAddr);
    printf("SendPacketNum is %d\n", Bt_Mesh_Test_Parameter.SendPacketNum);
    printf("PacketInterval is %d\n", Bt_Mesh_Test_Parameter.PacketInterval);
    printf("DelayTime is %d\n", Bt_Mesh_Test_Parameter.DelayTime);
    printf("--------------------------------------------------\n");
}

void send_config_test_item_response (uint16_t dst)
{
    uint8_t *cmd_array = NULL;
    int cmd_length = 3;
    cmd_array = (uint8_t *)bt_mesh_test_malloc(cmd_length);

    if (!cmd_array) {
        printf("send_config_test_item_response malloc fail!\r\n");
		return;
    }

    cmd_array[0] = Configure_Test_Item_Response;
    cmd_array[1] = cmd_length;
    cmd_array[2] = Bt_Mesh_Test_Parameter.TestItemIndex;

    bt_mesh_cmd_datatrans_write_api(dst, cmd_array, cmd_length);
    bt_mesh_test_free(cmd_array);
}

extern Bt_Mesh_Test_Node_Point Bt_Mesh_Test_Result;

void report_test_result_request_post (uint16_t dst, uint8_t * data_array)
{
    int Node_Num = link_list_length();
    uint16_t test_item_index = data_array[2];

    if (Node_Num == 0) {
        printf("This is an empty LinkList\n");
        return;
    }

    if (Current_TestItemIndex != test_item_index) {
        printf("You use the wrong test item index!!!\n");
        return;
    }

    if (test_item_index > 2) {
        printf("test_item_index is not allowed to larger than 2!!!\n");
        return;
    }

    uint8_t * cmd_array = NULL;
    int cmd_length = 1 + 1 + 1 + 4 * Node_Num;
    cmd_array = (uint8_t *)bt_mesh_test_malloc(cmd_length);

    if (!cmd_array) {
        printf("report_test_result_request_post malloc fail!\r\n");
		return;
    }

    cmd_array[0] = Report_Test_Result_Response;
    cmd_array[1] = cmd_length;
    cmd_array[2] = test_item_index;

    obtain_data_from_link_list(cmd_array, test_item_index);

    bt_mesh_cmd_datatrans_write_api(dst, cmd_array, cmd_length);
    bt_mesh_test_free(cmd_array);
}

plt_timer_t start_test_timer;
u8 *start_test_array = NULL;

void start_test_post ()
{
    printf("You are the DeviceSrc\n");

    uint16_t cmd_length = 0x3;

    if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x0) {
       cmd_length = 0x3;
    } else if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x1) {
       cmd_length = 0x42; /* No Reply */
    } else if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x2) {
        cmd_length = 0x42; /* Need Reply */
    }

    start_test_array = (uint8_t *)bt_mesh_test_malloc(cmd_length);

    if (!start_test_array) {
        printf("start_test_post malloc fail!\r\n");
		return;
    }

    start_test_array[0] = Test_Data_Packet;
    start_test_array[1] = cmd_length;
    start_test_array[2] = 0x0;

    if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x2) {
        start_test_array[2] = 0x1; /* Need Reply */
    }

	start_test_timer = plt_timer_create("start_test_timer_handler", ORIGINAL_DELAY_TIME + Bt_Mesh_Test_Parameter.DelayTime, true, 0, start_test_timer_handler);

    if (start_test_timer != NULL) {
        plt_timer_start(start_test_timer, 0);
    }
}

void test_data_packet_post (uint16_t dst, uint8_t * data_array)
{
    uint16_t test_item_index = 0;
    test_item_index = judge_test_item_index(data_array);

    insert_dst_link_list (dst, test_item_index);
#if 0
    print_link_list();
#endif
}

static int judge_test_item_index (uint8_t * data_array)
{
    uint16_t test_item_index = 0;

    if (data_array[1] == 0x3) {
        test_item_index = 0;
    } else if ((data_array[1] == 0x42) && (data_array[2] == 0x0)) {
        test_item_index = 1;
    } else if ((data_array[1] == 0x42) && (data_array[2] == 0x1)) {
        test_item_index = 2;
    }

    return test_item_index;
}