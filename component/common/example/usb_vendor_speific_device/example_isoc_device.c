#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "basic_types.h"
#include "platform_opts.h"

#include "usb.h"
//#include "msc/inc/usbd_msc_config.h"
//#include "msc/inc/usbd_msc.h"

#define MSC_NBR_BUFHD	2 /* number of buffer header for bulk in/out data*/
#define MSC_BUFLEN	(32*512)/* Default size of buffer length. Minmun of 512 byte*/

extern unsigned int iso_out_count;
extern unsigned int iso_in_count;
extern int usbd_vendor_init(int N_bh, int Size_bh, int type);

void example_isoc_device_thread(void* param){
	int status = 0;

	_usb_init();
	status = wait_usb_ready();
	if(status != USB_INIT_OK){
		if(status == USB_NOT_ATTACHED)
			printf("\r\n NO USB device attached\n");
		else
			printf("\r\n USB init fail\n");
		goto exit;
	}

	// load usb mass storage driver

        status = usbd_vendor_init(MSC_NBR_BUFHD, MSC_BUFLEN, 0);


	if(status){
		printf("USB MSC driver load fail.\n");
	}else
		printf("USB MSC driver load done, Available heap [0x%x]\n", xPortGetFreeHeapSize());

        vTaskDelay(10000);
        //printf("iso_out_count=%d\n",iso_out_count);
        //printf("iso_in_count=%d\n",iso_in_count);
exit:
	vTaskDelete(NULL);
}


void example_isoc_device(void)
{
	if(xTaskCreate(example_isoc_device_thread, ((const char*)"example_fatfs_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_fatfs_thread) failed", __FUNCTION__);
}