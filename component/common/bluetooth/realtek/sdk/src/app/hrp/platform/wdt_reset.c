#include "wdt_reset.h"
#ifdef AMEBAD_BOARD
#include "ameba_soc.h"
#include "rtl8721d.h"
#endif
#ifdef PLATFORM_STO
#include <btltp_uart.h>
#endif
#ifdef CONFIG_PLATFORM_8710C
#include "ota_8710c.h"
#endif

void wdt_reset(void)
{
#ifdef PLATFORM_STO   /* stollman only */
    SCB->AIRCR = 0x05FA0000 | SCB_AIRCR_SYSRESETREQ;
#endif

#ifdef AMEBAD_BOARD
    u32 Temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_BOOT_CFG);
    Temp |= BIT_SOC_BOOT_PATCH_KM4_RUN;
    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_BOOT_CFG, Temp);
    NVIC_SystemReset();
#endif
#ifdef CONFIG_PLATFORM_8710C
    ota_platform_reset(); 
#endif
}
