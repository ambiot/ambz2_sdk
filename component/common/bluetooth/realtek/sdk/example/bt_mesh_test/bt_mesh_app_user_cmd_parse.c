#include "stdio.h"
#include "osdep_service.h"
#include "bt_mesh_user_api.h"

#include "bt_mesh_malloc_free.h"
#include "bt_mesh_provisioner_api.h"
#include "bt_mesh_app_user_cmd_parse.h"
#include "bt_mesh_timer_handler.h"
#include "bt_mesh_datatrans_write_api.h"

void config_cmd_parsing (char ** argv)
{
    uint16_t SrcAddr        = 0;
    uint16_t DstAddr        = 0;
    uint16_t TestItemIndex  = atoi(argv[3]);
    uint16_t SendPacketNum  = atoi(argv[4]);
    uint16_t PacketInterval = atoi(argv[5]);
    uint16_t DelayTime      = atoi(argv[6]);

    SrcAddr = user_cmd_string2uint32(argv[1]);
    DstAddr = user_cmd_string2uint32(argv[2]);

    u8 *cmd_array = NULL;
    int cmd_length = 0xB;
    cmd_array = (u8 *)bt_mesh_test_malloc(cmd_length);

    if (!cmd_array) {
        printf("config_cmd_parsing malloc fail!\r\n");
		return;
    }

    cmd_array[0]  = Configure_Test_Item_Request;
    cmd_array[1]  = cmd_length;
    cmd_array[2]  = TestItemIndex;
    cmd_array[3]  = (DstAddr >> 8) & 0xFF;
    cmd_array[4]  = DstAddr & 0xFF;
    cmd_array[5]  = (SendPacketNum >> 8) & 0xFF;
    cmd_array[6]  = SendPacketNum & 0xFF;
    cmd_array[7]  = (PacketInterval >> 8) & 0xFF;
    cmd_array[8]  = PacketInterval & 0xFF;
    cmd_array[9]  = (DelayTime >> 8) & 0xFF;
    cmd_array[10] = DelayTime & 0xFF;

    bt_mesh_cmd_datatrans_write_api(SrcAddr, cmd_array, cmd_length);
    bt_mesh_test_free(cmd_array);
}

plt_timer_t start_cmd_timer;
u8 *start_cmd_array = NULL;

void start_cmd_parsing (char ** argv)
{
    int cmd_length = 2;

    start_cmd_array = (u8 *)bt_mesh_test_malloc(cmd_length);

    if (!start_cmd_array) {
        printf("start_cmd_parsing malloc fail!\r\n");
		return;
    }

    start_cmd_array[0] = Start_Test;
    start_cmd_array[1] = cmd_length;

    start_cmd_timer = plt_timer_create("start_cmd_timer_timer", CMD_SEND_INTERVAL, true, 0, start_cmd_timer_handler);

    if (start_cmd_timer != NULL) {
        plt_timer_start(start_cmd_timer, 0);
    }
}

void report_cmd_parsing (char ** argv)
{
    u8 *cmd_array = NULL;
    int cmd_length = 3;
    uint16_t TestItemIndex = atoi(argv[1]);

    cmd_array = (u8 *)bt_mesh_test_malloc(cmd_length);

    if (!cmd_array) {
        printf("report_cmd_parsing malloc fail!\r\n");
		return;
    }

    cmd_array[0] = Report_Test_Result_Request;
    cmd_array[1] = cmd_length;
    cmd_array[2] = TestItemIndex;

    bt_mesh_cmd_datatrans_write_api(0xffff, cmd_array, cmd_length);
    bt_mesh_test_free(cmd_array);
}