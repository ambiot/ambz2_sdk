/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_flags.h
   * @brief     This file is used to config app functions.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _BT_DATATRANS_APP_FLAGS_H_
#define _BT_DATATRANS_APP_FLAGS_H_

/*============================================================================*
 *                              Constants
 *============================================================================*/

#define BT_DATATRANS_APP_MAX_LINKS   1

/*if define CENTRAL_MODE to 1,pls include datatrans_client.c & gaps_client.c in target build.*/
#define CENTRAL_MODE    1

/** @brief  Config set physical: : 0-Not built in, 1-built in, use user command to set*/
#if defined(CONFIG_PLATFORM_8721D)
#define F_BT_LE_5_0_SET_PHY_SUPPORT         1
#elif defined(CONFIG_PLATFORM_8710C)
#define F_BT_LE_5_0_SET_PHY_SUPPORT         0
#endif

/** @} */ /* End of group SCATTERNET_Config */
/** @} */ /* End of group SCATTERNET_DEMO */
#endif
