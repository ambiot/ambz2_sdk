#include "FreeRTOS.h"
#include "freertos_pmu.h"
#include "rtl8710c_freertos_pmu.h"

uint32_t pmu_set_sysactive_time(uint32_t timeout_ms)
{
#if defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE > 0)
	return _pmu_set_sysactive_time(timeout_ms);
#else
	return 0;
#endif
}

void pmu_register_sleep_callback(u32 nDeviceId, PSM_HOOK_FUN sleep_hook_fun, void* sleep_param_ptr, PSM_HOOK_FUN wakeup_hook_fun, void* wakeup_param_ptr)
{
#if defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE > 0)
	_pmu_register_sleep_callback(nDeviceId, sleep_hook_fun, sleep_param_ptr, wakeup_hook_fun, wakeup_param_ptr);
#else
	;
#endif
}

void pmu_unregister_sleep_callback(u32 nDeviceId)
{
#if defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE > 0)
	_pmu_unregister_sleep_callback(nDeviceId);
#else
	;
#endif
}

uint32_t pmu_yield_os_check(void)
{
#if defined(configUSE_TICKLESS_IDLE) && (configUSE_TICKLESS_IDLE > 0)
	return _pmu_yield_os_check();
#else
	return 1;
#endif
}

