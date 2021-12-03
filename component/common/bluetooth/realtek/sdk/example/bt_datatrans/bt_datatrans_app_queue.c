enum { __FILE_NUM__ = 0 };

/**
*********************************************************************************************************
*               Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
**********************************************************************************************************
* @file     app_queue.c
* @brief
* @details
* @author   jane
* @date     2015-03-29
* @version  v0.2
*********************************************************************************************************
*/
#include <platform_opts_bt.h>
#if defined(CONFIG_BT_DATATRANS) && CONFIG_BT_DATATRANS
#include "bt_datatrans_app_queue.h"

#if ! defined (FX_NOFLAGS)
#endif

void datatrans_app_queue_in(QUEUE_P QueuePtr, void *pQueueElement)
{

    ELEMENT_P QueueElementPtr = (ELEMENT_P)pQueueElement;
    ELEMENT_P LastPtr;

    if ((LastPtr = QueuePtr->Last) == (ELEMENT_P)0)    /* if queue is empty,  */
    {
        QueuePtr->First = QueueElementPtr;    /* q->first = q->last = new entry */
    }
    else                                    /* if it is not empty, new entry  */
    {
        LastPtr->Next  = QueueElementPtr;    /* is next from last entry        */
    }
    QueuePtr->Last = QueueElementPtr;
    QueueElementPtr->Next = (ELEMENT_P)0;
    QueuePtr->ElementCount++;               /* increment element count        */
}

void *datatrans_app_queue_out(QUEUE_P QueuePtr)
{
    ELEMENT_P FirstPtr;

    if ((FirstPtr = QueuePtr->First) != (ELEMENT_P)0)
    {
        /* if queue not empty and    */
        /* it is the last entry      */
        if ((QueuePtr->First = FirstPtr->Next) == (ELEMENT_P)0)
        {
            QueuePtr->Last = (ELEMENT_P)0;    /* set queue empty           */
        }
        QueuePtr->ElementCount--;                  /* decrement element count   */
    }
    return (FirstPtr);
}

#endif
