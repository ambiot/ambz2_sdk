#include "stdio.h"
#include <osdep_service.h>
#include "bt_mesh_user_api.h"

#include "bt_mesh_malloc_free.h"
#include "bt_mesh_app_user_cmd_parse.h"
#include "bt_mesh_device_test.h"
#include "bt_mesh_timer_handler.h"
#include "bt_mesh_test_result.h"

Bt_Mesh_Test_Node_Point Bt_Mesh_Test_Result = NULL;

static Bt_Mesh_Test_Node_Point find_node_link_list (Bt_Mesh_Test_Node_Point L, int x);

_lock BT_MEST_TEST_LOCK;

void insert_dst_link_list (uint16_t insert_num, uint16_t test_item_index)
{
    rtw_enter_critical_bh(&BT_MEST_TEST_LOCK, NULL);

    Bt_Mesh_Test_Node_Point q = NULL, p = NULL;
    p = find_node_link_list(Bt_Mesh_Test_Result, insert_num);

    if (p == NULL) {
        q = (Bt_Mesh_Test_Node_Point) bt_mesh_test_malloc (sizeof(Bt_Mesh_Test_Node));

        if (!q) {
            printf("insert_dst_link_list malloc fail!\r\n");
            rtw_exit_critical_bh(&BT_MEST_TEST_LOCK, NULL);
            return;
        }

        rtw_memset(q, 0, sizeof(Bt_Mesh_Test_Node));

        q->SrcAddr = insert_num;

        switch (test_item_index) {
            case 0: q->Test0_PacketNum = 1; break;
            case 1: q->Test1_PacketNum = 1; break;
            case 2: q->Test2_PacketNum = 1; break;
        }

        q->next = Bt_Mesh_Test_Result->next;
        Bt_Mesh_Test_Result->next = q;
    } else {
        switch (test_item_index) {
            case 0: p->Test0_PacketNum++; break;
            case 1: p->Test1_PacketNum++; break;
            case 2: p->Test2_PacketNum++; break;
        }
    }

    rtw_exit_critical_bh(&BT_MEST_TEST_LOCK, NULL);
}

Bt_Mesh_Test_Node_Point find_node_link_list (Bt_Mesh_Test_Node_Point L, int x)
{
    Bt_Mesh_Test_Node_Point p;
    p = L;

    while (p->next && p->next->SrcAddr != x) {
        p = p->next;
    }

    if (p->next) {
        return p->next;
    } else {
        return NULL;
    }
}

void destroy_list (void)
{
    rtw_enter_critical_bh(&BT_MEST_TEST_LOCK, NULL);

    Bt_Mesh_Test_Node_Point p = Bt_Mesh_Test_Result->next;
    Bt_Mesh_Test_Node_Point q = Bt_Mesh_Test_Result->next;

    while (q != NULL) {
        q = p->next;
        Bt_Mesh_Test_Result->next = q;
        printf("Destroy the node, SrcAddr in this node is %X\n", p->SrcAddr);
        bt_mesh_test_free(p);
        p = q;
    }

    rtw_exit_critical_bh(&BT_MEST_TEST_LOCK, NULL);
}

void print_link_list (void)
{
    rtw_enter_critical_bh(&BT_MEST_TEST_LOCK, NULL);

    Bt_Mesh_Test_Node_Point p;
    p = Bt_Mesh_Test_Result->next;

    while (p) {
        printf ("SrcAddr is %X\n", p->SrcAddr);
        printf ("Test0_PacketNum is %d\n", p->Test0_PacketNum);
        printf ("Test1_PacketNum is %d\n", p->Test1_PacketNum);
        printf ("Test2_PacketNum is %d\n", p->Test2_PacketNum);
        printf ("------------------\n" );
        p = p->next;
    }

    rtw_exit_critical_bh(&BT_MEST_TEST_LOCK, NULL);
}

int link_list_length (void)
{
    rtw_enter_critical_bh(&BT_MEST_TEST_LOCK, NULL);

    int num = 0;
    Bt_Mesh_Test_Node_Point p;
    p = Bt_Mesh_Test_Result->next;

    while (p) {
        num++;
        p = p->next;
    }

    rtw_exit_critical_bh(&BT_MEST_TEST_LOCK, NULL);

    return num;
}

void link_list_init ()
{
    rtw_spinlock_init(&BT_MEST_TEST_LOCK);

    Bt_Mesh_Test_Result = (Bt_Mesh_Test_Node_Point) bt_mesh_test_malloc (sizeof(Bt_Mesh_Test_Node));

    if (!Bt_Mesh_Test_Result) {
        printf("link_list_init malloc fail!\r\n");
        return;
    }

    rtw_memset(Bt_Mesh_Test_Result, 0, sizeof(Bt_Mesh_Test_Node));
    Bt_Mesh_Test_Result->next = NULL;
    Bt_Mesh_Test_Result->SrcAddr = 0;
}

void obtain_data_from_link_list (uint8_t * cmd_array, uint16_t test_item_index)
{
    rtw_enter_critical_bh(&BT_MEST_TEST_LOCK, NULL);

    Bt_Mesh_Test_Node_Point p;
    p = Bt_Mesh_Test_Result->next;

    int Node_Index = 0;

    while (p) {
        int Source_Address = p->SrcAddr;
        int Packet_Num = 0;

        switch (test_item_index) {
            case 0: Packet_Num = p->Test0_PacketNum; break;
            case 1: Packet_Num = p->Test1_PacketNum; break;
            case 2: Packet_Num = p->Test2_PacketNum; break;
        }

        cmd_array[3 + 4 * Node_Index + 0] = (Source_Address >> 8) & 0xFF;
        cmd_array[3 + 4 * Node_Index + 1] = Source_Address & 0xFF;
        cmd_array[3 + 4 * Node_Index + 2] = (Packet_Num >> 8) & 0xFF;
        cmd_array[3 + 4 * Node_Index + 3] = Packet_Num & 0xFF;
        p = p->next;
        Node_Index++;
    }

    rtw_exit_critical_bh(&BT_MEST_TEST_LOCK, NULL);
}