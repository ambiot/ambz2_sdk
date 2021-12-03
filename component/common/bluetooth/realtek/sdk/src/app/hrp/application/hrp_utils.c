#include <hrp_utils.h>
#include <string.h>
#include "trace_app.h"
#include <hrp_btif_entry.h>
#include <hrp_profile_entry.h>
#include <hrp_gap_ble_cmd_table.h>
#include <hrp_application.h>
#include <os_sched.h>
#include <bt_types.h>
#include "wdt_reset.h"

#define LTP_SOURCE_FILE_ID 0x83

bool LTPLibHandleUnkownCommand(P_HRP_LIB pLTPLib, uint8_t cmd);


/*--------------------------------------------------------------------------*/
/* FCS lookup table.                                                        */
/* generator polynomial: x**8 + x**2 + x + 1                                */
/* -------------------------------------------------------------------------*/
static const uint8_t crc8EtsTable[256] =
{
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
    0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
    0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
    0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
    0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
    0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
    0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
    0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
    0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
    0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
    0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
    0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
    0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
    0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
    0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
    0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
    0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF
};
static const uint16_t crc16EtsTable[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

P_BT_HRP_ACTION BTLTPAllocateAction(P_BT_HRP pBTLtp)
{
    uint16_t loop;

    for (loop = 0; loop < BTHRP_ACTION_POOL_SIZE; loop++)
    {
        if (pBTLtp->ActionPool[loop].Action == btltpActionNotUsed)
        {
            return &pBTLtp->ActionPool[loop];
        }
    }

    if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
    {
        APP_PRINT_ERROR0("LTP: out of action queue elements");
    }

    return NULL;
}

extern P_BT_HRP  P_BtHrp;


void bt_hrp_buffer_callback(void *Handle)
{
    P_BT_HRP_ACTION         pAction    = (P_BT_HRP_ACTION)Handle;
    TBTLtpActionCommand  thisAction = pAction->Action;

    pAction->Action = btltpActionNotUsed;

    switch (thisAction)
    {
    case btltpActionReset: /*---------------------------------------------*/

        APP_PRINT_INFO0("LTP: reset (wait for WD to kick in)");
        /* wait for last char of ResetRsp (buffercallback is executed on txempty, NOT on txcomplete) */
        os_delay(20);   /* 20 ms delay */
        wdt_reset();
        break;
#if 0
    case btltpActionSendDataConf: /*--------------------------------------*/
        {
            PBTLtpMDLContext pMDLContext = BTLTPFindMDLContext(P_BtHrp, pAction->p.MDL_ID); //&BtLtp
            //bool             ret;

            if (pMDLContext != NULL)
            {
                pMDLContext->pendingDataConfs++;
                while (pMDLContext->pendingDataConfs > 0)
                {
#if 0
                    ret = btif_DataConf(//NULL,
                              //tBTLtp.BTIFHandle,
                              pAction->p.MDL_ID,
                              BTIF_CAUSE_SUCCESS
                          );

                    /* if msg fails, keep number of failed dataConfs and retry later */
                    if (ret == false)
                    {
                        DBG_BUFFER(MODULE_LTP, LEVEL_ERROR, "!!LTP: failed to send [%d] COM_DataConf(s)", 1,
                                   pMDLContext->pendingDataConfs
                                  );
                        break;
                    }
#endif
                    pMDLContext->pendingDataConfs--;
                }
            }
            //     hrp_lib_trigger_hrp_proccess(&P_BtHrp->HRPLib);
            //       to do
        }
        break;
#endif

    default: /*-----------------------------------------------------------*/
        break;
    }
}


/****************************************************************************/
/* bool bt_hrp_tgt_send_hrp_message                                              */
/* (                                                                        */
/*    HRP_TGT_APPHANDLE AppHandle : Handle to identity Application Context  */
/*    uint8_t *            pMsg      : pointer to of LTP msg buffer to be send */
/*    uint16_t              offset                                              */
/*    uint16_t              dataLen                                             */
/* )                                                                        */
/* return:------------------------------------------------------------------*/
/* false in case the message could be send successfully,                    */
/* true in case the message could not be send but was dumped                */
/*                                                                          */
/* Description:-------------------------------------------------------------*/
/* This function is used to send an LTP message to an Application with the  */
/* BT_LTP_Sendxxx functions of this library                                 */
/****************************************************************************/
bool bt_hrp_tgt_send_hrp_message(HRP_TGT_APPHANDLE AppHandle, uint8_t *p_buffer, uint16_t offset,
                                 uint16_t data_len)
{
    P_BT_HRP    pBTHrp   = (P_BT_HRP)P_BtHrp;

    if (pBTHrp->State == btltpStateIdle)
    {
        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_INFO)
        {
            //       APP_PRINT_INFO3("bt_hrp_tgt_send_hrp_message: 0x%x(%s) dataLen = %d", *pBuffer,
            //                      LTPLIB_Message(*pBuffer), dataLen);
        }

        /* put real buffer address and buffer callback in front of message */
        hrp_write((uint8_t *)(p_buffer + offset), data_len);
        return (true);
    }
    else
    {
        pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx = pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx = 0;

        if (pBTHrp->pBufferAction)
        {
            void    *Handle = (void *)pBTHrp->pBufferAction;

            pBTHrp->pBufferAction = NULL;
            bt_hrp_buffer_callback(Handle);
        }

        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
        {
            APP_PRINT_ERROR0("bt_hrp_tgt_send_hrp_message: did not send message (configurator active)");
        }

        return true;
    }
}

/****************************************************************************/
/* uint8_t * BTLTPTgtSendBfferAlloc                                            */
/* (                                                                        */
/*    HRP_TGT_APPHANDLE AppHandle : Handle to identity Application Context  */
/*    uint16_t    len                 : size of buffer to be allocated (bytes)  */
/* )                                                                        */
/* return:------------------------------------------------------------------*/
/* pointer to allocated memory in case of success                           */
/* NULL pointer in case of an error                                         */
/*                                                                          */
/* Description:-------------------------------------------------------------*/
/* This target specific function is used to allocate buffers for LTP        */
/* messages that are send to an Application with the BT_LTP_Sendxxx         */
/* functions of this library.                                               */
/****************************************************************************/
uint8_t *bt_hrp_tgt_send_buffer_alloc(HRP_TGT_APPHANDLE AppHandle, uint16_t len)
{
    uint8_t *p_buf = NULL;
    P_BT_HRP    pBTHrp   = (P_BT_HRP)P_BtHrp;

    if (NULL == pBTHrp->p_aci_tcb)
    {
        return NULL;
    }

    /* free index > tx index  */
    if ((pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx >= pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx)
        && (pBTHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size != pBTHrp->p_aci_tcb->tx_mem_tcb.free_size))
    {
        /* [A---tx_idx----free_idx----B], have enough serial ram in  [free_idx----B]  */
        if ((pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx + len) <= TX_BUFFER_SIZE1)

        {
            p_buf = pBTHrp->p_aci_tcb->p_tx_buf + pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx += len;
            pBTHrp->p_aci_tcb->tx_mem_tcb.free_size -= len;
            /* if reach tx buffer size, return to index 0 */
            // pBTLtp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx &= (TX_BUFFER_SIZE1 - 1);
            if (pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx >= TX_BUFFER_SIZE1)
            {
                pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx -= TX_BUFFER_SIZE1;
            }
        }
        /* [A---tx_idx----free_idx----B], have enough serial ram in  [A---tx_idx],
            discard [free_idx----B] */
        else if ((pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx + len) > TX_BUFFER_SIZE1
                 && pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx >= len)
        {
            p_buf = pBTHrp->p_aci_tcb->p_tx_buf;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size =
                TX_BUFFER_SIZE1 - pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx = len;
            pBTHrp->p_aci_tcb->tx_mem_tcb.free_size -= len;
        }
        //Lorna: There have one case: when tx_blk_idx ==tx_free_blk_idx, as long as len<=  TX_BUFFER_SIZE1, ram can be allocated
        else if ((len <= TX_BUFFER_SIZE1) && (pBTHrp->p_aci_tcb->tx_mem_tcb.free_size == TX_BUFFER_SIZE1)
                 && (pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx == pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx))
        {
            p_buf = pBTHrp->p_aci_tcb->p_tx_buf;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size = 0;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx = len;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx = 0;
            pBTHrp->p_aci_tcb->tx_mem_tcb.free_size -= len;
            /* if reach tx buffer size, return to index 0 */
            // pBTLtp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx &= (TX_BUFFER_SIZE1 - 1);
            if (pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx >= TX_BUFFER_SIZE1)
            {
                pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx -= TX_BUFFER_SIZE1;
            }
        }
        else
        {
            p_buf = NULL;   /* no enough free size */

            if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
            {
                APP_PRINT_ERROR5("Have no serial ram 1,free index:0x%x, tx index:0x%x, unused size:0x%x, free size:0x%x, len:0x%x",
                                 pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx, pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx, \
                                 pBTHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size, pBTHrp->p_aci_tcb->tx_mem_tcb.free_size, len);
            }
        }
    }
    /* free index < tx index */
    else if (pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx < pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx)
    {
        /* [A---free_idx----tx_idx----B],  have enough ram in  [free_idx----tx_idx]  */
        if ((pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx + len) <
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx)

        {
            p_buf = pBTHrp->p_aci_tcb->p_tx_buf + pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx;
            pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx += len;
            pBTHrp->p_aci_tcb->tx_mem_tcb.free_size -= len;
        }
        else
        {
            p_buf = NULL;   /* no enough free size */

            if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
            {
                APP_PRINT_ERROR5("Have no serial ram 2,free index:0x%x, tx index:0x%x, unused size:0x%x, free size:0x%x, len:0x%x",
                                 pBTHrp->p_aci_tcb->tx_mem_tcb.tx_free_blk_idx, pBTHrp->p_aci_tcb->tx_mem_tcb.tx_blk_idx, \
                                 pBTHrp->p_aci_tcb->tx_mem_tcb.tx_un_used_size, pBTHrp->p_aci_tcb->tx_mem_tcb.free_size, len);
            }
        }
    }
    else    /* no possiable */
    {
        p_buf = NULL;   /* no enough free size */

        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
        {
            APP_PRINT_ERROR0("Have no serial ram 3");
        }
    }

    return p_buf;
}

/****************************************************************************/
/* void bt_hrp_tgt_receive_buffer_release                                        */
/* (                                                                        */
/*    HRP_TGT_APPHANDLE AppHandle : Handle to identity Application Context  */
/*    uint8_t *            pBuffer   : pointer to receive buffer to be released*/
/* )                                                                        */
/* return:------------------------------------------------------------------*/
/* non                                                                      */
/*                                                                          */
/* Description:-------------------------------------------------------------*/
/* This target specific function is used to released buffers for LTP        */
/* messages that are received and consumed by the 'LTPLibHandleReceiveData' */
/* function of this library.                                                */
/****************************************************************************/

void bt_hrp_tgt_receive_buffer_release(HRP_TGT_APPHANDLE AppHandle, uint8_t *pBuffer)
{
    hrp_buffer_release((void *)pBuffer);
}

/****************************************************************************/
/* uint8_t * bt_hrp_tgt_assembly_buffer_alloc                                       */
/* (                                                                        */
/*    HRP_TGT_APPHANDLE AppHandle : Handle to identity Application Context  */
/* )                                                                        */
/* return:------------------------------------------------------------------*/
/* pointer to allocated memory in case of success                           */
/* NULL pointer in case of an error                                         */
/*                                                                          */
/* Description:-------------------------------------------------------------*/
/* This target specific function is used to allocate buffers for LTP-       */
/* message assembly that is processed by functions of this library.         */
/****************************************************************************/
uint8_t *bt_hrp_tgt_assembly_buffer_alloc(HRP_TGT_APPHANDLE AppHandle)
{
    P_BT_HRP p_bt_hrp  = (P_BT_HRP)AppHandle;

    return p_bt_hrp->p_aci_tcb->p_rx_handle_buf;
}

/****************************************************************************/
/* PLTPElement bt_hrp_tgt_queue_element_alloc                                    */
/* (                                                                        */
/*    HRP_TGT_APPHANDLE AppHandle : Handle to identity Application Context  */
/* )                                                                        */
/* return:------------------------------------------------------------------*/
/* pointer to allocated queue element in case of success                    */
/* NULL pointer in case of an error                                         */
/*                                                                          */
/* Description:-------------------------------------------------------------*/
/* This target specific function is used to allocate queue elements for LTP-*/
/* message assembly that is processed by functions of this library.         */
/****************************************************************************/
P_HRP_ElEMENT bt_hrp_tgt_queue_element_alloc(HRP_TGT_APPHANDLE AppHandle)
{
    P_BT_HRP      p_bt_hrp   = (P_BT_HRP)AppHandle;
    P_HRP_ElEMENT pElement;

    pElement = hrp_queue_out(&p_bt_hrp->FreeElementQueue);

    if (!pElement)
    {
        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
        {
            APP_PRINT_ERROR0("bt_hrp_tgt_queue_element_alloc: no element in BTLTPTgtQueueElementAlloc");
        }
    }

    return pElement;
}

/****************************************************************************/
/* void bt_hrp_tgt_queue_element_release                                         */
/* (                                                                        */
/*    HRP_TGT_APPHANDLE AppHandle : Handle to identity Application Context  */
/* )                                                                        */
/* return:------------------------------------------------------------------*/
/* non                                                                      */
/*                                                                          */
/* Description:-------------------------------------------------------------*/
/* This target specific function is used to release queue elements for LTP- */
/* message assembly that is processed by functions of this library.         */
/****************************************************************************/
void bt_hrp_tgt_queue_element_release(HRP_TGT_APPHANDLE AppHandle, P_HRP_ElEMENT pHRPElement)
{
    P_BT_HRP pBTLtp   = (P_BT_HRP)AppHandle;

    hrp_queue_in(&pBTLtp->FreeElementQueue, pHRPElement);
}


uint8_t bt_hrp_tgt_do_crc8(HRP_TGT_APPHANDLE AppHandle, uint8_t *pStart, uint16_t length)
{
    uint8_t fcs = 0xff;

    while (length--)
    {
        fcs = crc8EtsTable[fcs ^ *pStart++];
    }
    return 0xff - fcs;
}
unsigned short bt_hrp_tgt_do_crc16(unsigned char *q, int len)
{
    unsigned short crc = 0;

    while (len-- > 0)
    {
        crc = crc16EtsTable[(crc >> 8 ^ *q++) & 0xff] ^ (crc << 8);
    }
    return crc;
}

void hrp_queue_in(HRP_QUEUE_P QueuePtr, void *pQueueElement)
{
    P_HRP_ELEMENT QueueElementPtr = (P_HRP_ELEMENT)pQueueElement;
    P_HRP_ELEMENT LastPtr;

    if ((LastPtr = QueuePtr->Last) == (P_HRP_ELEMENT)0)    /* if queue is empty,  */
    {
        QueuePtr->First = QueueElementPtr;    /* q->first = q->last = new entry */
    }
    else                                    /* if it is not empty, new entry  */
    {
        LastPtr->Next  = QueueElementPtr;    /* is next from last entry        */
    }
    QueuePtr->Last = QueueElementPtr;
    QueueElementPtr->Next = (P_HRP_ELEMENT)0;
    QueuePtr->ElementCount++;               /* increment element count        */
}

void *hrp_queue_out(HRP_QUEUE_P QueuePtr)
{
    P_HRP_ELEMENT FirstPtr;

    if ((FirstPtr = QueuePtr->First) != (P_HRP_ELEMENT)0)
    {
        /* if queue not empty and    */
        /* it is the last entry      */
        if ((QueuePtr->First = FirstPtr->Next) == (P_HRP_ELEMENT)0)
        {
            QueuePtr->Last = (P_HRP_ELEMENT)0;    /* set queue empty           */
        }
        QueuePtr->ElementCount--;                  /* decrement element count   */
    }
    return (FirstPtr);
}


static void bt_hrp_init_hrp_assembly(P_HRP_LIB pHRPLib, bool reUseBuffer)
{
    if (reUseBuffer)
    {
        pHRPLib->HRPMsgStart      = pHRPLib->ReceiveOffset;
        pHRPLib->HRPMsgPos        = pHRPLib->ReceiveOffset;
        pHRPLib->HRPDataCollected = 0;
        pHRPLib->HRPMsgLength     = 0; /* not known */
    }
    else
    {
        pHRPLib->pHRPMsg = NULL;
    }
}

bool hrp_lib_initialize(P_HRP_LIB pHRPLib, HRP_TGT_APPHANDLE AppHandle, uint16_t ReceiveOffset,
                        uint16_t ReceiveMaxLen, uint16_t SendOffset)
{
    /* first of all, clear context data                                       */
    memset(pHRPLib, 0, sizeof(T_HRP_LIB));

    /* initialize context                                                     */
    pHRPLib->AppHandle        = AppHandle;
    pHRPLib->ReceiveOffset    = ReceiveOffset;
    pHRPLib->ReceiveMaxLength = ReceiveMaxLen;
    pHRPLib->SendOffset       = SendOffset;

    /* initialize message assembly                                            */
    bt_hrp_init_hrp_assembly(pHRPLib, true);

    /* ready to rock...                                                       */
    pHRPLib->Status = HRPLibStatusIdle;

    return true; /* OK */
}


bool hrp_lib_shutdown(P_HRP_LIB pHRPLib)
{
    if (pHRPLib == NULL)
    {
        return false;
    }

    if (pHRPLib->pHRPMsg != NULL)
    {
        pHRPLib->pHRPMsg = NULL;
    }

    if (pHRPLib->pActiveElement != NULL)
    {
        bt_hrp_tgt_queue_element_release(pHRPLib->AppHandle, pHRPLib->pActiveElement);
        pHRPLib->pActiveElement = NULL;
    }

    return true;
}

bool hrp_lib_handle_receive_data(P_HRP_LIB p_hrp_lib, uint8_t *p_rx_buffer, uint16_t rx_length,
                                 uint16_t rx_offset)
{
    //to do: process downstream commands
    P_HRP_ElEMENT p_hrp_element;

    /* try to get new storage element                                         */
    p_hrp_element = bt_hrp_tgt_queue_element_alloc(p_hrp_lib->AppHandle);
    if (p_hrp_element)
    {
        /* store information                                                   */
        p_hrp_element->DataCB.BufferAddress = p_rx_buffer;
        p_hrp_element->DataCB.Length        = rx_length;
        p_hrp_element->DataCB.Offset        = rx_offset;

        hrp_queue_in(&p_hrp_lib->UsedElementQueue, p_hrp_element);
        //DBG_BUFFER(MODULE_LTP, LEVEL_TRACE, "hrp_lib_handle_receive_data: Alloc new element: len = %d ", 1, rxLength);
    }
    else
    {
        if (P_AciConfig->ltp_trace_level >= LTP_TRACE_ERROR)
        {
            APP_PRINT_ERROR0("hrp_lib_handle_receive_data: Alloc new element failed!!!");
        }
    }

    if (!p_hrp_element)
    {
        return false; /* data could not be handled, please re-try later...      */
    }

    /* trigger HRP statemachine                                               */
    hrp_lib_trigger_hrp_proccess(p_hrp_lib);

    return true;
}
void hrp_lib_trigger_hrp_proccess(P_HRP_LIB p_hrp_lib)
{
    if (p_hrp_lib->Status >= HRPLibStatusBusy) /* off-sync or re-entrant          */
    {
        return;
    }
    else                                /* now we are busy                    */
    {
        p_hrp_lib->Status = HRPLibStatusBusy;
    }

    while (hrp_lib_trigger(p_hrp_lib));

    if (p_hrp_lib->Status == HRPLibStatusBusy) /* might be off-sync               */
    {
        p_hrp_lib->Status = HRPLibStatusIdle;
    }
}
static bool hrp_lib_trigger(P_HRP_LIB p_hrp_lib)
{
    uint16_t        len_field;
    uint32_t        info_field;
    uint16_t        opt_len;
    uint16_t        crc16;
    uint8_t         module_id;
    uint8_t         cmd_group;
    uint16_t        cmd_entry;
    uint16_t        param_len;
    uint8_t         *p_param;
    /* if we have no assembly buffer => try to get one */
    if (!p_hrp_lib->HRPDataCollected && !p_hrp_lib->HRPMsgLength)
    {
        p_hrp_lib->pHRPMsg = bt_hrp_tgt_assembly_buffer_alloc(p_hrp_lib->AppHandle);

        if (!p_hrp_lib->pHRPMsg)
        {
            return false; /* no re-trigger */
        }
    }
    /* if we have no LTP data element to work with => try to get one */
    if (!p_hrp_lib->pActiveElement)
    {
        p_hrp_lib->pActiveElement = (P_HRP_ElEMENT)hrp_queue_out(&p_hrp_lib->UsedElementQueue);

        if (!p_hrp_lib->pActiveElement)
        {
            return false; /* no re-trigger */
        }
    }
    /* if we don't know how long the LTP msg is we try to assemble */
    /* ==> try to determine LTP msg length */
    if (!p_hrp_lib->HRPMsgLength)
    {
        uint16_t copy_length;
        if (p_hrp_lib->HRPDataCollected < HRP_DATA_MIN_HEADER_LENGTH) //header length is 7
        {
            copy_length = HRP_DATA_MIN_HEADER_LENGTH - p_hrp_lib->HRPDataCollected;
            if (bt_hrp_transfer_hrp_element_data(p_hrp_lib, copy_length))
            {
                return true; /* not enough data => re-trigger*/
            }
        }
        //lenfield, inforfield
        LE_ARRAY_TO_UINT16(len_field, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + 1 ]);
        LE_ARRAY_TO_UINT32(info_field, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + 3]);
        opt_len = len_field - 4;
        if (p_hrp_lib->HRPDataCollected < (HRP_DATA_MIN_HEADER_LENGTH + opt_len))
        {
            copy_length = (HRP_DATA_MIN_HEADER_LENGTH + opt_len) - p_hrp_lib->HRPDataCollected;
            if (bt_hrp_transfer_hrp_element_data(p_hrp_lib, copy_length))
            {
                return true; // not enough data => re-trigger
            }
        }
        switch (info_field & HRP_OPT_MASK_HEADER_CRC)
        {
        case 0x03:   //crc16 &crc 8
            {
                LE_ARRAY_TO_UINT16(crc16, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + HRP_DATA_MIN_HEADER_LENGTH +
                                                                                     opt_len - 2]);
                if (bt_hrp_tgt_do_crc8(p_hrp_lib, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart], 7)
                    != p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + HRP_DATA_MIN_HEADER_LENGTH ] ||
                    bt_hrp_tgt_do_crc16(&p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart], opt_len + 5)
                    != crc16)
                {
                    //hrp_lib_send_internal_event_info(p_hrp_lib, 0, NULL, HRP_CAUSE_CONNECTION_LOST,
                    //                                 HRP_INTERNAL_EVENT_COMMUNICATION_OUT_OF_SYNC, HRP_GENERATE_EVENT_ID);
                    bt_hrp_crc_error(p_hrp_lib);
                    return true;
                }
            }
            break;
        case 0x02:   //crc16
            {
                LE_ARRAY_TO_UINT16(crc16, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + HRP_DATA_MIN_HEADER_LENGTH +
                                                                                     opt_len - 2]);
                if (bt_hrp_tgt_do_crc16(&p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart], opt_len + 5) != crc16)
                {
                    //hrp_lib_send_internal_event_info(p_hrp_lib, 0, NULL, HRP_CAUSE_CONNECTION_LOST,
                    //                                 HRP_INTERNAL_EVENT_COMMUNICATION_OUT_OF_SYNC, HRP_GENERATE_EVENT_ID);
                    bt_hrp_crc_error(p_hrp_lib);
                    return true;
                }
            }
            break;
        case 0x01: //crc8
            {
                if (bt_hrp_tgt_do_crc8(p_hrp_lib, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart], 7)
                    != p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + HRP_DATA_MIN_HEADER_LENGTH ])
                {
                    //hrp_lib_send_internal_event_info(p_hrp_lib, 0, NULL, HRP_CAUSE_CONNECTION_LOST,
                    //                                 HRP_INTERNAL_EVENT_COMMUNICATION_OUT_OF_SYNC, HRP_GENERATE_EVENT_ID);
                    bt_hrp_crc_error(p_hrp_lib);
                    return true;
                }
            }
            break;
        default:
            break;
        }
        p_hrp_lib->HRPMsgLength = opt_len + HRP_DATA_MIN_HEADER_LENGTH;
    }
    /* wait for message completed */
    if (p_hrp_lib->HRPMsgLength > p_hrp_lib->HRPDataCollected)
    {
        /* try to complete LTP message */
        if (bt_hrp_transfer_hrp_element_data(p_hrp_lib,
                                             p_hrp_lib->HRPMsgLength - p_hrp_lib->HRPDataCollected))
        {
            return true; /* not enough data => re-trigger */
        }
    }
    /*------------------------------------------------------------------------*/
    /* message is completed => process it                                     */
    /*------------------------------------------------------------------------*/
    LE_ARRAY_TO_UINT16(len_field, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + 1 ]);
    LE_ARRAY_TO_UINT32(info_field, &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + 3]);

    module_id = info_field >> 8;
    module_id &= 0x3f;

    switch (info_field & HRP_OPT_MASK_HEADER_CRC8)
    {

    case 0x01: //crc8
        {
            p_param = &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + 7 + 1];
        }
        break;
    default: //no crc8
        {
            p_param = &p_hrp_lib->pHRPMsg[p_hrp_lib->HRPMsgStart + 7];
        }
        break;
    }
    LE_ARRAY_TO_UINT8(cmd_group, p_param);
    p_param += 1;
    LE_ARRAY_TO_UINT16(cmd_entry, p_param);
    p_param += 2;
    LE_ARRAY_TO_UINT16(param_len, p_param);
    p_param += 2;

    hrp_handle_downstream_req((HRP_MODULE_ID)module_id, cmd_group, cmd_entry, param_len, p_param);

    bt_hrp_init_hrp_assembly(p_hrp_lib, true);
    return true;
}
bool hrp_lib_send_internal_event_info(P_HRP_LIB p_hrp_lib, uint8_t copmsk, uint8_t *p_opt,
                                      uint8_t cause,
                                      uint8_t event_type, uint32_t event_info)
{
    uint16_t   pos     = 0;
    uint8_t *p_buffer = bt_hrp_tgt_send_buffer_alloc(p_hrp_lib->AppHandle, 4);

    if (!p_buffer)
    {
        return true;
    }

    p_buffer[pos++] = (uint8_t)cause;
    p_buffer[pos++] = (uint8_t)event_type;
    NETLONG2CHAR(&p_buffer[pos], event_info);
    pos += 4;

    return bt_hrp_tgt_send_hrp_message(p_hrp_lib->AppHandle, p_buffer, 0,
                                       pos);

}
static void bt_hrp_crc_error(P_HRP_LIB pLTPLib)
{
    uint16_t invalidCount = 1;
    APP_PRINT_ERROR1("bt_hrp_crc_error cut 0x%x", pLTPLib->pHRPMsg[pLTPLib->HRPMsgStart]);
    while (pLTPLib->pHRPMsg[pLTPLib->HRPMsgStart + invalidCount] != 0x5A &&
           invalidCount < pLTPLib->HRPDataCollected)
    {
        //APP_PRINT_ERROR1("bt_hrp_crc_error cut 0x%x", pLTPLib->pHRPMsg[pLTPLib->HRPMsgStart + invalidCount]);
        invalidCount++;
    }
    APP_PRINT_ERROR1("bt_hrp_crc_error cut %d byte", invalidCount);
    pLTPLib->HRPMsgStart += invalidCount;
    pLTPLib->HRPDataCollected -= invalidCount;

    pLTPLib->HRPMsgLength = 0;
}

static bool bt_hrp_transfer_hrp_element_data(P_HRP_LIB pHRPLib, uint16_t copyLength)
{
    bool        retVal = false;     /* ==> all requested data copied */
    P_HRP_ElEMENT pActiveElement = pHRPLib->pActiveElement;

    if (pActiveElement == NULL)
    {
        return true;    /* ==> new queue element required                       */
    }

    if (copyLength > pActiveElement->DataCB.Length)
    {
        copyLength = pActiveElement->DataCB.Length;
        retVal = true;    /* ==> not enough data in element!!!!!!                     */
    }
    /*find the sync data*/

    if (pHRPLib->HRPDataCollected)
    {
        while (0x5A != pHRPLib->pHRPMsg[pHRPLib->HRPMsgStart] && pHRPLib->HRPDataCollected)
        {
            pHRPLib->HRPMsgStart += 1;
            pHRPLib->HRPDataCollected -= 1;
        }
        if (0 == pHRPLib->HRPDataCollected)
        {
            pHRPLib->HRPMsgStart = 0;
        }
    }
    if (!pHRPLib->HRPDataCollected)
    {
        while (0x5A != pActiveElement->DataCB.BufferAddress[pActiveElement->DataCB.Offset] &&
               pActiveElement->DataCB.Length)
        {
            pActiveElement->DataCB.Offset++;
            pActiveElement->DataCB.Length--;
        }
    }

    if (pActiveElement->DataCB.Length)
    {
        /* copy element data to msg buffer                                        */
        memcpy(&pHRPLib->pHRPMsg[pHRPLib->HRPMsgPos],
               &pActiveElement->DataCB.BufferAddress[pActiveElement->DataCB.Offset],
               copyLength
              );

        /* updata data structures for element and msg buffer                      */
        pHRPLib->HRPDataCollected += copyLength;
        pHRPLib->HRPMsgPos        += copyLength;

        pActiveElement->DataCB.Offset += copyLength;
        pActiveElement->DataCB.Length -= copyLength;
    }

    /* check if element is consumed completely                                */
    else
    {
        bt_hrp_tgt_receive_buffer_release(pHRPLib->AppHandle, pActiveElement->DataCB.BufferAddress);

        bt_hrp_tgt_queue_element_release(pHRPLib->AppHandle, pActiveElement);

        pHRPLib->pActiveElement = NULL;
    }

    return retVal;
}

bool hrp_handle_downstream_req(HRP_MODULE_ID module_id, uint8_t cmd_group,
                               uint16_t cmd_index, uint16_t param_list_len, uint8_t *p_param_list)
{
    if (P_AciConfig->ltp_trace_level >= LTP_TRACE_DEBUG)
    {
        APP_PRINT_INFO4("hrp downstream: module %d cmd_group 0x%x cmd_index 0x%x param_len %d",
                        module_id, cmd_group, cmd_index, param_list_len);
    }

    switch (module_id)
    {
    case HRP_MODULE_LOWER_STACK:
        break;

    case HRP_MODULE_UPPER_STACK:
#if F_BT_LE_BTIF_SUPPORT
        hrp_btif_handle_req(cmd_group, cmd_index, param_list_len, p_param_list);
#endif
        break;

    case HRP_MODULE_PROFILE:
        hrp_profile_handle_req(cmd_group, cmd_index, param_list_len, p_param_list);
        break;

    case HRP_MODULE_AUDIO:
        break;

    case HRP_MODULE_BLE_AUTO_TEST:
        hrp_gap_ble_handle_req(cmd_group, cmd_index, param_list_len, p_param_list);
        break;

    default:
        break;
    }
    return true;
}

HRP_SEQ_ID g_hrp_seq_id = {0};
bool hrp_handle_upperstream_events(HRP_MODULE_ID module_id, uint8_t cmd_group,
                                   uint16_t cmd_index, uint16_t param_list_len, uint8_t *p_param_list)
{
    //add hrp header
    uint8_t       *p_buffer;
    uint16_t        pos = 0;
    uint8_t       sync = 0x5A;
    uint16_t       len_field;
    uint32_t     info_rfield;
    uint16_t     buffer_Len;

    if (P_AciConfig->ltp_trace_level >= LTP_TRACE_DEBUG)
    {
        APP_PRINT_INFO4("hrp upstream: module %d cmd_group 0x%x cmd_index 0x%x param_len %d",
                        module_id, cmd_group, cmd_index, param_list_len);
    }

    info_rfield = g_hrp_seq_id.crc8 | g_hrp_seq_id.crc16 << 1 | g_hrp_seq_id.seq_id << 2 | module_id <<
                  8;
    //crc8 &crc16
    if (g_hrp_seq_id.crc8 && g_hrp_seq_id.crc16)
    {
        buffer_Len = HRP_DATA_MIN_HEADER_LENGTH + 3 + 5 + param_list_len;
        len_field = 4 + 1 + 2 + 5 + param_list_len;
    }
    //crc8
    else if (g_hrp_seq_id.crc8 && !g_hrp_seq_id.crc16)
    {
        buffer_Len = HRP_DATA_MIN_HEADER_LENGTH + 1 + 5 + param_list_len;
        len_field = 4 + 1 + 5 + param_list_len;

    }
    //crc16
    else if (!g_hrp_seq_id.crc8 && g_hrp_seq_id.crc16)
    {
        buffer_Len = HRP_DATA_MIN_HEADER_LENGTH + 2 + 5 + param_list_len;
        len_field = 4 + 2 + 5 + param_list_len;

    }
    //no crc
    else
    {
        buffer_Len = HRP_DATA_MIN_HEADER_LENGTH + 5 + param_list_len;
        len_field = 4 + 5 + param_list_len;
    }

    p_buffer = bt_hrp_tgt_send_buffer_alloc(0, buffer_Len);
    if (!p_buffer)
    {
        APP_PRINT_ERROR0("hrp_handle_upperstream_events: buffer allocated failed");
        return false;
    }

    p_buffer[pos++] = sync;

    LE_UINT16_TO_ARRAY(&p_buffer[pos], len_field);
    pos += 2;

    LE_UINT32_TO_ARRAY(&p_buffer[pos], info_rfield);
    pos += 4;

    g_hrp_seq_id.seq_id += 1;
    if (g_hrp_seq_id.seq_id >= 32)
    {
        g_hrp_seq_id.seq_id = 0;
    }
    if (g_hrp_seq_id.crc8)
    {
        p_buffer[pos++] = bt_hrp_tgt_do_crc8(0, &p_buffer[0], HRP_DATA_MIN_HEADER_LENGTH);
    }
    p_buffer[pos++] = cmd_group;

    LE_UINT16_TO_ARRAY(&p_buffer[pos], cmd_index);
    pos += 2;
    LE_UINT16_TO_ARRAY(&p_buffer[pos], param_list_len);
    pos += 2;

    memcpy(&p_buffer[pos], p_param_list, param_list_len);
    pos += param_list_len;


    if (g_hrp_seq_id.crc16)
    {
        if (g_hrp_seq_id.crc8)
        {
            LE_UINT16_TO_ARRAY(&p_buffer[pos], bt_hrp_tgt_do_crc16(&p_buffer[0],
                                                                   HRP_DATA_MIN_HEADER_LENGTH + 1 + 5 + param_list_len));
        }
        else
        {
            LE_UINT16_TO_ARRAY(&p_buffer[pos], bt_hrp_tgt_do_crc16(&p_buffer[0],
                                                                   HRP_DATA_MIN_HEADER_LENGTH + 5 + param_list_len));
        }
        pos += 2;
    }

    return (bt_hrp_tgt_send_hrp_message(0, p_buffer, 0, pos));
}


