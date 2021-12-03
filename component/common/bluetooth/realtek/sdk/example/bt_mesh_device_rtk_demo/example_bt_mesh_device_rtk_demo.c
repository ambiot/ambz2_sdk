#include "platform_opts.h"
#include <platform/platform_stdlib.h>
#include <wifi_conf.h>
#include <osdep_service.h>
#include "platform_os.h"
#include "os_task.h"
#if defined(CONFIG_PLATFORM_8721D)
#include <platform_opts_bt.h>
#endif
#include "bt_config_wifi.h"

#include <gap_conn_le.h> 

#if defined(CONFIG_BT_MESH_DEVICE_RTK_DEMO) && CONFIG_BT_MESH_DEVICE_RTK_DEMO

//media
extern void amebacam_broadcast_demo_thread(void);
//httpd
extern void httpd_demo_init_thread(void);
#if defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE
extern int bt_mesh_device_app_main(void);
#elif defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
extern int bt_mesh_device_multiple_profile_app_main(void);
#endif
extern void bt_coex_init(void);
extern uint8_t bt_mesh_device_api_init(void);
extern void wifi_btcoex_set_bt_on(void);

void bt_mesh_example_init_thread(void *param)
{
    /* avoid gcc compile warning */
    (void)param;
	T_GAP_DEV_STATE new_state;

	/*Wait WIFI init complete*/
	while(!(wifi_is_up(RTW_STA_INTERFACE) || wifi_is_up(RTW_AP_INTERFACE))) {
		os_delay(1000);
	}
#if defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE
	/*Init BT mesh device*/
	bt_mesh_device_app_main();
#elif defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
    bt_mesh_device_multiple_profile_app_main();
#endif

	bt_coex_init();

	/*Wait BT init complete*/
	do {
		os_delay(100);
		le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	} while (new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	/*Start BT WIFI coexistence*/
	wifi_btcoex_set_bt_on();

    if (bt_mesh_device_api_init()) {
        printf("[BT Mesh Device] bt_mesh_device_api_init fail ! \n\r");
    }

	vTaskDelete(NULL);
}

void example_bt_mesh(void)
{
	//init bt config/bt mesh
	void *task = NULL;

	if (os_task_create(&task, ((const char*)"bt_mesh_example_demo_init_thread"), bt_mesh_example_init_thread,
                    NULL, 2048, 5) != true) {
        printf("\n\r%s xTaskCreate(bt_mesh_example_demo_init_thread) failed", __FUNCTION__);
    }
}

#endif /* CONFIG_EXAMPLE_BT_MESH_DEMO */


