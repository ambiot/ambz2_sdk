#include <platform_opts_bt.h>
#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) || \
	(defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE))
#include <string.h>
#include <trace_app.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap.h>
#include <gap_conn_le.h>
#include <gcs_client.h>
#include "ble_central_client_app.h"
#include "log_service.h"
#include "atcmd_bt.h"
#include "ble_central_at_cmd.h"
#include "os_msg.h"
#include <platform/platform_stdlib.h>
#include "os_sched.h"
#include "os_mem.h"

#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
#include "ble_central_app_flags.h"
extern void *ble_central_evt_queue_handle;
extern void *ble_central_io_queue_handle;
extern T_GAP_DEV_STATE ble_central_gap_dev_state;
#endif

#if defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET
#include "ble_scatternet_app_flags.h"
extern int ble_scatternet_central_app_max_links;
extern void *ble_scatternet_evt_queue_handle;
extern void *ble_scatternet_io_queue_handle;
extern T_GAP_DEV_STATE ble_scatternet_gap_dev_state;
#endif

#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
	(defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
#if defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE
#include "bt_mesh_provisioner_multiple_profile_app_flags.h"
extern void *bt_mesh_provisioner_multiple_profile_evt_queue_handle;
extern void *bt_mesh_provisioner_multiple_profile_io_queue_handle;
extern T_GAP_DEV_STATE bt_mesh_provisioner_multiple_profile_gap_dev_state;
#elif defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
#include "bt_mesh_device_multiple_profile_app_flags.h"
extern void *bt_mesh_device_multiple_profile_evt_queue_handle;
extern void *bt_mesh_device_multiple_profile_io_queue_handle;
extern T_GAP_DEV_STATE bt_mesh_device_multiple_profile_gap_dev_state;
#endif
extern int bt_mesh_multiple_profile_scan_state;
#endif

#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
extern int bt_mesh_scatternet_central_app_max_links;
#endif

#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
#if defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
#include "bt_mesh_device_matter_app_flags.h"
#include "bt_mesh_device_matter_app_task.h"
extern T_GAP_DEV_STATE bt_mesh_device_matter_gap_dev_state;
extern int bt_mesh_device_matter_scan_state;
extern int bt_mesh_device_matter_central_app_max_links;
#endif
#endif

#if defined (CONFIG_BT_WHITE_LIST_TO_FLASH) && CONFIG_BT_WHITE_LIST_TO_FLASH
#include <flash_api.h>
#include "device_lock.h"
#endif

#define BD_ADDR_LEN							6
#if defined (CONFIG_BT_WHITE_LIST_TO_FLASH) && CONFIG_BT_WHITE_LIST_TO_FLASH
#define MAX_N								16
#define ADDR_LEN							8
#define ADDR_NO								32
#define VALID_SIGN							0x10
#define INVALID_SIGN							0xff
#define BT_TYPE_PUBLIC							0x10
#define BT_TYPE_RANDOM							0x01

u8 page_id = INVALID_SIGN;
u8 row_id = INVALID_SIGN;
u8 erase_current_page = INVALID_SIGN;
u8 read_data[ADDR_LEN*ADDR_NO];
u8 whitelist_flash_init_flag = 0;
#endif

static u8 ctoi(char c)
{
	if((c >= 'A') && (c <= 'F')) {
		return (c - 'A' + 0x0A);
	}

	if((c >= 'a') && (c <= 'f')) {
		return (c - 'a' + 0x0A);
	}

	if((c >= '0') && (c <= '9')) {
		return (c - '0' + 0x00);
	}

	return 0xFF;
}

static u8 hex_str_to_bd_addr(u32 str_len, s8 *str, u8 *num_arr)
{
	num_arr += str_len/2 -1;
	u32 n = 0;
	u8 num = 0;

	if (str_len < 2) {
		return FALSE;
	}
	while (n < str_len) {
		if ((num = ctoi(str[n++])) == 0xFF) {
			return FALSE;
		}
		*num_arr = num << 4;
		if ((num = ctoi(str[n++])) == 0xFF) {
			return FALSE;
		}
		*num_arr |= num;
		num_arr--;
	}
	return TRUE;
}

static int hex_str_to_int(u32 str_len, s8*str)
{
	int result = 0;
	unsigned int n = 2;
	if((str_len < 3) || (str[0] != '0') || ((str[1] != 'x') && (str[1] != 'X'))){
		return -1;
	}
	while(n < str_len){
		result = (result << 4) | (ctoi(str[n++]));
	}
	return result;
}

#if 0
static int dec_str_to_int(u32 str_len, s8*str)
{
	int result = 0;
	unsigned int n = 0;

	while(n < str_len){
		result = result*10 + (ctoi(str[n++]));
	}
	return result;
}
#endif

#if defined (CONFIG_BT_WHITE_LIST_TO_FLASH) && CONFIG_BT_WHITE_LIST_TO_FLASH
void read_white_list_data(int page_id, int row_id);
u8 whitelist_find(uint32_t address)
{
	flash_t flash;
	u8 valid_data[1] = {VALID_SIGN};
	u8 block_content[MAX_N];
	u8 read_bt_address[ADDR_LEN];
	u8 empty_data[ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	u8 invalid_flag = INVALID_SIGN;
	u8 block_sum = 0;
	u8 valid_value = 0;
	u8 n = INVALID_SIGN;

	//read data block by block in 4k(1) or 4k(2), each block has 8*32=256 bytes
	for(u8 i = 0; i < MAX_N; i++){
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, address + i*ADDR_LEN*ADDR_NO, ADDR_LEN*ADDR_NO, read_data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		//analyse read_data
		for(u8 j = 0; j<ADDR_NO; j++){
			memcpy(read_bt_address, &read_data[j*ADDR_LEN], ADDR_LEN);
			if(memcmp(&read_data[j*ADDR_LEN],valid_data,1) == 0){
				valid_value = 1;
				break;
			}else if(memcmp(read_bt_address,empty_data,ADDR_LEN) == 0){
				valid_value = 0;
			}else{
				invalid_flag = 1;
				break;
			}
		}
		if(invalid_flag == 1){
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			flash_erase_sector(&flash, address);
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			return n = INVALID_SIGN;
		}
		block_content[i] = valid_value;
		block_sum += block_content[i];
		if((block_content[i] == 1) && (block_sum == i + 1)){
			n = i;
		}else{
			break;
		}
	}
	if(block_sum == 0){
		n = INVALID_SIGN;
	}
	return n;
}

void save_white_list_data(uint8_t white_list_data[ADDR_LEN*ADDR_NO])
{
	flash_t flash;
	if(erase_current_page == 1){
		//dbg_printf("[%s]erase_current_page: page %d.\n", __func__, page_id);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, BT_WHITELIST_BASE_1 + ((page_id == 1)? 0: 1)*BT_WHITELIST_PAGE_SIZE);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		page_id = (page_id == 1)? 2: 1;
		row_id = 0;
		erase_current_page = 0;
		read_white_list_data(page_id, row_id);
	}else{
		//dbg_printf("[%s]write white list data to page %d, row %d.\n", __func__, page_id, row_id);
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_write(&flash, BT_WHITELIST_BASE_1 + (page_id - 1)*BT_WHITELIST_PAGE_SIZE + row_id*ADDR_LEN*ADDR_NO, ADDR_LEN*ADDR_NO, white_list_data);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		if(row_id == 0){
			//dbg_printf("[%s]erase_last_page: page %d.\n", __func__, (page_id == 1)? 2: 1);
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			flash_erase_sector(&flash, BT_WHITELIST_BASE_1 + ((page_id == 1)? 1: 0)*BT_WHITELIST_PAGE_SIZE);
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
		}

		row_id++;
		if(row_id == MAX_N){
			page_id = (page_id == 1)? 2: 1;
			row_id = 0;
			//dbg_printf("[%s]row 15 has data, next write to the next page.\n", __func__);
		}
	}
	/*
	dbg_printf("[%s]display real-time white_list_data.\n", __func__);
	for(u8 i = 0; i < ADDR_NO; i++){
			if(white_list_data[i*ADDR_LEN] == VALID_SIGN){
				if(white_list_data[i*ADDR_LEN + 1] == BT_TYPE_PUBLIC){
					printf("[Public] ");
					for(u8 j = 0; j < BD_ADDR_LEN; j++){
						printf("0x%x ", white_list_data[i*ADDR_LEN + 2 + j]);
					}
					printf("\n");
				}else if (white_list_data[i*ADDR_LEN + 1] == BT_TYPE_RANDOM){
					printf("[Random] ");
					for(u8 j = 0; j < BD_ADDR_LEN; j++){
						printf("0x%x ", white_list_data[i*ADDR_LEN + 2 + j]);
					}
					printf("\n");
				}
			}
		}
	*/
}

void clear_white_list_data(void)
{
	if(whitelist_flash_init_flag == 1){
		erase_current_page = 1;
		save_white_list_data(read_data);
	}else{
		printf("White list haven't be initiated from flash, enter ATBn = 3 to load white list data from flash.\n");
	}
}

void remove_white_list_data(T_GAP_REMOTE_ADDR_TYPE DestAddrType, u8 DestAddr[BD_ADDR_LEN])
{
	if(whitelist_flash_init_flag == 1){
		u8 count = 0;
		int find_flag=0;
		u8 i;

		for( i = 0; i < ADDR_NO; i++){
			if(read_data[i*ADDR_LEN] == VALID_SIGN){
				count++;
			}
		}

		for( i = 0; i < ADDR_NO; i++){
			if((read_data[i*ADDR_LEN] == VALID_SIGN)
			&& (read_data[i*ADDR_LEN + 1] == ((DestAddrType == GAP_REMOTE_ADDR_LE_PUBLIC)? BT_TYPE_PUBLIC : BT_TYPE_RANDOM))
			&& (read_data[i*ADDR_LEN + 2] == DestAddr[5])
			&& (read_data[i*ADDR_LEN + 3] == DestAddr[4])
			&& (read_data[i*ADDR_LEN + 4] == DestAddr[3])
			&& (read_data[i*ADDR_LEN + 5] == DestAddr[2])
			&& (read_data[i*ADDR_LEN + 6] == DestAddr[1])
			&& (read_data[i*ADDR_LEN + 7] == DestAddr[0])){
				find_flag=1;
				break;
			}
		}

		if(count == 0){
			printf("All invalid, no address to remove.\n");
		}else if((count == 1) && (find_flag == 1)){
			clear_white_list_data();
		}else if(find_flag == 1){
			printf("Address match, ready to remove.\n");
			for (u8 j = 0; j < ADDR_LEN; j++){
				read_data[i*ADDR_LEN + j] = 0xff;
			}
			save_white_list_data(read_data);
		}else{
			printf("Address wrong.\n");
		}
	}else{
		printf("White list haven't be initiated from flash, enter ATBn = 3 to load white list data from flash.\n");
	}
}

void read_white_list_data(int page_id, int row_id)
{
	flash_t flash;
	int ret;
	uint32_t BT_WHITELIST_BASE;
	u8 DestAddr[BD_ADDR_LEN];
	T_GAP_REMOTE_ADDR_TYPE DestAddrType;

	switch(page_id){
		case 1:
			BT_WHITELIST_BASE = BT_WHITELIST_BASE_1 + row_id*ADDR_LEN*ADDR_NO;
			break;
		case 2:
			BT_WHITELIST_BASE = BT_WHITELIST_BASE_2 + row_id*ADDR_LEN*ADDR_NO;
			break;
		default:
			break;
	}

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, BT_WHITELIST_BASE, ADDR_LEN*ADDR_NO, read_data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	for(u8 i = 0; i < ADDR_NO; i++){
		if(read_data[i*ADDR_LEN] == VALID_SIGN){
			if(read_data[i*ADDR_LEN + 1] == BT_TYPE_PUBLIC){
				DestAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
			}else if(read_data[i*ADDR_LEN + 1] == BT_TYPE_RANDOM){
				DestAddrType = GAP_REMOTE_ADDR_LE_RANDOM;
			}
		DestAddr[5] = read_data[i*ADDR_LEN + 2];
		DestAddr[4] = read_data[i*ADDR_LEN + 3];
		DestAddr[3] = read_data[i*ADDR_LEN + 4];
		DestAddr[2] = read_data[i*ADDR_LEN + 5];
		DestAddr[1] = read_data[i*ADDR_LEN + 6];
		DestAddr[0] = read_data[i*ADDR_LEN + 7];

		ret = le_modify_white_list(GAP_WHITE_LIST_OP_ADD, DestAddr, DestAddrType);
			if(ret == 0){
				printf("cmd_modify, DestAddr: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\r\n",
				DestAddr[5], DestAddr[4], DestAddr[3], DestAddr[2], DestAddr[1], DestAddr[0]);
			}else{
				printf("white list addr error. Remove DestAddr: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\r\n",
				DestAddr[5], DestAddr[4], DestAddr[3], DestAddr[2], DestAddr[1], DestAddr[0]);
				remove_white_list_data(DestAddrType, DestAddr);
			}
		}
	}
}

void whitelist_flash_init(void)
{
	flash_t flash;
	u8 n1 = INVALID_SIGN;
	u8 n2 = INVALID_SIGN;

	//find which block saved the latest white list data
	n1 = whitelist_find(BT_WHITELIST_BASE_1);
	n2 = whitelist_find(BT_WHITELIST_BASE_2);
	//dbg_printf("[%s] find block %d in 4k(1), find block %d in 4k(2).\n", __func__, n1, n2);
	if((n1 == n2) && (n1 == INVALID_SIGN)){
		page_id = 1;
		row_id = 0;
		read_white_list_data(page_id, row_id);
	}else if(n1 == INVALID_SIGN){
		if (n2 != 15){
			page_id = 2;
			row_id = n2 + 1;
			read_white_list_data(page_id, n2);
		}else{
			page_id = 1;
			row_id = 0;
			read_white_list_data(page_id + 1, n2);
		}
	}else if(n2 == INVALID_SIGN){
		if (n1 != 15){
			page_id = 1;
			row_id = n1 + 1;
			read_white_list_data(page_id, n1);
		}else{
			page_id = 2;
			row_id = 0;
			read_white_list_data(page_id - 1, n1);
		}
	}else if((n1 < n2) && (n1 >= 0)){
		page_id = 1;
		row_id = n1 + 1;
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, BT_WHITELIST_BASE_2);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		read_white_list_data(page_id, n1);
	}else if((n2 < n1) && (n2 >= 0)){
		page_id = 2;
		row_id = n2 + 1;
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, BT_WHITELIST_BASE_1);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		read_white_list_data(page_id, n2);
	}else{
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_erase_sector(&flash, BT_WHITELIST_BASE_1);
		flash_erase_sector(&flash, BT_WHITELIST_BASE_2);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		page_id = 1;
		row_id = 0;
		read_white_list_data(page_id, row_id);
	}
	//whitelist_flash_init complete
	whitelist_flash_init_flag = 1;
	//dbg_printf("[%s] record next data in page %d block %d.\n", __func__, page_id, row_id);
	printf("White list initiated from flash.");
}

void display_white_list_data(uint8_t white_list_data[ADDR_LEN*ADDR_NO])
{
	if(whitelist_flash_init_flag == 1){
		for(u8 i = 0; i < ADDR_NO; i++){
			if(white_list_data[i*ADDR_LEN] == VALID_SIGN){
				if(white_list_data[i*ADDR_LEN + 1] == BT_TYPE_PUBLIC){
					printf("[Public] ");
					for(u8 j = 0; j < BD_ADDR_LEN; j++){
						printf("0x%x ", white_list_data[i*ADDR_LEN + 2 + j]);
					}
					printf("\n");
				}else if(white_list_data[i*ADDR_LEN + 1] == BT_TYPE_RANDOM){
					printf("[Random] ");
					for(u8 j = 0; j < BD_ADDR_LEN; j++){
						printf("0x%x ", white_list_data[i*ADDR_LEN + 2 + j]);
					}
					printf("\n");
				}
			}
		}
	}else{
		printf("White list haven't be initiated from flash, enter ATBn = 3 to load white list data from flash.\n");
	}
}

void add_white_list_data(T_GAP_REMOTE_ADDR_TYPE DestAddrType, u8 DestAddr[BD_ADDR_LEN])
{
	if(whitelist_flash_init_flag == 1){
		int save_flag = 0;

		for(u8 i = 0; i < ADDR_NO; i++){
			if ((read_data[i*ADDR_LEN] == VALID_SIGN)
			&& (read_data[i*ADDR_LEN+1] == ((DestAddrType==GAP_REMOTE_ADDR_LE_PUBLIC)? BT_TYPE_PUBLIC : BT_TYPE_RANDOM))
			&& (read_data[i*ADDR_LEN+2] == DestAddr[5])
			&& (read_data[i*ADDR_LEN+3] == DestAddr[4])
			&& (read_data[i*ADDR_LEN+4] == DestAddr[3])
			&& (read_data[i*ADDR_LEN+5] == DestAddr[2])
			&& (read_data[i*ADDR_LEN+6] == DestAddr[1])
			&& (read_data[i*ADDR_LEN+7] == DestAddr[0])){
				printf("BT address already saved.\n");
				return;
			}
		}

		for(u8 i = 0; i < ADDR_NO; i++){
			if(read_data[i*ADDR_LEN] == INVALID_SIGN){
				read_data[i*ADDR_LEN + 1] = (DestAddrType == GAP_REMOTE_ADDR_LE_PUBLIC) ? BT_TYPE_PUBLIC : BT_TYPE_RANDOM;
				for(u8 j = 0; j < BD_ADDR_LEN; j++){
					read_data[i*ADDR_LEN + 2 + j]= DestAddr[5 - j];
				}
				read_data[i*ADDR_LEN] = VALID_SIGN;
				save_flag = 1;
				break;
			}
		}

		if(save_flag){
			save_white_list_data(read_data);
			printf("Address saved.\n");
			save_flag = 0;
		}else{
			printf("Adress list full.\n");
		}
	}else{
		printf("White list haven't be initiated from flash, enter ATBn = 3 to load white list data from flash.\n");
	}
}
#endif

void ble_central_at_cmd_send_msg(uint16_t sub_type)
{
	uint8_t event = EVENT_IO_TO_APP;

	T_IO_MSG io_msg;

	io_msg.type = IO_MSG_TYPE_QDECODE;
	io_msg.subtype = sub_type;

#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
	if (ble_central_evt_queue_handle != NULL && ble_central_io_queue_handle != NULL) {
		if (os_msg_send(ble_central_io_queue_handle, &io_msg, 0) == false) {
			BLE_PRINT("ble central at cmd send msg fail: subtype 0x%x", io_msg.subtype);
		} else if (os_msg_send(ble_central_evt_queue_handle, &event, 0) == false) {
			BLE_PRINT("ble central at cmd send event fail: subtype 0x%x", io_msg.subtype);
		}
	}
#endif
#if defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET
	if (ble_scatternet_evt_queue_handle != NULL && ble_scatternet_io_queue_handle != NULL) {
		if (os_msg_send(ble_scatternet_io_queue_handle, &io_msg, 0) == false) {
			BLE_PRINT("ble central at cmd send msg fail: subtype 0x%x", io_msg.subtype);
		} else if (os_msg_send(ble_scatternet_evt_queue_handle, &event, 0) == false) {
			BLE_PRINT("ble central at cmd send event fail: subtype 0x%x", io_msg.subtype);
		}
	}
#endif
#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
	(defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
#if defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE
	if (bt_mesh_provisioner_multiple_profile_evt_queue_handle != NULL && bt_mesh_provisioner_multiple_profile_io_queue_handle != NULL) {
		if (os_msg_send(bt_mesh_provisioner_multiple_profile_io_queue_handle, &io_msg, 0) == false) {
			BLE_PRINT("ble central at cmd send msg fail: subtype 0x%x", io_msg.subtype);
		} else if (os_msg_send(bt_mesh_provisioner_multiple_profile_evt_queue_handle, &event, 0) == false) {
			BLE_PRINT("ble central at cmd send event fail: subtype 0x%x", io_msg.subtype);
		}
	}
#endif
#if defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
	if (bt_mesh_device_multiple_profile_evt_queue_handle != NULL && bt_mesh_device_multiple_profile_io_queue_handle != NULL) {
		if (os_msg_send(bt_mesh_device_multiple_profile_io_queue_handle, &io_msg, 0) == false) {
			BLE_PRINT("ble central at cmd send msg fail: subtype 0x%x", io_msg.subtype);
		} else if (os_msg_send(bt_mesh_device_multiple_profile_evt_queue_handle, &event, 0) == false) {
			BLE_PRINT("ble central at cmd send event fail: subtype 0x%x", io_msg.subtype);
		}
	}
#endif //device multiple profile end
#endif
#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
#if defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
	if (bt_mesh_device_matter_evt_queue_handle != NULL && bt_mesh_device_matter_io_queue_handle != NULL) {
		if (os_msg_send(bt_mesh_device_matter_io_queue_handle, &io_msg, 0) == false) {
			BLE_PRINT("ble central at cmd send msg fail: subtype 0x%x", io_msg.subtype);
		} else if (os_msg_send(bt_mesh_device_matter_evt_queue_handle, &event, 0) == false) {
			BLE_PRINT("ble central at cmd send event fail: subtype 0x%x", io_msg.subtype);
		}
	}
#endif
#endif
}

int ble_central_at_cmd_connect(int argc, char **argv)
{
	(void) argc;
#if defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET
	if(ble_scatternet_central_app_max_links >= BLE_SCATTERNET_CENTRAL_APP_MAX_LINKS){
		BLE_PRINT("scatternet: exceed the max links number\r\n");
		return 0;
	}
#endif
#if defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET
	if(bt_mesh_scatternet_central_app_max_links >= BLE_SCATTERNET_CENTRAL_APP_MAX_LINKS){
		BLE_PRINT("scatternet: exceed the max links number\r\n");
		return 0;
	}
#endif
#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
	if (bt_mesh_device_matter_central_app_max_links >= BT_MESH_DEVICE_MATTER_CENTRAL_APP_MAX_LINKS) {
		BLE_PRINT("scatternet: exceed the max links number\r\n");
		return 0;
	}
#endif

	u8 DestAddr[6] = {0};
	u8 DestAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
#if (F_BT_LE_USE_RANDOM_ADDR==1)
	T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_RANDOM;
#elif (F_BT_LE_USE_RANDOM_ADDR==2) && F_BT_LE_LOCAL_IRK_SETTING_SUPPORT
	T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_RANDOM;
#else
	T_GAP_LOCAL_ADDR_TYPE local_addr_type = GAP_LOCAL_ADDR_LE_PUBLIC;
#endif
	T_GAP_LE_CONN_REQ_PARAM conn_req_param;

	if(strcmp(argv[1], "P") == 0)
		DestAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
	else if(strcmp(argv[1], "R") == 0)
		DestAddrType = GAP_REMOTE_ADDR_LE_RANDOM;

	if (strlen(argv[2]) != 2*BD_ADDR_LEN) {
		BLE_PRINT("ERROR: mac address length error!\r\n");
		return -1;
	}

	hex_str_to_bd_addr(strlen(argv[2]), ( s8 *)argv[2], (u8*)DestAddr);

	conn_req_param.scan_interval = 0xA0;	//100ms
	conn_req_param.scan_window = 0x80;		//80ms
	conn_req_param.conn_interval_min = 0x60;	//120ms
	conn_req_param.conn_interval_max = 0x60;	//120ms
	conn_req_param.conn_latency = 0;
	conn_req_param.supv_tout = 1000;
	conn_req_param.ce_len_min = 2 * (conn_req_param.conn_interval_min - 1);
	conn_req_param.ce_len_max = 2 * (conn_req_param.conn_interval_max - 1);
	le_set_conn_param(GAP_CONN_PARAM_1M, &conn_req_param);

	BLE_PRINT("cmd_con, DestAddr: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\r\n",
			DestAddr[5], DestAddr[4], DestAddr[3], DestAddr[2], DestAddr[1], DestAddr[0]);

	le_connect(0, DestAddr, (T_GAP_REMOTE_ADDR_TYPE)DestAddrType, local_addr_type, 1000);

	return 0;
}

int ble_central_at_cmd_modify_whitelist(int argc, char **argv)
{
	int ret;
	(void) argc;
	u8 DestAddr[6] = {0};
	T_GAP_REMOTE_ADDR_TYPE DestAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
	T_GAP_WHITE_LIST_OP operation = GAP_WHITE_LIST_OP_ADD;
	u8 type;

	type = atoi(argv[1]);
#if defined (CONFIG_BT_WHITE_LIST_TO_FLASH) && CONFIG_BT_WHITE_LIST_TO_FLASH
	if((type != 0) && (type != 1) && (type !=2) && (type != 3) && (type !=4)){
#else
	if((type != 0) && (type != 1) && (type !=2)){
#endif
		BLE_PRINT("unknow operation code, return\r\n");
		return -1;
	}

	if(type == 0){
		ret = le_modify_white_list(GAP_WHITE_LIST_OP_CLEAR, NULL, GAP_REMOTE_ADDR_LE_PUBLIC);
#if defined (CONFIG_BT_WHITE_LIST_TO_FLASH) && CONFIG_BT_WHITE_LIST_TO_FLASH
		if (ret == 0) clear_white_list_data();
	}else if(type == 3){
		whitelist_flash_init();
	}else if(type == 4){
		display_white_list_data(read_data);
#endif
	}else{
		if (argc != 4){
			BLE_PRINT("ERROR:input parameter error!\r\n");
			return -1;
		}

		if(type == 1)
			operation = GAP_WHITE_LIST_OP_ADD;
		else if(type == 2)
			operation = GAP_WHITE_LIST_OP_REMOVE;

		if(strcmp(argv[2], "P") == 0)
			DestAddrType = GAP_REMOTE_ADDR_LE_PUBLIC;
		else if(strcmp(argv[2], "R") == 0)
			DestAddrType = GAP_REMOTE_ADDR_LE_RANDOM;

		if (strlen(argv[3]) != 2*BD_ADDR_LEN)
			return -1;

		hex_str_to_bd_addr(strlen(argv[3]), ( s8 *)argv[3], (u8*)DestAddr);

		BLE_PRINT("cmd_modify, DestAddr: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\r\n",
			DestAddr[5], DestAddr[4], DestAddr[3], DestAddr[2], DestAddr[1], DestAddr[0]);

		ret = le_modify_white_list(operation, DestAddr, DestAddrType);
#if defined (CONFIG_BT_WHITE_LIST_TO_FLASH) && CONFIG_BT_WHITE_LIST_TO_FLASH
		if(ret==0){
			if (operation == GAP_WHITE_LIST_OP_ADD)
				add_white_list_data(DestAddrType, DestAddr);
			else if(operation == GAP_WHITE_LIST_OP_REMOVE)
				remove_white_list_data(DestAddrType, DestAddr);
		}
#endif
	}

	return 0;
}

int ble_central_at_cmd_disconnect(int argc, char **argv)
{
	(void) argc;
	u8 conn_id = atoi(argv[1]);
	int ret;
	ret = le_disconnect(conn_id);
	return ret;
}

int ble_central_at_cmd_get_conn_info(int argc, char **argv)
{
	(void) argc;
	(void) argv;
	u8 conn_id;
	u8 conn_max_link;
	T_GAP_CONN_INFO conn_info;
#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL))
	conn_max_link = BLE_CENTRAL_APP_MAX_LINKS;
#endif
#if ((defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET) || \
	(defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
	conn_max_link = BLE_SCATTERNET_APP_MAX_LINKS;
#endif
#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
	conn_max_link = BT_MESH_DEVICE_MATTER_APP_MAX_LINKS;
#endif

	for (conn_id = 0; conn_id < conn_max_link; conn_id++)
	{
		if (le_get_conn_info(conn_id, &conn_info))
		{
			BLE_PRINT("ShowCon conn_id %d state 0x%x role %d\r\n", conn_id,
							conn_info.conn_state, conn_info.role);
			BLE_PRINT("RemoteBd = [%02x:%02x:%02x:%02x:%02x:%02x] type = %d\r\n",
							conn_info.remote_bd[5], conn_info.remote_bd[4],
							conn_info.remote_bd[3], conn_info.remote_bd[2],
							conn_info.remote_bd[1], conn_info.remote_bd[0],
							conn_info.remote_bd_type);
		}
	}
	BLE_PRINT("active link num %d, idle link num %d\r\n", le_get_active_link_num(), le_get_idle_link_num());
	return 0;
}

int ble_central_at_cmd_update_conn_request(int argc, char **argv)
{
	(void) argc;
	int ret;
	u8 conn_id = atoi(argv[1]);
	u16 conn_interval_min = hex_str_to_int(strlen(argv[2]),(s8 *) argv[2]);
	u16 conn_interval_max = hex_str_to_int(strlen(argv[3]), (s8 *)argv[3]);
	u16 conn_latency = hex_str_to_int(strlen(argv[4]), (s8 *)argv[4]);
	u16 supervision_timeout = hex_str_to_int(strlen(argv[5]), (s8 *)argv[5]);

	ret = le_update_conn_param(conn_id,
								conn_interval_min,
								conn_interval_max,
								conn_latency,
								supervision_timeout,
								2 * (conn_interval_min - 1),
								2 * (conn_interval_max - 1));
	return ret;
}

int ble_central_at_cmd_bond_information(int argc, char **argv)
{
	(void) argc;

	if(strcmp(argv[1],"CLEAR") == 0) {
		le_bond_clear_all_keys();
	}else if(strcmp(argv[1], "INFO") == 0){
		u8 i;
		T_LE_KEY_ENTRY *p_entry;
		for (i = 0; i < bond_storage_num; i++) {
			p_entry = le_find_key_entry_by_idx(i);
			if (p_entry != NULL) {
			BLE_PRINT("bond_dev[%d]: bd 0x%02x%02x%02x%02x%02x%02x, addr_type %d, flags 0x%x\r\n",
							p_entry->idx,
							p_entry->remote_bd.addr[5],
							p_entry->remote_bd.addr[4],
							p_entry->remote_bd.addr[3],
							p_entry->remote_bd.addr[2],
							p_entry->remote_bd.addr[1],
							p_entry->remote_bd.addr[0],
							p_entry->remote_bd.remote_bd_type,
							p_entry->flags);
			}
		}
	}else{
		return -1;
	}

	return 0;
}

#define UUID_16_LEN		2
#define UUID_128_LEN	16
int ble_central_at_cmd_get(int argc, char **argv)
{
	u8 conn_id;
	u8 uuid_type;
	u8 uuid[16] = {0};
	u16 start_handle;
	u16 end_handle;
	u16 uuid16;

	conn_id = atoi(argv[2]);

	if(strcmp(argv[1], "ALL") == 0) {
		gcs_all_primary_srv_discovery(conn_id);
	}else if(strcmp(argv[1], "SRV") == 0){
		if(argc != 5){
			BLE_PRINT("ERROR:input parameter error!\n\r");
			return -1;
		}
		uuid_type = atoi(argv[3]);
		if(uuid_type == 0){
			if (strlen(argv[4]) != 2 * UUID_16_LEN){
				BLE_PRINT("ERROR:uuid length error!\n\r");
				return -1;
			}
			hex_str_to_bd_addr(strlen(argv[4]), (s8 *)argv[4], (u8 *)uuid);
			uuid16 = (uuid[1]<<8) | uuid[0];
			gcs_by_uuid_srv_discovery(conn_id, uuid16);
		}else if(uuid_type == 1){
			if (strlen(argv[4]) != 2 * UUID_128_LEN){
				BLE_PRINT("ERROR:uuid length error!\n\r");
				return -1;
			}
			hex_str_to_bd_addr(strlen(argv[4]), (s8 *)argv[4],	(u8 *)uuid);
			gcs_by_uuid128_srv_discovery(conn_id, uuid);
		}else{
			BLE_PRINT("ERROR:uuid type error!\n\r");
			return -1;
		}
	}	else if(strcmp(argv[1], "CHARDIS") == 0){
		if(argc != 5){
			BLE_PRINT("ERROR:input parameter error!\n\r");
			return -1;
		}
		start_handle = hex_str_to_int(strlen(argv[3]), (s8 *)argv[3]);
		end_handle = hex_str_to_int(strlen(argv[4]), (s8 *)argv[4]);
		gcs_all_char_discovery(conn_id, start_handle, end_handle);
	}else if(strcmp(argv[1], "CHARUUID") == 0){
		if(argc != 7){
			BLE_PRINT("ERROR:input parameter error!\n\r");
			return -1;
		}

		start_handle = hex_str_to_int(strlen(argv[3]), (s8 *)argv[3]);
		end_handle = hex_str_to_int(strlen(argv[4]), (s8 *)argv[4]);
		uuid_type = atoi(argv[5]);
		if(uuid_type == 0){
			if (strlen(argv[6]) != 2 * UUID_16_LEN){
				BLE_PRINT("ERROR:uuid length error!\n\r");
				return -1;
			}
			hex_str_to_bd_addr(strlen(argv[6]), (s8 *)argv[6], (u8 *)uuid);
			uuid16 = (uuid[1]<<8) | uuid[0];
			gcs_by_uuid_char_discovery(conn_id, start_handle, end_handle, uuid16);
		}else if(uuid_type == 1){
			if (strlen(argv[6]) != 2 * UUID_128_LEN){
				BLE_PRINT("ERROR:uuid length error!\n\r");
				return -1;
			}
			hex_str_to_bd_addr(strlen(argv[6]), (s8 *)argv[6], (u8 *)uuid);
			gcs_by_uuid128_char_discovery(conn_id, start_handle, end_handle, uuid);
		}else{
			BLE_PRINT("ERROR:uuid type error!\n\r");
			return -1;
		}

	}else if(strcmp(argv[1], "CHARDDIS") == 0){
		if(argc != 5)
			return -1;
		start_handle = hex_str_to_int(strlen(argv[3]), (s8 *)argv[3]);
		end_handle = hex_str_to_int(strlen(argv[4]), (s8 *)argv[4]);
		gcs_all_char_descriptor_discovery(conn_id, start_handle, end_handle);
	}
	return 0;

}

int ble_central_at_cmd_scan(int argc, char **argv)
{
	T_GAP_CAUSE cause;
	u8 scan_filter_policy = GAP_SCAN_FILTER_ANY;
	uint8_t scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;
	u8 scan_enable = 0;
	static u8 scan_is_processing = 0;
	T_GAP_DEV_STATE new_state = {0};

	if (argc >= 2) {
		scan_enable = atoi(argv[1]);
		if (scan_enable == 1) {
			if (argc == 3)
				scan_filter_policy = atoi(argv[2]);
			else if (argc == 4) {
				scan_filter_policy = atoi(argv[2]);
				scan_filter_duplicate = atoi(argv[3]);
			}
		}
	}

#if defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET
	if (ble_central_gap_dev_state.gap_init_state || ble_scatternet_gap_dev_state.gap_init_state)
#elif defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
	if (ble_central_gap_dev_state.gap_init_state)
#endif
#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_SCATTERNET) && CONFIG_BT_SCATTERNET))
	{
		if (scan_enable) {
			if (scan_is_processing) {
				BLE_PRINT("Scan is processing, please stop it first\n\r");
			} else {
				scan_is_processing = 1;
				BLE_PRINT("Start scan, scan_filter_policy = %d, scan_filter_duplicate = %d\n\r", scan_filter_policy, scan_filter_duplicate);
				le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy), &scan_filter_policy);
				le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate), &scan_filter_duplicate);
				ble_central_at_cmd_send_msg(3);
			}
		} else {
			if (scan_is_processing) {
				ble_central_at_cmd_send_msg(2);
				BLE_PRINT("Stop scan\n\r");
				scan_is_processing = 0;
			} else
				BLE_PRINT("There is no scan\n\r");
		}
	}
#endif

#if ((defined(CONFIG_BT_MESH_CENTRAL) && CONFIG_BT_MESH_CENTRAL) || \
	(defined(CONFIG_BT_MESH_SCATTERNET) && CONFIG_BT_MESH_SCATTERNET))
#if defined(CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE) && CONFIG_BT_MESH_PROVISIONER_MULTIPLE_PROFILE
	new_state = bt_mesh_provisioner_multiple_profile_gap_dev_state;
#elif defined(CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE) && CONFIG_BT_MESH_DEVICE_MULTIPLE_PROFILE
#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
	new_state = bt_mesh_device_matter_gap_dev_state;
#else
	new_state = bt_mesh_device_multiple_profile_gap_dev_state;
#endif
#endif
	if (new_state.gap_init_state) {
		if (scan_enable) {
			if (scan_is_processing) {
				BLE_PRINT("Scan is processing, please stop it first\n\r");
			} else {
				scan_is_processing = 1;
				BLE_PRINT("Start scan\n\r");

				ble_central_at_cmd_send_msg(2);
				do {
					os_delay(1);
					le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
				} while (new_state.gap_scan_state != GAP_SCAN_STATE_IDLE);

				uint8_t scan_mode = GAP_SCAN_MODE_ACTIVE;
				le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);

				ble_central_at_cmd_send_msg(3);
				do {
					os_delay(1);
					le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
				} while (new_state.gap_scan_state != GAP_SCAN_STATE_SCANNING);
#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
				bt_mesh_device_matter_scan_state = 1;
#else
				bt_mesh_multiple_profile_scan_state = 1;
#endif
			}
		} else {
			if (scan_is_processing) {
#if defined(CONFIG_BT_MESH_DEVICE_MATTER) && CONFIG_BT_MESH_DEVICE_MATTER
				bt_mesh_device_matter_scan_state = 0;
#else
				bt_mesh_multiple_profile_scan_state = 0;
#endif

				ble_central_at_cmd_send_msg(2);
				do {
					os_delay(1);
					le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
				} while (new_state.gap_scan_state != GAP_SCAN_STATE_IDLE);

				uint8_t scan_mode = GAP_SCAN_MODE_PASSIVE;
				le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);

				ble_central_at_cmd_send_msg(3);
				do {
					os_delay(1);
					le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
				} while (new_state.gap_scan_state != GAP_SCAN_STATE_SCANNING);

				BLE_PRINT("Stop scan\n\r");
				scan_is_processing = 0;
			} else
				BLE_PRINT("There is no scan\n\r");
		}
	}
#endif

	return 0;
}

int ble_central_at_cmd_auth(int argc, char **argv)
{
	u8 conn_id;
	u32 passcode;
	T_GAP_CFM_CAUSE confirm;
	int ret;

	if(strcmp(argv[1],"SEND") == 0) {
		if(argc != 3){
			BLE_PRINT("ERROR:input parameter error!\n\r");
			return -1;
		}
		conn_id = atoi(argv[2]);
		le_bond_pair(conn_id);
	}else if(strcmp(argv[1], "KEY") == 0){
		if(argc !=4){
			BLE_PRINT("ERROR:input parameter error!\n\r");
			return -1;
		}
		conn_id = atoi(argv[2]);
		if(strlen(argv[3]) > 6){
			confirm = GAP_CFM_CAUSE_REJECT;
		}else{
			confirm = GAP_CFM_CAUSE_ACCEPT;
		}
		s8* str = (s8 *)argv[3];
		for(unsigned int i = 0; i < strlen(argv[3]); i ++){
			if((str[i ++] < '0') || (str[i ++] > '9')){
				BLE_PRINT("ERROR:input parameter error!\n\r");
				return -1;
			}
		}

		passcode = atoi(argv[3]);
		if (passcode > GAP_PASSCODE_MAX)
		{
			BLE_PRINT("ERROR:passcode is out of range[0-999999] !\n\r");
			confirm = GAP_CFM_CAUSE_REJECT;
		}
		le_bond_passkey_input_confirm(conn_id, passcode, confirm);
	}else if(strcmp(argv[1], "MODE") == 0){
		u8 auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
		u16 auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
		u8 auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
#if F_BT_LE_SMP_OOB_SUPPORT
		u8 oob_enable = false;
#endif
		u8 auth_sec_req_enable = false;
		u16 auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

		if (argc >= 3) {
			auth_flags = hex_str_to_int(strlen(argv[2]), ( s8 *)argv[2]);
			auth_sec_req_flags = auth_flags;
		}
		if (argc >= 4) {
			auth_io_cap = atoi(argv[3]);
		}
		if (argc >= 5) {
			auth_sec_req_enable = atoi(argv[4]);
		}
#if F_BT_LE_SMP_OOB_SUPPORT
//		if (argc >= 6) {
//			oob_enable = atoi(argv[5]);
//		}
#endif

		gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
		gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
		gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
		gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(uint8_t), &oob_enable);
#endif
		le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
		le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags), &auth_sec_req_flags);
		ret = gap_set_pairable_mode();

		if(ret == GAP_CAUSE_SUCCESS)
			BLE_PRINT("\n\rSet pairable mode success!\r\n");
		else
			BLE_PRINT("\n\rSet pairable mode fail!\r\n");
	}else{
		BLE_PRINT("ERROR:input parameter error!\n\r");
		return -1;
	}

	return ret;
}

int ble_central_at_cmd_send_userconf(int argc, char **argv)
{
	(void) argc;
	u8 conn_id;
	u8 conf;
	int ret;
	T_GAP_CFM_CAUSE confirm;

	conn_id = atoi(argv[1]);
	conf = atoi(argv[2]);
	if (conf == 0){
		confirm = GAP_CFM_CAUSE_REJECT;
	}else{
		confirm = GAP_CFM_CAUSE_ACCEPT;
	}

	ret = le_bond_user_confirm(conn_id, confirm);
	return ret;
}

int ble_central_at_cmd_read(int argc, char **argv)
{
	u8 conn_id;
	u16 handle;
	u16 start_handle;
	u16 end_handle;
	u8 uuid_type;
	u8 uuid[16] = {0};
	u16 uuid16;

	conn_id = atoi(argv[1]);

	if(argc == 3){
		handle = hex_str_to_int(strlen(argv[2]), (s8 *)argv[2]);
		gcs_attr_read(conn_id, handle);
	}

	if(argc == 6){
		start_handle = hex_str_to_int(strlen(argv[2]), (s8 *)argv[2]);
		end_handle = hex_str_to_int(strlen(argv[3]), (s8 *)argv[3]);
		uuid_type = atoi(argv[4]);
		BLE_PRINT("conn_id = %d, start_handle = 0x%x, end_handle = 0x%x, uuid_type = %d\n\r", conn_id, start_handle, end_handle, uuid_type);
		if(uuid_type == 0){
			if (strlen(argv[5]) != 2 * UUID_16_LEN){
				BLE_PRINT("ERROR:uuid length error!\n\r");
				return -1;
			}
			hex_str_to_bd_addr(strlen(argv[5]), (s8 *)argv[5], (u8 *)uuid);
			uuid16 = uuid[1]<<8 | uuid[0];
			BLE_PRINT("uuid16 = 0x%x\n\r", uuid16);
			gcs_attr_read_using_uuid16(conn_id, start_handle, end_handle, uuid16);

		}else if(uuid_type == 1){
			if (strlen(argv[5]) != 2 * UUID_128_LEN){
				BLE_PRINT("ERROR:uuid length error!\n\r");
				return -1;
			}
			hex_str_to_bd_addr(strlen(argv[5]), (s8 *)argv[5], (u8 *)uuid);
			gcs_attr_read_using_uuid128(conn_id, start_handle, end_handle, uuid);
		}else{
			BLE_PRINT("ERROR:uuid type error!\n\r");
			return -1;
		}

	}
	return 0;
}

int ble_central_at_cmd_write(int argc, char **argv)
{
	u8 conn_id;
	u8 write_type;
	u16 handle;
	int length;
	int data_count;

	conn_id = atoi(argv[1]);
	write_type = atoi(argv[2]);
	handle = hex_str_to_int(strlen(argv[3]), (s8 *)argv[3]);
	length = hex_str_to_int(strlen(argv[4]), (s8 *)argv[4]);

	if (length == -1) {
		printf("\n\rError:value length should be hexadecimal and start with '0X' or '0x'\r\n");
		return -1;
	} else if (length == 0) {
		printf("\n\rError:value length should larger than 0\r\n");
		return -1;
	}

	u8 *data = (u8 *)os_mem_alloc(0, length * sizeof(u8));

	data_count = argc - 5;
	for (u8 i = 0; i < length; ++ i) {
		if (i < data_count)
			data[i] = hex_str_to_int(strlen(argv[i + 5]), (s8 *)argv[i + 5]);
		else
			data[i] = 0xff;
	}

	T_GAP_CAUSE ret = gcs_attr_write(conn_id, (T_GATT_WRITE_TYPE)write_type, handle, length, data);

	if (data != NULL)
		os_mem_free(data);
	(void) ret;
	return 0;
}

int ble_central_at_cmd_set_scan_param(int argc, char **argv)
{
	u16 scan_interval = 0;
	u16 scan_window = 0;

	int param = atoi(argv[1]);
	u16 scan_param = hex_str_to_int(strlen(argv[2]), (s8 *) argv[2]);

	if (param == 1) {//modify scan interval
		scan_interval = scan_param;
		le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
	} else if (param == 2) {//modify scan window
		scan_window = scan_param;
		le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
	} else
		return -1;

	return 0;
}

#if F_BT_LE_5_0_SET_PHY_SUPPORT
int ble_central_at_cmd_set_phy(int argc, char **argv)
{
	(void) argc;
	uint8_t conn_id;
	uint8_t phys;
	uint8_t all_phys;
	uint8_t tx_phys;
	uint8_t rx_phys;
	T_GAP_PHYS_OPTIONS phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S8;
	T_GAP_CAUSE cause;
	conn_id = atoi(argv[1]);
	phys = atoi(argv[2]);
	if (phys == 0) //set PHY tx/rx support 1M
	{
		all_phys = GAP_PHYS_PREFER_ALL;
		tx_phys = GAP_PHYS_PREFER_1M_BIT;
		rx_phys = GAP_PHYS_PREFER_1M_BIT;
	}
	else if (phys == 1) //set PHY tx/rx support 2M
	{
		all_phys = GAP_PHYS_PREFER_ALL;
		tx_phys = GAP_PHYS_PREFER_2M_BIT;
		rx_phys = GAP_PHYS_PREFER_2M_BIT;
	}
	else if (phys == 2) //set PHY tx support 2M, rx support 1M
	{
		all_phys = GAP_PHYS_PREFER_ALL;
		tx_phys = GAP_PHYS_PREFER_2M_BIT;
		rx_phys = GAP_PHYS_PREFER_1M_BIT;
	}
	else if (phys == 3) //set PHY tx support 1M, rx support 2M
	{
		all_phys = GAP_PHYS_PREFER_ALL;
		tx_phys = GAP_PHYS_PREFER_1M_BIT;
		rx_phys = GAP_PHYS_PREFER_2M_BIT;
	}
	else if (phys == 4) //set PHY tx/rx support CODED_S2
	{
		all_phys = GAP_PHYS_PREFER_ALL;
		tx_phys = GAP_PHYS_PREFER_CODED_BIT;
		rx_phys = GAP_PHYS_PREFER_CODED_BIT;
		phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S2;
	}
	else if (phys == 5) //set PHY tx/rx support CODED_S8
	{
		all_phys = GAP_PHYS_PREFER_ALL;
		tx_phys = GAP_PHYS_PREFER_CODED_BIT;
		rx_phys = GAP_PHYS_PREFER_CODED_BIT;
		phy_options = GAP_PHYS_OPTIONS_CODED_PREFER_S8;
	}
	else
	{
		printf("Error parameter!\r\n");
		return -1;
	}

	cause = le_set_phy(conn_id, all_phys, tx_phys, rx_phys, phy_options);

	return cause;
}
#endif

int ble_central_app_handle_at_cmd(uint16_t subtype, void *arg)
{
	int common_cmd_flag = 0;
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if (arg) {
		argc = parse_param(arg, argv);
	}

	switch (subtype) {
		case BT_ATCMD_SCAN:
			//ble_central_at_cmd_scan(argc, argv);
			break;
		case BT_ATCMD_CONNECT:
			ble_central_at_cmd_connect(argc, argv);
			break;
		case BT_ATCMD_DISCONNECT:
			ble_central_at_cmd_disconnect(argc, argv);
			break;
		case BT_ATCMD_AUTH:
			ble_central_at_cmd_auth(argc, argv);
			common_cmd_flag = 1;
			break;
		case BT_ATCMD_GET:
			ble_central_at_cmd_get(argc, argv);
			break;
		case BT_ATCMD_GET_COON_INFO:
			ble_central_at_cmd_get_conn_info(argc, argv);
			break;
		case BT_ATCMD_SEND_USERCONF:
			ble_central_at_cmd_send_userconf(argc, argv);
			common_cmd_flag = 1;
			break;
		case BT_ATCMD_UPDATE_CONN_REQUEST:
			ble_central_at_cmd_update_conn_request(argc, argv);
			common_cmd_flag = 1;
			break;
		case BT_ATCMD_BOND_INFORMATION:
			ble_central_at_cmd_bond_information(argc, argv);
			common_cmd_flag = 1;
			break;
		case BT_ATCMD_READ:
			ble_central_at_cmd_read(argc, argv);
			break;
		case BT_ATCMD_WRITE:
			ble_central_at_cmd_write(argc, argv);
			break;
		case BT_ATCMD_MODIFY_WHITELIST:
			ble_central_at_cmd_modify_whitelist(argc, argv);
			break;
		case BT_ATCMD_SET_SCAN_PARAM:
			ble_central_at_cmd_set_scan_param(argc, argv);
			break;
		case BT_ATCMD_SET_PHY:
#if F_BT_LE_5_0_SET_PHY_SUPPORT
			ble_central_at_cmd_set_phy(argc, argv);
#endif
			break;
		default:
			break;
	}

	return common_cmd_flag;
}
#endif
