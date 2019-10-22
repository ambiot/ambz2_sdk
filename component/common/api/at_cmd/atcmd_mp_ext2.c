#include <platform_stdlib.h>
#include <platform_opts.h>
#include <gpio_api.h>
#include "log_service.h"
#include "osdep_service.h"
#include "atcmd_mp.h"
#ifdef CONFIG_PLATFORM_8710C
#include "rtl8710c_pin_name.h"
#endif

#if CONFIG_ATCMD_MP_EXT2

#define MP_EXT2_PREFIX		"[ATM2]: "
#define MP_EXT2_PRINTF(...) \
		do{ \
			_AT_PRINTK(MP_EXT2_PREFIX); \
			_AT_PRINTK(__VA_ARGS__); \
		}while(0)

#define UART_BRIDGE_USAGE	"ATM2=bridge\n"
#define BT_POWER_USAGE		"ATM2=bt_power,ACT <ACT: on/off>\n"
#define GNT_BT_USAGE		"ATM2=gnt_bt,TARGET <TARGET: wifi/bt>\n"
			

#ifdef CONFIG_PLATFORM_8710C
#define UART_BRIDGE_TX    			PIN_UART3_TX
#define UART_BRIDGE_RX    			PIN_UART3_RX
#endif

#ifdef CONFIG_PLATFORM_8721D
#define UART_BRIDGE_TX    			_PA_7//Add TX for further use
#define UART_BRIDGE_RX    			_PA_8//Add RX for further use
#endif

extern void bt_uart_bridge_set(uint32_t baudrate_0, uint32_t baudrate_1, uint8_t parity, uint8_t debug);
extern void bt_uart_bridge_close(void);
extern void bt_uart_bridge_open(PinName tx, PinName rx);
extern void console_reinit_uart(void);
extern bool bte_init(void);
extern void bte_deinit(void);
extern bool bt_trace_init(void);
extern bool bt_trace_uninit(void);


static int mp_ext2_uart_bridge(void **argv, int argc)
{
	return 0;
}

static int mp_ext2_bt_power(void **argv, int argc)
{
	return 0;
}

static int mp_ext2_gnt_bt(void **argv, int argc)
{
	(void)argc;
	if(strcmp(argv[0], "wifi" ) == 0){
		MP_EXT2_PRINTF("Switch GNT_BT to WIFI.\n\r");
		
	}else if(strcmp(argv[0], "bt" ) == 0){
		MP_EXT2_PRINTF("Switch GNT_BT to BT.\n\r");
	}
	return 0;
}

at_mp_ext_item_t at_mp_ext2_items[] = {
	{"bridge",		mp_ext2_uart_bridge,		UART_BRIDGE_USAGE},
	{"bt_power",	mp_ext2_bt_power,			BT_POWER_USAGE},
	{"gnt_bt",		mp_ext2_gnt_bt,				GNT_BT_USAGE},
};


void fATM2(void *arg)
{
	int argc = 0, idx, cmd_cnt;
	char *argv[MAX_ARGC] = {0};
	
	cmd_cnt = sizeof(at_mp_ext2_items)/sizeof(at_mp_ext2_items[0]);
	argc = parse_param(arg, argv);
	if(argc == 1){
		_AT_PRINTK("\n");
		MP_EXT2_PRINTF("Command usage :\n");
		for(idx = 0; idx < cmd_cnt; idx++)
			_AT_PRINTK("%s", at_mp_ext2_items[idx].mp_ext_usage);
	}else{
		for(idx = 0; idx < cmd_cnt; idx++){
			if(strcmp(argv[1], at_mp_ext2_items[idx].mp_ext_cmd) == 0){
				int (*fun)(void **argv, int argc) = at_mp_ext2_items[idx].mp_ext_fun;
				fun((void**)&argv[2], argc-2);
				return;
			}
		}
		MP_EXT2_PRINTF("unknown command %s.\n", argv[1]);
	}
}

#endif //#if CONFIG_ATCMD_MP_EXT2
