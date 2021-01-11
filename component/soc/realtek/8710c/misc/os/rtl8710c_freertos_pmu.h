#ifndef _FREERTOS_PMU_8710C_H_
#define _FREERTOS_PMU_8710C_H_



#define PMU_DEVICE_TIMER_DEFAULT_INTERVAL	2000
#define PMU_DEVICE_TIMER_MAX_INTERVAL		(1000*30) /* max 30sec */

typedef struct
{
	u32					nDeviceId;
	PSM_HOOK_FUN		sleep_hook_fun;
	void*				sleep_param_ptr;
	PSM_HOOK_FUN		wakeup_hook_fun;
	void*				wakeup_param_ptr;
}PSM_DD_HOOK_INFO;

typedef struct
{
	u32					nDeviceId;
	PSM_HOOK_FUN		late_resume_hook_fun;
	void*				late_resume_param_ptr;
}PSM_DD_DELAY_HOOK_INFO;

/**
  * @brief  init system active timer for PMU.
  * @param  none.          
  * @retval status value:
  *          - 0: _FAIL
  *          - 1: _SUCCESS
  * @note can just used in late resume or later, can not used in wakeup_hook_fun.
  */
uint32_t _pmu_yield_os_check(void);
uint32_t pmu_yield_os_check(void);
uint32_t _pmu_sysactive_timer_init(void);
uint32_t pmu_sysactive_timer_init(void);
uint32_t _pmu_set_sysactive_time(uint32_t timeout_ms);
void _pmu_register_sleep_callback(u32 nDeviceId, PSM_HOOK_FUN sleep_hook_fun, void* sleep_param_ptr, PSM_HOOK_FUN wakeup_hook_fun, void* wakeup_param_ptr);
void _pmu_unregister_sleep_callback(u32 nDeviceId);
void _pmu_register_delay_callback(u32 nDeviceId, PSM_HOOK_FUN late_resume_hook_fun, void* late_resume_param_ptr);
void pmu_register_delay_callback(u32 nDeviceId, PSM_HOOK_FUN late_resume_hook_fun, void* late_resume_param_ptr);
void _pmu_unregister_delay_callback(u32 nDeviceId);
void pmu_unregister_delay_callback(u32 nDeviceId);
u32 pmu_exec_sleep_hook_funs(void);
void pmu_exec_wakeup_hook_funs(u32 nDeviceIdMax);
uint32_t pmu_set_sleep_type(uint32_t type);
uint32_t pmu_get_sleep_type(void);
void pmu_tickless_debug(u32 NewStatus);
void pmu_set_dev_wakeup_tick(u32 nDeviceId, u32 Ms);
 extern uint32_t LWIP_Get_Dynamic_Sleep_Interval(void);
uint32_t pmu_set_wakeup_event(uint32_t event, uint32_t option);
uint32_t pmu_clear_wakeup_event(uint32_t event, uint32_t option);

extern u32 tickless_debug;
extern u32 tick_last_tcp;

/**
  * @brief  set roaming awake or not
  * @param  enable/threshhold:
  			1: enable roaming awake and set the threshhold 
  			0: disable roaming awake and ignore the threshhold
  * @retval none
  */
void pmu_set_roaming_awake(u8 enable, u8 threshhold);
void pmu_set_broadcast_awake(u32 NewStatus);
u8 pmu_get_broadcast_awake(void);
void pmu_set_broadcast_awake_port(u16 AwakePort);
u16 pmu_get_broadcast_awake_port(void);
void pmu_set_broadcast_arp_awake(u32 NewStatus);

#endif
