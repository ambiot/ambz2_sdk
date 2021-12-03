#ifndef _INSERT_WRITE_H_
#define _INSERT_WRITE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <profile_client.h>
#include <os_queue.h>

/* used for storage write request info during ota process */
typedef struct write_req_info_t
{
	struct write_req_info_t *p_next; // Pointer to the next item, must be the first field.
	uint8_t			status; //0 -- default, 1 -- mark service discovery process.
	T_CLIENT_ID		client_id;
	uint8_t			conn_id;
	uint16_t		handle;
	uint16_t		length;
	uint8_t			*p_data;
} WRITE_REQ_INFO;


extern T_OS_QUEUE write_req_queue[4];

extern uint8_t request_in_process_flag[4]; //if send request then flag=1, receive response then flag=0

int if_queue_in(uint8_t type, uint8_t conn_id, T_CLIENT_ID client_id, uint16_t handle, uint16_t length, uint8_t *p_data);

int if_queue_out_and_send(uint8_t conn_id);

void disconnect_and_queue_out(uint8_t conn_id);

void connect_and_init(uint8_t conn_id);


#ifdef __cplusplus
}
#endif

#endif
