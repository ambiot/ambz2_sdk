#include <platform_stdlib.h>
#include <platform_opts.h>
#ifdef CONFIG_PLATFORM_8195A
#include <hal_adc.h>
#endif

#include <gpio_api.h>   // mbed
#include <sys_api.h>
#include <flash_api.h>
#include "analogin_api.h"

#if !defined(CONFIG_PLATFORM_8195BHP) && !defined(CONFIG_PLATFORM_8710C)
#include <rtl_lib.h>
#endif
#include <build_info.h>
#include "log_service.h"
#include "atcmd_sys.h"
#include "main.h"
#include "atcmd_wifi.h"
#include "device_lock.h"

#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
#include "freertos_pmu.h"
#endif

#if !defined(CONFIG_PLATFORM_8195BHP) && !defined(CONFIG_PLATFORM_8710C)
extern u32 ConfigDebugErr;
extern u32 ConfigDebugInfo;
extern u32 ConfigDebugWarn;
#endif
#if defined(CONFIG_PLATFORM_8710C)
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
#include "rtl8710c_freertos_pmu.h"
#endif
#if defined(CONFIG_BUILD_NONSECURE) && (CONFIG_BUILD_NONSECURE==1)
extern s32 cmd_dump_word_s(u32 argc, u8 *argv[]);
extern s32 cmd_write_word_s(u32 argc, u8 *argv[]);
#define CmdDumpWord(argc, argv) cmd_dump_word_s(argc, argv)
#define CmdWriteWord(argc, argv) cmd_write_word_s(argc, argv)
#else
extern s32 cmd_dump_word(u32 argc, u8 *argv[]);
extern s32 cmd_write_word(u32 argc, u8 *argv[]);
#define CmdDumpWord(argc, argv) cmd_dump_word(argc, argv)
#define CmdWriteWord(argc, argv) cmd_write_word(argc, argv)
#endif
#else
extern u32 CmdDumpWord(IN u16 argc, IN u8 *argv[]);
extern u32 CmdWriteWord(IN u16 argc, IN u8 *argv[]);
#endif
#if defined(CONFIG_UART_YMODEM) && CONFIG_UART_YMODEM
extern int uart_ymodem(void);
#endif

//#if ATCMD_VER == ATVER_1
#if (configGENERATE_RUN_TIME_STATS == 1)
static char cBuffer[512];
#endif
//#endif

//-------- AT SYS commands ---------------------------------------------------------------
#if ATCMD_VER == ATVER_1

#if defined(CONFIG_PLATFORM_8710C)
extern void sys_uart_download_mode(void);
extern void sys_download_mode(u8 mode);
void fATXX(void *arg)
{
#if defined(CONFIG_BUILD_NONSECURE) && (CONFIG_BUILD_NONSECURE==1)
	flash_t flash_Ptable;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash_Ptable, 0x00000000);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	sys_reset();
#else
	u8 download_mode = 0;
	if (arg)
		download_mode = (unsigned char) atoi((const char *)arg);
	sys_download_mode(download_mode);
#endif
}
#endif

void fATSD(void *arg)
{
#if !defined(CONFIG_PLATFORM_8195BHP)
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSD]: _AT_SYSTEM_DUMP_REGISTER_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSD] Usage: ATSD=REGISTER");
		return;
	}
	argc = parse_param(arg, argv);
	if(argc == 2 || argc == 3)
		CmdDumpWord(argc-1, (unsigned char**)(argv+1));
#endif
}

void fATSE(void *arg)
{
#if !defined(CONFIG_PLATFORM_8195BHP)
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	AT_DBG_MSG(AT_FLAG_EDIT, AT_DBG_ALWAYS, "[ATSE]: _AT_SYSTEM_EDIT_REGISTER_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_EDIT, AT_DBG_ALWAYS, "[ATSE] Usage: ATSE=REGISTER[VALUE]");
		return;
	}
	argc = parse_param(arg, argv);
	if(argc == 3)
		CmdWriteWord(argc-1, (unsigned char**)(argv+1));
#endif
}

void fATSC(void *arg)
{
	/* To avoid gcc warnings */
	( void ) arg;

	AT_DBG_MSG(AT_FLAG_OTA, AT_DBG_ALWAYS, "[ATSC]: _AT_SYSTEM_CLEAR_OTA_SIGNATURE_");
	sys_clear_ota_signature();
}

void fATSR(void *arg)
{
	/* To avoid gcc warnings */
	( void ) arg;
	
	AT_DBG_MSG(AT_FLAG_OTA, AT_DBG_ALWAYS, "[ATSR]: _AT_SYSTEM_RECOVER_OTA_SIGNATURE_");
	sys_recover_ota_signature();
}

#if defined(CONFIG_UART_YMODEM) && CONFIG_UART_YMODEM
void fATSY(void *arg)
{
	uart_ymodem();
}
#endif

#if defined(CONFIG_PLATFORM_8711B)
void fATSK(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	u8 key[16];
	u32 key32[16];

	AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK]: _AT_SYSTEM_RDP/RSIP_CONFIGURE_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RDP_EN");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RDP_KEY[value(hex)]");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] 	Example: ATSK=RDP_KEY[345487bbaa435bfe382233445ba359aa]");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_EN");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_DIS");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_KEY[value(hex)]");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=SB_EN");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=SB_PK_MD5[value(hex)]");
		return;
	}

	argc = parse_param(arg, argv);
	if(strcmp(argv[1], "RDP_EN") == 0){
		EFUSE_RDP_EN();
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] RDP is enable");
	}else if(strcmp(argv[1], "RDP_KEY") == 0){
		if(argc != 3){
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RDP_KEY[value(hex)]");
			return;
		}

		if(strlen(argv[2]) != 32){
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Err: RDP key length should be 16 bytes");
			return;
		}

		memset(key32, 0, sizeof(key32));
		sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
			&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15]);
		for(i=0; i<16; i++){
			key[i] = key32[i] & 0xFF;
		}

		EFUSE_RDP_KEY(key);
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Set RDP key done");
	}else if(strcmp(argv[1], "RSIP_EN") == 0){
		efuse_otf_cmd(ENABLE);
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] RSIP is enable");
	}else if(strcmp(argv[1], "RSIP_DIS") == 0){
		efuse_otf_cmd(DISABLE);
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] RSIP is disable");
	}else if(strcmp(argv[1], "RSIP_KEY") == 0){
		if(argc != 3){
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_KEY[value(hex)]");
			return;
		}

		if(strlen(argv[2]) != 32){
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Err: RSIP key length should be 16 bytes");
			return;
		}

		memset(key32, 0, sizeof(key32));
		sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
			&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15]);
		for(i=0; i<16; i++){
			key[i] = key32[i] & 0xFF;
		}

		EFUSE_OTF_KEY(key);
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Set RSIP key done");
	}else if(strcmp(argv[1], "SB_EN") == 0){
		u8 data = 0;
		u32 efuse_ctrl = HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL);

		EFUSERead8(efuse_ctrl, 0xD3, &data, L25EOUTVOLTAGE);
		if ((data & EFUSE_PHYSICAL_SBOOT_ON) != 0) {
			EFUSEWrite8(efuse_ctrl, 0xD3, data & (~EFUSE_PHYSICAL_SBOOT_ON), L25EOUTVOLTAGE);
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] : Security Boot is enable!");
		}else{
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] : Security Boot is already enabled!");
		}
	}else if(strcmp(argv[1], "SB_PK_MD5") == 0){
		u8 i = 0;

		if(argc != 3){
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=SB_PK_MD5[value(hex)]");
			return;
		}

		if(strlen(argv[2]) != 32){
			AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Err: MD5 value of public key should be 16 bytes");
			return;
		}

		memset(key32, 0, sizeof(key32));
		sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
			&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15]);
		for(i=0; i<16; i++){
			key[i] = key32[i] & 0xFF;
		}

		for(i = 0; i < 16; i++) {
			EFUSEWrite8(HAL_READ32(SYSTEM_CTRL_BASE, REG_SYS_EFUSE_CTRL), 0xC1 + i, key[i], L25EOUTVOLTAGE);
		}
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Set SB md5 value of public key done");

	}else{
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RDP_EN");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RDP_KEY[value(hex)]");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] 	Example: ATSK=RDP_KEY[345487bbaa435bfe382233445ba359aa]");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_EN");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_DIS");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=RSIP_KEY[value(hex)]");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=SB_EN");
		AT_DBG_MSG(AT_FLAG_RDP, AT_DBG_ALWAYS, "[ATSK] Usage: ATSK=SB_PK_MD5[value(hex)]");

	}

}
#endif

#if defined(CONFIG_PLATFORM_8710C)
#include "hal_wdt.h"
#include "efuse_api.h"
#include "efuse_logical_api.h"
#include "crypto_api.h"

#define FLASH_SECTOR_SIZE				0x1000
#define FLASH_OFFSET_PARTITION_TABLE  0x0
#define FLASH_OFFSET_BOOTLOADER           0x4000
#define PRIV_KEY_LEN 32
#define MAC_LEN 6

//if close, ATSK will check if enc key or hash key or root key has been written, if yes, will return error.
//if open, ATSK will not check if enc key or hash key or root key has been written, but will check if the key that needs to be written 
//is same with what has been written or not, if yes, will bypass write, if no, will write the key.
//#define BYPASS_CHECK_KEY_WRITTEN
#if defined(BYPASS_CHECK_KEY_WRITTEN)
extern uint32_t hal_sec_key_write_ext(uint8_t *psec_key, uint8_t key_num);
#endif

extern void sys_disable_fast_boot (void);
#if defined(__ICCARM__)
/* Suppress warnings that are generated by the IAR tools, but cannot be fixed in
the source code. */
#pragma diag_suppress=Pe181
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat="
#endif
void fATSK(void *arg)
{
	int argc = 0;
	int ret;
	char *argv[MAX_ARGC] = {0};
	u8 key[PRIV_KEY_LEN] = {0}, read_buffer[PRIV_KEY_LEN] = {0}, read_buf_mac[MAC_LEN] = {0}, hash_result[PRIV_KEY_LEN] = {0};
	u32 key32[PRIV_KEY_LEN];
	u32 sscanf_ret = 0;
	flash_t flash;
	char *ptmp;
	u32 offset_1, offset_2, len;
	u32 test_mode;
	u32 test_mode_bit;
	int i;
	unsigned char *enc_img;
	unsigned char *enc_img_tmp;
	u8 mac_empty[MAC_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

	at_printf("[ATSK]: _AT_SYSTEM_ENABLE_SECURE_BOOT_\r\n");

	//check if the chip is in test mode
	test_mode = HAL_READ32(0x40000000, 0x000001F4);
	test_mode_bit = (test_mode & BIT26)>>26;
	if(test_mode_bit & 0x1){
		at_printf("[ATSK] Err: the chip is still in test mode, please reset it first.\r\n");
		return;
	}

	if(!arg){
		at_printf("[ATSK] Err: argument error.\r\n");
		at_printf("[ATSK] Usage: ATSK=ENC_KEY[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=ENC_KEY[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=HASH_KEY[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=HASH_KEY[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=SB_KEY\r\n");
		at_printf("[ATSK] Usage: ATSK=ROOT_KEY[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=ROOT_KEY[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=ROOT_SEED[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=ROOT_SEED[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=SEC_BOOT_EN[offset,offset,length]\r\n");
		at_printf("[ATSK] Example: ATSK=SEC_BOOT_EN[0x110000,0x111000,0x8000]\r\n");
		at_printf("[ATSK] Usage: ATSK=CHECK_MAC\r\n");
		return;
	}

	argc = parse_param(arg, argv);

	//verify secure boot is enabled or not, if it has been enabled, return.
	device_mutex_lock(RT_DEV_LOCK_EFUSE);
	ret = efuse_fw_verify_check();
	device_mutex_unlock(RT_DEV_LOCK_EFUSE);
	if(ret){
		at_printf("[ATSK] Err: secure boot has been already enabled.\r\n");
		return;
	}


	if(strcmp(argv[1], "ENC_KEY") == 0){
		if(argc != 3){
			at_printf("[ATSK] Err: ATSK=ENC_KEY[value(string)].\r\n");
			return;
		}

		if(strlen(argv[2]) != 64){
			at_printf("[ATSK] Err: ENC key length should be 32 bytes.\r\n");
			return;
		}

		memset(key32, 0, sizeof(key32));
		sscanf_ret = sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
				&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15],
				&key32[16], &key32[17], &key32[18], &key32[19], &key32[20], &key32[21], &key32[22], &key32[23],
				&key32[24], &key32[25], &key32[26], &key32[27], &key32[28], &key32[29], &key32[30], &key32[31]);
		for(i=0; i<PRIV_KEY_LEN; i++){
			key[i] = key32[i] & 0xFF;
		}

		if(sscanf_ret != PRIV_KEY_LEN)
		{
			at_printf("[ATSK] Err: Parse ENC key to hex failed.\r\n");
			return;
		}

		// read SS key
		// init read buffer as 0xFF
		for(int i=0;i<PRIV_KEY_LEN;i++){
			read_buffer[i]=0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = hal_susec_key_get(read_buffer);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(!ret){
			at_printf("[ATSK] Err: read efuse ss key failed.\r\n");
			return;
		}
#if defined(BYPASS_CHECK_KEY_WRITTEN)
		if(memcmp(key,read_buffer,32)!=0){
			for(i = 0; i < PRIV_KEY_LEN; i++){
				for(int t = 0; t < 8; t++){
					if(((key[i] & BIT(t)) >> t) && (!((read_buffer[i] & BIT(t)) >> t))){
						at_printf("[ATSK] Err: efuse cannot rewrite because it will be messy up.\r\n");
						return;
					}
				}
				key[i] |= (~read_buffer[i]);
			}
			//write SS key
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = efuse_susec_key_write(key);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(ret<0){
				at_printf("[ATSK] Err: ENC key write address and length error.\r\n");
				return;
			}
			else{
				at_printf("[ATSK] Write ENC key done.\r\n");
			}

			//read SS key
			//read efuse for 3 times
			for(int j=0;j<3;j++){
				//init read_buffer as 0xFF before read
				for(int i=0;i<PRIV_KEY_LEN;i++){
					read_buffer[i]=0xFF;
				}
				device_mutex_lock(RT_DEV_LOCK_EFUSE);
				ret = hal_susec_key_get(read_buffer);
				device_mutex_unlock(RT_DEV_LOCK_EFUSE);
				if(memcmp(key,read_buffer,32)!=0){
					at_printf("[ATSK] Err: write ENC key failed.\r\n");
					return;
				}
				hal_delay_ms(10);
			}
		}else{
			at_printf("[ATSK] bypass writing done.\r\n");
		}
#else
		for(i=0;i<PRIV_KEY_LEN;i++){
			if(read_buffer[i]!=0xFF)
			{
				at_printf("[ATSK] Err: the ENC Key has already been written.\r\n");
				return;
			}
		}
		//write SS key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_susec_key_write(key);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret<0){
			at_printf("[ATSK] Err: ENC key write address and length error.\r\n");
			return;
		}
		else{
			at_printf("[ATSK] Write ENC key done.\r\n");
		}
		//read SS key
		//read efuse for 3 times
		for(int j=0;j<3;j++){
			//init read_buffer as 0xFF before read
			for(int i=0;i<PRIV_KEY_LEN;i++){
				read_buffer[i]=0xFF;
			}
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_susec_key_get(read_buffer);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(memcmp(key,read_buffer,32)!=0){
				at_printf("[ATSK] Err: write ENC key failed.\r\n");
				return;
			}
			hal_delay_ms(10);
		}
#endif
		//hash read result
		ret = crypto_init();
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto engine init failed.\r\n");
			return;
		}
		ret = crypto_sha2_256(read_buffer, 32, hash_result);
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto enc key failed.\r\n");
			return;
		}
		at_printf("hash result is $");
		for(i=0; i<PRIV_KEY_LEN; i+=8){
			at_printf("%02X%02X%02X%02X%02X%02X%02X%02X",
				hash_result[i], hash_result[i+1], hash_result[i+2], hash_result[i+3], hash_result[i+4], hash_result[i+5], hash_result[i+6], hash_result[i+7]);
		}
		at_printf("$\r\n");
	}else if(strcmp(argv[1], "HASH_KEY") == 0){
		if(argc != 3){
			at_printf("[ATSK] Err: ATSK=HASH_KEY[value(string)].\r\n");
			return;
		}

		if(strlen(argv[2]) != 64){
			at_printf("[ATSK] Err: HASH key length should be 32 bytes.\r\n");
			return;
		}

		memset(key32, 0, sizeof(key32));
		sscanf_ret = sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
				&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15],
				&key32[16], &key32[17], &key32[18], &key32[19], &key32[20], &key32[21], &key32[22], &key32[23],
				&key32[24], &key32[25], &key32[26], &key32[27], &key32[28], &key32[29], &key32[30], &key32[31]);
		for(i=0; i<PRIV_KEY_LEN; i++){
			key[i] = key32[i] & 0xFF;
		}

		if(sscanf_ret != PRIV_KEY_LEN)
		{
			at_printf("[ATSK] Err: Parse HASH key to hex failed.\r\n");
			return;
		}

		// read S key
		//init read_buffer as 0xFF before read
		for(int i=0;i<PRIV_KEY_LEN;i++){
			read_buffer[i]=0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = hal_sec_key_get(read_buffer, 0, PRIV_KEY_LEN);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(!ret){
			at_printf("[ATSK] Err: read efuse HASH key failed.\r\n");
			return;
		}
#if defined(BYPASS_CHECK_KEY_WRITTEN)
		if(memcmp(key,read_buffer,32)!=0){
			for(i = 0; i < PRIV_KEY_LEN; i++){
				for(int t = 0; t < 8; t++){
					if(((key[i] & BIT(t)) >> t) && (!((read_buffer[i] & BIT(t)) >> t))){
						at_printf("[ATSK] Err: efuse cannot rewrite because it will be messy up.\r\n");
						return;
					}
				}
				key[i] |= (~read_buffer[i]);
			}

			//write S key
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_write_ext(key,0);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(ret<0){
				at_printf("[ATSK] Err: HASH key write address and length error.\r\n");
				return;
			}
			else{
				at_printf("[ATSK] Write HASH key done.\r\n");
			}

			//read S key
			//read efuse for 3 times
			for(int j=0;j<3;j++){
				//init read_buffer as 0xFF before read
				for(int i=0;i<PRIV_KEY_LEN;i++){
					read_buffer[i]=0xFF;
				}
				device_mutex_lock(RT_DEV_LOCK_EFUSE);
				ret = hal_sec_key_get(read_buffer, 0, PRIV_KEY_LEN);
				device_mutex_unlock(RT_DEV_LOCK_EFUSE);
				if(memcmp(key,read_buffer,32)!=0){
					at_printf("[ATSK] Err: write HASH key failed.\r\n");
					return;
				}
				hal_delay_ms(10);
			}
		}else{
			at_printf("[ATSK] bypass writing done.\r\n");
		}
#else
		for(i=0;i<PRIV_KEY_LEN;i++){
			if(read_buffer[i]!=0xFF)
			{
				at_printf("[ATSK] Err: the HASH Key has been written already.\r\n");
				return;
			}
		}
		//write S key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_sec_key_write(key,0);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret<0){
			at_printf("[ATSK] Err: HASH key write address and length error.\r\n");
			return;
		}
		else{
			at_printf("[ATSK] Write HASH key done.\r\n");
		}
		//read S key
		//read efuse for 3 times
		for(int j=0;j<3;j++){
			//init read_buffer as 0xFF before read
			for(int i=0;i<PRIV_KEY_LEN;i++){
				read_buffer[i]=0xFF;
			}
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_get(read_buffer, 0, PRIV_KEY_LEN);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(memcmp(key,read_buffer,32)!=0){
				at_printf("[ATSK] Err: write HASH key failed.\r\n");
				return;
			}
			hal_delay_ms(10);
		}
#endif

		//hash read result
		ret = crypto_init();
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto engine init failed.\r\n");
			return;
		}
		ret = crypto_sha2_256(read_buffer, 32, hash_result);
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto hash key failed.\r\n");
			return;
		}
		at_printf("hash result is $");
		for(i=0; i<PRIV_KEY_LEN; i+=8){
			at_printf("%02X%02X%02X%02X%02X%02X%02X%02X",
				hash_result[i], hash_result[i+1], hash_result[i+2], hash_result[i+3], hash_result[i+4], hash_result[i+5], hash_result[i+6], hash_result[i+7]);
		}
		at_printf("$\r\n");
	}else if(strcmp(argv[1], "SB_KEY") == 0){
		// super secure key : privkey_enc
		char ss_default_key[65] = "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F\0";
		// secure key : privkey_hash
		char hash_default_key[65] = "64A7433FCF027D19DDA4D446EEF8E78A22A8C33CB2C337C07366C040612EE0F2\0";

		if(argc != 2){
			at_printf("[ATSK] Err: ATSK=SB_KEY.\r\n");
			return;
		}
		at_printf("\r\n1. Super Sec Key write...\r\n" );

		memset(key32, 0, sizeof(key32));
		ret = sscanf(ss_default_key, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
				&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15],
				&key32[16], &key32[17], &key32[18], &key32[19], &key32[20], &key32[21], &key32[22], &key32[23],
				&key32[24], &key32[25], &key32[26], &key32[27], &key32[28], &key32[29], &key32[30], &key32[31]);
		for(i=0; i<PRIV_KEY_LEN; i++){
			key[i] = key32[i] & 0xFF;
		}

		if(ret != PRIV_KEY_LEN)
		{
			at_printf("[ATSK] Err: Parse SS key to hex failed, ret(%d).\r\n", ret);
			return;
		}

#if 0
		at_printf("\r\nParsed SS Key : " );
		for(i=0; i<PRIV_KEY_LEN; i+=8){
			at_printf("%02X%02X%02X%02X%02X%02X%02X%02X",
				key[i], key[i+1], key[i+2], key[i+3], key[i+4], key[i+5], key[i+6], key[i+7]);
		}
#endif

		// read SS key
		// init read buffer as 0xFF
		for(int i=0;i<PRIV_KEY_LEN;i++){
			read_buffer[i]=0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = hal_susec_key_get(read_buffer);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(!ret){
			at_printf("[ATSK] Err: read efuse ss key failed.\r\n");
			goto ss_key_hash;
		}
#if defined(BYPASS_CHECK_KEY_WRITTEN)
		if(memcmp(key,read_buffer,32)!=0){
			for(i = 0; i < PRIV_KEY_LEN; i++){
				for(int t = 0; t < 8; t++){
					if(((key[i] & BIT(t)) >> t) && (!((read_buffer[i] & BIT(t)) >> t))){
						at_printf("[ATSK] Err: efuse cannot rewrite because it will be messy up.\r\n");
						return;
					}
				}
				key[i] |= (~read_buffer[i]);
			}
			//write SS key
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = efuse_susec_key_write(key);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(ret<0){
				at_printf("[ATSK] Err: ENC key write address and length error.\r\n");
				goto ss_key_hash;
			}
			else{
				at_printf("[ATSK] Write ENC key done.\r\n");
			}

			//read SS key
			//read efuse for 3 times
			for(int j=0;j<3;j++){
				//init read_buffer as 0xFF before read
				for(int i=0;i<PRIV_KEY_LEN;i++){
					read_buffer[i]=0xFF;
				}
				device_mutex_lock(RT_DEV_LOCK_EFUSE);
				ret = hal_susec_key_get(read_buffer);
				device_mutex_unlock(RT_DEV_LOCK_EFUSE);
				if(memcmp(key,read_buffer,32)!=0){
					at_printf("[ATSK] Err: write ENC key failed.\r\n");
					goto ss_key_hash;
				}
				hal_delay_ms(10);
			}
		}else{
			at_printf("[ATSK] bypass writing done.\r\n");
		}
#else
		for(i=0;i<PRIV_KEY_LEN;i++){
			if(read_buffer[i]!=0xFF)
			{
				at_printf("[ATSK] Err: the ENC Key has already been written.\r\n");
				goto ss_key_hash;
			}
		}
		//write SS key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_susec_key_write(key);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret<0){
			at_printf("[ATSK] Err: ENC key write address and length error.\r\n");
			goto ss_key_hash;
		}
		else{
			at_printf("[ATSK] Write ENC key done.\r\n");
		}
		//read SS key
		//read efuse for 3 times 
		for(int j=0;j<3;j++){
			//init read_buffer as 0xFF before read
			for(int i=0;i<PRIV_KEY_LEN;i++){
				read_buffer[i]=0xFF;
			}
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_susec_key_get(read_buffer);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(memcmp(key,read_buffer,32)!=0){
				at_printf("[ATSK] Err: write ENC key failed.\r\n");
				goto ss_key_hash;
			}
			hal_delay_ms(10);
		}
#endif
		//hash read result
		ret = crypto_init();
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto engine init failed.\r\n");
			goto ss_key_hash;
		}
		ret = crypto_sha2_256(read_buffer, 32, hash_result);
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto enc key failed.\r\n");
			goto ss_key_hash;
		}
		at_printf("hash result is $");
		for(i=0; i<PRIV_KEY_LEN; i+=8){
			at_printf("%02X%02X%02X%02X%02X%02X%02X%02X",
				hash_result[i], hash_result[i+1], hash_result[i+2], hash_result[i+3], hash_result[i+4], hash_result[i+5], hash_result[i+6], hash_result[i+7]);
		}
		at_printf("$\r\n");

ss_key_hash:
		// Hash Key write
		at_printf("\r\n2. Hash Key write ...\r\n" );

		memset(key32, 0, sizeof(key32));
		sscanf_ret = sscanf(hash_default_key, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
				&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15],
				&key32[16], &key32[17], &key32[18], &key32[19], &key32[20], &key32[21], &key32[22], &key32[23],
				&key32[24], &key32[25], &key32[26], &key32[27], &key32[28], &key32[29], &key32[30], &key32[31]);
		for(i=0; i<PRIV_KEY_LEN; i++){
			key[i] = key32[i] & 0xFF;
		}

		if(sscanf_ret != PRIV_KEY_LEN)
		{
			at_printf("[ATSK] Err: Parse HASH key to hex failed.\r\n");
			return;
		}

#if 0
		at_printf("\r\nParsed HASH Key : " );
		for(i=0; i<PRIV_KEY_LEN; i+=8){
			at_printf("%02X%02X%02X%02X%02X%02X%02X%02X",
				key[i], key[i+1], key[i+2], key[i+3], key[i+4], key[i+5], key[i+6], key[i+7]);
		}
#endif

		// read S key
		//init read_buffer as 0xFF before read
		for(int i=0;i<PRIV_KEY_LEN;i++){
			read_buffer[i]=0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = hal_sec_key_get(read_buffer, 0, PRIV_KEY_LEN);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(!ret){
			at_printf("[ATSK] Err: read efuse HASH key failed.\r\n");
			return;
		}
#if defined(BYPASS_CHECK_KEY_WRITTEN)
		if(memcmp(key,read_buffer,32)!=0){
			for(i = 0; i < PRIV_KEY_LEN; i++){
				for(int t = 0; t < 8; t++){
					if(((key[i] & BIT(t)) >> t) && (!((read_buffer[i] & BIT(t)) >> t))){
						at_printf("[ATSK] Err: efuse cannot rewrite because it will be messy up.\r\n");
						return;
					}
				}
				key[i] |= (~read_buffer[i]);
			}
			//write S key
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_write_ext(key,0);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(ret<0){
				at_printf("[ATSK] Err: HASH key write address and length error.\r\n");
				return;
			}
			else{
				at_printf("[ATSK] Write HASH key done.\r\n");
			}

			//read S key
			//read efuse for 3 times
			for(int j=0;j<3;j++){
				//init read_buffer as 0xFF before read
				for(int i=0;i<PRIV_KEY_LEN;i++){
					read_buffer[i]=0xFF;
				}
				device_mutex_lock(RT_DEV_LOCK_EFUSE);
				ret = hal_sec_key_get(read_buffer, 0, PRIV_KEY_LEN);
				device_mutex_unlock(RT_DEV_LOCK_EFUSE);
				if(memcmp(key,read_buffer,32)!=0){
					at_printf("[ATSK] Err: write HASH key failed.\r\n");
					return;
				}
				hal_delay_ms(10);
			}
		}else{
			at_printf("[ATSK] bypass writing done.\r\n");
		}
#else
		for(i=0;i<PRIV_KEY_LEN;i++){
			if(read_buffer[i]!=0xFF)
			{
				at_printf("[ATSK] Err: the HASH Key has been written already.\r\n");
				return;
			}
		}
		//write S key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_sec_key_write(key,0);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret<0){
			at_printf("[ATSK] Err: HASH key write address and length error.\r\n");
			return;
		}
		else{
			at_printf("[ATSK] Write HASH key done.\r\n");
		}
		//read S key
		//read efuse for 3 times
		for(int j=0;j<3;j++){
			//init read_buffer as 0xFF before read
			for(int i=0;i<PRIV_KEY_LEN;i++){
				read_buffer[i]=0xFF;
			}
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_get(read_buffer, 0, PRIV_KEY_LEN);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(memcmp(key,read_buffer,32)!=0){
				at_printf("[ATSK] Err: write HASH key failed.\r\n");
				return;
			}
			hal_delay_ms(10);
		}
#endif

		//hash read result
		ret = crypto_init();
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto engine init failed.\r\n");
			return;
		}
		ret = crypto_sha2_256(read_buffer, 32, hash_result);
		if (SUCCESS != ret) {
			at_printf("[ATSK] Err: crypto hash key failed.\r\n");
			return;
		}
		at_printf("hash result is $");
		for(i=0; i<PRIV_KEY_LEN; i+=8){
			at_printf("%02X%02X%02X%02X%02X%02X%02X%02X",
				hash_result[i], hash_result[i+1], hash_result[i+2], hash_result[i+3], hash_result[i+4], hash_result[i+5], hash_result[i+6], hash_result[i+7]);
		}
		at_printf("$\r\n");
	}else if(strcmp(argv[1], "ROOT_KEY") == 0){
		if(argc != 3){
			at_printf("[ATSK] Err: ATSK=ROOT_KEY[value(string)].\r\n");
			return;
		}

		if(strlen(argv[2]) != 64){
			at_printf("[ATSK] Err: ROOT key length should be 32 bytes.\r\n");
			return;
		}

		memset(key32, 0, sizeof(key32));
		sscanf_ret = sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
				&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15],
				&key32[16], &key32[17], &key32[18], &key32[19], &key32[20], &key32[21], &key32[22], &key32[23],
				&key32[24], &key32[25], &key32[26], &key32[27], &key32[28], &key32[29], &key32[30], &key32[31]);
		for(i=0; i<PRIV_KEY_LEN; i++){
			key[i] = key32[i] & 0xFF;
		}

		if(sscanf_ret != PRIV_KEY_LEN)
		{
			at_printf("[ATSK] Err: Parse ROOT key to hex failed.\r\n");
			return;
		}

		// read root key
		//init read_buffer as 0xFF before read
		for(int i=0;i<PRIV_KEY_LEN;i++){
			read_buffer[i]=0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = hal_sec_key_get(read_buffer, 1, PRIV_KEY_LEN);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(!ret){
			at_printf("[ATSK] Err: read efuse ROOT key failed.\r\n");
			return;
		}
#if defined(BYPASS_CHECK_KEY_WRITTEN)
		if(memcmp(key,read_buffer,32)!=0){
			for(i = 0; i < PRIV_KEY_LEN; i++){
				key[i] |= (~read_buffer[i]);
			}
			//write root key
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_write_ext(key,1);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(ret<0){
				at_printf("[ATSK] Err: ROOT key write address and length error.\r\n");
				return;
			}
			else{
				at_printf("[ATSK] Write ROOT key done.\r\n");
			}
		}else{
			at_printf("[ATSK] bypass writing done.\r\n");
		}
#else
		for(i=0;i<PRIV_KEY_LEN;i++){
			if(read_buffer[i]!=0xFF)
			{
				at_printf("[ATSK] Err: the ROOT Key has been written already.\r\n");
				return;
			}
		}
		//write root key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_sec_key_write(key,1);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret<0){
			at_printf("[ATSK] Err: ROOT key write address and length error.\r\n");
			return;
		}
		else{
			at_printf("[ATSK] Write ROOT key done.\r\n");
		}
		//read root key
		//read efuse for 3 times
		for(int j=0;j<3;j++){
			//init read_buffer as 0xFF before read
			for(int i=0;i<PRIV_KEY_LEN;i++){
				read_buffer[i]=0xFF;
			}
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_get(read_buffer, 1, PRIV_KEY_LEN);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(memcmp(key,read_buffer,32)!=0){
				at_printf("[ATSK] Err: write ROOT key failed.\r\n");
				return;
			}
			hal_delay_ms(10);
		}
#endif
	}else if(strcmp(argv[1], "ROOT_SEED") == 0){
		if(argc != 3){
			at_printf("[ATSK] Err: ATSK=ROOT_SEED[value(string)].\r\n");
			return;
		}

		if(strlen(argv[2]) != 64){
			at_printf("[ATSK] Err: root seed length should be 32 bytes.\r\n");
			return;
		}

		u8 seed[PRIV_KEY_LEN];
		u32 seed_in = 0;
		u32 key_tmp[8];

		memset(key32, 0, sizeof(key32));
		sscanf_ret = sscanf(argv[2], "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7],
				&key32[8], &key32[9], &key32[10], &key32[11], &key32[12], &key32[13], &key32[14], &key32[15],
				&key32[16], &key32[17], &key32[18], &key32[19], &key32[20], &key32[21], &key32[22], &key32[23],
				&key32[24], &key32[25], &key32[26], &key32[27], &key32[28], &key32[29], &key32[30], &key32[31]);
		for(i=0; i<PRIV_KEY_LEN; i++){
			seed[i] = key32[i] & 0xFF;
		}

		if(sscanf_ret != PRIV_KEY_LEN)
		{
			at_printf("[ATSK] Err: Parse SS seed to hex failed.\r\n");
			return;
		}

		for(i = 0; i < 8; i++){
			seed_in = *((int*)(seed)+i);
			srand(seed_in);
			key_tmp[i] = rand();
		}

		for(i=0; i<8 ; i++){
			key[i*4]= (key_tmp[i] >> 24) & 0xFF;
			key[i*4+1]= (key_tmp[i] >> 16) & 0xFF;
			key[i*4+2]= (key_tmp[i] >> 8) & 0xFF;
			key[i*4+3]= (key_tmp[i]) & 0xFF;
		}

#if 0
		printf("key is :\n");
		for(int i=0;i<PRIV_KEY_LEN;i++){
			printf("%02X",key[i]);
			}
		printf("\n");
#endif

		// read root key
		//init read_buffer as 0xFF before read
		for(int i=0;i<PRIV_KEY_LEN;i++){
			read_buffer[i]=0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = hal_sec_key_get(read_buffer, 1, PRIV_KEY_LEN);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(!ret){
			at_printf("[ATSK] Err: read efuse ROOT key failed.\r\n");
			return;
		}
#if defined(BYPASS_CHECK_KEY_WRITTEN)
		if(memcmp(key,read_buffer,32)!=0){
			for(i = 0; i < PRIV_KEY_LEN; i++){
				key[i] |= (~read_buffer[i]);
			}
			//write root key
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_write_ext(key,1);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(ret<0){
				at_printf("[ATSK] Err: ROOT key write address and length error.\r\n");
				return;
			}
			else{
				at_printf("[ATSK] Write ROOT key done.\r\n");
			}
		}else{
			at_printf("[ATSK] bypass writing done.\r\n");
		}
#else
		for(i=0;i<PRIV_KEY_LEN;i++){
			if(read_buffer[i]!=0xFF)
			{
				at_printf("[ATSK] Err: the ROOT Key has been written already.\r\n");
				return;
			}
		}
		//write root key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_sec_key_write(key,1);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret<0){
			at_printf("[ATSK] Err: ROOT key write address and length error.\r\n");
			return;
		}
		else{
			at_printf("[ATSK] Write ROOT key done.\r\n");
		}
		//read root key
		//read efuse for 3 times
		for(int j=0;j<3;j++){
			//init read_buffer as 0xFF before read
			for(int i=0;i<PRIV_KEY_LEN;i++){
				read_buffer[i]=0xFF;
			}
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = hal_sec_key_get(read_buffer, 1, PRIV_KEY_LEN);
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(memcmp(key,read_buffer,32)!=0){
				at_printf("[ATSK] Err: write ROOT key failed.\r\n");
				return;
			}
			hal_delay_ms(10);
		}
#endif
	}else if(strcmp(argv[1], "SEC_BOOT_EN") == 0){
		len = strtoul(argv[4], &ptmp, 16);
		if(len % FLASH_SECTOR_SIZE != 0)
		{
			at_printf("[ATSK] Err: length parameter must be 4K alignment.\r\n");
			return;
		}

		//check if encrypted partition.bin is empty
		offset_1 = strtoul(argv[2], &ptmp, 16);
		enc_img_tmp = malloc(FLASH_SECTOR_SIZE);
		if(!enc_img_tmp){
			at_printf("[ATSK] Err: malloc failed for enc_img_tmp.\r\n");
			return;	
		}
		enc_img = malloc(FLASH_SECTOR_SIZE);
		if(!enc_img){
			at_printf("[ATSK] Err: malloc failed for enc_img.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			return;
		}

		//init enc_img_tmp as 0xFF
		for(i = 0; i < FLASH_SECTOR_SIZE; i++){
			enc_img_tmp[i] = 0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, offset_1, FLASH_SECTOR_SIZE, enc_img);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);

		//check if encrypted partition.bin is empty
		if(memcmp(enc_img, enc_img_tmp, FLASH_SECTOR_SIZE) == 0){
			at_printf("[ATSK] Err: encrypted partition.bin is empty.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			if(enc_img){
				free(enc_img);
			}
			return;
		}
		if(enc_img_tmp){
			free(enc_img_tmp);
		}
		if(enc_img){
			free(enc_img);
		}

		//check if encrypted bootloader is empty
		u8 sector_len = 0;//how many sectors to read and write
		sector_len = len/FLASH_SECTOR_SIZE;

		offset_2 = strtoul(argv[3], &ptmp, 16);
		enc_img_tmp = malloc(len);
		if(!enc_img_tmp){
			at_printf("[ATSK] Err: malloc failed for enc_img_tmp.\r\n");
			return;
		}
		enc_img = malloc(len);
		if(!enc_img){
			at_printf("[ATSK] Err: malloc failed for enc_img.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			return;
		}
		//init enc_img_tmp as 0xFF
		for(i = 0; i < len; i++){
			enc_img_tmp[i] = 0xFF;
		}
		device_mutex_lock(RT_DEV_LOCK_FLASH);
		flash_stream_read(&flash, offset_2, len, enc_img);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		if(memcmp(enc_img, enc_img_tmp, len) == 0){
			at_printf("[ATSK] Err: encrypted bootloader.bin is empty.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			if(enc_img){
				free(enc_img);
			}
			return;
		}
		if(enc_img_tmp){
			free(enc_img_tmp);
		}
		if(enc_img){
			free(enc_img);
		}

		//rewrite encrypted partition.bin
		//malloc
		enc_img_tmp = malloc(FLASH_SECTOR_SIZE);
		if(!enc_img_tmp){
			at_printf("[ATSK] Err: malloc failed for enc_img_tmp.\r\n");
			return;	
		}
		enc_img = malloc(FLASH_SECTOR_SIZE);
		if(!enc_img){
			at_printf("[ATSK] Err: malloc failed for enc_img.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			return;
		}

		device_mutex_lock(RT_DEV_LOCK_FLASH);
		//read encrypted partition.bin
		flash_stream_read(&flash, offset_1, FLASH_SECTOR_SIZE, enc_img);
		//erase partition.bin
		flash_erase_sector(&flash, FLASH_OFFSET_PARTITION_TABLE);
		//write partition.bin
		flash_burst_write(&flash,  FLASH_OFFSET_PARTITION_TABLE , FLASH_SECTOR_SIZE, enc_img);
		//read and check if write correctly
		flash_stream_read(&flash, FLASH_OFFSET_PARTITION_TABLE, FLASH_SECTOR_SIZE, enc_img_tmp);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		if(memcmp(enc_img, enc_img_tmp, FLASH_SECTOR_SIZE) != 0){
			at_printf("[ATSK] Err: encrypted partition.bin is not written correctly.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			if(enc_img){
				free(enc_img);
			}
			return;
		}
		if(enc_img_tmp){
			free(enc_img_tmp);
		}
		if(enc_img){
			free(enc_img);
		}

		//rewrite encrypted bootloader.bin
		//malloc
		enc_img_tmp = malloc(FLASH_SECTOR_SIZE);
		if(!enc_img_tmp){
			at_printf("[ATSK] Err: malloc failed for enc_img_tmp.\r\n");
			return;
		}
		enc_img = malloc(FLASH_SECTOR_SIZE);
		if(!enc_img){
			at_printf("[ATSK] Err: malloc failed for enc_img.\r\n");
			if(enc_img_tmp){
				free(enc_img_tmp);
			}
			return;
		}

		for(i = 0; i < sector_len; i++){
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			//read sector
			flash_stream_read(&flash, offset_2 + i * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, enc_img);
			//erase sector
			flash_erase_sector(&flash, FLASH_OFFSET_BOOTLOADER + i * FLASH_SECTOR_SIZE);
			//write sector
			flash_burst_write(&flash,  FLASH_OFFSET_BOOTLOADER + i * FLASH_SECTOR_SIZE , FLASH_SECTOR_SIZE, enc_img);
			//read and compare the write is correctly or not
			flash_stream_read(&flash, FLASH_OFFSET_BOOTLOADER + i * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, enc_img_tmp);
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			if(memcmp(enc_img, enc_img_tmp, FLASH_SECTOR_SIZE) != 0){
				at_printf("[ATSK] Err: encrypted bootloader.bin is not written correctly.\r\n");
				if(enc_img_tmp){
					free(enc_img_tmp);
				}
				if(enc_img){
					free(enc_img);
				}
				return;
			}
		}
		if(enc_img_tmp){
			free(enc_img_tmp);
		}
		if(enc_img){
			free(enc_img);
		}

		//lock SS key
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_lock_susec_key();
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret < 0){
			at_printf("[ATSK] Err: efuse SS key lock error.\r\n");
			return;
		}
		else{
			at_printf("[ATSK] SS key is locked!\r\n");
		}

		//enable secure boot
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_fw_verify_enable();
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(ret < 0){
			at_printf("[ATSK] Err: secure boot enable error.\r\n");
			return;
		}
		else{
			device_mutex_lock(RT_DEV_LOCK_EFUSE);
			ret = efuse_fw_verify_check();
			device_mutex_unlock(RT_DEV_LOCK_EFUSE);
			if(!ret){
				at_printf("[ATSK] Err: secure boot enable error.\r\n");
				return;
			}
			else{
				at_printf("[ATSK] secure boot is enabled!\r\n");
#if 0
				device_mutex_lock(RT_DEV_LOCK_FLASH);
				//erase encrypted partitiontable.bin
				flash_erase_sector(&flash, offset_1);
				//erase encrypted bootloader.bin
				for(i = 0; i < sector_len; i++)
				{
					flash_erase_sector(&flash, offset_2 + i * FLASH_SECTOR_SIZE);
				}
				device_mutex_unlock(RT_DEV_LOCK_FLASH);
#endif
				//reset
				ret = hal_efuse_autoload_en(1);
				if(!ret)
				{
					at_printf("[ATSK] Err: secure boot has been already enabled but reset failed, please reset the chip.\r\n");
					return;
				}
				hal_delay_ms(10);
#if defined(CONFIG_BUILD_NONSECURE) && (CONFIG_BUILD_NONSECURE==1)
				sys_disable_fast_boot();
#else
				ret = hal_sys_set_fast_boot(NULL, 0);
				if(ret)
				{
					at_printf("[ATSK] Err: secure boot has been already enabled but reset failed, please reset the chip.\r\n");
					return;
				}
#endif
				hal_misc_rst_by_wdt();
			}
		}
	}else if(strcmp(argv[1], "CHECK_MAC") == 0){
		//verify if the chip has already been MP
		//WIFI MAC start address is 0x11A
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_logical_read(0x11A, 6, read_buf_mac);
		at_printf("WIFI MAC is %02X%02X%02X%02X%02X%02X, ",read_buf_mac[0],read_buf_mac[1],read_buf_mac[2],read_buf_mac[3],read_buf_mac[4],read_buf_mac[5]);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(memcmp(read_buf_mac,mac_empty,6)==0){
			at_printf("[ATSK] Err: The chip has not been MP calibration, please calibrate it first.\r\n");
			return;
		}
		//BT MAC start address is 0x190
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		ret = efuse_logical_read(0x190, 6, read_buf_mac);
		at_printf("BT MAC is %02X%02X%02X%02X%02X%02X.\r\n",read_buf_mac[0],read_buf_mac[1],read_buf_mac[2],read_buf_mac[3],read_buf_mac[4],read_buf_mac[5]);
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		if(memcmp(read_buf_mac,mac_empty,6)==0){
			dbg_printf("[ATSK] Err: The chip has not been MP calibration, please calibrate it first.\r\n");
			return;
		}
	}else{
		at_printf("[ATSK] Usage: ATSK=ENC_KEY[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=ENC_KEY[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=HASH_KEY[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=HASH_KEY[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=SB_KEY\r\n");
		at_printf("[ATSK] Usage: ATSK=ROOT_KEY[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=ROOT_KEY[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=ROOT_SEED[value(string)]\r\n");
		at_printf("[ATSK] Example: ATSK=ROOT_SEED[000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E5F]\r\n");
		at_printf("[ATSK] Usage: ATSK=SEC_BOOT_EN[offset,offset,length]\r\n");
		at_printf("[ATSK] Example: ATSK=SEC_BOOT_EN[0x110000,0x111000,0x8000]\r\n");
		at_printf("[ATSK] Usage: ATSK=CHECK_MAC\r\n");
		return;
	}
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif


#if defined(SUPPORT_MP_MODE) && SUPPORT_MP_MODE
void fATSA(void *arg)
{
#if !defined(CONFIG_PLATFORM_8195BHP) && !defined(CONFIG_PLATFORM_8710C) && !defined(CONFIG_PLATFORM_8721D)
	u32 tConfigDebugInfo = ConfigDebugInfo;
	int argc = 0, channel;
	char *argv[MAX_ARGC] = {0}, *ptmp;
	u16 offset, gain;
	
	AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA]: _AT_SYSTEM_ADC_TEST_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=CHANNEL(1~3)");
		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=k_get");
		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=k_set[offet(hex),gain(hex)]");
		return;
	}
	
	argc = parse_param(arg, argv);
	if(strcmp(argv[1], "k_get") == 0){
		sys_adc_calibration(0, &offset, &gain);
//		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] offset = 0x%04X, gain = 0x%04X", offset, gain);
	}else if(strcmp(argv[1], "k_set") == 0){
		if(argc != 4){
			AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=k_set[offet(hex),gain(hex)]");
			return;
		}
		offset = strtoul(argv[2], &ptmp, 16);
		gain = strtoul(argv[3], &ptmp, 16);
		sys_adc_calibration(1, &offset, &gain);
//		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] offset = 0x%04X, gain = 0x%04X", offset, gain);
	}else{
		channel = atoi(argv[1]);
		if(channel < 1 || channel > 3){
			AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=CHANNEL(1~3)");
			return;
		}
		analogin_t   adc;
		u16 adcdat;
		
		// Remove debug info massage
		ConfigDebugInfo = 0;
		if(channel == 1)
			analogin_init(&adc, AD_1);
		else if(channel == 2)
			analogin_init(&adc, AD_2);
		else
			analogin_init(&adc, AD_3);
		adcdat = analogin_read_u16(&adc)>>4;
		analogin_deinit(&adc);
		// Recover debug info massage
		ConfigDebugInfo = tConfigDebugInfo;
		
		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] A%d = 0x%04X", channel, adcdat);
	}
#elif defined(CONFIG_PLATFORM_8721D)
	int argc = 0, channel;
	char *argv[MAX_ARGC] = {0};
	analogin_t   adc;
	u16 adcdat;
	u8 max_ch_num =   8;
	u32 ch_list[8] = {AD_0, AD_1, AD_2, AD_3, AD_4, AD_5, AD_6, AD_7};

	AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA]: _AT_SYSTEM_ADC_TEST_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=CHANNEL(0~7)");
		return;
	}

	argc = parse_param(arg, argv);

	channel = atoi(argv[1]);
	if(channel >= max_ch_num){
		AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] Usage: ATSA=CHANNEL(0~7)");
		return;
	}

	analogin_init(&adc, ch_list[channel]);
	adcdat = analogin_read_u16(&adc);
	analogin_deinit(&adc);
	
	AT_DBG_MSG(AT_FLAG_ADC, AT_DBG_ALWAYS, "[ATSA] A%d = 0x%04X", channel, adcdat);
#endif
}

void fATSG(void *arg)
{
#if defined(CONFIG_PLATFORM_8195A)
    gpio_t gpio_test;
    int argc = 0, val;
	char *argv[MAX_ARGC] = {0}, port, num;
	PinName pin = NC;
	u32 tConfigDebugInfo = ConfigDebugInfo;
    
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: _AT_SYSTEM_GPIO_TEST_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
		return;
	}else{
		argc = parse_param(arg, argv);
		if(argc != 2){
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
			return;
		}
	}
	port = argv[1][0];
	num = argv[1][1];
	if(port >= 'a' && port <= 'z')
		port -= ('a' - 'A');
	if(num >= 'a' && num <= 'z')
		num -= ('a' - 'A');
	switch(port){
		case 'A':
			switch(num){
				case '0': pin = PA_0; break; case '1': pin = PA_1; break; case '2': pin = PA_2; break; case '3': pin = PA_3; break;
				case '4': pin = PA_4; break; case '5': pin = PA_5; break; case '6': pin = PA_6; break; case '7': pin = PA_7; break;
			}
			break;
		case 'B':
			switch(num){
				case '0': pin = PB_0; break; case '1': pin = PB_1; break; case '2': pin = PB_2; break; case '3': pin = PB_3; break;
				case '4': pin = PB_4; break; case '5': pin = PB_5; break; case '6': pin = PB_6; break; case '7': pin = PB_7; break;
			}
			break;
		case 'C':
			switch(num){
				case '0': pin = PC_0; break; case '1': pin = PC_1; break; case '2': pin = PC_2; break; case '3': pin = PC_3; break;
				case '4': pin = PC_4; break; case '5': pin = PC_5; break; case '6': pin = PC_6; break; case '7': pin = PC_7; break;
				case '8': pin = PC_8; break; case '9': pin = PC_9; break;
			}
			break;
		case 'D':
			switch(num){
				case '0': pin = PD_0; break; case '1': pin = PD_1; break; case '2': pin = PD_2; break; case '3': pin = PD_3; break;
				case '4': pin = PD_4; break; case '5': pin = PD_5; break; case '6': pin = PD_6; break; case '7': pin = PD_7; break;
				case '8': pin = PD_8; break; case '9': pin = PD_9; break;
			}
			break;
		case 'E':
			switch(num){
				case '0': pin = PE_0; break; case '1': pin = PE_1; break; case '2': pin = PE_2; break; case '3': pin = PE_3; break;
				case '4': pin = PE_4; break; case '5': pin = PE_5; break; case '6': pin = PE_6; break; case '7': pin = PE_7; break;
				case '8': pin = PE_8; break; case '9': pin = PE_9; break; case 'A': pin = PE_A; break;
			}
			break;
		case 'F':
			switch(num){
				case '0': pin = PF_0; break; case '1': pin = PF_1; break; case '2': pin = PF_2; break; case '3': pin = PF_3; break;
				case '4': pin = PF_4; break; case '5': pin = PF_5; break;
			}
			break;
		case 'G':
			switch(num){
				case '0': pin = PG_0; break; case '1': pin = PG_1; break; case '2': pin = PG_2; break; case '3': pin = PG_3; break;
				case '4': pin = PG_4; break; case '5': pin = PG_5; break; case '6': pin = PG_6; break; case '7': pin = PG_7; break;
			}
			break;
		case 'H':
			switch(num){
				case '0': pin = PH_0; break; case '1': pin = PH_1; break; case '2': pin = PH_2; break; case '3': pin = PH_3; break;
				case '4': pin = PH_4; break; case '5': pin = PH_5; break; case '6': pin = PH_6; break; case '7': pin = PH_7; break;
			}
			break;
		case 'I':
			switch(num){
				case '0': pin = PI_0; break; case '1': pin = PI_1; break; case '2': pin = PI_2; break; case '3': pin = PI_3; break;
				case '4': pin = PI_4; break; case '5': pin = PI_5; break; case '6': pin = PI_6; break; case '7': pin = PI_7; break;
			}
			break;
		case 'J':
			switch(num){
				case '0': pin = PJ_0; break; case '1': pin = PJ_1; break; case '2': pin = PJ_2; break; case '3': pin = PJ_3; break;
				case '4': pin = PJ_4; break; case '5': pin = PJ_5; break; case '6': pin = PJ_6; break;
			}
			break;
		case 'K':
			switch(num){
				case '0': pin = PK_0; break; case '1': pin = PK_1; break; case '2': pin = PK_2; break; case '3': pin = PK_3; break;
				case '4': pin = PK_4; break; case '5': pin = PK_5; break; case '6': pin = PK_6; break;
			}
			break;
	}
	if(pin == NC){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: Invalid Pin Name");
		return;
	}
	// Remove debug info massage
	ConfigDebugInfo = 0;
	// Initial input control pin
	gpio_init(&gpio_test, pin);
	gpio_dir(&gpio_test, PIN_INPUT);     // Direction: Input
	gpio_mode(&gpio_test, PullUp);       // Pull-High
	val = gpio_read(&gpio_test);
	// Recover debug info massage
	ConfigDebugInfo = tConfigDebugInfo;
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] %c%c = %d", port, num, val);

#elif defined(CONFIG_PLATFORM_8711B)
    gpio_t gpio_test;
    int argc = 0, val, num;
	char *argv[MAX_ARGC] = {0}, port;
	PinName pin = NC;
	u32 tConfigDebugInfo = ConfigDebugInfo;
    
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: _AT_SYSTEM_GPIO_TEST_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
		return;
	}else{
		argc = parse_param(arg, argv);
		if(argc != 2){
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
			return;
		}
	}
	port = argv[1][0];
	if(port >= 'a' && port <= 'z')
		port -= ('a' - 'A');
	num = atoi(argv[1] + 1);
	
	//PA_6~PA_11 are not allowed to be tested when code running on flash. 
	//PA_16~PA_17 or PA_29~PA_30 should not be tested when they are used as log UART RX and TX.
	switch(port){
		case 'A':
			switch(num){
				case 0: pin = PA_0; break; case 1: pin = PA_1; break; case 2: pin = PA_2; break; case 3: pin = PA_3; break;
				case 4: pin = PA_4; break; case 5: pin = PA_5; break;  /*case 6:pin = PA_6; break; case 7: pin = PA_7; break;
				case 8: pin = PA_8; break; case 9: pin = PA_9; break; case 10: pin = PA_10; break; case 11: pin = PA_11; break;*/
				case 12: pin = PA_12; break; case 13: pin = PA_13; break; case 14: pin = PA_14; break; case 15: pin = PA_15; break;
				case 16: pin = PA_16; break; case 17: pin = PA_17; break; case 18: pin = PA_18; break; case 19: pin = PA_19; break;
				case 20: pin = PA_20; break; case 21: pin = PA_21; break; case 22: pin = PA_22; break; case 23: pin = PA_23; break;
				case 24: pin = PA_24; break; case 25: pin = PA_25; break; case 26: pin = PA_26; break; case 27: pin = PA_27; break;
				case 28: pin = PA_28; break; case 29: pin = PA_29; break; case 30: pin = PA_30; break; case 31: pin = PA_31; break;
			}
			break;
		case 'B':
			switch(num){
				case 0: pin = PB_0; break; case 1: pin = PB_1; break; case 2: pin = PB_2; break; case 3: pin = PB_3; break;
				case 4: pin = PB_4; break; case 5: pin = PB_5; break; case 6: pin = PB_6; break; case 7: pin = PB_7; break;
			}
			break;
	}
	if(pin == NC){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: Invalid Pin Name");
		return;
	}
	// Remove debug info massage
	ConfigDebugInfo = 0;
	// Initial input control pin
	gpio_init(&gpio_test, pin);
	gpio_dir(&gpio_test, PIN_INPUT);     // Direction: Input
	gpio_mode(&gpio_test, PullUp);       // Pull-High
	val = gpio_read(&gpio_test);
	// Recover debug info massage
	ConfigDebugInfo = tConfigDebugInfo;
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] %c%d = %d", port, num, val);

#elif defined(CONFIG_PLATFORM_8721D)
    gpio_t gpio_test;
    int argc = 0, val, num;
	char *argv[MAX_ARGC] = {0}, port;
	PinName pin = NC;
    
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: _AT_SYSTEM_GPIO_TEST_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
		return;
	}else{
		argc = parse_param(arg, argv);
		if(argc != 2){
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
			return;
		}
	}
	port = argv[1][0];
	if(port >= 'a' && port <= 'z')
		port -= ('a' - 'A');
	num = atoi(argv[1] + 1);
	
	//PB_12~PB_17 or PB_18~PB_23 are not allowed to be tested when code running on flash. 
	//PA_7~PA_8 should not be tested when they are used as log UART RX and TX.
	switch(port){
		case 'A':
			switch(num){
				case 0: pin = PA_0; break; case 1: pin = PA_1; break; case 2: pin = PA_2; break; case 3: pin = PA_3; break;
				case 4: pin = PA_4; break; case 5: pin = PA_5; break;  case 6:pin = PA_6; break; case 7: pin = PA_7; break;
				case 8: pin = PA_8; break; case 9: pin = PA_9; break; case 10: pin = PA_10; break; case 11: pin = PA_11; break;
				case 12: pin = PA_12; break; case 13: pin = PA_13; break; case 14: pin = PA_14; break; case 15: pin = PA_15; break;
				case 16: pin = PA_16; break; case 17: pin = PA_17; break; case 18: pin = PA_18; break; case 19: pin = PA_19; break;
				case 20: pin = PA_20; break; case 21: pin = PA_21; break; case 22: pin = PA_22; break; case 23: pin = PA_23; break;
				case 24: pin = PA_24; break; case 25: pin = PA_25; break; case 26: pin = PA_26; break; case 27: pin = PA_27; break;
				case 28: pin = PA_28; break; case 29: pin = PA_29; break; case 30: pin = PA_30; break; case 31: pin = PA_31; break;
			}
			break;
		case 'B':
			switch(num){
				case 0: pin = PB_0; break; case 1: pin = PB_1; break; case 2: pin = PB_2; break; case 3: pin = PB_3; break;
				case 4: pin = PB_4; break; case 5: pin = PB_5; break; case 6: pin = PB_6; break; case 7: pin = PB_7; break;
				case 8: pin = PB_8; break; case 9: pin = PA_9; break; case 10: pin = PA_10; break; case 11: pin = PA_11; break;
				/*case 12: pin = PB_12; break; case 13: pin = PB_13; break; case 14: pin = PB_14; break; case 15: pin = PB_15; break;
				case 16: pin = PB_16; break; case 17: pin = PB_17; break; case 18: pin = PB_18; break; case 19: pin = PB_19; break;*/
				case 20: pin = PB_20; break; case 21: pin = PB_21; break; case 22: pin = PB_22; break; case 23: pin = PB_23; break;
				case 24: pin = PB_24; break; case 25: pin = PB_25; break; case 26: pin = PB_26; break; case 27: pin = PB_27; break;
				case 28: pin = PB_28; break; case 29: pin = PB_29; break; case 30: pin = PB_30; break; case 31: pin = PB_31; break;
			}
			break;
	}
	if(pin == NC){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: Invalid Pin Name");
		return;
	}

	// Initial input control pin
	gpio_init(&gpio_test, pin);
	gpio_dir(&gpio_test, PIN_INPUT);     // Direction: Input
	gpio_mode(&gpio_test, PullUp);       // Pull-High
	val = gpio_read(&gpio_test);
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] %c%d = %d", port, num, val);

#elif defined(CONFIG_PLATFORM_8710C)
	gpio_t gpio_test;
	int argc = 0, val, num;
	char *argv[MAX_ARGC] = {0}, port;

	PinName pin = NC;
	u32 tConfigDebugInfo = ConfigDebugInfo;
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: _AT_SYSTEM_GPIO_TEST_");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
		return;
	}else{
		argc = parse_param(arg, argv);
		if(argc != 2){
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] Usage: ATSG=PINNAME(ex:A0)");
			return;
		}
	}
	port = argv[1][0];
	if(port >= 'a' && port <= 'z')
		port -= ('a' - 'A');
	num = atoi(argv[1] + 1);
	//PA_7~PA_12  only available on RTL8720CF
	//PA_0~PA_4 should not be tested when they are used as JTAG/SWD.
	//PA_15~PA_16 or PA_0~PA_1 or PA_2~PA_3 or PA_13~PA_14 should not be tested when they are used as UART RX and TX.
	//PA_5, PA_6, PA_21 and PA_22 are not able to be tested
	switch(port){
		case 'A':
			switch(num){
				case 0: pin = PA_0; break; case 1: pin = PA_1; break; case 2: pin = PA_2; break; case 3: pin = PA_3; break;
				case 4: pin = PA_4; break; /*case 5: pin = PA_5; break;  case 6:pin = PA_6; break; case 7: pin = PA_7; break;
				case 8: pin = PA_8; break; case 9: pin = PA_9; break; case 10: pin = PA_10; break; case 11: pin = PA_11; break;
				case 12: pin = PA_12; break;*/ case 13: pin = PA_13; break; case 14: pin = PA_14; break; case 15: pin = PA_15; break;
				case 16: pin = PA_16; break; case 17: pin = PA_17; break; case 18: pin = PA_18; break; case 19: pin = PA_19; break;
				case 20: pin = PA_20; break; /* case 21: pin = PA_21; break; case 22: pin = PA_22; break;*/ case 23: pin = PA_23; break;
			}
			break;
	}
	if(pin == NC){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: Invalid Pin Name");
		return;
	}
	// Remove debug info massage
	ConfigDebugInfo = 0;
	// Initial input control pin
	gpio_init(&gpio_test, pin);
	gpio_dir(&gpio_test, PIN_INPUT);     // Direction: Input
	gpio_mode(&gpio_test, PullUp);       // Pull-High
	val = gpio_read(&gpio_test);
	// Recover debug info massage
	ConfigDebugInfo = tConfigDebugInfo;
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG] %c%d = %d", port, num, val);
#endif
}

void fATSP(void *arg)
{
#if defined(CONFIG_PLATFORM_8195A)
	int   argc           = 0;
	char *argv[MAX_ARGC] = {0};

	unsigned long timeout; // ms
	unsigned long time_begin, time_current;

	gpio_t gpiob_1;
	int val_old, val_new;

	int expected_zerocount, zerocount;
	int test_result;

	// parameter check
	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSP]: _AT_SYSTEM_POWER_PIN_TEST_");
	if(!arg) {
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSP]: Usage: ATSP=gpiob1[timeout,zerocount]");
		return;
	} else {
		argc = parse_param(arg, argv);
		if (argc < 2) {
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSP]: Usage: ATSP=gpiob1[timeout,zerocount]");
			return;
		}
	}

	if ( strcmp(argv[1], "gpiob1" ) == 0 ) {
		if (argc < 4) {
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSP]: Usage: ATSP=gpiob1[timeout,zerocount]");
			return;
		}

		// init gpiob1 test
		test_result         = 0;
		timeout             = strtoul(argv[2], NULL, 10);
		expected_zerocount  = atoi(argv[3]);
		zerocount           = 0;
		val_old             = 1;

		sys_log_uart_off();

		gpio_init(&gpiob_1, PB_1);
		gpio_dir(&gpiob_1, PIN_INPUT);
		gpio_mode(&gpiob_1, PullDown);

		// gpiob1 test ++
		time_begin = time_current = xTaskGetTickCount();
		while (time_current < time_begin + timeout) {
			val_new = gpio_read(&gpiob_1);

			if (val_new != val_old && val_new == 0) {

				zerocount ++;
				if (zerocount == expected_zerocount) {
					test_result = 1;
					break;
				}
			}

			val_old = val_new;
			time_current = xTaskGetTickCount();
		}
		// gpio test --

		sys_log_uart_on();

		if (test_result == 1) {
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSP]: success");
		} else {
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSP]: fail, it only got %d zeros", zerocount);
		}
	}
#endif
}

#if !defined(CONFIG_PLATFORM_8195BHP) && !defined(CONFIG_PLATFORM_8710C)
int write_otu_to_system_data(flash_t *flash, uint32_t otu_addr)
{
	uint32_t data, i = 0;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_read_word(flash, FLASH_SYSTEM_DATA_ADDR+0xc, &data);
	//printf("\n\r[%s] data 0x%x otu_addr 0x%x", __FUNCTION__, data, otu_addr);
	AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: data 0x%x otu_addr 0x%x", data, otu_addr);	
	if(data == ~0x0){
		flash_write_word(flash, FLASH_SYSTEM_DATA_ADDR+0xc, otu_addr);
	}else{
		//erase backup sector
		flash_erase_sector(flash, FLASH_RESERVED_DATA_BASE);
		//backup system data to backup sector
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(flash, FLASH_SYSTEM_DATA_ADDR + i, &data);
			if(i == 0xc)
				data = otu_addr;
			flash_write_word(flash, FLASH_RESERVED_DATA_BASE + i,data);
		}
		//erase system data
		flash_erase_sector(flash, FLASH_SYSTEM_DATA_ADDR);
		//write data back to system data
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(flash, FLASH_RESERVED_DATA_BASE + i, &data);
			flash_write_word(flash, FLASH_SYSTEM_DATA_ADDR + i,data);
		}
		//erase backup sector
		flash_erase_sector(flash, FLASH_RESERVED_DATA_BASE);
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return 0;
}
#endif

void fATSB(void *arg)
{
#if !defined(CONFIG_PLATFORM_8195BHP) && !defined(CONFIG_PLATFORM_8710C)
	int   argc           = 0;
	char *argv[MAX_ARGC] = {0};
	u32 boot_gpio, rb_boot_gpio;
	u8 gpio_pin;
	u8 uart_port, uart_index;
	u8 gpio_pin_bar;
	u8 uart_port_bar;		
	flash_t flash;

	// parameter check
	AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: _AT_SYSTEM_BOOT_OTU_PIN_SET_");
	if(!arg) {
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: Usage: ATSB=[GPIO_PIN,TRIGER_MODE,UART]");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: GPIO_PIN: PB_1, PC_4 ....");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: TRIGER_MODE: low_trigger, high_trigger");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: UART: UART0, UART2");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: example: ATSB=[PC_2,low_trigger,UART2]");
		return;
	} else {
		argc = parse_param(arg, argv);
		if (argc != 4 ) {
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: Usage: ATSB=[GPIO_PIN,TRIGER_MODE,UART]");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: GPIO_PIN: PB_1, PC_4 ....");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: TRIGER_MODE: low_trigger, high_trigger");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: UART: UART0, UART2");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: example: ATSB=[PC_2,low_trigger,UART2]");
			return;
		}
	}

	if ( strncmp(argv[1], "P", 1) == 0 && strlen(argv[1]) == 4
		&& (strcmp(argv[2], "low_trigger") == 0 || strcmp(argv[2], "high_trigger") == 0)
		&& strncmp(argv[3], "UART", 4) == 0 && strlen(argv[3]) == 5) {
		if((argv[1][1] >= 0x41 && argv[1][1] <= 0x45) && (argv[1][3] >= 0x30 && argv[1][3] <= 0x39) &&(argv[3][4] >= 0x30 && argv[3][4] <= 0x32)){
			if(strcmp(argv[2], "high_trigger") == 0)
				gpio_pin = 1<< 7 | ((argv[1][1]-0x41)<<4) | (argv[1][3] - 0x30);
			else
				gpio_pin = ((argv[1][1]-0x41)<<4) | (argv[1][3] - 0x30);
			gpio_pin_bar = ~gpio_pin;
			uart_index = argv[3][4] - 0x30;
			if(uart_index == 0)
				uart_port = (uart_index<<4)|2;
			else if(uart_index == 2)
				uart_port = (uart_index<<4)|0;
			else{
				AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: Input UART index error. Please choose UART0 or UART2.");
				AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: example: ATSB=[PC_2,low_trigger,UART2]");
				return;
			}
			uart_port_bar = ~uart_port;
			boot_gpio = uart_port_bar<<24 | uart_port<<16 | gpio_pin_bar<<8 | gpio_pin;
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]:gpio_pin 0x%x", gpio_pin);
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]:gpio_pin_bar 0x%x", gpio_pin_bar);
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]:uart_port 0x%x", uart_port);
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]:uart_port_bar 0x%x", uart_port_bar);
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]:boot_gpio 0x%x", boot_gpio);
			write_otu_to_system_data(&flash, boot_gpio);
			device_mutex_lock(RT_DEV_LOCK_FLASH);
			flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR+0x0c, &rb_boot_gpio);			
			device_mutex_unlock(RT_DEV_LOCK_FLASH);
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]:Read 0x900c 0x%x", rb_boot_gpio);
		}else{
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: Usage: ATSB=[GPIO_PIN,TRIGER_MODE,UART]");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: GPIO_PIN: PB_1, PC_4 ....");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: TRIGER_MODE: low_trigger, high_trigger");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: UART: UART0, UART2");
			AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: example: ATSB=[PC_2,low_trigger,UART2]");
		}		
	}else{
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: Usage: ATSB=[GPIO_PIN,TRIGER_MODE,UART]");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: GPIO_PIN: PB_1, PC_4 ....");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: TRIGER_MODE: low_trigger, high_trigger");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: UART: UART0, UART2");
		AT_DBG_MSG(AT_FLAG_DUMP, AT_DBG_ALWAYS, "[ATSB]: example: ATSB=[PC_2,low_trigger,UART2]");
		return;
	}
#endif
}

#if defined(CONFIG_MIIO_MP) && CONFIG_MIIO_MP
#include "efuse_api.h"
#include "soft_crc.h"
/*pin1 == 0==> pin2
  * pin1 == 1==> pin2
  * pin2 == 0==> pin1
  * pin2 == 1==> pin1 */
static int gpio_test(PinName pin1, PinName pin2)
{
	gpio_t gpio_out;
	gpio_t gpio_in;
	PinName pinout;
	PinName pinin;

	for(int i=0; i<2; i++) {
		if(i == 0) {
			pinout = pin1;
			pinin = pin2;
		}else {
			pinout = pin2;
			pinin = pin1;
		}
		
		gpio_init(&gpio_out, pinout);
		gpio_dir(&gpio_out, PIN_OUTPUT);
		gpio_mode(&gpio_out, PullNone);
		gpio_init(&gpio_in, pinin);
		gpio_dir(&gpio_in, PIN_INPUT);
		gpio_mode(&gpio_in, PullNone);

		gpio_write(&gpio_out, 1);
		if(gpio_read(&gpio_in) != 1) {
			gpio_deinit(&gpio_out);
			gpio_deinit(&gpio_in);
			return -1;
		}
		gpio_write(&gpio_out, 0);
		if(gpio_read(&gpio_in) != 0) {
			gpio_deinit(&gpio_out);
			gpio_deinit(&gpio_in);
			return -1;
		}
		gpio_deinit(&gpio_out);
		gpio_deinit(&gpio_in);
	}
	return 0;
}

void fATSF(void *arg)
{
	int argc, valid_i=0;
	unsigned char *argv[MAX_ARGC] = {0};
	
	argv[0] = "MIIO_MP";
	argc = parse_param(arg, argv);

	if(2 == argc && !strcmp(argv[1], "g")) {
		// ATSF=g (GPIO test)
		//PA_(0,3)(1,23)(2,13)(4,17)(14,18)(19,20)
		//error:2,13;4,17;14,18	
		valid_i = 1;
		char len;
		PinName pin = NC;
		u32 tConfigDebugInfo = ConfigDebugInfo;
		unsigned char error_msg[50] =  "error:";
		len = strlen(error_msg);
		
		// Remove pinmux and debug info massage
		sys_jtag_off();
		ConfigDebugInfo = 0;
		if(gpio_test(PA_0, PA_3) < 0) {
			memcpy(error_msg+len, "0,3", 3);
			len = strlen(error_msg);	
		}
		if(gpio_test(PA_1, PA_23) < 0) {
			if(':' == error_msg[len-1])
				memcpy(error_msg+len, "1,23", 4);
			else
				memcpy(error_msg+len, ";1,23", 5);
			len = strlen(error_msg);
		}
		if(gpio_test(PA_2, PA_13) < 0) {
			if(':' == error_msg[len-1])
				memcpy(error_msg+len, "2,13", 4);
			else
				memcpy(error_msg+len, ";2,13", 5);
			len = strlen(error_msg);
		}
		if(gpio_test(PA_4, PA_17) < 0) {
			if(':' == error_msg[len-1])
				memcpy(error_msg+len, "4,17", 4);
			else
				memcpy(error_msg+len, ";4,17", 5);
			len = strlen(error_msg);
		}
		if(gpio_test(PA_14, PA_18) < 0) {
			if(':' == error_msg[len-1])
				memcpy(error_msg+len, "14,18", 5);
			else
				memcpy(error_msg+len, ";14,18", 6);
			len = strlen(error_msg);
		}
		if(gpio_test(PA_19, PA_20) < 0) {
			if(':' == error_msg[len-1])
				memcpy(error_msg+len, "19,20", 5);
			else
				memcpy(error_msg+len, ";19,20", 6);
			len = strlen(error_msg);
		}
		// Recover debug info massage
		ConfigDebugInfo = tConfigDebugInfo;
		if(len == strlen("error:")) {
			printf("pass");
		}else {
			printf("%s", error_msg);
		}		
	}else if(2 == argc && !strcmp(argv[1], "r")) {
		// ATSF=r (readback written data as written format)
		valid_i = 1;
		unsigned char ascii_buf[68+1] = {0};
		unsigned char output_buf[34+1] = {0};

		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		if(efuse_logical_read(0x11A, 6, output_buf) < 0 || efuse_otp_read(0, 34-6, &output_buf[6]) < 0) {
			printf("error");
		}
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
		
		for(int i=0; i<68; i++) {
			if(i%2 == 0) {
				ascii_buf[i] = (output_buf[i/2] >> 4)&0x0F;
			}else {
				ascii_buf[i] = output_buf[i/2]&0x0F;
			}

			if(ascii_buf[i] >= 0 && ascii_buf[i] <= 9) {
				ascii_buf[i] += '0';
			}else if(ascii_buf[i] >= 10 && ascii_buf[i] <= 15) {
				ascii_buf[i] = ascii_buf[i] - 10 + 'A';
			}else {
			}
		}
		printf("%s", ascii_buf);
	}else if(3 == argc && !strcmp(argv[1], "w") && 68 == strlen(argv[2])) {
		// ATSF=w,xxxxxxxx (write 68-byte-ascii triple_group (did/psk/mac/crc) )
		valid_i = 1;
		unsigned int crc32_c = 0, crc32_i = 0;
		unsigned char otp_buf[32+1] = {0};
		unsigned char ascii_buf[68+1] = {0};
		unsigned char input_buf[34+1] = {0};
		memcpy(ascii_buf, argv[2], 68);
		
		// 1. If data already existed, return "existed"
		if(efuse_otp_read(0, 32, otp_buf) < 0) {
			printf("error");
			return;
		}
		for(int i=0; i<32; i++) {
			if(otp_buf[i] != 0xFF) {
				printf("existed");
				return;
			}
		}
		
		// 2. check crc32 first
		for(int i=0; i<68; i++) {
			if(ascii_buf[i] >= '0' && ascii_buf[i] <= '9') {
				input_buf[i/2] |= (ascii_buf[i] - '0')&0x0F;	
				if(i%2 == 0) {
					input_buf[i/2] <<=  4;	
				}
			}else if(ascii_buf[i] >= 'A' && ascii_buf[i] <= 'F') {
				input_buf[i/2] |= (ascii_buf[i] - 'A' + 10)&0x0F;	
				if(i%2 == 0) {
					input_buf[i/2] <<= 4;	
				}
			}else if(ascii_buf[i] >= 'a' && ascii_buf[i] <= 'f') {
				input_buf[i/2] |= (ascii_buf[i] - 'a' + 10)&0x0F;	
				if(i%2 == 0) {
					input_buf[i/2] <<= 4;	
				}
			}else {
				printf("error");
				return;
			}
		}

		crc32_c = soft_crc32(input_buf, 30, 0);
		for(int i=0; i<4; i++) {
			crc32_i <<= 8;
			crc32_i |= input_buf[i+30];
		}
		if(crc32_c != crc32_i) {
			printf("error");
			return;
		}

		// 3. write MAC to logical-map area and did/psk/crc32 to otp area.
		memset(otp_buf, 0, sizeof(otp_buf));
		memcpy(otp_buf, &input_buf[6], 34-6);
		device_mutex_lock(RT_DEV_LOCK_EFUSE);
		if(efuse_otp_write(0, 34-6, otp_buf) < 0 || efuse_logical_write(0x11A, 6, input_buf) < 0) {
			printf("error");
		}else {
			printf("ok");
		}		
		device_mutex_unlock(RT_DEV_LOCK_EFUSE);
	}else {
	}

	if(!valid_i) {
		printf("\r\n[Usage] ATSF=g\t(Test GPIO)");
		printf("\r\n[Usage] ATSF=r\t(Read Triple_Group)");
		printf("\r\n[Usage] ATSF=w,xxxxxxxx\t(Write 68-BYTE Triple_Group)\n");
	}
}
#endif
#endif

#if (configGENERATE_RUN_TIME_STATS == 1)
void fATSS(void *arg)	// Show CPU stats
{
	AT_PRINTK("[ATSS]: _AT_SYSTEM_CPU_STATS_");
	vTaskGetRunTimeStats((char *)cBuffer);
	AT_PRINTK("%s", cBuffer);
}
#endif

void fATSs(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	AT_PRINTK("[ATS@]: _AT_SYSTEM_DBG_SETTING_");
	if(!arg){
		AT_PRINTK("[ATS@] Usage: ATS@=[LEVLE,FLAG]");
	}else{
		argc = parse_param(arg, argv);
		if(argc == 3){
			char *ptmp;
			gDbgLevel = atoi(argv[1]);
			gDbgFlag = strtoul(argv[2], &ptmp, 16);
		}
	}
	AT_PRINTK("[ATS@] level = %d, flag = 0x%08X", gDbgLevel, gDbgFlag);
}

void fATSc(void *arg)
{
#if defined (CONFIG_PLATFORM_8721D)
	int argc = 0, config = 0;
	char *argv[MAX_ARGC] = {0};

	AT_PRINTK("[ATS!]: _AT_SYSTEM_CONFIG_SETTING_");
	if(!arg){
		AT_PRINTK("[ATS!] Usage: ATS!=[CONFIG(0,1,2),FLAG]");
	}else{
		argc = parse_param(arg, argv);
		if(argc == 3){
			char *ptmp;
			config = atoi(argv[1]);
			if(config == 0)
				ConfigDebug[LEVEL_ERROR] = strtoul(argv[2], &ptmp, 16);
			if(config == 1)
				ConfigDebug[LEVEL_INFO] = strtoul(argv[2], &ptmp, 16);
			if(config == 2)
				ConfigDebug[LEVEL_WARN] = strtoul(argv[2], &ptmp, 16);
		}
	}
	AT_PRINTK("[ATS!] ConfigDebugErr  = 0x%08X", ConfigDebug[LEVEL_ERROR]);
	AT_PRINTK("[ATS!] ConfigDebugInfo = 0x%08X", ConfigDebug[LEVEL_INFO]);
	AT_PRINTK("[ATS!] ConfigDebugWarn = 0x%08X", ConfigDebug[LEVEL_WARN]);
#else
	int argc = 0, config = 0;
	char *argv[MAX_ARGC] = {0};

	AT_PRINTK("[ATS!]: _AT_SYSTEM_CONFIG_SETTING_");
	if(!arg){
		AT_PRINTK("[ATS!] Usage: ATS!=[CONFIG(0,1,2),FLAG]");
	}else{
		argc = parse_param(arg, argv);
		if(argc == 3){
			char *ptmp;
			config = atoi(argv[1]);
			if(config == 0)
				ConfigDebugErr = strtoul(argv[2], &ptmp, 16);
			if(config == 1)
				ConfigDebugInfo = strtoul(argv[2], &ptmp, 16);
			if(config == 2)
				ConfigDebugWarn = strtoul(argv[2], &ptmp, 16);
		}
	}
	AT_PRINTK("[ATS!] ConfigDebugErr  = 0x%08X", ConfigDebugErr);
	AT_PRINTK("[ATS!] ConfigDebugInfo = 0x%08X", ConfigDebugInfo);
	AT_PRINTK("[ATS!] ConfigDebugWarn = 0x%08X", ConfigDebugWarn);
#endif
}

#define SUPPORT_CP_TEST 0
#if SUPPORT_CP_TEST
extern void MFi_auth_test(void);
void fATSM(void *arg)
{
	AT_PRINTK("[ATSM]: _AT_SYSTEM_CP_");
	MFi_auth_test();
}
#endif

void fATSt(void *arg)
{
	/* To avoid gcc warnings */
	( void ) arg;

	AT_PRINTK("[ATS#]: _AT_SYSTEM_TEST_");
}

#if defined(CONFIG_PLATFORM_8711B)
/*Function: Check if the input jtag key is matched with the jtag password derived from the SB key stored in EFUSE.
		    If the input jtag key is correct, it will be stored in system data area of the flash.
		    Otherwise, the last 1 of the error map will be written to 0, which is also stored in system data of the flash. */
static void sys_enable_jtag_by_password(char *keystring)
{
	flash_t flash;
	u8 key[8];
	u32 data, key32[8], i = 0, errmap = 0;
	int is_match = 0;

	if(strlen(keystring) < 16){
		AT_PRINTK("%s(): Key length should be 16 characters.", __func__);
		return;
	}
	AT_PRINTK("Enter JTAG Key: %s\n", keystring);
	sscanf((const char*)keystring, "%02x%02x%02x%02x%02x%02x%02x%02x",
		&key32[0], &key32[1], &key32[2], &key32[3], &key32[4], &key32[5], &key32[6], &key32[7]);
	for(i=0; i<8; i++){
		key[i] = key32[i] & 0xFF;
	}

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR + 0x40, &errmap);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	AT_PRINTK("Error Map: 0x%x\n", (errmap & 0xFF));
	if((errmap & 0xFF) == 0){
		AT_PRINTK("You have tried too much times!Locked!\n", keystring);
		return;
	}

	// check if jtag key is correct
	is_match = boot_export_symbol.is_jtag_key_match(key);

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR + 0x44, &data);
	if(data != ~0x0){
		//erase backup sector
		flash_erase_sector(&flash, FLASH_RESERVED_DATA_BASE);
		//backup system data to backup sector
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR + i, &data);
			//Erase Errmap and key
			if((i == 0x44) || (i == 0x48))
				data = 0xFFFFFFFF;
			if((i == 0x40) && is_match)
				data = 0xFFFFFFFF;
			flash_write_word(&flash, FLASH_RESERVED_DATA_BASE + i,data);
		}
		
		//erase system data
		flash_erase_sector(&flash, FLASH_SYSTEM_DATA_ADDR);
		//write data back to system data
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(&flash, FLASH_RESERVED_DATA_BASE + i, &data);
			flash_write_word(&flash, FLASH_SYSTEM_DATA_ADDR + i,data);
		}
		//erase backup sector
		flash_erase_sector(&flash, FLASH_RESERVED_DATA_BASE);
	}
	//write jtag key
	flash_stream_write(&flash, FLASH_SYSTEM_DATA_ADDR + 0x44, 8, key);
	//update error map
	if(is_match == 0){
		flash_write_word(&flash, FLASH_SYSTEM_DATA_ADDR + 0x40, errmap<<1);
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
}
#endif

void fATSJ(void *arg)
{
	/* To avoid gcc warnings */
	( void ) arg;
	
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
  
	( void )argc;

#if defined (CONFIG_PLATFORM_8721D)
	#ifdef AMEBAD_TODO
	AT_PRINTK("[ATSJ]: _AT_SYSTEM_JTAG_");
	if(!arg){
		AT_PRINTK("[ATSJ] Usage: ATSJ=off");
	}else{
		argc = parse_param(arg, argv);
		if (strcmp(argv[1], "off" ) == 0)
			sys_jtag_off();
#if defined(CONFIG_PLATFORM_8711B)
		else if (strcmp(argv[1], "key" ) == 0)
			sys_enable_jtag_by_password(argv[2]); //Enter "FFFFFFFFFFFFFFFF" to clear key in flash
#endif			
		else
			AT_PRINTK("ATSJ=%s is not supported!", argv[1]);
	}
	#endif
#else

	AT_PRINTK("[ATSJ]: _AT_SYSTEM_JTAG_");
	if(!arg){
		AT_PRINTK("[ATSJ] Usage: ATSJ=off");
	}else{
		argc = parse_param(arg, argv);
		if (strcmp(argv[1], "off" ) == 0)
			sys_jtag_off();
#if defined(CONFIG_PLATFORM_8711B)
		else if (strcmp(argv[1], "key" ) == 0)
			sys_enable_jtag_by_password(argv[2]); //Enter "FFFFFFFFFFFFFFFF" to clear key in flash
#endif			
		else
			AT_PRINTK("ATSJ=%s is not supported!", argv[1]);
	}
#endif
}


#if WIFI_LOGO_CERTIFICATION_CONFIG

#define FLASH_ADDR_SW_VERSION 	FAST_RECONNECT_DATA+0x900
#define SW_VERSION_LENGTH 	32


void fATSV(void *arg)
{
	unsigned char sw_version[SW_VERSION_LENGTH+1];
	flash_t flash;

	if(!arg){
		printf("[ATSV]Usage: ATSV=[SW_VERSION]\n\r");
		return;
    }

	if(strlen((char*)arg) > SW_VERSION_LENGTH){
		printf("[ATSV] ERROR : SW_VERSION length can't exceed %d\n\r",SW_VERSION_LENGTH);
		return;
	}

	memset(sw_version,0,SW_VERSION_LENGTH+1);
	strncpy(sw_version, (char*)arg, strlen((char*)arg));

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash, FAST_RECONNECT_DATA);
	flash_stream_write(&flash, FLASH_ADDR_SW_VERSION, strlen((char*)arg), (uint8_t *) sw_version);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	printf("[ATSV] Write SW_VERSION to flash : %s\n\r",sw_version);

}
#endif


void fATSx(void *arg)
{
	/* To avoid gcc warnings */
	( void ) arg;
	
//	uint32_t ability = 0;
	char buf[64];

	AT_PRINTK("[ATS?]: _AT_SYSTEM_HELP_");
	AT_PRINTK("[ATS?]: COMPILE TIME: %s", RTL_FW_COMPILE_TIME);
//	wifi_get_drv_ability(&ability);
	strncpy(buf, "v", sizeof(buf));
//	if(ability & 0x1)
//		strcat(buf, "m");
#if defined(CONFIG_PLATFORM_8710C)
	strcat(buf, ".7.1." RTL_FW_COMPILE_DATE);
#elif defined(CONFIG_PLATFORM_8195BHP)
	strcat(buf, ".5.2." RTL_FW_COMPILE_DATE);
#else
	strcat(buf, ".4.0." RTL_FW_COMPILE_DATE);
#endif

#if WIFI_LOGO_CERTIFICATION_CONFIG
	flash_t		flash;
	unsigned char sw_version[SW_VERSION_LENGTH+1];

	memset(sw_version,0,SW_VERSION_LENGTH+1);

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, FLASH_ADDR_SW_VERSION, SW_VERSION_LENGTH, (uint8_t *)sw_version);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	if(sw_version[0] != 0xff)
		AT_PRINTK("[ATS?]: SW VERSION: %s", sw_version);
	else
		AT_PRINTK("[ATS?]: SW VERSION: %s", buf);

#else
	AT_PRINTK("[ATS?]: SW VERSION: %s", buf);
#endif
}
#elif ATCMD_VER == ATVER_2

#define ATCMD_VERSION		"v2" //ATCMD MAJOR VERSION, AT FORMAT CHANGED
#define ATCMD_SUBVERSION	"2" //ATCMD MINOR VERSION, NEW COMMAND ADDED
#define ATCMD_REVISION		"1" //ATCMD FIX BUG REVISION
#define SDK_VERSION		"v7.1" //SDK VERSION
extern void sys_reset(void);
void print_system_at(void *arg);
extern void print_wifi_at(void *arg);
extern void print_tcpip_at(void *arg);

// uart version 2 echo info
extern unsigned char gAT_Echo;


void fATS0(void *arg){
	at_printf("\r\n[AT] OK");
}

void fATSh(void *arg){
	// print common AT command
	at_printf("\r\n[ATS?] ");
	at_printf("\r\nCommon AT Command:");
	print_system_at(arg);
#if CONFIG_WLAN
	at_printf("\r\nWi-Fi AT Command:");
	print_wifi_at(arg);
#endif

#if CONFIG_TRANSPORT
	at_printf("\r\nTCP/IP AT Command:");
	print_tcpip_at(arg);
#endif

	at_printf("\r\n[ATS?] OK");
}

void fATSR(void *arg){
	at_printf("\r\n[ATSR] OK");
	sys_reset();
}

void fATSV(void *arg){
	char at_buf[32];
	char fw_buf[32];

	// get at version
	strncpy(at_buf, ATCMD_VERSION"."ATCMD_SUBVERSION"."ATCMD_REVISION, sizeof(at_buf));

	// get fw version
	strncpy(fw_buf, SDK_VERSION, sizeof(fw_buf));
#if defined CONFIG_PLATFORM_8195A
	at_printf("\r\n[ATSV] OK:%s,%s(%s)",at_buf,fw_buf,RTL8195AFW_COMPILE_TIME);
#elif defined CONFIG_PLATFORM_8710C
	at_printf("\r\n[ATSV] OK:%s,%s(%s)",at_buf,fw_buf,RTL8710CFW_COMPILE_TIME);
#endif
}

#if defined(CONFIG_PLATFORM_8710C)
#include "power_mode_api.h"
#include "gpio_irq_api.h"
#include "gpio_irq_ex_api.h"

static gpio_irq_t my_GPIO_IRQ;

int valid_wake_pin(int i) {
	if (i >= 0 && i <= 23) {
		if (i==5||i==6||i==21||i==22) return 0;
		else return 1;
	} else return 0;
}
#endif

void fATSP(void *arg){

	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	uint32_t lock_id;
	uint32_t bitmap;
#if defined(CONFIG_PLATFORM_8710C)
	int sleep_duration = 0;
	int wake_pin = 0;
	u8 sleep_option = 0;
#endif

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] Usage: ATSP=<a/r/d/?>");
		at_printf("\r\n[ATSP] ERROR:1");
		return;
	} else {
		if((argc = parse_param(arg, argv)) < 2){
			AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] Usage: ATSP=<a/r/d/?>");
			at_printf("\r\n[ATSP] ERROR:1");
			return;
		}
	}

	switch(argv[1][0]) {
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
		case 'a': // acquire
		{
			pmu_acquire_wakelock(PMU_OS);
			//at_printf("\r\n[ATSP] wakelock:0x%08x", pmu_get_wakelock_status());
			break;
		}

		case 'r': // release
		{
			pmu_release_wakelock(PMU_OS);
			//at_printf("\r\n[ATSP] wakelock:0x%08x", pmu_get_wakelock_status());
			break;
		}
		case '?': // get status
			break;
#else /* define configUSE_WAKELOCK_PMU */
		case 'a':
		case 'r':
		case '?':
		{
			AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] need to set configUSE_WAKELOCK_PMU to be 1 to use tickless feature");
			break;
		}
#endif /* define configUSE_WAKELOCK_PMU */
#if defined(CONFIG_PLATFORM_8710C)
		case 'd':
		{
			if (argc < 3) {
				AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] Usage: ATSP=<d,sleep_duration,wake_pin>");
				break;
			}
			sleep_duration = atoi(argv[2]);
			if (sleep_duration) sleep_option |= DS_STIMER;
			if (argc > 3) {
				wake_pin = atoi(argv[3]);
				if (valid_wake_pin(wake_pin)) {
					gpio_irq_init(&my_GPIO_IRQ, PIN_NAME(PORT_A, wake_pin), NULL, (uint32_t)&my_GPIO_IRQ);
					gpio_irq_pull_ctrl(&my_GPIO_IRQ, PullNone);
					gpio_irq_set(&my_GPIO_IRQ, IRQ_FALL, 1);
					sleep_option |= DS_GPIO;
				} else {
					AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] Usage: Valid pin are A0-4,A7-20,A23");
				}
			}

			if (sleep_option) DeepSleep(sleep_option, sleep_duration, 0);
			break;
		}

		default:
			AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] Usage: ATSP=<a/r/d/?>");
			at_printf("\r\n[ATSP] ERROR:2");
			return;
#else /* defined CONFIG_PLATFORM_8710C */
		default:
			AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "\r\n[ATSP] Usage: ATSP=<a/r/?>");
			at_printf("\r\n[ATSP] ERROR:2");
			return;
#endif /* defined CONFIG_PLATFORM_8710C */
	}
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
	bitmap = pmu_get_wakelock_status();
	at_printf("\r\n[ATSP] OK:%s", (bitmap&BIT(PMU_OS))?"1":"0");
#endif

}

void fATSE(void *arg){
	int argc = 0;
	int echo = 0, mask = gDbgFlag, dbg_lv = gDbgLevel;
	char *argv[MAX_ARGC] = {0};
	int err_no = 0;

	AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ALWAYS, "[ATSE]: _AT_SYSTEM_ECHO_DBG_SETTING");
	if(!arg){
		AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, "[ATSE] Usage: ATSE=<echo>,<dbg_msk>,<dbg_lv>");
		err_no = 1;
		goto exit;
	}

	argc = parse_param(arg, argv);

	if(argc < 2 || argc > 4){
		err_no = 2;
		goto exit;
	}

#if (defined(CONFIG_EXAMPLE_UART_ATCMD) && CONFIG_EXAMPLE_UART_ATCMD)
	if(argv[1] != NULL){
		echo = atoi(argv[1]);
		if(echo>1 || echo <0){
			err_no = 3;
			goto exit;
		}
		gAT_Echo = echo?1:0;
	}
#endif

	if((argc > 2) && (argv[2] != NULL)){
		mask = strtoul(argv[2], NULL, 0);
		at_set_debug_mask(mask);
	}
	
	if((argc == 4) && (argv[3] != NULL)){
		dbg_lv = strtoul(argv[3], NULL, 0);
		at_set_debug_level(dbg_lv);
	}
	
exit:
	if(err_no)
		at_printf("\r\n[ATSE] ERROR:%d", err_no);
	else
		at_printf("\r\n[ATSE] OK");
	return;
}
#if CONFIG_WLAN
extern int EraseApinfo();
extern int Erase_Fastconnect_data();

void fATSY(void *arg){
#if CONFIG_EXAMPLE_WLAN_FAST_CONNECT
	Erase_Fastconnect_data();
#endif

#if (defined(CONFIG_EXAMPLE_UART_ATCMD) && CONFIG_EXAMPLE_UART_ATCMD)
	reset_uart_atcmd_setting();
#endif

#if CONFIG_OTA_UPDATE
	// Reset ota image  signature
	cmd_ota_image(0);
#endif	
	
	at_printf("\r\n[ATSY] OK");
	// reboot
	sys_reset();
}

#if CONFIG_OTA_UPDATE
extern int wifi_is_connected_to_ap( void );
void fATSO(void *arg){
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(!arg){
		AT_DBG_MSG(AT_FLAG_OTA, AT_DBG_ERROR, "\r\n[ATSO] Usage: ATSO=<ip>,<port>");
		at_printf("\r\n[ATSO] ERROR:1");
		return;
	}
	argv[0] = "update";
	if((argc = parse_param(arg, argv)) != 3){
		AT_DBG_MSG(AT_FLAG_OTA, AT_DBG_ERROR, "\r\n[ATSO] Usage: ATSO=<ip>,<port>");
		at_printf("\r\n[ATSO] ERROR:1");
		return;
	}

	// check wifi connect first
	if(wifi_is_connected_to_ap()==0){
		cmd_update(argc, argv);
		at_printf("\r\n[ATSO] OK");
		
	}else{
		at_printf("\r\n[ATSO] ERROR:3");
	}
}

void fATSC(void *arg){
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	int cmd = 0;
	
	if(!arg){
		AT_DBG_MSG(AT_FLAG_OTA, AT_DBG_ERROR, "\r\n[ATSC] Usage: ATSC=<0/1>");
		at_printf("\r\n[ATSC] ERROR:1");
		return;
	}
	if((argc = parse_param(arg, argv)) != 2){
		AT_DBG_MSG(AT_FLAG_OTA, AT_DBG_ERROR, "\r\n[ATSC] Usage: ATSC=<0/1>");
	  at_printf("\r\n[ATSC] ERROR:1");
	  return;
	}

	cmd = atoi(argv[1]);

	if((cmd!=0)&&(cmd!=1)){
		at_printf("\r\n[ATSC] ERROR:2");
		return;
	}
		
	at_printf("\r\n[ATSC] OK");

 	if(cmd == 1){
 		cmd_ota_image(1);
 	}
	else{
		cmd_ota_image(0);
	}
	// reboot
	sys_reset();
}
#endif

#if (defined(CONFIG_EXAMPLE_UART_ATCMD) && CONFIG_EXAMPLE_UART_ATCMD)
extern const u32 log_uart_support_rate[];

void fATSU(void *arg){
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	u32 baud = 0;
	u8 databits = 0;
	u8 stopbits = 0;
	u8 parity = 0;
	u8 flowcontrol = 0;
	u8 configmode = 0;
	int i;
	UART_LOG_CONF uartconf;
	
	if(!arg){
		AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, 
		"[ATSU] Usage: ATSU=<baud>,<databits>,<stopbits>,<parity>,<flowcontrol>,<configmode>");
		at_printf("\r\n[ATSU] ERROR:1");
		return;
	}
	if((argc = parse_param(arg, argv)) != 7){
		AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ERROR, 
		"[ATSU] Usage: ATSU=<baud>,<databits>,<stopbits>,<parity>,<flowcontrol>,<configmode>");
		at_printf("\r\n[ATSU] ERROR:1");
		return;
	}

	baud = atoi(argv[1]);
	databits = atoi(argv[2]);
	stopbits = atoi(argv[3]);
	parity = atoi(argv[4]);
	flowcontrol = atoi(argv[5]);
	configmode = atoi(argv[6]);
/*
    // Check Baud rate
    for (i=0; log_uart_support_rate[i]!=0xFFFFFF; i++) {
        if (log_uart_support_rate[i] == baud) {
            break;
        }
    }
    
    if (log_uart_support_rate[i]== 0xFFFFFF) {
		at_printf("\r\n[ATSU] ERROR:2");
        return;
    }
*/
	if(((databits < 5) || (databits > 8))||\
		((stopbits < 1) || (stopbits > 2))||\
		((parity < 0) || (parity > 2))||\
		((flowcontrol < 0) || (flowcontrol > 1))||\
		((configmode < 0) || (configmode > 3))\
		){
		at_printf("\r\n[ATSU] ERROR:2");
		return;
	}
	
	memset((void*)&uartconf, 0, sizeof(UART_LOG_CONF));
	uartconf.BaudRate = baud;
	uartconf.DataBits = databits;
	uartconf.StopBits = stopbits;
	uartconf.Parity = parity;
	uartconf.FlowControl = flowcontrol;
	AT_DBG_MSG(AT_FLAG_COMMON, AT_DBG_ALWAYS, 
		"AT_UART_CONF: %d,%d,%d,%d,%d", uartconf.BaudRate, uartconf.DataBits,uartconf.StopBits,uartconf.Parity,uartconf.FlowControl);
	switch(configmode){
		case 0: // set current configuration, won't save
			uart_atcmd_reinit(&uartconf);
			break;
		case 1: // set current configuration, and save
			write_uart_atcmd_setting_to_system_data(&uartconf);
			uart_atcmd_reinit(&uartconf);
			break;
		case 2: // set configuration, reboot to take effect
			write_uart_atcmd_setting_to_system_data(&uartconf);
			break;
	}
	
	at_printf("\r\n[ATSU] OK");
}
#endif //#if (defined(CONFIG_EXAMPLE_UART_ATCMD) && CONFIG_EXAMPLE_UART_ATCMD)
#endif //#if CONFIG_WLAN

#if defined(CONFIG_PLATFORM_8710C)
struct gpio_str
{
	gpio_t gpio[24];
	u32 pinmux_manager;
};
#endif

void fATSG(void *arg)
{
#if defined(CONFIG_PLATFORM_8710C)
	static struct gpio_str *gpio_ctrl = NULL;
#else
	gpio_t gpio_ctrl;
#endif
	int argc = 0, val, error_no = 0;
	char *argv[MAX_ARGC] = {0}, port, num;
	PinName pin = NC;

	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "[ATSG]: _AT_SYSTEM_GPIO_CTRL_");

	if(!arg){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ERROR, 
		"[ATSG] Usage: ATSG=<R/W>,<PORT>,<DATA>,<DIR>,<PULL>");
		error_no = 1;
		goto exit;
	}
	if((argc = parse_param(arg, argv)) < 3){
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ERROR, 
			"[ATSG] Usage: ATSG=<R/W>,<PORT>,<DATA>,<DIR>,<PULL>");
		error_no = 2;
		goto exit;
	}	

	port = argv[2][1];
	num = strtoul(&argv[2][3], NULL, 0);
	port -= 'A';
#if defined(CONFIG_PLATFORM_8710C)
	if((0 == port) && (num <= PIN_A23)){	// If Port A, and pin num is in range
		pin = num;
	}
	else{		// If Other Port
		pin = (u8)NC;
	}
#else
	pin = (port << 4 | num);
#endif

	AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "PORT: %s[%d]", argv[2], pin);

	if(gpio_set(pin) == 0xff)
	{
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ERROR, "[ATSG]: Invalid Pin Name [%d]", pin);
		error_no = 3;
		goto exit;
	}

#if defined(CONFIG_PLATFORM_8710C)

	if(gpio_ctrl == NULL){
		gpio_ctrl = malloc(sizeof(struct gpio_str));
		if(gpio_ctrl == NULL){
			AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ERROR, "[ATSG]: Gpio malloc fail");
			error_no = 4;
			goto exit;
		}
		memset(gpio_ctrl,0,sizeof(struct gpio_str));
	}

	if((0 == port) && (num <= PIN_A23)){	// If Port A, and pin num is in range
		if (!(gpio_ctrl->pinmux_manager & (1<<num))){
			gpio_init(&gpio_ctrl->gpio[num], pin);
			gpio_ctrl->pinmux_manager |= (1<<num);
		}
	}

	if(argv[4]){
		int dir = atoi(argv[4]);
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "DIR: %s", argv[4]);
		gpio_dir(&gpio_ctrl->gpio[num], dir);
	}
	if(argv[5]){
		int pull = atoi(argv[5]);
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "PULL: %s", argv[5]);
		gpio_mode(&gpio_ctrl->gpio[num], pull);
	}
	if(argv[1][0] == 'R'){
		val = gpio_read(&gpio_ctrl->gpio[num]);
	}
	else if(argv[1][0] == 'W'){
		val = atoi(argv[3]);
		gpio_dir(&gpio_ctrl->gpio[num], PIN_OUTPUT);
		gpio_write(&gpio_ctrl->gpio[num], val);
	}

#else
	gpio_init(&gpio_ctrl, pin);

	if(argv[4]){
		int dir = atoi(argv[4]);
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "DIR: %s", argv[4]);
		gpio_dir(&gpio_ctrl, dir);
	}
	if(argv[5]){
		int pull = atoi(argv[5]);
		AT_DBG_MSG(AT_FLAG_GPIO, AT_DBG_ALWAYS, "PULL: %s", argv[5]);
		gpio_mode(&gpio_ctrl, pull);
	}
	if(argv[1][0] == 'R'){
		val = gpio_read(&gpio_ctrl);
	}
	else if(argv[1][0] == 'W'){
		val = atoi(argv[3]);
		gpio_dir(&gpio_ctrl, PIN_OUTPUT);
		gpio_write(&gpio_ctrl, val);
	}
#endif

exit:
	if(error_no){
		at_printf("\r\n[ATSG] ERROR:%d", error_no);
	}
	else{
		at_printf("\r\n[ATSG] OK:%d", val);
	}
}

#endif //#elif ATCMD_VER == ATVER_2
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
void fATSL(void *arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
#if ATCMD_VER == ATVER_2
	int err_no = 0;
#endif
	uint32_t lock_id;
#if defined(CONFIG_PLATFORM_8710C)
	uint32_t var;
	extern uint32_t sys_force_sleep_time;
#endif

	AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL]: _AT_SYS_WAKELOCK_TEST_");

	if (!arg) {
		AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] Usage ATSL=[a/r/?][bitmask]");
#if ATCMD_VER == ATVER_2
		err_no = 1;
#endif
		goto exit;
	} else {
		argc = parse_param(arg, argv);
		if (argc < 2) {
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] Usage ATSL=[a/r/?][bitmask]");
#if ATCMD_VER == ATVER_2
			err_no = 2;
#endif
			goto exit;
		}
	}

	switch(argv[1][0]) {
		case 'a': // acquire
		{
			if (argc == 3) {
				lock_id = strtoul(argv[2], NULL, 16);
				pmu_acquire_wakelock(lock_id);
			}
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] wakelock:0x%08x", pmu_get_wakelock_status());			
			break;
		}

		case 'r': // release
		{
			if (argc == 3) {
				lock_id = strtoul(argv[2], NULL, 16);
				pmu_release_wakelock(lock_id);
			}
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] wakelock:0x%08x", pmu_get_wakelock_status());
			break;
		}

		case '?': // get status
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] wakelock:0x%08x", pmu_get_wakelock_status());
#if (configGENERATE_RUN_TIME_STATS == 1)
			//pmu_get_wakelock_hold_stats((char *)cBuffer);
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "%s", cBuffer);
#endif
			break;

#if (configGENERATE_RUN_TIME_STATS == 1)
		case 'c': // clean wakelock info (for recalculate wakelock hold time)
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] clean wakelock stat");
			//pmu_clean_wakelock_stat();
			break;
#endif
#if defined(CONFIG_PLATFORM_8710C)
		case 't':
			if (argc == 3) {
				var = strtoul(argv[2], NULL, 10);
				pmu_set_sleep_type(var);
			}
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] sleep type: %d", pmu_get_sleep_type());
			break;
		case 'd':
			if (argc == 3) {
				sys_force_sleep_time = strtoul(argv[2], NULL, 10);
			}
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] sleep time: %d", sys_force_sleep_time);
			break;
		case 'l':
			if (argc == 3) {
				var = strtoul(argv[2], NULL, 10);
				pmu_tickless_debug(var);
			}
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] debug: %d", var);
			break;
		case 'b':
			if (argc == 4) {
				var = strtoul(argv[2], NULL, 10);
				pmu_set_broadcast_awake(var);
				var = strtoul(argv[3], NULL, 10);
				pmu_set_broadcast_awake_port(var);
			} else if (argc == 3) {
				var = strtoul(argv[2], NULL, 10);
				pmu_set_broadcast_arp_awake(var);
			}
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] broadcast awake: %d", pmu_get_broadcast_awake());
			break;
#endif
		default:
			AT_DBG_MSG(AT_FLAG_OS, AT_DBG_ALWAYS, "[ATSL] Usage ATSL=[a/r/?][bitmask]");
#if ATCMD_VER == ATVER_2
			err_no = 3;
#endif
			break;
	}
exit:
#if ATCMD_VER == ATVER_2
	if(err_no)
		at_printf("\r\n[ATSL] ERROR:%d", err_no);
	else
		at_printf("\r\n[ATSL] OK:0x%08x",pmu_get_wakelock_status());
#endif
	return;
}
#endif

log_item_t at_sys_items[] = {
#ifndef CONFIG_INIC_NO_FLASH
#if ATCMD_VER == ATVER_1
#if defined(CONFIG_PLATFORM_8710C)
	{"ATXX", fATXX,{NULL,NULL}},
#endif
	{"ATSD", fATSD,{NULL,NULL}},	// Dump register
	{"ATSE", fATSE,{NULL,NULL}},	// Edit register
#if defined(CONFIG_PLATFORM_8711B) || defined(CONFIG_PLATFORM_8710C)
	{"ATSK", fATSK,},   // Set RDP/RSIP enable and key(AmebaZ), secure boot (AmebaZ2)
#endif
#if CONFIG_OTA_UPDATE
	{"ATSC", fATSC,},	// Clear OTA signature
	{"ATSR", fATSR,},	// Recover OTA signature
#endif
#if defined(CONFIG_UART_YMODEM) && CONFIG_UART_YMODEM
	{"ATSY", fATSY,},	// uart ymodem upgrade
#endif
#if defined(SUPPORT_MP_MODE) && SUPPORT_MP_MODE
	{"ATSA", fATSA,},	// MP ADC test
	{"ATSG", fATSG,},	// MP GPIO test
	{"ATSP", fATSP,},	// MP Power related test
	{"ATSB", fATSB,},	// OTU PIN setup			
#if defined(CONFIG_MIIO_MP) && CONFIG_MIIO_MP
	{"ATSF", fATSF,},
#endif
#endif
#if (configGENERATE_RUN_TIME_STATS == 1)
	{"ATSS", fATSS,},	// Show CPU stats
#endif
#if SUPPORT_CP_TEST
	{"ATSM", fATSM,},	// Apple CP test
#endif
#if !defined(CONFIG_PLATFORM_8195BHP) && !defined(CONFIG_PLATFORM_8710C)
	{"ATSJ", fATSJ,{NULL,NULL}}, //trun off JTAG
#endif
	{"ATS@", fATSs,{NULL,NULL}},	// Debug message setting
	{"ATS!", fATSc,{NULL,NULL}},	// Debug config setting
	{"ATS#", fATSt,{NULL,NULL}},	// test command
	{"ATS?", fATSx,{NULL,NULL}},	// Help
#if WIFI_LOGO_CERTIFICATION_CONFIG
	{"ATSV", fATSV},				// Write SW version for wifi logo test
#endif
#elif ATCMD_VER == ATVER_2 //#if ATCMD_VER == ATVER_1
	{"AT", 	 fATS0,},	// test AT command ready
	{"ATS?", fATSh,},	// list all AT command
	{"ATSR", fATSR,},	// system restart
	{"ATSV", fATSV,},	// show version info
	{"ATSP", fATSP,},	// power saving mod
	{"ATSE", fATSE,},	// enable and disable echo
#if CONFIG_WLAN
	{"ATSY", fATSY,},	// factory reset
#if CONFIG_OTA_UPDATE
	{"ATSO", fATSO,},	// ota upgrate
	{"ATSC", fATSC,},	// chose the activited image
#endif
#if (defined(CONFIG_EXAMPLE_UART_ATCMD) && CONFIG_EXAMPLE_UART_ATCMD)
	{"ATSU", fATSU,},	// AT uart configuration
#endif
#endif
	{"ATSG", fATSG,},	// GPIO control
#endif // end of #if ATCMD_VER == ATVER_1

// Following commands exist in two versions
#if defined(configUSE_WAKELOCK_PMU) && (configUSE_WAKELOCK_PMU == 1)
	{"ATSL", fATSL,{NULL,NULL}},	 // wakelock test
#endif
#endif
};

#if ATCMD_VER == ATVER_2
void print_system_at(void *arg){
	int index;
	int cmd_len = 0;

	cmd_len = sizeof(at_sys_items)/sizeof(at_sys_items[0]);
	for(index = 0; index < cmd_len; index++)
		at_printf("\r\n%s", at_sys_items[index].log_cmd);
}
#endif
void at_sys_init(void)
{
	log_service_add_table(at_sys_items, sizeof(at_sys_items)/sizeof(at_sys_items[0]));
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_sys_init);
#endif
