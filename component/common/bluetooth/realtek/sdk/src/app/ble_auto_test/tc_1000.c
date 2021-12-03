
#include <string.h>
#include "app_msg.h"
#include "trace_app.h"
#include "gap_scan.h"
#include "gap.h"
#include "gap_msg.h"
#include "gap_bond_le.h"
#include "ble_auto_test_application.h"
#include "user_cmd.h"
#include "user_cmd_parse.h"
#include "profile_server.h"
#include "vendor_tp_service.h"
#include <os_mem.h>
#include <ble_auto_test_case.h>
#include "privacy_mgnt.h"
#if F_BT_UPPER_STACK_USE_VIRTUAL_HCI
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"

#include <rtl876x_gpio.h>
#include <cp.h>
#endif
#include <tc_common.h>
#include <tc_1000.h>

#if TC_1000_SUPPORT
void tc_1000_hw_init(void)
{
#if 1
    //if reset PIN pull high
    uint8_t cp_reset_pinmux = P2_7;
    Pad_Config(cp_reset_pinmux,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
#if 0
    //if reset PIN pull low
    Pad_Config(cp_reset_pinmux,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_LOW);
#endif
    Pinmux_Config(cp_reset_pinmux, DWGPIO);

    Pad_Config(ADC_2,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
    Pad_Config(ADC_3,
               PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);

    Pinmux_Config(ADC_2, I2C0_DAT);
    Pinmux_Config(ADC_3, I2C0_CLK);
#endif


    //if reset PIN pull high
    cp_hw_init(I2C0, cp_reset_pinmux, false);
#if 0
    //if reset PIN pull low
    cp_hw_init(I2C0, cp_reset_pinmux, true);
#endif

}



void tc_1000_start(void)
{
    APP_PRINT_TRACE0("tc_1000_start: ++");

#if 1   //test read device verison
    uint8_t i = 0;
    T_CP_CAUSE cp_cause = CP_CAUSE_SUCCESS;

    T_CP_INFO info = {0};

    for (i = 1; i <= 1; i++)
    {
        cp_cause = cp_read_cp_info(&info);
        if (cp_cause == CP_CAUSE_SUCCESS)
        {
            APP_PRINT_TRACE8("dev_ver 0x%02x, fw_ver 0x%02x, aup_mar_ver 0x%02x, aup_mir_ver 0x%02x, "
                             "dev_id 0x%02x%02x%02x%02x",
                             info.dev_ver,
                             info.fw_ver,
                             info.aup_mar_ver,
                             info.aup_mir_ver,
                             info.dev_id[0],
                             info.dev_id[1],
                             info.dev_id[2],
                             info.dev_id[3]
                            );

        }
        else
        {
            APP_PRINT_ERROR2("cp_read_cp_info: timeout i %d, cp_cause %d", i, cp_cause);
        }
    }


#endif

#if 0 //test read all device information
    T_CP_INFO info;
    uint8_t a[8];
    uint32_t dev_id = 0xFFFFFFFF;
    cp_read_cp_info(&info);
    //cp_burst_read(0x00, a, 8);
    dev_id = (((uint32_t)info.dev_id[0] << 24) | ((uint32_t)info.dev_id[1] << 16)\
              | ((uint32_t)info.dev_id[2] << 8) | ((uint32_t)info.dev_id[3] << 0));
    DBG_BUFFER(MODULE_APP, LEVEL_INFO,
               "dev_verison = 0x%x, fw_verison = 0x%x, auth_protocol_mar = 0x%x,\
        auth_protocol_mir = 0x%x, device_id = 0x%x", 5, \
               info.dev_ver, info.fw_ver, info.aup_mar_ver, info.aup_mir_ver, dev_id);
#endif

#if 0 //test cp_write
    uint8_t cmd = 0x01;
    cp_write(CP_REG_CS, &cmd, 1);
#endif

#if 0   //test write challenge data
    uint8_t cha_buf[34];
    uint8_t i = 0;
    cha_buf[0] = 0;
    cha_buf[1] = 32;
    for (i = 0; i < 32; i++)
    {
        cha_buf[i + 2] = i + 1;
    }
    //cp_write_cha_info(cha_buf, 34);

    //uint8_t cha_buf1[34] = {0};
    //cp_burst_read(CP_REG_CHA_LEN, cha_buf1, 34);

#endif

#if 0
    T_CP_PRO_RES cp_res = CP_PRO_RES_NO_VALID;
    T_CP_ERROR_CODE cp_err = CP_ERR_NO_ERR;
    cp_write_cha_len(20);
    cp_write_cha_data(cha_buf, 20);
    //start signature
    cp_ctrl(CP_CMD_START_SIG_GEN);
    cp_res = cp_read_proc_result();
    if (cp_res == CP_PRO_RES_SIG_GEN_OK)
    {

    }
    else
    {
        cp_err = cp_read_err_code();
    }

    return;

#endif
    APP_PRINT_TRACE0("tc_1000_start: --");

}

void cp_reset_internal(bool poll_low);

void tc_1001_start(void)
{
    APP_PRINT_TRACE0("tc_1001_start: ++ poll high");

    cp_reset_internal(false);
    cp_set_slave_address(false);
    uint8_t i = 0;
    T_CP_CAUSE cp_cause = CP_CAUSE_SUCCESS;

    T_CP_INFO info = {0};

    for (i = 1; i <= 1; i++)
    {
        cp_cause = cp_read_cp_info(&info);
        if (cp_cause == CP_CAUSE_SUCCESS)
        {
            APP_PRINT_TRACE8("dev_ver 0x%02x, fw_ver 0x%02x, aup_mar_ver 0x%02x, aup_mir_ver 0x%02x, "
                             "dev_id 0x%02x%02x%02x%02x",
                             info.dev_ver,
                             info.fw_ver,
                             info.aup_mar_ver,
                             info.aup_mir_ver,
                             info.dev_id[0],
                             info.dev_id[1],
                             info.dev_id[2],
                             info.dev_id[3]
                            );

        }
        else
        {
            APP_PRINT_ERROR2("cp_read_cp_info: timeout i %d, cp_cause %d", i, cp_cause);
        }
    }

    APP_PRINT_TRACE0("tc_1001_start: --");
}


void tc_1002_start(void)
{
    APP_PRINT_TRACE0("tc_1002_start: ++ poll low");

    cp_reset_internal(true);
    cp_set_slave_address(true);
    uint8_t i = 0;
    T_CP_CAUSE cp_cause = CP_CAUSE_SUCCESS;

    T_CP_INFO info = {0};

    for (i = 1; i <= 1; i++)
    {
        cp_cause = cp_read_cp_info(&info);
        if (cp_cause == CP_CAUSE_SUCCESS)
        {
            APP_PRINT_TRACE8("dev_ver 0x%02x, fw_ver 0x%02x, aup_mar_ver 0x%02x, aup_mir_ver 0x%02x, "
                             "dev_id 0x%02x%02x%02x%02x",
                             info.dev_ver,
                             info.fw_ver,
                             info.aup_mar_ver,
                             info.aup_mir_ver,
                             info.dev_id[0],
                             info.dev_id[1],
                             info.dev_id[2],
                             info.dev_id[3]
                            );

        }
        else
        {
            APP_PRINT_ERROR2("cp_read_cp_info: timeout i %d, cp_cause %d", i, cp_cause);
        }
    }
    APP_PRINT_TRACE0("tc_1002_start: --");
}
#endif
