//#include <hrp_btif_le_utils.h>
#include <hrp_utils.h>

//#include "hrp_btif_cmd_table.h"
//#include <hrp_module_id.h>


#define LTP_SOURCE_FILE_ID 0x83



bool hrp_gap_ble_send_adv_0000_perf_config_rsp(uint16_t len, uint8_t *p_param_list)
{
#if 0
    return LTPLibSendSubMessage_WORD(pLTPLib, copmsk, pOpt, LTP_SUB_DEVICE_CONFIG_DEVICE_NAME_SET_RSP,
                                     cause);
#endif
    return true;
}

bool hrp_gap_ble_send_adv_0000_perf_start_rsp(P_HRP_LIB pLTPLib, uint8_t copmsk, uint8_t *pOpt,
                                              uint16_t cause)
{
    return true;
}

bool hrp_gap_ble_send_adv_0000_perf_cmpl_info(P_HRP_LIB pLTPLib, uint8_t copmsk, uint8_t *pOpt,
                                              uint16_t cause)
{
    return true;
}



