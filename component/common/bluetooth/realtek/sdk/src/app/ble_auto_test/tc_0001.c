
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "user_cmd.h"
#include "user_cmd_parse.h"
#include <os_mem.h>
#include <os_timer.h>
#include <ble_auto_test_case.h>



#if F_BT_LE_GAP_PERIPHERAL_SUPPORT
#include "gap_adv.h"
#endif

#include "gap.h"


#include <tc_0001.h>
#include <tc_common.h>


#if TC_0001_SUPPORT
#include "ameba_soc.h"


#define ADV_TEST_PARAM_COUNT        8

T_ADV_TEST_PARAM g_adv_test_param[ADV_TEST_PARAM_COUNT];
uint8_t g_adv_cur_test_index = 0;
uint8_t g_adv_total_test_index = ADV_TEST_PARAM_COUNT;



// GAP - SCAN RSP data (max size = 31 bytes)
const uint8_t scan_rsp_data_for_adv_test[] =
{
    0x03,           /* length     */
    0x03,           /* type="More 16-bit UUIDs available" */
    0x12,
    0x18,
    /* place holder for Local Name, filled by BT stack. if not present */
    /* BT stack appends Local Name.                                    */
    0x03,           /* length     */
    0x19,           /* type="Appearance" */
    0xc2, 0x03,     /* Mouse */
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
const uint8_t adv_data_for_adv_test[] =
{
    /* Core spec. Vol. 3, Part C, Chapter 18 */
    /* Flags */
    0x02,            /* length     */
    //XXXXMJMJ 0x01, 0x06,      /* type="flags", data="bit 1: LE General Discoverable Mode", BR/EDR not supp. */
    0x01, 0x05,      /* type="flags", data="bit 1: LE General Discoverable Mode" */
    /* Service */
    0x03,           /* length     */
    0x03,           /* type="More 16-bit UUIDs available" */
    0x12,
    0x18,
    /* place holder for Local Name, filled by BT stack. if not present */
    /* BT stack appends Local Name.                                    */
    0x03,           /* length     */
    0x19,           /* type="Appearance" */
    0xc2, 0x03,     /* Mouse */
    0x0C,           /* length     */
    0x09,           /* type="Complete local name" */
//    0x42, 0x65, 0x65, 0x5F, 0x6D, 0x6F, 0x75, 0x73, 0x65  /* Bee_perip */
    'B', 'e', 'e', '_', 'G', 'a', 'p', 'T', 'e', 's', 't' /* Bee_perip */
};


void tc_0001_adv_start(uint16_t advIntMin, uint16_t advIntMax)
{
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;

    g_adv_cur_test_index = 0;
    g_adv_total_test_index = ADV_TEST_PARAM_COUNT;

    for (uint8_t i = 0; i < g_adv_total_test_index; i++)
    {
        g_adv_test_param[i].adv_interval_min = advIntMin;
        g_adv_test_param[i].adv_interval_max = advIntMax;
    }
    if (NULL == g_test_timer_handle)
    {
        os_timer_create(&g_test_timer_handle, "testTimer", 1, 1000, false, tc_0001_adv_timeout_handler);
    }
    //start total timer
    os_timer_start(&g_test_timer_handle);
    //reset_vendor_counter();
    g_test_begin_time = RTIM_GetCount(TIMM00);

    g_adv_test_param[g_adv_cur_test_index].start_adv_begin_time = RTIM_GetCount(TIMM00);

    APP_PRINT_INFO1("cmd_adv begin_time = %d",
                    g_adv_test_param[g_adv_cur_test_index].start_adv_begin_time);


    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data_for_adv_test),
                     (void *)adv_data_for_adv_test);
    le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data_for_adv_test),
                     (void *)scan_rsp_data_for_adv_test);
    le_adv_start();

}

static bool flag2 = false;
void tc_0001_adv_timeout_handler(void *pxTimer)
{
    g_test_end_time = RTIM_GetCount(TIMM00);

    APP_PRINT_INFO3("tc_0001_adv_timeout_handler begin time=%d, end time=%d, time elapsed = %dus",
                    g_test_begin_time, g_test_end_time,
                    amebad_time_get_elapsed(g_test_begin_time, g_test_end_time));
    if (false == flag2)
    {
        data_uart_print("begin time,          end time,            elapase time(us)\r\n");
        flag2 = true;
    }

    data_uart_print(
        "  %d,                %d,                  %d\r\n",
        g_test_begin_time,
        g_test_end_time,
        amebad_time_get_elapsed(g_test_begin_time, g_test_end_time));
    flag2 = false;
}
bool flag1 = false;
void tc_0001_adv_adv_state_change_to_idle(void)
{

    g_adv_test_param[g_adv_cur_test_index].stop_adv_end_time = RTIM_GetCount(TIMM00);
    /*
                APP_PRINT_INFO3(" ble stop adv begin_time =%d, end_time =%d, elapsed time = %d",
                g_adv_test_param[g_adv_cur_test_index].stop_adv_begin_time,
                g_adv_test_param[g_adv_cur_test_index].stop_adv_end_time,
                (g_adv_test_param[g_adv_cur_test_index].stop_adv_end_time - g_adv_test_param[g_adv_cur_test_index].stop_adv_begin_time));
    */

    g_adv_cur_test_index++;
    if (g_adv_cur_test_index < g_adv_total_test_index)
    {

        g_adv_test_param[g_adv_cur_test_index].start_adv_begin_time = RTIM_GetCount(TIMM00);


        APP_PRINT_INFO2(" ble start adv index = %d, begin_time=%d", g_adv_cur_test_index,
                        g_adv_test_param[g_adv_cur_test_index].start_adv_begin_time);

        uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
        uint8_t  advChanMap = GAP_ADVCHAN_ALL;
        uint8_t  advDirectType = GAP_REMOTE_ADDR_LE_PUBLIC;
        uint8_t  advDirectAddr[GAP_BD_ADDR_LEN] = {0};
        uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;
        uint16_t advIntMin = (uint16_t)0x100;
        uint16_t advIntMax = (uint16_t)0x200;

        //advertising parameters
        le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR_TYPE, sizeof(advDirectType), &advDirectType);
        le_adv_set_param(GAP_PARAM_ADV_DIRECT_ADDR, sizeof(advDirectAddr), advDirectAddr);
        le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
        le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);

        le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
        le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

        le_adv_set_param(GAP_PARAM_ADV_DATA, sizeof(adv_data_for_adv_test),
                         (void *)adv_data_for_adv_test);
        le_adv_set_param(GAP_PARAM_SCAN_RSP_DATA, sizeof(scan_rsp_data_for_adv_test),
                         (void *)scan_rsp_data_for_adv_test);

        le_adv_start();
    }
    else
    {
        for (uint8_t i = 0; i < g_adv_total_test_index; i++)
        {
            if (false == flag1)
            {
                data_uart_print(" adv_interval_min , adv_interval_max ,  start adv elapase time(us)\r\n");
                flag1 = true;
            }
            data_uart_print("  %d,                   %d,                      %d\r\n",
                            g_adv_test_param[i].adv_interval_min,
                            g_adv_test_param[i].adv_interval_max,
                            amebad_time_get_elapsed(g_adv_test_param[i].start_adv_begin_time,
                                                    g_adv_test_param[i].start_adv_end_time)
                           );
        }
        flag1 = false;
        for (uint8_t i = 0; i < g_adv_total_test_index; i++)
        {
            if (false == flag1)
            {
                data_uart_print(" adv_interval_min , adv_interval_max ,  stop adv elapase time(us)\r\n");
                flag1 = true;
            }
            data_uart_print("  %d,                   %d,                      %d\r\n",
                            g_adv_test_param[i].adv_interval_min,
                            g_adv_test_param[i].adv_interval_max,
                            (g_adv_test_param[i].stop_adv_end_time - g_adv_test_param[i].stop_adv_begin_time) / TEST_CPU_CLOCK);


        }
        flag1 = false;
        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0001_ADV_PERFORMANCE, 0, NULL);
        }


    }

}

void tc_0001_adv_adv_state_change_to_advertising(void)
{
    //APP_PRINT_INFO0("GAP adv start");


    g_adv_test_param[g_adv_cur_test_index].start_adv_end_time = RTIM_GetCount(TIMM00);
    /*
                APP_PRINT_INFO3(" ble adv g_start_adv_begin_time =%d, end_time =%d, elapsed time = %d",
                g_adv_test_param[g_adv_cur_test_index].start_adv_begin_time,
                g_adv_test_param[g_adv_cur_test_index].start_adv_end_time,
                (g_adv_test_param[g_adv_cur_test_index].start_adv_end_time - g_adv_test_param[g_adv_cur_test_index].start_adv_begin_time));

    */
    g_adv_test_param[g_adv_cur_test_index].stop_adv_begin_time = RTIM_GetCount(TIMM00);
    APP_PRINT_INFO2(" ble stop adv index = %d, begin_time=%d", g_adv_cur_test_index,
                    g_adv_test_param[g_adv_cur_test_index].stop_adv_begin_time);
    le_adv_stop();

}



void *g_tc_adv_enable_disable_timer_handle = NULL;
void *g_tc_adv_enable_disable_monitor_timer_handle = NULL;

uint32_t g_tc_adv_enable_disable_begin_time = 0;
uint32_t g_tc_adv_enable_disable_max_start_adv_ellapsed_time = 0;
uint32_t g_tc_adv_enable_disable_max_stop_adv_ellapsed_time = 0;
uint32_t g_tc_adv_enable_disable_max_count = 65535;
uint32_t g_tc_adv_enable_disable_cur_count = 0;

void tc_adv_start_stop_TimeoutHandler(void *pxTimer)
{
    data_uart_print(
        "tc_adv_start_stop_TimeoutHandler max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
        g_tc_adv_enable_disable_max_start_adv_ellapsed_time,
        g_tc_adv_enable_disable_max_stop_adv_ellapsed_time,
        g_tc_adv_enable_disable_cur_count);
    APP_PRINT_ERROR3("tc_adv_start_stop_TimeoutHandler max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
                     g_tc_adv_enable_disable_max_start_adv_ellapsed_time,
                     g_tc_adv_enable_disable_max_stop_adv_ellapsed_time,
                     g_tc_adv_enable_disable_cur_count);

}

void tc_adv_start_stop_monitor_TimeoutHandler(void *pxTimer)
{
    /*
        data_uart_print(
            "tc_adv_start_stop_monitor_TimeoutHandler 10s max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
            g_tc_adv_enable_disable_max_start_adv_ellapsed_time,
            g_tc_adv_enable_disable_max_stop_adv_ellapsed_time,
            g_tc_adv_enable_disable_cur_count);
        APP_PRINT_INFO3( "tc_adv_start_stop_monitor_TimeoutHandler 10s max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
            g_tc_adv_enable_disable_max_start_adv_ellapsed_time,
            g_tc_adv_enable_disable_max_stop_adv_ellapsed_time,
            g_tc_adv_enable_disable_cur_count);
    */
}

void tc_0002_adv_start_stop_start(uint16_t advIntMin, uint16_t advIntMax, uint32_t max_count)
{
    uint8_t  advEventType = GAP_ADTYPE_ADV_IND;
    uint8_t  advChanMap = GAP_ADVCHAN_ALL;
    uint8_t  advFilterPolicy = GAP_ADV_FILTER_ANY;


    g_tc_adv_enable_disable_max_count = max_count;
    g_tc_adv_enable_disable_cur_count = 0;


    if (NULL == g_tc_adv_enable_disable_timer_handle)
    {
        os_timer_create(&g_tc_adv_enable_disable_timer_handle, "g_tc_adv_enable_disable_timer_handle", 1,
                        1000, false, tc_adv_start_stop_TimeoutHandler);
    }

    if (NULL == g_tc_adv_enable_disable_monitor_timer_handle)
    {
        os_timer_create(&g_tc_adv_enable_disable_monitor_timer_handle, "adv_monitor_timer", 1, 10000, false,
                        tc_adv_start_stop_monitor_TimeoutHandler);
    }

    //start total timer
    os_timer_start(&g_tc_adv_enable_disable_timer_handle);
    os_timer_start(&g_tc_adv_enable_disable_monitor_timer_handle);
    //reset_vendor_counter();
    g_tc_adv_enable_disable_begin_time = RTIM_GetCount(TIMM00);

    APP_PRINT_INFO1("app_handle_tc_adv_enable_disable_start begin_time = %d",
                    g_tc_adv_enable_disable_begin_time);

    le_adv_set_param(GAP_PARAM_ADV_EVENT_TYPE, sizeof(advEventType), &advEventType);
    le_adv_set_param(GAP_PARAM_ADV_CHANNEL_MAP, sizeof(advChanMap), &advChanMap);
    le_adv_set_param(GAP_PARAM_ADV_FILTER_POLICY, sizeof(advFilterPolicy), &advFilterPolicy);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MIN, sizeof(advIntMin), &advIntMin);
    le_adv_set_param(GAP_PARAM_ADV_INTERVAL_MAX, sizeof(advIntMax), &advIntMax);

    le_adv_start();

}

void tc_0002_adv_start_stop_adv_state_change_to_advertising(void)
{
    uint32_t cur_time = RTIM_GetCount(TIMM00);
    uint32_t ellapsed_time = amebad_time_get_elapsed(g_tc_adv_enable_disable_begin_time, cur_time);

    if (ellapsed_time > g_tc_adv_enable_disable_max_start_adv_ellapsed_time)
    {
        g_tc_adv_enable_disable_max_start_adv_ellapsed_time = ellapsed_time;
        /*
                data_uart_print(
                    "tc_adv_start_stop start adv ellapsed time =%dus,  cur count = %d \r\n",
                    ellapsed_time,
                    g_tc_adv_enable_disable_cur_count);
        */
        APP_PRINT_INFO2(
            "tc_adv_start_stop start adv ellapsed time =%dus,  cur count = %d \r\n",
            ellapsed_time,
            g_tc_adv_enable_disable_cur_count);


    }

    os_timer_start(&g_tc_adv_enable_disable_timer_handle);
    //reset_vendor_counter();
    g_tc_adv_enable_disable_begin_time = RTIM_GetCount(TIMM00);
    le_adv_stop();
}

void tc_0002_adv_start_stop_adv_state_change_to_idle(void)
{
    uint32_t cur_time = RTIM_GetCount(TIMM00);
    uint32_t ellapsed_time = amebad_time_get_elapsed(g_tc_adv_enable_disable_begin_time, cur_time);

    if (ellapsed_time > g_tc_adv_enable_disable_max_stop_adv_ellapsed_time)
    {
        g_tc_adv_enable_disable_max_stop_adv_ellapsed_time = ellapsed_time;
        /*
                data_uart_print(
                    "tc_adv_start_stop stop adv ellapsed time =%dus,  cur count = %d \r\n",
                    ellapsed_time,
                    g_tc_adv_enable_disable_cur_count);
        */
        APP_PRINT_INFO2(
            "tc_adv_start_stop stop adv ellapsed time =%dus,  cur count = %d \r\n",
            ellapsed_time,
            g_tc_adv_enable_disable_cur_count);
    }


    os_timer_start(&g_tc_adv_enable_disable_timer_handle);
    g_tc_adv_enable_disable_cur_count++;
    if (g_tc_adv_enable_disable_cur_count != g_tc_adv_enable_disable_max_count)
    {
        le_adv_start();
    }
    else
    {
        //test case end here
        //stop timer
        os_timer_stop(&g_tc_adv_enable_disable_timer_handle);
        os_timer_stop(&g_tc_adv_enable_disable_monitor_timer_handle);

        //print result
        data_uart_print(
            "tc_adv_start_stop finish max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
            g_tc_adv_enable_disable_max_start_adv_ellapsed_time,
            g_tc_adv_enable_disable_max_stop_adv_ellapsed_time,
            g_tc_adv_enable_disable_cur_count);

        APP_PRINT_INFO3("tc_adv_start_stop finish max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
                        g_tc_adv_enable_disable_max_start_adv_ellapsed_time,
                        g_tc_adv_enable_disable_max_stop_adv_ellapsed_time,
                        g_tc_adv_enable_disable_cur_count);

        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0002_ADV_STRESS_START_STOP, 0, NULL);
        }

    }
}

void tc_0001_add_case(uint16_t adv_interval_min, uint16_t adv_interval_max)
{
    T_TC_0001_PARAM_DATA *p_tc_0001_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_0001_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_0001_PARAM_DATA));

    p_tc_0001_param_data->id = TC_0001_ADV_PERFORMANCE;
    p_tc_0001_param_data->adv_interval_max = adv_interval_max;
    p_tc_0001_param_data->adv_interval_min = adv_interval_min;


    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_0001_param_data;


    os_queue_in(&tc_q, p_tc_param);

}

void tc_0002_add_case(uint16_t adv_interval_min, uint16_t adv_interval_max, uint32_t max_count)
{

    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    T_TC_0002_PARAM_DATA  *p_tc_0002_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON,
                                                                sizeof(T_TC_0002_PARAM_DATA));

    p_tc_0002_param_data->id = TC_0002_ADV_STRESS_START_STOP;
    p_tc_0002_param_data->adv_interval_max = adv_interval_max;
    p_tc_0002_param_data->adv_interval_min = adv_interval_min;
    p_tc_0002_param_data->max_count = max_count;

    p_tc_param->p_data = (void *)p_tc_0002_param_data;

    os_queue_in(&tc_q, p_tc_param);
}

#endif

