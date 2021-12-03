#include "osdep_service.h"
#include "bt_mesh_malloc_free.h"
#include "bt_mesh_timer_handler.h"
#include "bt_mesh_datatrans_write_api.h"
#include "bt_mesh_device_test.h"

extern struct BT_MESH_TEST_PARAMETER Bt_Mesh_Test_Parameter;

extern plt_timer_t start_cmd_timer;
extern u8 *start_cmd_array;

void start_cmd_timer_handler(void *FunctionContext)
{
    static u32 test_count = 0;
    int cmd_length = 2;

    if (test_count < CMD_SEND_NUM) {
        bt_mesh_cmd_datatrans_write_api(0xffff, start_cmd_array, cmd_length);
        test_count++;
    } else {
		test_count = 0;
		plt_timer_stop(start_cmd_timer, 0);
        bt_mesh_test_free(start_cmd_array);
        start_cmd_array = NULL;
    }
}

extern plt_timer_t start_test_timer;
extern u8 *start_test_array;

void start_test_timer_handler(void *FunctionContext)
{
    uint16_t cmd_length = 0x3;
    static u32 test_count = 0;

    if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x0) {
        cmd_length = 0x3;
    } else if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x1) {
        cmd_length = 0x42; /* No Reply */
    } else if (Bt_Mesh_Test_Parameter.TestItemIndex == 0x2) {
        cmd_length = 0x42; /* Need Reply */
    }

    if (test_count < Bt_Mesh_Test_Parameter.SendPacketNum) {
        test_count++;
        printf("Current test_count is %d\n", test_count);
    if(cmd_length >= 5){
		start_test_array[3] = (test_count >> 8) & 0xFF;
		start_test_array[4] = test_count        & 0xFF;
    }
        bt_mesh_cmd_datatrans_write_api(Bt_Mesh_Test_Parameter.DstAddr, start_test_array, cmd_length);
		plt_timer_change_period(start_test_timer, Bt_Mesh_Test_Parameter.PacketInterval, 0);
    } else {
        test_count = 0;
		plt_timer_stop(start_test_timer, 0);
        printf("\n--------------Send Finished-----------\n");
        bt_mesh_test_free(start_test_array);
		start_test_array = NULL;
    }
}