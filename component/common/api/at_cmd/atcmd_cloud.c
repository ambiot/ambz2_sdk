#include <stdio.h>
#include "log_service.h"
#include "cmsis_os.h"
#include <platform/platform_stdlib.h>

#if CONFIG_JOYLINK
void fATCJ(void *arg)
{
	extern void joylink_erase(void);
	printf("\r\n[ATCJ] Erase wifi and joylink info.");
	if(arg){
		printf("\r\n[ATCJ]Usage : ATCJ");
		return;
	}        
	joylink_erase();
}
#endif

#if CONFIG_GAGENT
void fATCG(void *arg)
{
	example_gagent();
}
void fATCE(void *arg)
{//Erase gagent config flash
	extern int GAgent_DevEraseConfigData();
	GAgent_DevEraseConfigData();
}
#endif

#if CONFIG_QQ_LINK
void fATCQ(void *arg)
{	
	int argc;
	unsigned char *argv[MAX_ARGC] = {0};
	extern void device_write_sn_license(int argc, unsigned char **argv);
	extern void device_erase_all(int argc, unsigned char **argv);
	
	if(!arg)
	{
		printf("\r\n[ATCQ] Write sn/license into flash or Erase all info\r\n");
		printf("\r\n[ATCQ] Usage: ATCQ=erase");
		printf("\r\n[ATCQ] Usage: ATCQ=sn,xxxxxxxx\r\n             ATCQ=licensepart1,xxxxxxxx\r\n             ATCQ=licensepart2,xxxxxxxx");
		return;
	}        
	argv[0] = "sn&&license&&erase";
	argc = parse_param(arg, argv);
	if(argc == 3)		// Write sn&&license
	{
		device_write_sn_license(argc, argv);
	}
	else if(argc == 2)		// Erase all info : ATCQ=erase
	{
		device_erase_all(argc, argv);
	}
	else
	{
		printf("\r\n[ATCQ] Usage: ATCQ=erase");
		printf("\r\n[ATCQ]Usage: ATCQ=sn,xxxxxxxx\r\n             ATCQ=licensepart1,xxxxxxxx\r\n             ATCQ=licensepart2,xxxxxxxx");
	}
}
#endif

#if CONFIG_AIRKISS_CLOUD
void fATCW(void *arg)
{	
	int argc;
	unsigned char *argv[MAX_ARGC] = {0};
	extern void airkiss_cloud_write_device_info(int argc, unsigned char **argv);
	extern void airkiss_cloud_erase_ap_profile(int argc, unsigned char **argv);
	
	if(!arg) goto USAGE;
	
	argv[0] = "type/id/licese/erase";
	argc = parse_param(arg, argv);
	if(argc == 3)			// Write typw/id/license
	{
		airkiss_cloud_write_device_info(argc, argv);
		return;
	}
	else if(argc == 2)		// Erase wifi profile : ATCW=erase
	{
		airkiss_cloud_erase_ap_profile(argc, argv);
		return;
	}
	else
		goto USAGE;

USAGE:	
	printf("\r\n[ATCW] Write ORDERLY device's type/id/license into flash or Erase wifi profile");
	printf("\r\n[ATCW] Usage: ATCW=type,xxxxxxxx");
	printf("\r\n[ATCW] Usage: ATCW=id,xxxxxxxx");
	printf("\r\n[ATCW] Usage: ATCW=licensepart1,xxxxxxxx\t(80-Byte long)");
	printf("\r\n[ATCW] Usage: ATCW=licensepart2,xxxxxxxx\t(80-Byte long)");
	printf("\r\n[ATCW] Usage: ATCW=erase");
	return;	
}
#endif

#if CONFIG_ALINK
extern int alink_erase_wifi_config();
extern void alink_reset_to_factory(void *arg);
extern int alink_write_device_info(int argc, unsigned char **argv);
extern void alink_set_log_level(int argc, unsigned char **argv);

void fATCA(void *arg)
{
	int argc;
	int ret = -1;
	unsigned char *argv[MAX_ARGC] = {0};
	
	if(!arg) { 
		goto USAGE;
	}
	
	argv[0] = "k/s/l/e/r";
	argc = parse_param(arg, argv);
	if (argc == 3) {
		/*Set log level: ATCA=l, trace/debug/info/warn/fatal/none
		if set wrong, set info level by default*/
		if (!strcmp(argv[1], "l")) {
			alink_set_log_level(argc, argv);
			return;
		} else {
		/* Write device key/device secret */
			ret = alink_write_device_info(argc, argv);
			if (ret) {
				goto USAGE;
			} 
			return;
		}
	} else if (argc == 2)	{
	/* Erase wifi profile or reset user account binding : "ATCA=e" or "ATCA=r" */
		if (!strcmp(argv[1], "e")) {
			alink_erase_wifi_config();
		}else if(!strcmp(argv[1], "r")) {
			alink_reset_to_factory(NULL);
		}
	}

USAGE:	
	printf("\r\n[ATCA] Write ORDERLY device's key/secret into flash, set log level or erase wifi profile");
	printf("\r\n[ATCA] Usage: ATCA=k,xxxxxxxx\t(set device key, 20-Byte long)");
	printf("\r\n[ATCA] Usage: ATCA=s,xxxxxxxx\t(set device secret, 32-Byte long)");
	printf("\r\n[ATCA] Usage: ATCA=l,xxxx\t(set log level, trace/debug/info/warn/error/fatal/none)");
	printf("\r\n[ATCA] Usage: ATCA=e\t(erase AP  profile)");
	printf("\r\n[ATCA] Usage: ATCA=r\t(reset user account binding)");
	return;	
}
#endif

#if (defined(CONFIG_MIIO) && CONFIG_MIIO)
#include "arch_psm.h"

void fATCM(void *arg)
{
	int argc;
	unsigned char len1, len2;
	unsigned char *argv[MAX_ARGC] = {0};
	
	argv[0] = "miio_triple";
	argc = parse_param(arg, argv);
	if (argc == 3) {
		arch_psm_set_str(argv[0], "did", argv[1]);
		arch_psm_set_str(argv[0], "psk", argv[2]);
	}else if(argc == 2 && !strcmp("erase", argv[1])) { 
		arch_psm_erase_key(argv[0], "did");
		arch_psm_erase_key(argv[0], "psk");
	}else {
		goto USAGE;
	}
	return;

USAGE:	
	printf("\r\n[ATCM] Write miio did&&psk");
	printf("\r\n[ATCM] Usage: ATCM=did,psk\n");
	printf("\r\n[ATCM] Usage: ATCM=erase\n");
	return;	
}
#endif

#if (defined(CONFIG_RSC) && CONFIG_RSC)
#include "rsc/rsc_device.h"
#include "rsc/rsc_dev_ota.h"
void fATCR(void *arg)
{
	int argc;
	unsigned char len1, len2;
	unsigned char *argv[MAX_ARGC] = {0};
	rsc_msg_v v = {0};
	
	argv[0] = "rscloud";
	argc = parse_param(arg, argv);
	if(argc == 2 && !strcmp("ota", argv[1])) {
		v.msg_t = MSG_OTA_REQ;
		v.loop = 0;
		rsc_dev_msg_dump((void *)&v);
	}else if(argc == 3 && !strcmp("ota", argv[1]) && (!strcmp("0", argv[2]) || !strcmp("1", argv[2]))) {
		v.msg_t = MSG_OTA_REQ;
		v.loop = *argv[2] - '0';
		rsc_dev_msg_dump((void *)&v);
	}else if(argc == 3 && !strcmp("ota", argv[1]) && !strcmp("s", argv[2])) {
		v.msg_t = MSG_OTA_STP;
		rsc_dev_msg_dump((void *)&v);	
	}else if(argc == 4 && !strcmp("ota", argv[1]) && (!strcmp("0", argv[2]) || !strcmp("1", argv[2]))) {
		v.msg_t = MSG_OTA_REQ;
		v.loop = *argv[2] - '0';
		if(strlen(argv[3]) > sizeof(v.nv_num) -1) {
			printf("\r\n[ATCR] error: the new version num should no longer than %d !", sizeof(v.nv_num) - 1);
		}else {
			strcpy(v.nv_num, argv[3]);
			rsc_dev_msg_dump((void *)&v);
		}
	}else {
		goto USAGE;
	}
	return;

USAGE:	
	printf("\r\n[ATCR] Control ameba operation to RSCloud");
	printf("\r\n[ATCR] Usage: ATCR=ota[,loop]  ==>ota with default expected fw version num for one time [or one(loop:0) / infinite(loop:1) time(s)]");
	printf("\r\n[ATCR] Usage: ATCR=ota,loop,version  ==>ota with specified fw version num(version) for one(loop:0) or  infinite(loop:1) time(s)");
	printf("\r\n[ATCR] Usage: ATCR=ota,s  ==>stop the infinite ota if it's started before");
	return;
}
extern int rsc_set_userid(char *);
extern int rsc_set_server_ip(char *);
extern void  rsc_flash_erase_sector(uint32_t address);

void fATUS(void *arg)
{
	int argc;
	char *argv[MAX_ARGC] = {0};
	
	argv[0] = "rscloud";
	argc = parse_param(arg,argv);
	if(argc == 3){
		if(!strcmp("userid",argv[1]))
		{
			rsc_set_userid(argv[2]);	
		}else if(!strcmp("svrip",argv[1])){
			rsc_set_server_ip(argv[2]);
		}
	}else if(argc == 2){
          if(!strcmp("erase",argv[1])){
            rsc_flash_erase_sector(CONFIG_USER_INFO);
          }
    }else{
		goto USAGE;
	}
	return;
USAGE:	
	printf("\r\n[ATUS] Setup User and Server info RSCloud");
	printf("\r\n[ATUS] Usage: ATUS=userid,xxxx  ==>set registed user id");
	printf("\r\n[ATUS] Usage: ATUS=svrip,xxx.xxx.xxx.xxx ==>set rscloud server ip address");
	printf("\r\n[ATUS] Usage: ATUS=erase ==>erase user id and server ip address");
	return;		
}
#endif

void fATCx(void *arg)
{	
}

log_item_t at_cloud_items[ ] = {
#if CONFIG_JOYLINK
	{"ATCJ", fATCJ,},
#endif
#if CONFIG_GAGENT
	{"ATCG", fATCG,},
	{"ATCE", fATCE,},
#endif	
#if CONFIG_QQ_LINK
	{"ATCQ", fATCQ,},
#endif
#if CONFIG_AIRKISS_CLOUD
	{"ATCW", fATCW},
#endif
#if CONFIG_ALINK
	{"ATCA", fATCA,},
#endif
#if (defined(CONFIG_MIIO) && CONFIG_MIIO)
	{"ATCM", fATCM},
#endif

#if (defined(CONFIG_RSC) && CONFIG_RSC)
	{"ATCR", fATCR,},
    {"ATUS", fATUS,},
#endif

	{"ATC?", fATCx,},
};
void at_cloud_init(void)
{
	log_service_add_table(at_cloud_items, sizeof(at_cloud_items)/sizeof(at_cloud_items[0]));
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_cloud_init);
#endif
