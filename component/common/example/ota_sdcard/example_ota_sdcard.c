#if defined(CONFIG_PLATFORM_8195BHP)
#include <ota_8195b.h>
#endif

#define FILENAME	"ota_is_realtek.bin"

#ifdef SDCARD_OTA_UPDATE
void sdcard_update_ota_task(void *param){
	printf("\n\r\n\r\n\r\n\r<<<<<<OTA from SD card example start>>>>>>>\n\r\n\r\n\r\n\r");
	int ret = -1;

#if defined(CONFIG_PLATFORM_8195BHP)	
	ret = sdcard_update_ota(FILENAME);
#else
	printf("Not supported\n\r");
#endif
	
	printf("\n\r[%s] Update task exit", __FUNCTION__);
	if(!ret){
		printf("\n\r[%s] Ready to reboot", __FUNCTION__);	
		ota_platform_reset();
	}
	vTaskDelete(NULL);
}

void example_ota_sdcard(void){
	if(xTaskCreate(sdcard_update_ota_task, (char const *)"sdcard_update_ota_task", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS){
		printf("\n\r[%s] Create update task failed", __FUNCTION__);
	}
}
#endif