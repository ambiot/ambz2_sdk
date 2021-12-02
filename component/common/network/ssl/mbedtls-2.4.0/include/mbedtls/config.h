#if !defined (CONFIG_PLATFORM_8721D)
#define CONFIG_SSL_RSA          1
#endif

#include "rom_ssl_ram_map.h"
#include "platform_opts.h"
#define RTL_HW_CRYPTO
//#define SUPPORT_HW_SW_CRYPTO

#if (defined(CONFIG_MIIO)&&(CONFIG_MIIO))
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ECP_C
#define MBEDTLS_ENTROPY_C
#endif

#if defined(CONFIG_EXAMPLE_MBEDTLS_ECDHE) && (CONFIG_EXAMPLE_MBEDTLS_ECDHE == 1)
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_CTR_DRBG_C
#endif

#if defined(CONFIG_PLATFORM_8710C)
//#define SUPPORT_HW_SSL_HMAC_SHA256
#endif

/* RTL_CRYPTO_FRAGMENT should be less than 16000, and should be 16bytes-aligned */
#if defined (CONFIG_PLATFORM_8195A)
#define RTL_CRYPTO_FRAGMENT                4096
#else
#define RTL_CRYPTO_FRAGMENT               15360
#endif

#if defined(CONFIG_SSL_ROM)
#include <section_config.h>
#include "platform_stdlib.h"
#include "mbedtls/config_rom.h"
#define SUPPORT_HW_SW_CRYPTO
#elif defined(CONFIG_BAIDU_DUER) && CONFIG_BAIDU_DUER
#define CONFIG_SSL_RSA          0
#include "baidu_ca_mbedtls_config.h"
#elif defined(CONFIG_SSL_RSA) && CONFIG_SSL_RSA
#if defined(ENABLE_AMAZON_COMMON)
#include "platform_stdlib.h"
#include "mbedtls/config_rsa_amazon.h"
#elif (defined(CONFIG_EXAMPLE_FFS) && CONFIG_EXAMPLE_FFS)
#include "platform_stdlib.h"
#include "mbedtls/config_rsa_amazon.h"
#else
#include "platform_stdlib.h"
#include "mbedtls/config_rsa.h"
#endif
#else
#include "platform_stdlib.h"
#include "mbedtls/config_all.h"
#endif
