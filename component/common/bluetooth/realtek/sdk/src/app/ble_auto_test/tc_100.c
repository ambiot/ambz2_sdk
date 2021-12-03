#include <string.h>
#include "app_msg.h"
#include "trace_app.h"


#include "user_cmd.h"
#include "user_cmd_parse.h"


#include <os_mem.h>
#include <os_timer.h>

#include <ble_auto_test_case.h>

#if F_BT_LE_GAP_SCAN_SUPPORT
#include "gap_scan.h"
#endif
#include "gap.h"



#include <tc_common.h>
#include <tc_100.h>

#if TC_100_SUPPORT

#if F_BT_LE_GAP_SCAN_SUPPORT

//for case TC_0100_SCAN_PERFORMANCE
#define SCAN_TEST_PARAM_COUNT       8
T_SCAN_TEST_PARAM g_scan_test_param[SCAN_TEST_PARAM_COUNT];
uint8_t g_scan_cur_test_index = 0;
uint8_t g_scan_total_test_index = SCAN_TEST_PARAM_COUNT;
bool flag2 = false;
void tc_0100_scan_perf_timeout_handler(void *pxTimer)
{
    g_test_end_time = read_vendor_counter_no_display();
    APP_PRINT_INFO3("tc_0100_scan_perf_timeout_handler begin time=%d, end time=%d, time elapsed = %dus",
                    g_test_begin_time, g_test_end_time,
                    (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK);
    if (false == flag2)
    {
        data_uart_print("begin time,          end time,            elapase time(us)\r\n");
        flag2 = true;
    }
    data_uart_print(
        "  %d,                %d,                  %d\r\n",
        g_test_begin_time,
        g_test_end_time,
        (g_test_end_time - g_test_begin_time) / TEST_CPU_CLOCK);
    flag2 = false;
}

void tc_0100_scan_perf_start(uint16_t scan_interval, uint16_t scan_window)
{
    g_scan_cur_test_index = 0;
    g_scan_total_test_index = SCAN_TEST_PARAM_COUNT;

    for (uint8_t i = 0; i < g_scan_total_test_index; i++)
    {
        g_scan_test_param[i].scan_interval = scan_interval;
        g_scan_test_param[i].scan_window = scan_window;
    }
    if (NULL == g_test_timer_handle)
    {
        os_timer_create(&g_test_timer_handle, "testTimer", 1, 1000, false,
                        tc_0100_scan_perf_timeout_handler);
    }
    //start total timer
    os_timer_start(&g_test_timer_handle);
    reset_vendor_counter();
    g_test_begin_time = read_vendor_counter_no_display();

    g_scan_test_param[g_scan_cur_test_index].start_scan_begin_time = read_vendor_counter_no_display();

    APP_PRINT_INFO1("tc_0100_scan_perf_start begin_time = %d",
                    g_scan_test_param[g_scan_cur_test_index].start_scan_begin_time);

    le_scan_start();
}

void tc_0100_scan_state_change_to_scaning()
{
    le_scan_stop();
}

void tc_0100_scan_state_change_to_idle()
{
    le_scan_start();
}


/********************************************************


TC_0101


********************************************************/


T_TC_0101_PARAM *p_tc_0101_param = NULL;
void tc_0101_scan_stress_enable_disable_TimeoutHandler(void *pxTimer)
{
    data_uart_print(
        "!!!tc_0101_scan_stress_enable_disable_TimeoutHandler max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
        p_tc_0101_param->scan_enable_disable_max_enable_scan_ellapsed_time,
        p_tc_0101_param->scan_enable_disable_max_disable_scan_ellapsed_time,
        p_tc_0101_param->scan_enable_disable_cur_count);
    APP_PRINT_ERROR3("tc_0101_scan_stress_enable_disable_TimeoutHandler max start adv ellapsed time =%dus,  max stop adv ellapsed time =%dus cur count = %d \r\n",
                     p_tc_0101_param->scan_enable_disable_max_enable_scan_ellapsed_time,
                     p_tc_0101_param->scan_enable_disable_max_disable_scan_ellapsed_time,
                     p_tc_0101_param->scan_enable_disable_cur_count);

}
void tc_0101_scan_stress_enable_disable_start(T_GAP_SCAN_MODE mode,
                                              uint16_t                interval,
                                              uint16_t                window,
                                              T_GAP_SCAN_FILTER_POLICY filter_policy,
                                              uint8_t                 filter_duplicates,
                                              uint32_t max_count
                                             )
{
    if (p_tc_0101_param == NULL)
    {
        p_tc_0101_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_0101_PARAM));
    }
    if (p_tc_0101_param != NULL)
    {
        p_tc_0101_param->mode = mode;
        p_tc_0101_param->interval = interval;
        p_tc_0101_param->window = window;
        p_tc_0101_param->filter_policy = filter_policy;
        p_tc_0101_param->filter_duplicates = filter_duplicates;

        p_tc_0101_param->scan_enable_disable_max_count = max_count;
        p_tc_0101_param->scan_enable_disable_cur_count = 0;
        if (NULL == p_tc_0101_param->scan_enable_disable_timer_handle)
        {
            os_timer_create(&p_tc_0101_param->scan_enable_disable_timer_handle,
                            "tc_0101_scan_stress_enable_disable_start", 1, 1000, false,
                            tc_0101_scan_stress_enable_disable_TimeoutHandler);
        }

        //start total timer
        os_timer_start(&p_tc_0101_param->scan_enable_disable_timer_handle);

        reset_vendor_counter();
        p_tc_0101_param->scan_enable_disable_begin_time = read_vendor_counter_no_display();

        APP_PRINT_INFO1("app_handle_tc_adv_enable_disable_start begin_time = %d",
                        p_tc_0101_param->scan_enable_disable_begin_time);

        le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(p_tc_0101_param->mode), &p_tc_0101_param->mode);
        le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(p_tc_0101_param->interval),
                          &p_tc_0101_param->interval);
        le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(p_tc_0101_param->window),
                          &p_tc_0101_param->window);
        le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(p_tc_0101_param->filter_policy),
                          &p_tc_0101_param->filter_policy);
        le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(p_tc_0101_param->filter_duplicates),
                          &p_tc_0101_param->filter_duplicates);

        le_scan_start();
    }
    else
    {
        data_uart_print(
            "Fail to run tc_0101_scan_stress_enable_disable_start");

    }

}
void tc_0101_scan_stress_enable_disable_state_change_to_scaning(void)
{
    uint32_t cur_time = read_vendor_counter_no_display();
    uint32_t ellapsed_time = (cur_time - p_tc_0101_param->scan_enable_disable_begin_time) /
                             TEST_CPU_CLOCK;

    if (ellapsed_time > p_tc_0101_param->scan_enable_disable_max_enable_scan_ellapsed_time)
    {
        p_tc_0101_param->scan_enable_disable_max_enable_scan_ellapsed_time = ellapsed_time;
        /*
                data_uart_print(
                    "tc_adv_start_stop start adv ellapsed time =%dus,  cur count = %d \r\n",
                    ellapsed_time,
                    g_tc_adv_enable_disable_cur_count);
        */
        APP_PRINT_INFO2(
            "tc_adv_start_stop start adv ellapsed time =%dus,  cur count = %d \r\n",
            ellapsed_time,
            p_tc_0101_param->scan_enable_disable_cur_count);


    }

    os_timer_start(&p_tc_0101_param->scan_enable_disable_timer_handle);
    reset_vendor_counter();
    p_tc_0101_param->scan_enable_disable_begin_time = read_vendor_counter_no_display();
    le_scan_stop();
}
void tc_0101_scan_stress_enable_disable_state_change_to_idle(void)
{
    uint32_t cur_time = read_vendor_counter_no_display();
    uint32_t ellapsed_time = (cur_time - p_tc_0101_param->scan_enable_disable_begin_time) /
                             TEST_CPU_CLOCK;

    if (ellapsed_time > p_tc_0101_param->scan_enable_disable_max_disable_scan_ellapsed_time)
    {
        p_tc_0101_param->scan_enable_disable_max_disable_scan_ellapsed_time = ellapsed_time;
        /*
                data_uart_print(
                    "tc_adv_start_stop stop adv ellapsed time =%dus,  cur count = %d \r\n",
                    ellapsed_time,
                    g_tc_adv_enable_disable_cur_count);
        */
        APP_PRINT_INFO2(
            "tc_adv_start_stop stop adv ellapsed time =%dus,  cur count = %d \r\n",
            ellapsed_time,
            p_tc_0101_param->scan_enable_disable_cur_count);
    }


    os_timer_start(&p_tc_0101_param->scan_enable_disable_timer_handle);
    p_tc_0101_param->scan_enable_disable_cur_count++;
    if (p_tc_0101_param->scan_enable_disable_cur_count !=
        p_tc_0101_param->scan_enable_disable_max_count)
    {
        le_scan_start();
    }
    else
    {
        //test case end here
        //stop timer
        os_timer_stop(&p_tc_0101_param->scan_enable_disable_timer_handle);

        //print result
        data_uart_print(
            "tc_0101 finish max enable scan ellapsed time =%dus,  max disable scan ellapsed time =%dus cur count = %d \r\n",
            p_tc_0101_param->scan_enable_disable_max_enable_scan_ellapsed_time,
            p_tc_0101_param->scan_enable_disable_max_disable_scan_ellapsed_time,
            p_tc_0101_param->scan_enable_disable_cur_count);

        APP_PRINT_INFO3("tc_0101 finish max enable scan ellapsed time =%dus,  max disable scan ellapsed time =%dus cur count = %d \r\n",
                        p_tc_0101_param->scan_enable_disable_max_enable_scan_ellapsed_time,
                        p_tc_0101_param->scan_enable_disable_max_disable_scan_ellapsed_time,
                        p_tc_0101_param->scan_enable_disable_cur_count);

        os_mem_free(p_tc_0101_param);
        p_tc_0101_param = NULL;

        if (p_tc_result_cb != NULL)
        {
            p_tc_result_cb(TC_0101_SCAN_STRESS_ENABLE_DISABLE, 0, NULL);
        }

    }
}



void tc_0101_add_case(T_GAP_SCAN_MODE mode,
                      uint16_t                interval,
                      uint16_t                window,
                      T_GAP_SCAN_FILTER_POLICY filter_policy,
                      uint8_t                 filter_duplicates,
                      uint32_t max_count
                     )
{

    T_TC_0101_IN_PARAM_DATA *p_tc_0101_param_data;
    T_TC_PARAM *p_tc_param = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_PARAM));
    p_tc_0101_param_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_TC_0101_IN_PARAM_DATA));

    p_tc_0101_param_data->id = TC_0101_SCAN_STRESS_ENABLE_DISABLE;
    p_tc_0101_param_data->mode = mode;
    p_tc_0101_param_data->interval = interval;
    p_tc_0101_param_data->window = window;
    p_tc_0101_param_data->filter_policy = filter_policy;
    p_tc_0101_param_data->filter_duplicates = filter_duplicates;
    p_tc_0101_param_data->max_count = max_count;

    p_tc_param->p_data = (T_TC_PARAM_DATA *)p_tc_0101_param_data;

    os_queue_in(&tc_q, p_tc_param);


}
#endif

#endif
