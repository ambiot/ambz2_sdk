/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     delay_msg_resp.c
* @brief    Source file for delay message response.
* @details  Data types and external functions declaration.
* @author   hector_huang
* @date     2019-05-20
* @version  v1.0
* *************************************************************************************
*/
#include <stdlib.h>
#include "delay_msg_rsp.h"
#include "mesh_node.h"
#include "model_config.h"

#define DELAY_MSG_TRANS_DELAY_MIN       200 /* 200ms */

#define DELAY_MSG_UNICAST_ADDR_MIN      10
#define DELAY_MSG_UNICAST_ADDR_MAX      50
#define DELAY_MSG_GROUP_ADDR_MIN        500
#define DELAY_MSG_GROUP_ADDR_MAX        2000

uint32_t delay_msg_get_rsp_delay(uint16_t dst_addr)
{
    int delay = rand();
    uint32_t real_delay = delay;
    if (MESH_IS_UNICAST_ADDR(dst_addr))
    {
        /* random delay 10-50ms */
        real_delay %= (DELAY_MSG_UNICAST_ADDR_MAX - DELAY_MSG_UNICAST_ADDR_MIN);
        real_delay += DELAY_MSG_UNICAST_ADDR_MIN;
    }
    else
    {
        /* random delay 50-1000ms */
        real_delay %= (DELAY_MSG_GROUP_ADDR_MAX - DELAY_MSG_GROUP_ADDR_MIN);
        real_delay += DELAY_MSG_GROUP_ADDR_MIN;
    }

    return real_delay;
}

uint32_t delay_msg_get_trans_delay(uint32_t delay_time, generic_transition_time_t trans_time,
                                   uint32_t delay_rsp_time, bool send_immediately, bool ack)
{
    uint32_t delay_pub_time = delay_time + generic_transition_time_convert(trans_time);

#if MODEL_ENABLE_PARALLEL_ADV
    UNUSED(ack);
    if (send_immediately)
    {
        if (delay_pub_time < DELAY_MSG_TRANS_DELAY_MIN)
        {
            delay_pub_time = DELAY_MSG_TRANS_DELAY_MIN;
        }
        delay_pub_time += delay_rsp_time;
    }
    else
    {
        uint32_t temp = delay_rsp_time;
        if (delay_pub_time < DELAY_MSG_TRANS_DELAY_MIN)
        {
            temp += (DELAY_MSG_TRANS_DELAY_MIN - delay_pub_time);
        }
        delay_pub_time = temp;
    }
#else
    if (send_immediately)
    {
        if (delay_pub_time < DELAY_MSG_TRANS_DELAY_MIN)
        {
            delay_pub_time = DELAY_MSG_TRANS_DELAY_MIN;
        }
        if (!ack)
        {
            delay_pub_time += delay_rsp_time;
        }
    }
    else
    {
        if (delay_pub_time < delay_rsp_time)
        {
            if (delay_pub_time < DELAY_MSG_TRANS_DELAY_MIN)
            {
                delay_pub_time = DELAY_MSG_TRANS_DELAY_MIN;
            }
            if (!ack)
            {
                delay_pub_time += delay_rsp_time;
            }
        }
        else
        {
            uint32_t temp = delay_rsp_time;
            if (delay_pub_time < DELAY_MSG_TRANS_DELAY_MIN)
            {
                temp += (DELAY_MSG_TRANS_DELAY_MIN - delay_pub_time);
            }
            delay_pub_time = temp;
        }
    }
#endif

    return delay_pub_time;
}
