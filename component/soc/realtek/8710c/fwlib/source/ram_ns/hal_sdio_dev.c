/**************************************************************************//**
 * @file     hal_sdio_dev.c
 * @brief    This SDIO device HAL API functions.
 *
 * @version  V1.00
 * @date     2017-09-20
 *
 * @note
 *
 ******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/
#include "hal_cache.h"
#include "hal_gpio.h"
#include "hal_irq.h"
#include "hal_pinmux.h"
#include "osdep_service.h"
#include "hal_sdio_dev.h"

#if CONFIG_SDIO_DEVICE_EN

/**
 * @addtogroup hs_hal_sdio_dev
 * @{
 * @brief The SDIO Device HAL API.
 */

#if SDIO_API_DEFINED
#include "spdio_api.h"
#endif

#if CONFIG_INIC_EN
#include "freertos_pmu.h"
#include "skbuff.h"
#include "osdep_service.h"
#include "wrapper.h"
extern struct sk_buff *rltk_wlan_alloc_skb(unsigned int total_len);
extern unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
extern void inic_sdio_free_data(unsigned char *data);
#if (CONFIG_INIC_SKB_TX == 0) //pre-allocated memory for SDIO TX BD
//__ALIGNED(4) char inic_TX_Buf[SDIO_TX_BD_NUM][SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT];
#endif
#endif

const uint8_t sdio_dev_pins[] = {
    PIN_SDIO_D2,  PIN_SDIO_D3, PIN_SDIO_CMD,
    PIN_SDIO_CLK, PIN_SDIO_D0, PIN_SDIO_D1,
    PIN_SDIO_INT, 0xFF
};

#if !SDIO_NO_RTOS
_sema               sdiod_tx_sema;            /*!< Semaphore for SDIO TX, use to wakeup the SDIO TX task */
_sema               sdiod_rx_sema;            /*!< Semaphore for SDIO RX, use to wakeup the SDIO RX task */
struct task_struct  sdiod_tx_task;            /*!< The Task for TX */
struct task_struct  sdiod_rx_task;            /*!< The Task for RX */
_mutex              sdiod_rx_mutex;           /*!< The Mutex to protect RxPktList */
_mutex              sdiod_gmutex;             /*!< The Mutex to protect SDIO device HAL global resource */
_xqueue             sdiod_msg_queue;          /*!< The message quue for other driver module to send message to the SDIO HAL driver */
#endif

NC_BSS_SECTION  sdiod_txbd_t sdiod_tx_bd[SDIO_TX_BD_NUM] __ALIGNED(32);
NC_BSS_SECTION  sdiod_rxbd_t sdiod_rx_bd[SDIO_RX_BD_NUM] __ALIGNED(32);

int8_t hal_sdio_dev_msg_handler (hal_sdio_dev_adapter_t *psdio_adp, sdiod_msg_blk_t *pmblk);
void hal_sdio_dev_free_rx_pkt(hal_sdio_dev_adapter_t *psdio_adp, sdiod_rx_packet_t *ppkt);

#if defined(SDIO_API_DEFINED) && (SDIO_API_DEFINED != 0)
extern struct spdio_t *g_spdio_priv;    // the spdio object of the spdio API
#endif

hal_sdio_dev_adapter_t *g_sdio_adp;

#if defined (SDIO_API_DEFINED) && (SDIO_API_DEFINED != 0)
// for SPDIO API
hal_status_t hal_sdio_txbd_hdl_init (sdiod_txbd_hdl_t *ptxbd_hdl, uint16_t txbd_idx)
{
    hal_status_t ret = HAL_OK;
    
    // Pre-allocate buffer by User
    ptxbd_hdl->priv = (void *)&g_spdio_priv->rx_buf[txbd_idx];
    ptxbd_hdl->ptxbd->addr = (uint32_t)g_spdio_priv->rx_buf[txbd_idx].buf_addr;
    if (ptxbd_hdl->ptxbd->addr == NULL) {
        DBG_SDIO_DEV_ERR("RX buffer resource must be inited\n");
        ret = HAL_ERR_MEM;
    }
    
    if ((ptxbd_hdl->ptxbd->addr & 0x03) != 0) {
        DBG_SDIO_DEV_ERR("RX buffer address must be aligned to 4!!\n");
        ret = HAL_ERR_PARA;
    }

    return ret;
}

void hal_sdio_txbd_hdl_deinit (sdiod_txbd_hdl_t *ptxbd_hdl, uint16_t txbd_idx)
{
    DBG_SDIO_DEV_ERR("RX buffer resource must be inited\n");
}

void hal_sdio_txbd_buf_do_refill (sdiod_txbd_hdl_t *ptxbd_hdl)
{
    struct spdio_buf_t *sdio_buf = (struct spdio_buf_t *)ptxbd_hdl->priv;
    
    ptxbd_hdl->ptxbd->addr = sdio_buf->buf_addr;
    DBG_SDIO_DEV_INFO("pTxBdHdl->pTXBD->Address: 0x%x\n", ptxbd_hdl->ptxbd->addr);
}

int8_t hal_sdio_txbd_rdy_callback(hal_sdio_dev_adapter_t *psdio_adp, sdiod_txbd_hdl_t *ptxbd_hdl, 
                                  sdiod_tx_desc_t *ptx_desc)
{
    // packet data includes TX Desc
    return psdio_adp->tx_callback (psdio_adp->ptxcb_para, (uint8_t *)ptxbd_hdl->priv, ptx_desc->offset,
                                  ptx_desc->txpktsize, ptx_desc->type);    
}

void hal_sdio_rxbd_tr_done_callback (hal_sdio_dev_adapter_t *psdio_adp, sdiod_rxbd_hdl_t *prxbd_hdl)
{
    sdiod_rx_packet_t *ppkt;
    sdiod_rx_desc_t *prx_desc;

    ppkt = prxbd_hdl->ppkt;
    prx_desc = ppkt->prx_desc;

    if(psdio_adp->rx_done_callback != NULL) {
        psdio_adp->rx_done_callback (psdio_adp->prx_done_cb_para, ppkt->priv,\
            ppkt->offset, prx_desc->pkt_len, prx_desc->type);
    }
    ppkt->pdata = NULL;
    ppkt->priv = NULL;
}

#elif defined (CONFIG_INIC_EN) && (CONFIG_INIC_EN != 0)
// for INIC
hal_status_t hal_sdio_txbd_hdl_init (sdiod_txbd_hdl_t *ptxbd_hdl, uint16_t txbd_idx)
{
    hal_status_t ret = HAL_OK;
    
#if CONFIG_INIC_SKB_TX
    // allocate wlan skb here
    ptxbd_hdl->skb = rltk_wlan_alloc_skb (g_sdio_adp->tx_bd_buf_size);
    DBG_SDIO_DEV_INFO("SDIO_Device_Init: pTxBdHdl->pkt @ 0x%x\n", ptxbd_hdl->skb);
    if (ptxbd_hdl->skb) {
        ptxbd_hdl->ptxbd->addr = (uint32_t)ptxbd_hdl->skb->tail;
    } else {
        DBG_SDIO_DEV_ERR("SDIO_Device_Init: rltk_wlan_alloc_skb (%d) failed!!\n", 
            g_sdio_adp->tx_bd_buf_size);
        ret = HAL_ERR_MEM;
    }
#else
    ptxbd_hdl->ptxbd->addr = (uint32_t)(&inic_TX_Buf[txbd_idx][0]);
#endif

    return ret;
}

void hal_sdio_txbd_hdl_deinit (sdiod_txbd_hdl_t *ptxbd_hdl, uint16_t txbd_idx)
{
#if CONFIG_INIC_SKB_TX
    //free wlan skb here
    dev_kfree_skb_any(ptxbd_hdl->skb);
#endif
    ptxbd_hdl->ptxbd->addr =(uint32_t)NULL;
}

void hal_sdio_txbd_buf_do_refill (sdiod_txbd_hdl_t *ptxbd_hdl)
{
#if CONFIG_INIC_SKB_TX//or use bss for tx bd, not need to re-allocate memory
    //allocate wlan skb for each TX BD
    ptxbd_hdl->skb = rltk_wlan_alloc_skb (SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT);
    if (ptxbd_hdl->skb) {
        ptxbd_hdl->ptxbd->addr = (uint32_t)ptxbd_hdl->skb->tail;
    } else {
        ptxbd_hdl->ptxbd->addr = (uint32_t)NULL;
    }
#endif
}

int8_t hal_sdio_txbd_rdy_callback(hal_sdio_dev_adapter_t *psdio_adp, sdiod_txbd_hdl_t *ptxbd_hdl, 
                                  sdiod_tx_desc_t *ptx_desc)
{
#if CONFIG_INIC_SKB_TX
    // packet data includes TX Desc
    return psdio_adp->tx_callback (psdio_adp->ptxcb_para, (uint8_t *)ptxbd_hdl->skb, ptx_desc->offset,
                                  ptx_desc->txpktsize, ptx_desc->type);
#else
    // packet data includes TX Desc
    return psdio_adp->tx_callback (psdio_adp->ptxcb_para, (uint8_t *)ptxbd_hdl->ptxbd->addr,
                                  ptx_desc->offset, ptx_desc->txpktsize, ptx_desc->type);
#endif
}

void hal_sdio_rxbd_tr_done_callback (hal_sdio_dev_adapter_t *psdio_adp, sdiod_rxbd_hdl_t *prxbd_hdl)
{
    sdiod_rx_packet_t *ppkt;
    sdiod_rx_desc_t *prx_desc;

    ppkt = prxbd_hdl->ppkt;
    prx_desc = ppkt->prx_desc;

    if(psdio_adp->rx_done_callback != NULL) {
        psdio_adp->rx_done_callback (psdio_adp->prx_done_cb_para, (void *)ppkt->pdata,\
            ppkt->offset, prx_desc->pkt_len, prx_desc->type);
    }
#if CONFIG_INIC_SKB_RX
    dev_kfree_skb_any(ppkt->skb);
    ppkt->skb = NULL;
#else
    inic_sdio_free_data((uint8_t *) (ppkt->pdata));
#endif
    ppkt->pdata = NULL;

}

#else   // else of "#elif defined (CONFIG_INIC_EN) && (CONFIG_INIC_EN != 0)"

// for Regular SDIO device
hal_status_t hal_sdio_txbd_hdl_init (sdiod_txbd_hdl_t *ptxbd_hdl, uint16_t txbd_idx)
{
    hal_status_t ret = HAL_OK;
    
    // Allocate buffer for each TX BD
    ptxbd_hdl->ptxbd->addr = (uint32_t)rtw_malloc(g_sdio_adp->tx_bd_buf_size);
    if (ptxbd_hdl->ptxbd->addr == 0) {
        ret = HAL_ERR_MEM;
    }

    return ret;
}

void hal_sdio_txbd_hdl_deinit (sdiod_txbd_hdl_t *ptxbd_hdl, uint16_t txbd_idx)
{
    if (ptxbd_hdl->ptxbd->addr != 0) {
        rtw_mfree((uint8_t *)ptxbd_hdl->ptxbd->addr, (g_sdio_adp->tx_bd_buf_size));
        ptxbd_hdl->ptxbd->addr =(uint32_t)NULL;                    
    }    
}

void hal_sdio_txbd_buf_do_refill (sdiod_txbd_hdl_t *ptxbd_hdl)
{
    ptxbd_hdl->ptxbd->addr = (uint32_t)rtw_malloc (SDIO_TX_BD_BUF_USIZE * SDIO_TX_BUF_SZ_UNIT);
    if (ptxbd_hdl->ptxbd->addr == 0) {
        DBG_SDIO_DEV_WARN("%s Alloc Mem(size=%d) Failed\n", __func__, 
            SDIO_TX_BD_BUF_USIZE*SDIO_TX_BUF_SZ_UNIT);
        rtw_msleep_os(10);  // sleep to wait other task to free memory
    }
}

int8_t hal_sdio_txbd_rdy_callback(hal_sdio_dev_adapter_t *psdio_adp, sdiod_txbd_hdl_t *ptxbd_hdl, 
                                  sdiod_tx_desc_t *ptx_desc)
{
    // packet data includes TX Desc
    if (psdio_adp->tx_callback) {
        return psdio_adp->tx_callback (psdio_adp->ptxcb_para, (uint8_t *)ptxbd_hdl->ptxbd->addr,
                                      ptx_desc->offset, ptx_desc->txpktsize, ptx_desc->type);
    } else {
        DBG_SDIO_DEV_ERR("No callback for TX packet ready!!\r\n");
        return 0;
    }
}

void hal_sdio_rxbd_tr_done_callback (hal_sdio_dev_adapter_t *psdio_adp, sdiod_rxbd_hdl_t *prxbd_hdl)
{
    sdiod_rx_packet_t *ppkt;
    sdiod_rx_desc_t *prx_desc;
	uint32_t pkt_size;

    ppkt = prxbd_hdl->ppkt;
    prx_desc = ppkt->prx_desc;
    pkt_size = prx_desc->pkt_len;

    if(psdio_adp->rx_done_callback != NULL) {
        psdio_adp->rx_done_callback (psdio_adp->prx_done_cb_para, (void *)ppkt->pdata,\
            ppkt->offset, prx_desc->pkt_len, prx_desc->type);
    }

    rtw_mfree((uint8_t *) (ppkt->pdata), (ppkt->offset + pkt_size));    // free packet buffer
}

#endif  // end of else of "#elif defined (CONFIG_INIC_EN) && (CONFIG_INIC_EN != 0)"

#if !defined(SDIO_NO_RTOS) || (defined(SDIO_NO_RTOS) && (SDIO_NO_RTOS==0))
uint32_t hal_sdio_dev_tx_task_down (void *arg)
{
    uint32_t ret;
    
    (void) arg;
    
    ret = rtw_down_sema (&sdiod_tx_sema);
    return ret;
}

int32_t hal_sdio_dev_tx_task_up (void *arg)
{
    (void) arg;

    if (__get_IPSR () == 0U) {
        // Not in an ISR
        rtw_up_sema (&sdiod_tx_sema);
    } else {
        // In an ISR
        rtw_up_sema_from_isr (&sdiod_tx_sema);
    }    

    return 0;
}

uint32_t hal_sdio_dev_rx_task_down (void *arg)
{
    uint32_t ret;
    
    (void) arg;
    
    ret = rtw_down_sema (&sdiod_rx_sema);
    return ret;
}

int32_t hal_sdio_dev_rx_task_up (void *arg)
{
    (void) arg;

    if (__get_IPSR () == 0U) {
        // Not in an ISR
        rtw_up_sema (&sdiod_rx_sema);
    } else {
        // In an ISR
        rtw_up_sema_from_isr (&sdiod_rx_sema);
    }    

    return 0;
}

void hal_sdio_task_exit(void)
{
    rtw_thread_exit();
}

int hal_sdio_dev_pop_msg_queue ( void *arg, void* message)
{
    return rtw_pop_from_xqueue (&sdiod_msg_queue, message, 0);
}

void hal_sdio_rx_lock (void)
{
    rtw_mutex_get (&sdiod_rx_mutex);    
}

void hal_sdio_rx_unlock (void)
{
    rtw_mutex_put (&sdiod_rx_mutex);
}

void hal_sdio_os_wait_tx_bd (void)
{
    // sleep & wait free TX BD buffer
    rtw_msleep_os(20);
}

void hal_sdio_os_wait_rx_bd (void)
{
    // sleep & wait free RX BD
    rtw_msleep_os(1);
}

hal_status_t hal_sdio_dev_os_init (hal_sdio_dev_adapter_t *psdio_adp, void *tx_task, void *rx_task)
{
	int os_ret;

    rtw_init_sema(&sdiod_tx_sema, 0);
    if (sdiod_tx_sema == NULL){
        DBG_SDIO_DEV_ERR("SDIO_Device_Init Create Semaphore Err!!\n");
        return HAL_ERR_OS;
    }

    rtw_init_sema(&sdiod_rx_sema, 0);
    if (sdiod_rx_sema == NULL){
        DBG_SDIO_DEV_ERR("SDIO_Device_Init Create RX Semaphore Err!!\n");
    }

    /* create a message queue for other driver module to send message to the SDIO HAL driver */
    if (SUCCESS != rtw_init_xqueue(&sdiod_msg_queue, "SDIOD_MsgQ", sizeof(sdiod_msg_blk_t), SDIO_MSG_QUEUE_SIZE)) {
        DBG_SDIO_DEV_ERR("SDIO_Device_Init Create Msg Q Err!!\n");
        return HAL_ERR_OS;
    }
    
    // SDIO device driver is running in RTOS task
    os_ret = rtw_create_task (&sdiod_tx_task, "SDIO_TX_TASK", (SDIO_TX_TASK_STACK_SIZE >> 2), SDIO_TX_TASK_PRIORITY, 
                              (thread_func_t)tx_task, (void *)psdio_adp);
    if (!os_ret) {
        DBG_SDIO_DEV_ERR("SDIO_Device_Init: Create TX Task Err(%d)!!\n", os_ret);
        return HAL_ERR_OS;
    }

    os_ret = rtw_create_task (&sdiod_rx_task, "SDIO_RX_TASK", (SDIO_RX_TASK_STACK_SIZE >> 2), SDIO_RX_TASK_PRIORITY, 
                              (thread_func_t)rx_task, (void *)psdio_adp);
    if (!os_ret) {
        DBG_SDIO_DEV_ERR("SDIO_Device_Init: Create RX Task Err(%d)!!\n", os_ret);
        return HAL_ERR_OS;
    }

	rtw_mutex_init(&sdiod_rx_mutex);

    rtw_mutex_init(&sdiod_gmutex);

    // Hook OS API for ROM code    
    psdio_adp->tx_task_down = hal_sdio_dev_tx_task_down;
    psdio_adp->tx_task_up = hal_sdio_dev_tx_task_up;
    psdio_adp->rx_task_down = hal_sdio_dev_rx_task_down;
    psdio_adp->rx_task_up = hal_sdio_dev_rx_task_up;
    psdio_adp->pop_msg_queue = hal_sdio_dev_pop_msg_queue;
    psdio_adp->rx_lock = hal_sdio_rx_lock;
    psdio_adp->rx_unlock = hal_sdio_rx_unlock;
    psdio_adp->os_wait_tx_bd = hal_sdio_os_wait_tx_bd;
    psdio_adp->os_wait_rx_bd = hal_sdio_os_wait_rx_bd;
    psdio_adp->os_wait = (sdiod_os_wait_t)rtw_msleep_os;
    
    return HAL_OK;
}

hal_status_t hal_sdio_dev_os_deinit (hal_sdio_dev_adapter_t *psdio_adp)
{
    if (sdiod_tx_sema) {
        rtw_free_sema(&sdiod_tx_sema);
        sdiod_tx_sema = (_sema)NULL;
    }

    if (sdiod_rx_sema) {
        rtw_free_sema(&sdiod_rx_sema);
        sdiod_rx_sema = (_sema)NULL;
    }

    if (sdiod_rx_mutex) {
        rtw_mutex_free(&sdiod_rx_mutex);
        sdiod_rx_mutex = (_mutex)NULL;
    }

    if (sdiod_gmutex) {
        rtw_mutex_free(&sdiod_gmutex);
        sdiod_gmutex = (_mutex)NULL;
    }

    if (sdiod_msg_queue != NULL) {
        rtw_deinit_xqueue (&sdiod_msg_queue);
        sdiod_msg_queue = NULL;
    }

    return HAL_OK;
}

#endif  // end of "#if !defined(SDIO_NO_RTOS) || (defined(SDIO_NO_RTOS) && (SDIO_NO_RTOS==0))"

/**
  \brief   D-Cache Clean by address for SDIO device HAL code
  \details Cleans D-Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
void hal_sdio_dev_dcache_clean_by_addr (uint32_t *addr, int32_t dsize)
{
#if 1
    dcache_clean_by_addr ((uint32_t *)((uint32_t)addr & 0xFFFFFFE0UL),
        (int32_t)dsize + (((uint32_t)addr) & 0x1FUL));
#else
    dcache_clean_by_addr (addr, dsize);
#endif
}

/**
  \brief   D-Cache Invalidate by address
  \details Invalidates D-Cache for the given address
  \param[in]   addr    address (aligned to 32-byte boundary)
  \param[in]   dsize   size of memory block (in number of bytes)
*/
void hal_sdio_dev_dcache_invalidate_by_addr (uint32_t *addr, int32_t dsize)
{
#if 1
    dcache_invalidate_by_addr ((uint32_t *)((uint32_t)addr & 0xFFFFFFE0),
            (int32_t)((uint32_t)dsize + (((uint32_t)addr) & 0x0000001F)));
#else
    dcache_invalidate_by_addr (addr, dsize);
#endif
}

/**
 *  @brief Handles the CMD11 command. The SDIO Reset command will trigger
 *         a interrupt and then cause this function be called. This function will 
 *         change the SDIO IO pin voltage from 3.3v to 1.8v.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     void.
 */
void hal_sdio_dev_cmd11_handle (hal_sdio_dev_adapter_t *psdio_adp)
{
    uint32_t i;
        
    for (i=0; i<8; i++) {
        if (sdio_dev_pins[i] == 0xFF) {
            break;
        }
        hal_gpio_schmitt_ctrl (sdio_dev_pins[i], 0, IO_1p8V);  // switch IO pad as 3.3V
        hal_gpio_drive_ctrl (sdio_dev_pins[i], 3); // Max IO pad driving current
    }
    hal_gpio_schmitt_ctrl (PIN_SDIO_CLK, 1, IO_1p8V);  // Enable Smith trigger on SDIO_CLK pin 
}

/**
 *  @brief Handles the SDIO Reset command. The SDIO Reset command will trigger
 *         a interrupt and then cause this function be called. We do nothing
 *         for the SDIO Reset command.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     void.
 */
void hal_sdio_dev_reset_cmd (hal_sdio_dev_adapter_t *psdio_adp)
{
	// TODO:
}

/**
 *  @brief Handles the SDIO host power management command.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     always return 0.
 */
uint8_t hal_sdio_dev_process_rpwm (hal_sdio_dev_adapter_t *psdio_adp)
{
	sdio_dev_crpwm_t crpwm;
    SDIO_DEV_Type *sdio_dev = SDIO_DEV;

	crpwm.w = sdio_dev->crpwm;

	DBG_SDIO_DEV_INFO ("RPWM1: 0x%x\n", crpwm.w);
	// TODO: forward this RPWM message to WLan
	return 0;
}


/**
 *  @brief Handles the SDIO host power management command. The SDIO host use
 *         this command to instructs the SDIO device to enter the power gated
 *         sleep mode. The option of the sleep mode is in the RPWM2 register.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     always return 0.
 */
uint8_t hal_sdio_dev_process_rpwm2 (hal_sdio_dev_adapter_t *psdio_adp)
{
	sdio_dev_crpwm2_t crpwm2;
    SDIO_DEV_Type *sdio_dev = SDIO_DEV;
//	uint16_t rpwm;
//    uint32_t reg_value;
//    PRAM_FUNCTION_START_TABLE pRamStartFun = (PRAM_FUNCTION_START_TABLE) __ram_start_table_start__;

	crpwm2.w = sdio_dev->crpwm2;

	DBG_SDIO_DEV_INFO ("RPWM2: 0x%x\n", crpwm2);

#ifdef CONFIG_SOC_PS_MODULE
	if ((crpwm2.b.toggle) != (psdio_adp->crpwm2.b.toggle)) {
        psdio_adp->crpwm2.w = crpwm2.w;
        // Tgoole bit changed, means it's a new RPWM command
        if ((crpwm2.w & RPWM2_ACT_BIT) == 0) {
            // request to enter sleep mode
            psdio_adp->ccpwm2.w = sdio_dev->ccpwm2;
            psdio_adp->ccpwm2.b.active = 0;

#if PURE_SDIO_INIC
//?            hal_sdio_dev_deinit (psdio_adp);
#endif
            if ((crpwm2.w & RPWM2_DSTANDBY_BIT) == 0) {
                psdio_adp->ccpwm2.b.dstandby = 0;
                if((crpwm2.w & RPWM2_CG_BIT)){
                    //enter clock gated state
                    psdio_adp->ccpwm2.b.toggle ^= 1;
                    sdio_dev->ccpwm2 = psdio_adp->ccpwm2.w;
#if CONFIG_INIC_EN
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
                    hal_sdio_dev_enter_sleepcg();
#endif
#endif
                } else {
                    // enter power gated state
                    if ((crpwm2.w & RPWM2_FBOOT_BIT)) {
                        psdio_adp->ccpwm2.b.fboot = 1;
                        // setup the trap to call the wakeup callback when booting
                        // TODO: setup fast reboot
#if 0
                        reg_value = HAL_READ32(PERI_ON_BASE, REG_SOC_FUNC_EN);
                        reg_value |= BIT(29);
                        HAL_WRITE32(PERI_ON_BASE, REG_SOC_FUNC_EN, reg_value);
                        // Assign the RAM start address after boot from wakeup
                        pRamStartFun->RamWakeupFun = SDIO_Wakeup_From_PG;
#endif
                    }
                    psdio_adp->ccpwm2.b.toggle ^= 1;
                    sdio_dev->ccpwm2 = psdio_adp->ccpwm2.w;
#if PURE_SDIO_INIC
                    // TODO: enter power gated
//                    SleepPG(SLP_SDIO, 0);
#endif
                }
            } else {
                // enter Deep Standby state
                psdio_adp->ccpwm2.b.dstandby = 1;
                psdio_adp->ccpwm2.b.fboot = 0;
                psdio_adp->ccpwm2.b.toggle ^= 1;
                sdio_dev->ccpwm2 = psdio_adp->ccpwm2.w;

#if PURE_SDIO_INIC
                // TODO: setup GPIO wakeup event to wakeup the SDIO device from deep standby state
#if 0
                {
                uint8_t gpio_option, i;
                uint16_t gpio_pin, gpio_en, gpio_act, gpio_lv;


                gpio_option = 0;
                gpio_pin = RPWM2_WKPIN_A5_BIT;
                gpio_lv = RPWM2_PIN_A5_LV_BIT;
                gpio_en = BIT0;
                gpio_act = BIT4;
                // Loop 4 to check 4 GPIO wake up event
                for (i=0;i<4;i++) {
                    if (rpwm & gpio_pin) {
                        gpio_option |= gpio_en;
                        if (rpwm &  gpio_lv) {
                            // Active High
                            gpio_option |= gpio_act;
                        }
                    }
                    gpio_pin = gpio_pin << 1;
                    gpio_lv = gpio_lv << 1;
                    gpio_en = gpio_en << 1;
                    gpio_act = gpio_act << 1;
                }

                DeepStandby(DSTBY_GPIO, 0, gpio_option);
                }
#endif
#endif
            }
#if !PURE_SDIO_INIC
// TODO: change the SDIO power state as INACTIVE and notify Power Management Unit
#if 0
            {
                REG_POWER_STATE SDIOPwrState;
                uint8_t HwState;

                SDIOPwrState.FuncIdx = SDIOD;
                QueryRegPwrState(SDIOD, &(SDIOPwrState.PwrState), &HwState);

                if (SDIOPwrState.PwrState == ACT) {
                    SDIOPwrState.PwrState = INACT;
                    RegPowerState(SDIOPwrState);
                }
            }
#endif
#endif
        } else {
#if !PURE_SDIO_INIC

#if CONFIG_INIC_EN
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
            pmu_acquire_wakelock(BIT(PMU_SDIO_DEVICE));
#endif
#endif

// TODO: change the SDIO power state as ACTIVE and notify Power Management Unit
#if 0
            // Request to Active SDIO iNIC
            REG_POWER_STATE SDIOPwrState;

            // Let the power management task know SDIO is in active
            SDIOPwrState.FuncIdx = SDIOD;
            SDIOPwrState.PwrState = ACT;
            RegPowerState(SDIOPwrState);
#endif
            psdio_adp->ccpwm2.b.active = 1;
            psdio_adp->ccpwm2.b.toggle ^= 1;
            sdio_dev->ccpwm2 = psdio_adp->ccpwm2.w;

#endif
        }
    }
#endif  // #ifdef CONFIG_SOC_PS_MODULE
	return 0;
}

/**
 *  @brief To initial the SDIO device hardware and the SDIO device HAL adapter.
 *         This function will allocate and initial needed resource:
 *           - Allocates SDIO TX FIFO buffer and initial TX related register.
 *           - Allocates SDIO RX Buffer Descriptor and RX Buffer.
 *           - Initials RX related registers.
 *           - Registers a interrupt handler.
 *           - Creates tasks for data TX/RX.
 *           - Allocates RTOS resource(semaphore and mutex).
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     the result of the execution:
 *                - HAL_OK: initialization success.
 *                - HAL_ERR_PARA: input data or parameters incorrect.
 *                - HAL_ERR_MEM: memory allocation failed.
 */
hal_status_t hal_sdio_dev_init (void)
{
    hal_status_t ret = HAL_OK;
    uint16_t tx_bd_num = 0;
    uint16_t rx_bd_num = 0;
    uint32_t i;
	sdiod_rx_packet_t *prx_pkt;

    for (i=0; i<8; i++) {
        if (sdio_dev_pins[i] == 0xFF) {
            break;
        }
        ret = hal_pinmux_register (sdio_dev_pins[i], PID_SDIO);
        if (ret != HAL_OK) {
            break;
        }
        hal_gpio_schmitt_ctrl (sdio_dev_pins[i], 0, IO_3p3V);  // switch IO pad as 3.3V
        hal_gpio_drive_ctrl (sdio_dev_pins[i], 3); // Max IO pad driving current
    }
    
    if (ret != HAL_OK) {
		goto __hal_sdio_dev_init_err;
    }
    
    hal_gpio_schmitt_ctrl (PIN_SDIO_CLK, ON, IO_3p3V);  // Enable Smith trigger on SDIO_CLK pin 

    g_sdio_adp = (hal_sdio_dev_adapter_t *)rtw_malloc (sizeof(hal_sdio_dev_adapter_t));
    if (g_sdio_adp == NULL) {
		DBG_SDIO_DEV_ERR ("%s: malloc for SDIO device object failed!\r\n", __func__);
        return HAL_ERR_MEM;
    }
    memset ((void *)g_sdio_adp, 0, sizeof(hal_sdio_dev_adapter_t));
    
    g_sdio_adp->rx_pkt_16k = SDIO_RX_PKT_SIZE_OVER_16K;
    g_sdio_adp->dcache_clean_by_addr = hal_sdio_dev_dcache_clean_by_addr;
    g_sdio_adp->dcache_invalidate_by_addr = hal_sdio_dev_dcache_invalidate_by_addr;

#if SDIO_API_DEFINED
    if(g_spdio_priv == NULL){
        DBG_SDIO_DEV_ERR("struct spdio_t must be inited\n");
        return HAL_ERR_PARA;
    }
    g_sdio_adp->spdio_priv = g_spdio_priv;
    g_spdio_priv->priv = (void *)g_sdio_adp;
    // for HAL driver TX dirction is host -> device, that is for SPDIO API RX direction
    tx_bd_num = g_spdio_priv->rx_bd_num;
    rx_bd_num = g_spdio_priv->tx_bd_num;
    g_sdio_adp->tx_bd_buf_size = g_spdio_priv->rx_bd_bufsz;
#else
    tx_bd_num = SDIO_TX_BD_NUM;
    rx_bd_num = SDIO_RX_BD_NUM;
    g_sdio_adp->tx_bd_buf_size = (((SDIO_TX_BD_BUF_SIZE + sizeof(sdiod_tx_desc_t) - 1) >> 6) + 1) << 6;
#endif

    /// TX path init
	/**** Initial SDIO TX: initial TX BD, TX BD Handle, TX Buffer ****/
	DBG_SDIO_DEV_INFO("Tx BD Init==>\n");

    // allocate a buffer for TX BD. for cache sync consideration we need to make the TX BD
    // start with 32bytes aligned address
#if 0    
    // use memalloc for TX BD memory
	g_sdio_adp->ptxbd_addr = rtw_zmalloc((tx_bd_num * sizeof(sdiod_txbd_t))+31);
#else
    // use static No-cacheable memory for TX BD memory
    if (tx_bd_num > SDIO_TX_BD_NUM) {
		DBG_SDIO_DEV_ERR("Needs more TX BD: increase SDIO_TX_BD_NUM\n");
        tx_bd_num = SDIO_TX_BD_NUM;
    }
    g_sdio_adp->ptxbd_addr = (uint8_t *)sdiod_tx_bd;
#endif
	if (g_sdio_adp->ptxbd_addr == NULL) {
		DBG_SDIO_DEV_ERR("Malloc for TX_BD Err!!\n");
        ret = HAL_ERR_MEM;
		goto __hal_sdio_dev_init_err;
	}
    g_sdio_adp->tx_bd_num = tx_bd_num;

    g_sdio_adp->ptxbd_hdl = (sdiod_txbd_hdl_t *)rtw_zmalloc(tx_bd_num * sizeof(sdiod_txbd_hdl_t));
	if (g_sdio_adp->ptxbd_hdl == NULL) {
		DBG_SDIO_DEV_ERR("Malloc for TX_BD Handle Err!!\n");
        ret = HAL_ERR_MEM;
		goto __hal_sdio_dev_init_err;
	}
    g_sdio_adp->cmd11_callback = hal_sdio_dev_cmd11_handle;
    g_sdio_adp->txbd_hdl_init = hal_sdio_txbd_hdl_init;
    g_sdio_adp->txbd_hdl_deinit = hal_sdio_txbd_hdl_deinit;
    g_sdio_adp->txbd_buf_refill = hal_sdio_txbd_buf_do_refill;
    g_sdio_adp->rxbd_tr_done_callback = hal_sdio_rxbd_tr_done_callback;
    g_sdio_adp->txbd_rdy_callback = hal_sdio_txbd_rdy_callback;
    g_sdio_adp->free_rx_pkt = hal_sdio_dev_free_rx_pkt;

    /// RX Path init
    g_sdio_adp->rx_bd_num = rx_bd_num;
    // hardware design is the RX BD start address must be aligned to 8-bytes,
    // but for cache sync consideration we make it align to 32-bytes
#if 0
    // use dynamical memalloc for RX BD memory
//	g_sdio_adp->prxbd_addr = rtw_zmalloc((rx_bd_num * sizeof(sdiod_rxbd_t))+31);
#else
    // use static non-cacheable memory for RX BD memory
    if (rx_bd_num > SDIO_RX_BD_NUM) {
		DBG_SDIO_DEV_ERR("Needs more RX BD: increase SDIO_RX_BD_NUM\n");
        rx_bd_num = SDIO_RX_BD_NUM;
    }
	g_sdio_adp->prxbd_addr = (uint8_t *)sdiod_rx_bd;
#endif
	if (g_sdio_adp->prxbd_addr == NULL) {
		DBG_SDIO_DEV_ERR("SDIO_Device_Init: Malloc for RX_BD Err!!\n");
        ret = HAL_ERR_MEM;
		goto __hal_sdio_dev_init_err;
	}
    g_sdio_adp->free_rx_bd_cnt = RX_BD_FREE_TH;

	g_sdio_adp->prxbd_hdl = (sdiod_rxbd_hdl_t *)rtw_zmalloc(rx_bd_num * sizeof(sdiod_rxbd_hdl_t));

	if (g_sdio_adp->prxbd_hdl == NULL) {
		DBG_SDIO_DEV_ERR("SDIO_Device_Init: Malloc for RX_BD Handle Err!!\n");
        ret = HAL_ERR_MEM;
		goto __hal_sdio_dev_init_err;
	}

	INIT_LIST_HEAD (&g_sdio_adp->free_rx_pkt_list);	// Init the list for free packet handler

	/* Allocate memory for RX Packets handler */
	g_sdio_adp->prx_pkt_handler = (sdiod_rx_packet_t *)rtw_zmalloc(sizeof(sdiod_rx_packet_t) * SDIO_RX_PKT_NUM);
	if (g_sdio_adp->prx_pkt_handler == NULL) {
		DBG_SDIO_DEV_ERR("SDIO_Device_Init: Malloc for RX PKT Handler Err!!\n");
        ret = HAL_ERR_MEM;
		goto __hal_sdio_dev_init_err;
	}

	/* Add all RX packet handler into the Free Queue(list) */
	for (i = 0; i < SDIO_RX_PKT_NUM; i++) {
		prx_pkt = g_sdio_adp->prx_pkt_handler + i;
        // make sure the allocated memory address is 32-bytes aligned!! Otherwise will cause D-cache error.
        prx_pkt->prx_desc = (sdiod_rx_desc_t *)rtw_zmalloc(sizeof(sdiod_rx_desc_t));
		list_add_tail(&prx_pkt->list, &g_sdio_adp->free_rx_pkt_list);
	}
	INIT_LIST_HEAD(&g_sdio_adp->pend_rx_pkt_list);	// Init the list for RX packet to be send to the SDIO bus

#if !defined(SDIO_NO_RTOS) || (defined(SDIO_NO_RTOS) && (SDIO_NO_RTOS == 0))
    g_sdio_adp->os_init = hal_sdio_dev_os_init;
    g_sdio_adp->os_deinit = hal_sdio_dev_os_deinit;
#else
    g_sdio_adp->os_init = NULL;
    g_sdio_adp->os_deinit = NULL;
#endif
    g_sdio_adp->rst_cmd_callback = (sdiod_rst_cmd_callback_t)hal_sdio_dev_reset_cmd;
    g_sdio_adp->rpwm1_cmd_callback = (sdiod_rpwm_callback_t)hal_sdio_dev_process_rpwm;
    g_sdio_adp->rpwm2_cmd_callback = (sdiod_rpwm_callback_t)hal_sdio_dev_process_rpwm2;
    g_sdio_adp->msg_handler = hal_sdio_dev_msg_handler;
    
    ret = hal_sdiod_stubs.init (g_sdio_adp);
    if (ret != HAL_OK) {
		goto __hal_sdio_dev_init_err;
    }
    hal_irq_set_priority (SDIOD_IRQn, SDIOD_IRQPri);

#if (defined(SDIO_NO_RTOS) && (SDIO_NO_RTOS == 1))
    /* NO wait RTOS task start, Indicate the Host system that the TX/RX is ready */
    SDIO_DEV->sys_ind_b.sys_cpu_rdy_ind = 1;
#endif

    return HAL_OK;
    
__hal_sdio_dev_init_err:
    hal_gpio_schmitt_ctrl (PIN_SDIO_CLK, OFF, 1);

    if (g_sdio_adp->prx_pkt_handler != NULL) {
        for (i = 0; i < SDIO_RX_PKT_NUM; i++) {
            prx_pkt = g_sdio_adp->prx_pkt_handler + i;
            if (prx_pkt->prx_desc != 0) {
                rtw_mfree ((u8 *)prx_pkt->prx_desc, sizeof(sdiod_rx_desc_t));
                prx_pkt->prx_desc = 0;
            }
        }
        rtw_mfree ((uint8_t *)g_sdio_adp->prx_pkt_handler, (sizeof(sdiod_rx_packet_t) * SDIO_RX_PKT_NUM));
        g_sdio_adp->prx_pkt_handler = NULL;
    }

    if (g_sdio_adp->prxbd_hdl != NULL) {
        rtw_mfree ((uint8_t *)g_sdio_adp->prxbd_hdl, (rx_bd_num * sizeof(sdiod_rxbd_hdl_t)));
        g_sdio_adp->prxbd_hdl = NULL;
    }
    
    if (g_sdio_adp->prxbd_addr != NULL) {
        rtw_mfree (g_sdio_adp->prxbd_addr, ((rx_bd_num * sizeof(sdiod_rxbd_t))+31));
        g_sdio_adp->prxbd_addr = NULL;
    }
    
    if(g_sdio_adp->ptxbd_hdl != NULL) {
        rtw_mfree ((uint8_t *)g_sdio_adp->ptxbd_hdl, (g_sdio_adp->tx_bd_num * sizeof(sdiod_txbd_hdl_t)));
        g_sdio_adp->ptxbd_hdl = NULL;
    }
    
    if(g_sdio_adp->ptxbd_addr != NULL) {
        rtw_mfree (g_sdio_adp->ptxbd_addr, ((g_sdio_adp->tx_bd_num * sizeof(sdiod_txbd_t))+31));
        g_sdio_adp->ptxbd_addr = NULL;
    }
    
    if (g_sdio_adp != NULL) {
        rtw_mfree ((uint8_t *)g_sdio_adp, sizeof(hal_sdio_dev_adapter_t));
        g_sdio_adp = NULL;
    }
    return ret;
}

/**
 *  @brief To de-initial the SDIO device hardware and the SDIO device HAL adapter.
 *         It will free all allocated resource, including free memory and stop TX/RX task,
 *         and then disable the SDIO device hardware.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     void.
 */
void hal_sdio_dev_deinit (void)
{
    uint32_t i;
	sdiod_txbd_hdl_t *ptxbd_hdl;
	sdiod_rx_packet_t *prx_pkt;

    if (g_sdio_adp == NULL) {
        // SDIO device adapter didn't initialed
        return;
    }

    hal_sdiod_stubs.deinit (g_sdio_adp);

    if (g_sdio_adp->prx_pkt_handler) {
        for (i = 0; i < SDIO_RX_PKT_NUM; i++) {
            prx_pkt = g_sdio_adp->prx_pkt_handler + i;
            if (prx_pkt->prx_desc != 0) {
                rtw_mfree ((u8 *)prx_pkt->prx_desc, sizeof(sdiod_rx_desc_t));
                prx_pkt->prx_desc = 0;
            }
        }
        rtw_mfree((uint8_t *)g_sdio_adp->prx_pkt_handler, (sizeof(sdiod_rx_packet_t) * SDIO_RX_PKT_NUM));
        g_sdio_adp->prx_pkt_handler = NULL;
    }

    if (g_sdio_adp->prxbd_hdl) {
        rtw_mfree((uint8_t *)g_sdio_adp->prxbd_hdl, g_sdio_adp->rx_bd_num * sizeof(sdiod_rxbd_hdl_t));
        g_sdio_adp->prxbd_hdl = NULL;
    }

    if (g_sdio_adp->prxbd_addr) {
        rtw_mfree((uint8_t *)g_sdio_adp->prxbd_addr, ((g_sdio_adp->rx_bd_num * sizeof(sdiod_rxbd_t))+31));
        g_sdio_adp->prxbd_addr = NULL;
        g_sdio_adp->prxbd_addr_align = NULL;
    }

    if ((g_sdio_adp->ptxbd_hdl) && (g_sdio_adp->ptxbd_addr_align)) {
        for (i = 0; i < g_sdio_adp->tx_bd_num; i++) {
            ptxbd_hdl = g_sdio_adp->ptxbd_hdl + i;
            g_sdio_adp->txbd_hdl_deinit(ptxbd_hdl, i);
        }
    }

    if (g_sdio_adp->ptxbd_hdl) {
        rtw_mfree((uint8_t *)g_sdio_adp->ptxbd_hdl, (g_sdio_adp->tx_bd_num * sizeof(sdiod_txbd_hdl_t)));
        g_sdio_adp->ptxbd_hdl = NULL;
    }

    if (g_sdio_adp->ptxbd_addr) {
        rtw_mfree(g_sdio_adp->ptxbd_addr, (g_sdio_adp->tx_bd_num * sizeof(sdiod_txbd_t))+31);
        g_sdio_adp->ptxbd_addr = NULL;
        g_sdio_adp->ptxbd_addr_align = NULL;
    }

    for (i=0; i<8; i++) {
        if (sdio_dev_pins[i] == 0xFF) {
            break;
        }
        hal_pinmux_unregister (sdio_dev_pins[i], PID_SDIO);
    }
    hal_gpio_schmitt_ctrl (PIN_SDIO_CLK, OFF, 1);
}

/**
 *  @brief To send a IO message (passed by the C2H register) to the SDIO host.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @param[in]  c2h_msg  the message to be send to the SDIO host, only bits[30:0]
 *                       are valid.
 *  @return     void.
 */
void hal_sdio_dev_send_c2h_iomsg (uint32_t c2h_msg)
{
    SDIO_DEV_Type *sdio_dev = SDIO_DEV;
	sdio_dev_c2h_msg_t c2h_msg_tmp;

    if (g_sdio_adp == NULL) {
        // SDIO device adapter didn't initialed
        return;
    }

	// TODO: define C2H message type & format, currently we have  30 bits message only, may needs to extend the HW register

	// TODO: May needs to handle endian free
    c2h_msg_tmp.b.c2h_msg = c2h_msg;
    c2h_msg_tmp.b.toggle = sdio_dev->c2h_msg_b.toggle ^ 1;
    sdio_dev->c2h_msg = c2h_msg_tmp.w;
}

#if CONFIG_INIC_EN
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
u32 hal_sdio_dev_sleepcg_condition_match (void);

/**
 *  @brief If the SDIO device works as an INIC device, this function is a
 *         callback function which will be called when the INIC device is
 *         entering sleep power saving mode.
 *  @param[in]  expected_idle_time  the period, in mini-second, for the INIC
 *                                  device stay in sleep mode.
 *  @param[in]  param_ptr  the argument of the callback function(depends on
 *                         the application).
 *  @return     always return 0.
 */
u32 hal_sdio_dev_pre_sleep_callback (u32 expected_idle_time, void *param_ptr)
{
    SDIO_DEV_Type *sdio_dev = SDIO_DEV;

    if(hal_sdio_dev_sleepcg_condition_match() == _FALSE) {
        DBG_SDIO_DEV_ERR("!\n");
    }

    /* Indicate the Host system that the TX/RX is not ready */
    sdio_dev->sys_ind_b.sys_cpu_rdy_ind = 0;
    return 0;
}

/**
 *  @brief If the SDIO device works as an INIC device, this function is a
 *         callback function which will be called when the INIC device is
 *         resume from the sleep mode.
 *  @param[in]  expected_idle_time  the period, in mini-second, for the INIC
 *                                  device stay in sleep mode.
 *  @param[in]  param_ptr  the argument of the callback function(depends on
 *                         the application).
 *  @return     always return 0.
 */
u32 hal_sdio_dev_post_sleep_callback (u32 expected_idle_time, void *param_ptr)
{
	/* Indicate the Host system that the TX/RX is ready */
    SDIO_DEV->sys_ind_b.sys_cpu_rdy_ind = 1;
    return 0;
}

/**
 *  @brief If the SDIO device works as an INIC device, this function
 *         will be called to check whether the SDIO device is ready
 *         to enter the sleep mode or not.
 *  @return     _TRUE  the SDIO device is ready to enter the sleep mode.
 *  @return     _FALSE  the SDIO device is unavailable to enter the sleep mode now.
 */
u32 hal_sdio_dev_sleepcg_condition_match (void)
{
    SDIO_DEV_Type *sdio_dev = SDIO_DEV;

	/*RX FIFO is not empty*/
    if (sdio_dev->hci_rx_ctrl_b.rx_req) {
        goto _errSleepCG;
    }

	return _TRUE;

_errSleepCG:

	return _FALSE;
}

/**
 *  @brief If the SDIO device works as an INIC device, call this function
 *         will make the SDIO device to enter the clock gated sleep mode.
 *  @return     _TRUE  the SDIO device entered the sleep mode and resumed.
 *  @return     _FALSE  the SDIO device is unable to enter the sleep mode now.
 */
uint8_t hal_sdio_dev_enter_sleepcg(void)
{
	if(hal_sdio_dev_sleepcg_condition_match() == _FALSE) {
        return _FALSE;
    }

    // TODO:
    rtw_wowlan_dev_sleep();
    pmu_add_wakeup_event(SLEEP_WAKEUP_BY_SDIO);
    pmu_register_sleep_callback(PMU_SDIO_DEVICE, hal_sdio_dev_pre_sleep_callback, NULL,
                                hal_sdio_dev_post_sleep_callback, NULL);
    pmu_release_wakelock(BIT(PMU_SDIO_DEVICE));

    return _TRUE;
}
#endif
#endif

/**
 *  @brief To register a callback function for a RX packet has been transfered
 *         to the SDIO host. This registered callback function will be called when
 *         an RX packet has been transfered to the SDIO host. So the application
 *         layer can recycle the RX packet buffer.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @param[in]  callback  the callback function.
 *  @param[in]  para  a argument of the callback function, it is a priviate data
 *                    of the application layer.
 *  @return     void.
 */
void hal_sdio_dev_register_rx_done_callback (sdiod_rx_done_callback_t callback, void *para)
{
    if (g_sdio_adp == NULL) {
        // SDIO device adapter didn't initialed
        return;
    }

	g_sdio_adp->rx_done_callback = callback;
	g_sdio_adp->prx_done_cb_para = para;
}

/**
 *  @brief To register a callback function for a TX packet received from the
 *         SDIO host. This registered callback function will be called when
 *         an new TX packet from the SDIO host is received. So the SDIO device
 *         HAL can forward this TX packet to the application layer, ex. the
 *         WLan driver.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @param[in]  callback  the callback function.
 *  @param[in]  para  a argument of the callback function, it is a priviate data
 *                    of the application layer.
 *  @return     void.
 */
void hal_sdio_dev_register_tx_callback (sdiod_tx_callback_t callback, void *para)
{
    if (g_sdio_adp == NULL) {
        // SDIO device adapter didn't initialed
        return;
    }

	g_sdio_adp->tx_callback = callback;
	g_sdio_adp->ptxcb_para = para;
}

/**
 *  @brief To register a callback function for event of a H2C(SDIO host to SDIO device)
 *         IO message is received.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @param[in]  callback  the callback function.
 *  @param[in]  para  a argument of the callback function, it is a priviate data
 *                    of the application layer.
 *  @return     void.
 */
void hal_sdio_dev_register_h2c_msg_callback (sdiod_h2c_msg_callback_t callback, void *para)
{
    if (g_sdio_adp == NULL) {
        // SDIO device adapter didn't initialed
        return;
    }

	g_sdio_adp->h2c_msg_callback = callback;
	g_sdio_adp->ph2c_msg_cb_para = para;
}

/**
 *  @brief Gets a RX packet from the RX packets queue. A RX packet is used to
 *         handle a data packet which will be send to the SDIO host. The SDIO
 *         device HAL initialization function will allocate some RX packets
 *         and put them in the RX free packets queue. This function will takes
 *         a RX packet out from this RX free packet queue.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @return     value != NULL  the gotten RX packet.
 *  @return     value == NULL  failed to get RX packet(queue is empty).
 */
psdiod_rx_packet_t hal_sdio_dev_alloc_rx_pkt (hal_sdio_dev_adapter_t *psdio_adp)
{
	struct list_head *plist;
	sdiod_rx_packet_t *ppkt;
    uint32_t loop_cnt;

    hal_sdio_rx_lock();
	if (list_empty(&psdio_adp->free_rx_pkt_list)) {
        hal_sdio_rx_unlock();
        loop_cnt = 0;
        do {
            ppkt =(sdiod_rx_packet_t *)(rtw_zmalloc (sizeof(sdiod_rx_packet_t)));
            if (ppkt != NULL) {
                ppkt->is_malloc = 1;   // this packet handler is dynamic allocated
                DBG_SDIO_DEV_WARN("Warn! No Free RX PKT, Use Dyna Alloc\n");
            } else {
                rtw_msleep_os(10);
                loop_cnt++;
                if (loop_cnt > 100) {
                    DBG_SDIO_DEV_ERR("%s: Err!! Allocate RX PKT Failed!!\n", __func__);
                    break;
                }
            }
        } while (ppkt == NULL);
        return ppkt;
	}

	plist = psdio_adp->free_rx_pkt_list.next;
	ppkt = container_of (plist, sdiod_rx_packet_t, list);

	list_del_init (&ppkt->list);
    hal_sdio_rx_unlock();

	return ppkt;
}

/**
 *  @brief Frees a RX packet. This function will put the RX packet back to
 *         the RX free packet queue.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @param[in]  ppkt  the RX packet to be freed.
 *  @return     void.
 */
void hal_sdio_dev_free_rx_pkt(hal_sdio_dev_adapter_t *psdio_adp, sdiod_rx_packet_t *ppkt)
{
    if (ppkt->is_malloc) {
        rtw_mfree((uint8_t *)ppkt, sizeof(sdiod_rx_packet_t));
    } else {
        hal_sdio_rx_lock();
    	list_add_tail(&ppkt->list, &psdio_adp->free_rx_pkt_list);
        hal_sdio_rx_unlock();
    }
}

/**
 *  @brief To send a RX packet a the SDIO host. This is an API for application
 *         to send a block of data to the SDIO host.
 *  @param[in]  psdio_adp  the SDIO device HAL adapter.
 *  @param[in]  pdata  the buffer of the data to be send.
 *  @param[in]  offset the offset from the buffer start to the valid packet
 *              payload, in bytes. this offset will be passed to the SDIO host
 *              by the RX descripter.
 *  @param[in]  pkt_size  the size of the data to be send, in byte.
 *  @param[in]  cmd_type  the type of the packet which will be passed to the
 *              SDIO host by the RX descripter.
 *  @return     SUCCESS  the packet has been put into the RX packet pending queue.
 *  @return     FAIL  failed to send the packet, resource insufficient.
 */
int8_t hal_sdio_dev_rx_pkt_queue_push (void *pdata, uint16_t offset, uint16_t pkt_size, uint8_t cmd_type)
{
    sdiod_rx_desc_t *prx_desc;
    sdiod_rx_packet_t *ppkt;
    hal_status_t ret;
#if SDIO_API_DEFINED
    struct spdio_buf_t *buf = (struct spdio_buf_t *)pdata;
#else  
#if CONFIG_INIC_EN && CONFIG_INIC_SKB_RX
    struct sk_buff *skb = (struct sk_buff *)pdata;
#endif
#endif

    if (g_sdio_adp == NULL) {
        // SDIO device adapter didn't initialed
        DBG_SDIO_DEV_ERR ("%s: SDIO device adapter didn't be initialed\r\n", __func__);
        return FAIL;
    }

    ppkt = hal_sdio_dev_alloc_rx_pkt (g_sdio_adp);
    if (ppkt == NULL) {
        DBG_SDIO_DEV_ERR ("%s: Err!! No Free RX PKT!\n", __func__);
        return FAIL;
    }

    prx_desc = ppkt->prx_desc;
    prx_desc->type = cmd_type;
    prx_desc->pkt_len = pkt_size;
#if SDIO_API_DEFINED
    ppkt->priv = pdata;
    prx_desc->offset = sizeof(sdiod_rx_desc_t) /*+ buf->reserved*/;//for data alignment reason
    ppkt->pdata = (uint8_t *)buf->buf_addr;
    ppkt->offset = offset;
#else
#if CONFIG_INIC_EN
#if CONFIG_INIC_SKB_RX
    prx_desc->offset = sizeof(sdiod_rx_desc_t) + offset;//for data alignment reason
    ppkt->skb = skb;
    ppkt->pdata = skb->data;
    ppkt->offset = 0;
#else //CONFIG_INIC_SKB_RX
    prx_desc->offset = sizeof(sdiod_rx_desc_t);
    ppkt->pdata = pdata;
    ppkt->offset = offset;
#endif//CONFIG_INIC_SKB_RX
#else //CONFIG_INIC_EN
    prx_desc->offset = sizeof(sdiod_rx_desc_t);
    ppkt->pdata = pdata;
    ppkt->offset = offset;
#endif //CONFIG_INIC_EN
#endif

    ret = hal_sdio_dev_rx_pkt_enqueue (g_sdio_adp, ppkt);

    if (ret == HAL_OK) {
        return SUCCESS;
    } else {
        return FAIL;
    }
}

int8_t hal_sdio_dev_msg_handler (hal_sdio_dev_adapter_t *psdio_adp, sdiod_msg_blk_t *pmblk)
{
    sdiod_rx_desc_t *prx_desc;
    sdiod_rx_packet_t *ppkt;
	hal_status_t ret = HAL_OK;

	DBG_SDIO_DEV_INFO("hal_sdio_dev_msg_handler==> msg_type=0x%x\n", pmblk->msg_type);
    
	switch (pmblk->msg_type) {
    	case MSG_SDIO_RX_PKT:
            //pmblk->para0 = packet header Offset
            ppkt = hal_sdio_dev_alloc_rx_pkt (g_sdio_adp);
            if (ppkt == NULL) {
                DBG_SDIO_DEV_ERR ("%s: Err!! No Free RX PKT!\n", __func__);
                return FAIL;
            }

            prx_desc = ppkt->prx_desc;
            prx_desc->type = SDIO_CMD_RX_ETH;
            prx_desc->pkt_len = pmblk->msg_len;            
            prx_desc->offset = sizeof(sdiod_rx_desc_t);
            ppkt->pdata = pmblk->pmsg_buf;
            ppkt->offset = pmblk->para0;
            ret = hal_sdio_dev_rx_pkt_enqueue (g_sdio_adp, ppkt);
            if (HAL_OK != ret) {
                // failed to send this packet to the host, drop it
                rtw_mfree ((u8 *)pmblk->pmsg_buf, pmblk->para0 + pmblk->msg_len);
            }
    		break;

    	case MSG_SDIO_C2H:
    		break;
    		
    	case MSG_SDIO_RPWM:
    		break;
    		
    	default:
    		DBG_SDIO_DEV_WARN("hal_sdio_dev_msg_handler: UnKnown MsgType %d\n", pmblk->msg_type);
    		break;
	}

	return ret;
}

int8_t hal_sdio_dev_send_msg (hal_sdio_dev_adapter_t *psdio_adp, sdiod_msg_blk_t *pmblk, uint32_t timeout_ms)
{
    int32_t ret;

#if !defined(SDIO_NO_RTOS) || (defined(SDIO_NO_RTOS) && (SDIO_NO_RTOS==0))
    ret = rtw_push_to_xqueue(&sdiod_msg_queue, (void *)pmblk, timeout_ms);
    if (ret == SUCCESS) {
        // wake up SDIO RX task to process this message
        hal_sdio_dev_rx_task_up(psdio_adp);
    }

    return ret;
#else
    // this function is not supported for NO_RTOS mode
    return FAIL;
#endif
}

/** @} */ /* End of group hs_hal_sdio_dev */

#endif  // end of "#if CONFIG_SDIO_DEVICE_EN"

