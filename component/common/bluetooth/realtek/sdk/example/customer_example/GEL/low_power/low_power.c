/**
*****************************************************************************************
*     Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     low_power.c
  * @brief    Source file of power saving function specific for GEL.
  * @details  User command interfaces.
  * @author   sherman_sun
  * @date     2020_11_06
  * @version  v1.0
  * *************************************************************************************
  */
#include "low_power.h"
#include <lwip/sockets.h>

#define APP_TASK_PRIORITY             1         //!< Task priorities
#define APP_TASK_STACK_SIZE           256 * 10  //!< Task stack size

#define LOW_POWER_TCP_TEST            0
#define TCP_SERVER_PORT               8080
#define TCP_SERVER_IP                 "192.168.1.100"
#define TCP_TCP_BUF_SIZE              32

void *bt_mesh_ps_task_handle;
extern void rltk_coex_ps_leave(void);
extern void rltk_coex_ps_enter(void);

enum ps_state
{
    ps_state_disable,
    ps_state_enable
};
enum
{
    TCP_IDLE_CHECK_FALSE,
    TCP_IDLE_CHECK_TRUE
};
uint8_t ps_state = ps_state_disable;
uint8_t tcp_idle_state = TCP_IDLE_CHECK_TRUE;

#if LOW_POWER_TCP_TEST
void *bt_mesh_tcp_client_task_handle;
struct low_power_tcp_data_t{
	int       client_fd;
	char      tcp_recv_buf[TCP_TCP_BUF_SIZE];
	char      tcp_send_buf[TCP_TCP_BUF_SIZE];
};
struct low_power_tcp_data_t *low_power_tcp_data = NULL;
#endif

#if LOW_POWER_TCP_TEST
void bt_mesh_tcp_client_task(void *p_param)
{
	struct sockaddr_in  ser_addr;
	int                 i, recv_size = 0;
	
	printf("\n\rTCP: Start TCP client!");
	
	if(!low_power_tcp_data)
		return;
	
	//filling the buffer
	for (i = 0; i < TCP_TCP_BUF_SIZE; i++)
		low_power_tcp_data->tcp_send_buf[i] = (char)(i);
	
Reconnect:
	//create socket
	if( (low_power_tcp_data->client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf("\n\r[ERROR] %s: Create TCP socket failed",__func__);
		goto Exit2;
	}
	
	//initialize value in dest
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(TCP_SERVER_PORT);
	ser_addr.sin_addr.s_addr = inet_addr((char const*)TCP_SERVER_IP);

	printf("\n\r%s: Server IP=%s, port=%d", __func__, TCP_SERVER_IP, TCP_SERVER_PORT);
	printf("\n\r%s: Create socket fd = %d", __func__, low_power_tcp_data->client_fd);

	//Connecting to server
	if( connect(low_power_tcp_data->client_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0){
		printf("\n\r[ERROR] %s: Connect to server failed",__func__);
		goto Exit1;
	}
	printf("\n\r%s: Connect to server successfully", __func__);
	tcp_idle_state = TCP_IDLE_CHECK_TRUE;
	
	while(1){
		recv_size = recv(low_power_tcp_data->client_fd, low_power_tcp_data->tcp_recv_buf, TCP_TCP_BUF_SIZE, 0);
		if(recv_size < 0){
			printf("\n\r[ERROR] %s: TCP client receive data failed",__func__);
			goto Exit1;
		}
		else if(recv_size == 0){
			tcp_idle_state = TCP_IDLE_CHECK_FALSE;
			printf("\n\r%s: [END] Close TCP client socket", __func__);
			closesocket(low_power_tcp_data->client_fd);
			goto Reconnect;
		}
		if(recv_size < TCP_TCP_BUF_SIZE)
			low_power_tcp_data->tcp_recv_buf[recv_size] = '\0';
		printf("\n\r%s: TCP receive size = %d, %s", __func__, recv_size, low_power_tcp_data->tcp_recv_buf);
		if(send(low_power_tcp_data->client_fd, low_power_tcp_data->tcp_send_buf, TCP_TCP_BUF_SIZE, 0) <= 0){
			printf("\n\r[ERROR] %s: TCP client send data error", __func__);
			goto Exit1;
		}
	}
	
Exit1:
	closesocket(low_power_tcp_data->client_fd);
Exit2:
	printf("\n\r%s: Close client socket",__func__);
	os_task_delete(bt_mesh_tcp_client_task_handle);
	bt_mesh_tcp_client_task_handle = NULL;
	tcp_idle_state = TCP_IDLE_CHECK_TRUE;
	return;
}
#endif

void bt_mesh_ps_task(void *p_param)
{
    /* avoid gcc compile warning */
    (void)p_param;
    uint8_t state = DEVICE_IDLE_CHECK_FALSE;

    while (true) {
        state = bt_mesh_idle_check();
        if (state == DEVICE_IDLE_CHECK_TRUE && tcp_idle_state == TCP_IDLE_CHECK_TRUE && ps_state == ps_state_disable) {
            rltk_coex_ps_enter();
            ps_state = ps_state_enable;
            printf("\r\n Enter powersaving mode");
        } else if ((state == DEVICE_IDLE_CHECK_FALSE || tcp_idle_state == TCP_IDLE_CHECK_FALSE) && ps_state == ps_state_enable) {
            rltk_coex_ps_leave();
            ps_state = ps_state_disable;
            printf("\r\n Leave powersaving mode");
        }

        plt_delay_ms(200);
    }
}

void bt_mesh_power_saving_init(void)
{
    os_task_create(&bt_mesh_ps_task_handle, "bt_mesh_idle_check_app", bt_mesh_ps_task, 0, APP_TASK_STACK_SIZE,
                   APP_TASK_PRIORITY);
#if LOW_POWER_TCP_TEST
	if (!low_power_tcp_data) {
		low_power_tcp_data = (struct low_power_tcp_data_t*)pvPortMalloc(sizeof(struct low_power_tcp_data_t));
		if (!low_power_tcp_data) {
			printf("\n\r[ERROR] %s: Alloc buffer failed",__func__);
			return;
		}
	}
	os_task_create(&bt_mesh_tcp_client_task_handle, "bt_mesh_tcp_client_app", bt_mesh_tcp_client_task, 0, APP_TASK_STACK_SIZE,
                   APP_TASK_PRIORITY);
#endif
}

void bt_mesh_power_saving_deinit(void)
{
	//leave ps
    rltk_coex_ps_leave();
    if (bt_mesh_ps_task_handle) {
		os_task_delete(bt_mesh_ps_task_handle);
	}
#if LOW_POWER_TCP_TEST
    bt_mesh_tcp_client_task_handle = NULL;
	if (bt_mesh_tcp_client_task_handle) {
		os_task_delete(bt_mesh_tcp_client_task_handle);
	}
	if (low_power_tcp_data->client_fd >= 0) {
		closesocket(low_power_tcp_data->client_fd);
	}
	if (low_power_tcp_data) {
		vPortFree(low_power_tcp_data);
		low_power_tcp_data = NULL;
	}
#endif
}

