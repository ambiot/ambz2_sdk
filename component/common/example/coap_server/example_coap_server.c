#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "platform_stdlib.h"
#include "wifi_constants.h"
#include "wifi_conf.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <coap/config.h>
#include <coap/coap.h>
#include <coap/coap_time.h>

time_t clock_offset;

#if CONFIG_EXAMPLE_COAP_SERVER

#if !defined(LWIP_TIMEVAL_PRIVATE)||!defined(SO_REUSE)\
    ||!defined(MEMP_USE_CUSTOM_POOLS)||!defined(LWIP_IPV6)
#error ("some define missing, please check example_coap_server.c")
#endif

#define SERVER_HOST     NULL
#define SERVER_PORT     "5683"

static char response[256]= "";

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* temporary storage for dynamic resource representations */
static int quit = 0;

/* changeable clock base (see handle_put_time()) */
static time_t my_clock_base = 0;

struct coap_resource_t *time_resource = NULL;

#ifndef WITHOUT_ASYNC
/* This variable is used to mimic long-running tasks that require
 * asynchronous responses. */
static coap_async_state_t *async = NULL;
#endif /* WITHOUT_ASYNC */

/* SIGINT handler: set quit to 1 for graceful termination */
void handle_sigint(int signum)
{
    quit = 1;
}

#define INDEX "This is a test server made with libcoap (see http://libcoap.sf.net)\n" \
          "Copyright (C) 2010--2013 Olaf Bergmann <bergmann@tzi.org>\n\n"

void hnd_get_index(coap_context_t *ctx, struct coap_resource_t *resource, coap_address_t *peer,
        coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    unsigned char buf[3];

    response->hdr->code = COAP_RESPONSE_CODE(205);

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE,
            coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response, COAP_OPTION_MAXAGE, coap_encode_var_bytes(buf, 0x2ffff), buf);

    coap_add_data(response, strlen(INDEX), (unsigned char *) INDEX);
}

void hnd_get_time(coap_context_t *ctx, struct coap_resource_t *resource, coap_address_t *peer,
        coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;
    unsigned char buf[40];
    size_t len;
    time_t now;
    coap_tick_t t;
    coap_subscription_t *subscription;

    /* FIXME: return time, e.g. in human-readable by default and ticks
     * when query ?ticks is given. */

    /* if my_clock_base was deleted, we pretend to have no such resource */
    response->hdr->code = my_clock_base ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(404);

    if (request != NULL && coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter))
    {
        subscription = coap_add_observer(resource, peer, token);
        if (subscription)
        {
            subscription->non = request->hdr->type == COAP_MESSAGE_NON;
            coap_add_option(response, COAP_OPTION_OBSERVE, 0, NULL);
        }
    }
    if (resource->dirty == 1)
        coap_add_option(response, COAP_OPTION_OBSERVE, coap_encode_var_bytes(buf, ctx->observe),
                buf);

    if (my_clock_base)
        coap_add_option(response, COAP_OPTION_CONTENT_FORMAT,
                coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response, COAP_OPTION_MAXAGE, coap_encode_var_bytes(buf, 0x01), buf);

    if (my_clock_base)
    {

        /* calculate current time */
        coap_ticks(&t);
        now = my_clock_base + (t / COAP_TICKS_PER_SECOND);

        if (request != NULL
                && (option = coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter))
                && memcmp(COAP_OPT_VALUE(option), "ticks", min(5, COAP_OPT_LENGTH(option))) == 0)
        {
            /* output ticks */
            len = snprintf((char *) buf, min(sizeof(buf), response->max_size - response->length),
                    "%u", (unsigned int) now);
            coap_add_data(response, len, buf);

        }
        else
        { /* output human-readable time */
            struct tm *tmp;
            tmp = gmtime(&now);
            len = strftime((char *) buf, min(sizeof(buf), response->max_size - response->length),
                    "%b %d %H:%M:%S", tmp);
            coap_add_data(response, len, buf);
        }
    }
}

void hnd_put_time(coap_context_t *ctx, struct coap_resource_t *resource, coap_address_t *peer,
        coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    coap_tick_t t;
    size_t size;
    unsigned char *data;

    /* FIXME: re-set my_clock_base to clock_offset if my_clock_base == 0
     * and request is empty. When not empty, set to value in request payload
     * (insist on query ?ticks). Return Created or Ok.
     */

    /* if my_clock_base was deleted, we pretend to have no such resource */
    response->hdr->code = my_clock_base ? COAP_RESPONSE_CODE(204) : COAP_RESPONSE_CODE(201);

    resource->dirty = 1;

    coap_get_data(request, &size, &data);

    if (size == 0) /* re-init */
        my_clock_base = clock_offset;
    else
    {
        my_clock_base = 0;
        coap_ticks(&t);
        while (size--)
            my_clock_base = my_clock_base * 10 + *data++;
        my_clock_base -= t / COAP_TICKS_PER_SECOND;
    }
}

void hnd_delete_time(coap_context_t *ctx, struct coap_resource_t *resource, coap_address_t *peer,
        coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    my_clock_base = 0; /* mark clock as "deleted" */

    /* type = request->hdr->type == COAP_MESSAGE_CON  */
    /*   ? COAP_MESSAGE_ACK : COAP_MESSAGE_NON; */
}

#ifndef WITHOUT_ASYNC
void hnd_get_async(coap_context_t *ctx, struct coap_resource_t *resource, coap_address_t *peer,
        coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;
    unsigned long delay = 5;
    size_t size;

    if (async)
    {
        if (async->id != request->hdr->id)
        {
            coap_opt_filter_t f;
            coap_option_filter_clear(f);
            response->hdr->code = COAP_RESPONSE_CODE(503);
        }
        return;
    }

    option = coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter);
    if (option)
    {
        unsigned char *p = COAP_OPT_VALUE(option);

        delay = 0;
        for (size = COAP_OPT_LENGTH(option); size; --size, ++p)
            delay = delay * 10 + (*p - '0');
    }

    async = coap_register_async(ctx, peer, request, COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM,
            (void *) (COAP_TICKS_PER_SECOND * delay));
}

void check_async(coap_context_t *ctx, coap_tick_t now)
{
    coap_pdu_t *response;
    coap_async_state_t *tmp;

    size_t size = sizeof(coap_hdr_t) + 8;

    if (!async || now < async->created + (unsigned long) async->appdata)
        return;

    response = coap_pdu_init(async->flags & COAP_ASYNC_CONFIRM ? COAP_MESSAGE_CON : COAP_MESSAGE_NON,
    COAP_RESPONSE_CODE(205), 0, size);
    if (!response)
    {
        debug("check_async: insufficient memory, we'll try later\n");
        async->appdata = (void *) ((unsigned long) async->appdata + 15 * COAP_TICKS_PER_SECOND);
        return;
    }

    response->hdr->id = coap_new_message_id(ctx);

    if (async->tokenlen)
        coap_add_token(response, async->tokenlen, async->token);

    coap_add_data(response, 4, (unsigned char *) "done");

    if (coap_send(ctx, &async->peer, response) == COAP_INVALID_TID)
    {
        debug("check_async: cannot send response for message %d\n", response->hdr->id);
    }
    coap_delete_pdu(response);
    coap_remove_async(ctx, async->id, &tmp);
    coap_free_async(async);
    async = NULL;
}
#endif /* WITHOUT_ASYNC */

void init_resources(coap_context_t *ctx)
{
    coap_resource_t *r;

    r = coap_resource_init(NULL, 0, 0);
    coap_register_handler(r, COAP_REQUEST_GET, hnd_get_index);

    coap_add_attr(r, (unsigned char *) "ct", 2, (unsigned char *) "0", 1, 0);
    coap_add_attr(r, (unsigned char *) "title", 5, (unsigned char *) "\"General Info\"", 14, 0);
    coap_add_resource(ctx, r);

    /* store clock base to use in /time */
    my_clock_base = clock_offset;

    r = coap_resource_init((unsigned char *) "time", 4, 0);
    coap_register_handler(r, COAP_REQUEST_GET, hnd_get_time);
    coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_time);
    coap_register_handler(r, COAP_REQUEST_DELETE, hnd_delete_time);

    coap_add_attr(r, (unsigned char *) "ct", 2, (unsigned char *) "0", 1, 0);
    coap_add_attr(r, (unsigned char *) "title", 5, (unsigned char *) "\"Internal Clock\"", 16, 0);
    coap_add_attr(r, (unsigned char *) "rt", 2, (unsigned char *) "\"Ticks\"", 7, 0);
    r->observable = 1;
    coap_add_attr(r, (unsigned char *) "if", 2, (unsigned char *) "\"clock\"", 7, 0);

    coap_add_resource(ctx, r);
    time_resource = r;

#ifndef WITHOUT_ASYNC
    r = coap_resource_init((unsigned char *) "async", 5, 0);
    coap_register_handler(r, COAP_REQUEST_GET, hnd_get_async);

    coap_add_attr(r, (unsigned char *) "ct", 2, (unsigned char *) "0", 1, 0);
    coap_add_resource(ctx, r);
#endif /* WITHOUT_ASYNC */
}

static coap_context_t *
get_context(const char *node, const char *port)
{
    coap_context_t *ctx = NULL;
    int s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    s = getaddrinfo(node, port, &hints, &result);
    if (s != 0)
    {
        printf("\n getaddrinfo failed.\n");
        return NULL;
    }

    /* iterate through results until success */
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        coap_address_t addr;
        coap_address_init(&addr);
        addr.port = atoi(port);
        
        ctx = coap_new_context(&addr);
        if (ctx)
        {
          /* TODO: output address:port for successful binding */
          goto finish;
        } 
    }

    printf("no context available for interface '%s'\n", node);

    finish: freeaddrinfo(result);
    return ctx;
}


static void example_coap_server_thread(void *para){
  while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS){
    printf("Wait for WIFI connection ...\n");
    vTaskDelay(1000);
  }
  printf("\nCoAP Server Example starts...\n");
  
  coap_context_t *ctx;
  struct timeval tv, *timeout;
  coap_tick_t now,obs_start,obs_wait;
  coap_queue_t *nextpdu;
  coap_log_t log_level = LOG_WARNING;
  
  coap_set_log_level(log_level);
  
  clock_offset = xTaskGetTickCount();
    
  ctx = get_context(SERVER_HOST, SERVER_PORT);
  if (!ctx)
  {
    printf("\n [COAP_SERVER]get_context failed \n");
    goto exit;
  }
  
  init_resources(ctx);
  
  coap_ticks(&obs_start);
  while (!quit)
  {    
    nextpdu = coap_peek_next(ctx);
    
    coap_ticks(&now);
    while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime)
    {
      coap_retransmit(ctx, coap_pop_next(ctx));
      nextpdu = coap_peek_next(ctx);
    }

    if (nextpdu && nextpdu->t <= COAP_RESOURCE_CHECK_TIME)
    {
      /* set timeout if there is a pdu to send before our automatic timeout occurs */
      tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
      tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
      timeout = &tv;
    }
    else
    {
      tv.tv_usec = 0;
      tv.tv_sec = COAP_RESOURCE_CHECK_TIME;
      timeout = &tv;
    }
    
    coap_dispatch(ctx, (const char*)&response); /* and dispatch PDUs from receivequeue */

    coap_ticks(&obs_wait);
    if((obs_wait-obs_start)/COAP_TICKS_PER_SECOND > (timeout->tv_sec + timeout->tv_usec/10000)){
        coap_ticks(&obs_start);
        if (time_resource)
        {
            time_resource->dirty = 1;
        }
    }
#ifndef WITHOUT_ASYNC
    /* check if we have to send asynchronous responses */
    check_async(ctx, now);
#endif /* WITHOUT_ASYNC */
    
#ifndef WITHOUT_OBSERVE
    /* check if we have to send observe notifications */
    coap_check_notify(ctx);
#endif /* WITHOUT_OBSERVE */
  }
  
exit:
  coap_free_context(ctx); 
  printf("\nCoAP Server Example end.\n");
  
  vTaskDelete(NULL);
}

void example_coap_server(void)
{
  if(xTaskCreate(example_coap_server_thread, ((const char*)"example_coap_server_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
}

#endif