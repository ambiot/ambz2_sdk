#include <platform_opts_bt.h>
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT) && CONFIG_BT_OTA_CENTRAL_CLIENT
#if defined(CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT) && CONFIG_BT_OTA_CENTRAL_CLIENT_W_REQ_CONFLICT
#include "platform_stdlib.h"
#include <string.h>
#include <os_mem.h>
#include <os_queue.h>
#include <insert_write.h>

T_OS_QUEUE write_req_queue[4];

uint8_t request_in_process_flag[4] = {0, 0, 0, 0}; //e.g if send request then flag=1, receive response then flag=0

extern void bt_ota_central_client_app_discov_services(uint8_t conn_id, bool start);


/* if add write request information to queue */
/* @return 0 -- queue is empty
 *         1 -- add write request info to queue
           2 -- add discovery service info to queue
 *        -1 -- add data to queue fail
 */
int if_queue_in(uint8_t type, uint8_t conn_id, T_CLIENT_ID client_id, uint16_t handle, uint16_t length, uint8_t *p_data)
{
	if(os_queue_peek(&write_req_queue[conn_id]) == NULL && request_in_process_flag[conn_id] == 0)  // queue empty, and no requesting
	{
		return 0;
	}
	else //if (user_req_flag == 1) //queue empty and exist requesting, or queue not empty and no requesting, or  queue not empty and exist requesting
	{
		if(type == 0) //write request info queue in 
		{
			/* for send data to queue */
			WRITE_REQ_INFO *add_write_data = NULL;
			
			add_write_data = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(WRITE_REQ_INFO));
			if(add_write_data == NULL)
			{
				printf("WRITE_REQ_INFO malloc fail\r\n");
				return -1;
			}
			memset(add_write_data, 0, sizeof(WRITE_REQ_INFO));

			add_write_data->conn_id = conn_id;
			add_write_data->client_id = client_id;
			add_write_data->handle = handle;
			add_write_data->length = length;
			if(add_write_data->length != 0)
			{
				add_write_data->p_data = os_mem_zalloc(RAM_TYPE_DATA_ON, length);
				if(add_write_data->p_data == NULL)
				{
					printf("malloc fail\r\n");
					os_mem_free(add_write_data);
					return -1;
				}
				memcpy(add_write_data->p_data, p_data, length);
			}
			
			os_queue_in(&write_req_queue[conn_id], (void *) add_write_data);	
			printf("(conn_id %d, client_id %d)add write request to queue\r\n", conn_id, client_id);
			
			return 1;
		}

		if(type == 1) //discovery service info queue in
		{
			WRITE_REQ_INFO *add_service_info = NULL;
			
			add_service_info = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(WRITE_REQ_INFO));
			if(add_service_info == NULL)
			{
				printf("WRITE_REQ_INFO(of service info) malloc fail\r\n");
				return -1;
			}
			memset(add_service_info, 0, sizeof(WRITE_REQ_INFO));
			add_service_info->conn_id = conn_id;
			add_service_info->status = 1; //flag: exist a write request before discovery service.

			os_queue_in(&write_req_queue[conn_id], (void *) add_service_info); 
			printf("(conn_id %d) add discovery service info to queue\r\n", conn_id);

			return 2;
		}
	}

	return -1;
}


/* if take out write request(or discovery service) information from queue */
int if_queue_out_and_send(uint8_t conn_id)
{
	WRITE_REQ_INFO *out_write_data = NULL;
	out_write_data = os_queue_out(&write_req_queue[conn_id]);

	if(out_write_data)
	{
		if(out_write_data->status == 1)
		{
			printf("exist discovery service info, os_queue_out\r\n");

			//after user write request send success, then start discovery service
			bt_ota_central_client_app_discov_services(conn_id, true);

			os_mem_free(out_write_data);

			return 1;
		}
		else
		{
			printf("(conn_id %d, client_id %d) os_queue_out\r\n", conn_id, out_write_data->client_id);
			
			if(out_write_data->p_data)
			{
				request_in_process_flag[conn_id] = 1;
				if(client_attr_write(out_write_data->conn_id, out_write_data->client_id, GATT_WRITE_TYPE_REQ, out_write_data->handle, 
									out_write_data->length, out_write_data->p_data) != GAP_CAUSE_SUCCESS)
				{
					request_in_process_flag[conn_id] = 0;
					printf("(conn_id %d, queue out)write request fail, please check!!!\r\n", conn_id);
				}
				os_mem_free(out_write_data->p_data);
			}

			os_mem_free(out_write_data);

			return 2;
		}
	}

	return 0;
}

/* if disconnected during ota process, then need queue out and free memory */
void disconnect_and_queue_out(uint8_t conn_id)
{
	WRITE_REQ_INFO *out_req = NULL;
	int i = 0;

	while(true)
	{
		out_req = os_queue_out(&write_req_queue[conn_id]);
		if(out_req)
		{
			printf("exist items in queue, queue out item %d\r\n", i++);
			
			if(out_req->p_data)
			{
				os_mem_free(out_req->p_data);
				out_req->p_data = NULL;
			}

			os_mem_free(out_req);
			out_req = NULL;
		}
		else
		{
			/* clear flag and queue */
			request_in_process_flag[conn_id] = 0;
			memset(&write_req_queue[conn_id], 0, sizeof(T_OS_QUEUE));

			break;
		}
	}

}

void connect_and_init(uint8_t conn_id)
{
	/* init flag and queue*/
	request_in_process_flag[conn_id] = 0;
	memset(&write_req_queue[conn_id], 0, sizeof(T_OS_QUEUE));

	os_queue_init(&write_req_queue[conn_id]);
}
#endif
#endif
