/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      link_mgr.c
   * @brief     Multilink manager functions.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_FUZZ_TEST) && CONFIG_BT_FUZZ_TEST
#include <bt_fuzz_test_link_mgr.h>
#include <trace.h>
#include <string.h>
//#include <ftl.h>

/*============================================================================*
 *                              Constants
 *============================================================================*/

/*============================================================================*
 *                              Variables
 *============================================================================*/
/** @addtogroup  SCATTERNET_GAP_MSG
    * @{
    */
T_APP_LINK bt_fuzz_test_app_link_table[BT_FUZZ_TEST_APP_MAX_LINKS];
/** @} */
/** @addtogroup  SCATTERNET_SCAN_MGR
    * @{
    */
T_DEV_INFO bt_fuzz_test_dev_list[BT_FUZZ_TEST_APP_MAX_DEVICE_INFO];
uint8_t bt_fuzz_test_dev_list_count = 0;
/** @} */

/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @addtogroup  CENTRAL_SCAN_MGR
    * @{
    */
/**
 * @brief   Add device information to device list.
 *
 * @param[in] bd_addr Peer device address.
 * @param[in] bd_type Peer device address type.
 * @retval true Success.
 * @retval false Failed, device list is full.
 */
bool bt_fuzz_test_link_mgr_add_device(uint8_t *bd_addr, uint8_t bd_type)
{
    /* If result count not at max */
    if (bt_fuzz_test_dev_list_count < BT_FUZZ_TEST_APP_MAX_DEVICE_INFO)
    {
        uint8_t i;
        /* Check if device is already in device list*/
        for (i = 0; i < bt_fuzz_test_dev_list_count; i++)
        {
            if (memcmp(bd_addr, bt_fuzz_test_dev_list[i].bd_addr, GAP_BD_ADDR_LEN) == 0)
            {
                return true;
            }
        }

        /*Add addr to device list list*/
        memcpy(bt_fuzz_test_dev_list[bt_fuzz_test_dev_list_count].bd_addr, bd_addr, GAP_BD_ADDR_LEN);
        bt_fuzz_test_dev_list[bt_fuzz_test_dev_list_count].bd_type = bd_type;

        /*Increment device list count*/
        bt_fuzz_test_dev_list_count++;
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief Clear device list.
 * @retval None.
 */
void bt_fuzz_test_link_mgr_clear_device_list(void)
{
    bt_fuzz_test_dev_list_count = 0;
}
#endif
/** @} */
/** @addtogroup  SCATTERNET_APP
    * @{
    */
/** @} */
/** @} */
