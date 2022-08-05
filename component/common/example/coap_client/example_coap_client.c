#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "platform_stdlib.h"
#include "wifi_constants.h"
#include "wifi_conf.h"
#include <ctype.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <coap/coap.h>

#if CONFIG_EXAMPLE_COAP_CLIENT

#if !defined(LWIP_TIMEVAL_PRIVATE)||!defined(SO_REUSE)\
    ||!defined(MEMP_USE_CUSTOM_POOLS)||!defined(LWIP_IPV6)
#error ("some define missing, please check example_coap_client.c")
#endif
      
char SERVER_HOST[20] =  "coap://coap.me/hello";
#define SERVER_PORT     "5683"

/*4 method types: GET|PUT|POST|DELETE*/
#define CMD		"GET"
#define PAYLOAD		""
#define TOKEN           "" /*0 to 8 bytes*/

/*block size to be used in GET/PUT/POST requests
COAP_BLOCK_SIZE follow the format of "[num,]size"
block size must be the power of 2 between 16 and 1024
If num is present, the request chain will start at block num*/
#define COAP_BLOCK              0
#if COAP_BLOCK
#define COAP_BLOCK_SIZE         ""
#endif /*COAP_BLOCK*/

/*subscribe for given duration in seconds*/
#define COAP_SUBSCRIBE          0
#if COAP_SUBSCRIBE
#define SUBSCRIBE_TIME          ""
#endif

/*accepted media types as comma-separated list of symbolic or numeric values*/
#define COAP_MEDIA_TYPE         0
#if COAP_MEDIA_TYPE
#define COAP_MEDIA_TYPE_ITEM    ""
#endif

/*content type for given resource for PUT/POST as comma-separated list of symbolic or numeric values*/
#define COAP_CONTENT_TYPE       0
#if COAP_CONTENT_TYPE
#define COAP_CONTENT_TYPE_ITEM  ""
#endif

/*COAP_OPTION_CONTENT follw the format of "num,text"*/
#define COAP_ADD_OPTION         0
#if COAP_ADD_OPTION
#define COAP_ADD_OPTION_CONTENT ""
#endif

/*COAP_PROXY_URI follow the format of "addr[:port]"*/
#define COAP_PROXY              0
#if COAP_PROXY
#define COAP_PROXY_URI          ""
#endif 

#define COAP_MESSAGE_NONCON     0       /*send NON-confirmable message*/
#define COAP_DELETE_LIST        0

static char response[256]= "";
int flags = 0;

unsigned char _token_data[8];
str the_token =
{ 0, _token_data };

#define FLAGS_BLOCK 0x01

static coap_list_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
static str proxy =
{ 0, NULL };

/* reading is done when this flag is set */
static int ready = 0;

static str payload =
{ 0, NULL }; /* optional payload to send */

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

typedef unsigned char method_t;
method_t method = 1; /* the method we are using in our requests */

coap_block_t block =
{ .num = 0, .m = 0, .szx = 6 };

unsigned int wait_seconds = 90; /* default timeout in seconds */
coap_tick_t max_wait; /* global timeout (changed by set_timeout()) */

unsigned int obs_seconds = 30; /* default observe time */
coap_tick_t obs_wait = 0; /* timeout for current subscription */

#define min(a,b) ((a) < (b) ? (a) : (b))

static inline void set_timeout(coap_tick_t *timer, const unsigned int seconds)
{
    coap_ticks(timer);
    *timer += seconds * COAP_TICKS_PER_SECOND;
}

coap_pdu_t *
new_ack(coap_context_t *ctx, coap_queue_t *node)
{
    coap_pdu_t *pdu = coap_new_pdu();

    if (pdu)
    {
        pdu->hdr->type = COAP_MESSAGE_ACK;
        pdu->hdr->code = 0;
        pdu->hdr->id = node->pdu->hdr->id;
    }

    return pdu;
}

coap_pdu_t *
new_response(coap_context_t *ctx, coap_queue_t *node, unsigned int code)
{
    coap_pdu_t *pdu = new_ack(ctx, node);

    if (pdu)
        pdu->hdr->code = code;

    return pdu;
}

coap_pdu_t *
coap_new_request(coap_context_t *ctx, method_t m, coap_list_t *options)
{
    coap_pdu_t *pdu;
    coap_list_t *opt;

    if (!(pdu = coap_new_pdu()))
        return NULL;

    pdu->hdr->type = msgtype;
    pdu->hdr->id = coap_new_message_id(ctx);
    pdu->hdr->code = m;

    pdu->hdr->token_length = the_token.length;
    if (!coap_add_token(pdu, the_token.length, the_token.s))
    {
        debug("cannot add token to request\n");
        return NULL;
    }

    coap_show_pdu(pdu);
    
    for (opt = options; opt; opt = opt->next)
    {
        coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *)opt->data),
                COAP_OPTION_LENGTH(*(coap_option *)opt->data),
                COAP_OPTION_DATA(*(coap_option *)opt->data));
    }
    
    if (payload.length)
    {
        if ((flags & FLAGS_BLOCK) == 0)
            coap_add_data(pdu, payload.length, payload.s);
        else
            coap_add_block(pdu, payload.length, payload.s, block.num, block.szx);
    }

    return pdu;
}

coap_tid_t clear_obs(coap_context_t *ctx, const coap_address_t *remote)
{
    coap_list_t *option;
    coap_pdu_t *pdu;
    coap_tid_t tid = COAP_INVALID_TID;

    /* create bare PDU w/o any option  */
    pdu = coap_new_request(ctx, COAP_REQUEST_GET, NULL);

    if (pdu)
    {
        /* FIXME: add token */
        /* add URI components from optlist */
        for (option = optlist; option; option = option->next)
        {
            switch (COAP_OPTION_KEY(*(coap_option *)option->data))
            {
                case COAP_OPTION_URI_HOST:
                case COAP_OPTION_URI_PORT:
                case COAP_OPTION_URI_PATH:
                case COAP_OPTION_URI_QUERY:
                    coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *)option->data),
                            COAP_OPTION_LENGTH(*(coap_option *)option->data),
                            COAP_OPTION_DATA(*(coap_option *)option->data));
                    break;
                default:
                    ; /* skip other options */
            }
        }

        if (pdu->hdr->type == COAP_MESSAGE_CON)
            tid = coap_send_confirmed(ctx, remote, pdu);
        else
            tid = coap_send(ctx, remote, pdu);

        if (tid == COAP_INVALID_TID)
        {
            debug("clear_obs: error sending new request");
            coap_delete_pdu(pdu);
        }
        else if (pdu->hdr->type != COAP_MESSAGE_CON)
            coap_delete_pdu(pdu);
    }
    return tid;
}

int resolve_address(const str *server, ip_addr_t *dst)
{
    struct addrinfo *res, *ainfo;
    struct addrinfo hints;
    static char addrstr[256];
    int error, len = -1;
    struct sockaddr_in* sock_addr;
    struct sockaddr_in6* sock_addr6;
    
    memset(addrstr, 0, sizeof(addrstr));
    if (server->length)
        memcpy(addrstr, server->s, server->length);
    else
        memcpy(addrstr, "localhost", 9);

    memset((char *) &hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    error = getaddrinfo(addrstr, NULL, &hints, &res);

    if (error != 0)
    {
        printf("\n getaddrinfo failed.\n");
        return error;
    }

    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next)
    {
        switch (ainfo->ai_family)
        {
            case AF_INET:
                len = ainfo->ai_addrlen;
                dst->type = IPADDR_TYPE_V4;
                sock_addr = (struct sockaddr_in*) (ainfo->ai_addr);
                dst->u_addr.ip4.addr= sock_addr->sin_addr.s_addr;
                goto finish;
            case AF_INET6:
                len = ainfo->ai_addrlen;
                dst->type = IPADDR_TYPE_V6;
                sock_addr6 = (struct sockaddr_in6*) (ainfo->ai_addr);
                memcpy((u32_t*)dst->u_addr.ip6.addr,(u32_t*)sock_addr6->sin6_addr.un.u32_addr,4);
                goto finish;
            default:
                ;
        } 
    }

    finish: freeaddrinfo(res);
    return len;
}

static inline coap_opt_t *
get_block(coap_pdu_t *pdu, coap_opt_iterator_t *opt_iter)
{
    coap_opt_filter_t f;

    assert(pdu);

    memset(f, 0, sizeof(coap_opt_filter_t));
    coap_option_setb(f, COAP_OPTION_BLOCK1);
    coap_option_setb(f, COAP_OPTION_BLOCK2);

    coap_option_iterator_init(pdu, opt_iter, f);
    return coap_option_next(opt_iter);
}

#define HANDLE_BLOCK1(Pdu)                      \
  ((method == COAP_REQUEST_PUT || method == COAP_REQUEST_POST) &&   \
   ((flags & FLAGS_BLOCK) == 0) &&                  \
   ((Pdu)->hdr->code == COAP_RESPONSE_CODE(201) ||          \
    (Pdu)->hdr->code == COAP_RESPONSE_CODE(204)))

inline int check_token(coap_pdu_t *received)
{
    return received->hdr->token_length == the_token.length
            && memcmp(received->hdr->token, the_token.s, the_token.length) == 0;
}

void message_handler(struct coap_context_t *ctx, const coap_address_t *remote, coap_pdu_t *sent,
        coap_pdu_t *received, const coap_tid_t id)
{

    coap_pdu_t *pdu = NULL;
    coap_opt_t *block_opt;
    coap_opt_iterator_t opt_iter;
    unsigned char buf[4];
    coap_list_t *option;
    size_t len;
    unsigned char *databuf;
    coap_tid_t tid;

#ifndef NDEBUG
    if (LOG_DEBUG <= coap_get_log_level())
    {
        debug(
                "** process incoming %d.%02d response:\n", (received->hdr->code >> 5), received->hdr->code & 0x1F);
        coap_show_pdu(received);
    }
#endif

    /* check if this is a response to our original request */
    if (!check_token(received))
    {
        /* drop if this was just some message, or send RST in case of notification */
        if (!sent
                && (received->hdr->type == COAP_MESSAGE_CON
                        || received->hdr->type == COAP_MESSAGE_NON))
            coap_send_rst(ctx, remote, received);
        return;
    }

    switch (received->hdr->type)
    {
        case COAP_MESSAGE_CON:
            /* acknowledge received response if confirmable (TODO: check Token) */
            coap_send_ack(ctx, remote, received);
            break;
        case COAP_MESSAGE_RST:
            info("got RST\n");
            return;
        default:
            ;
    }

    /* output the received data, if any */
    if (received->hdr->code == COAP_RESPONSE_CODE(205))
    {

        /* set obs timer if we have successfully subscribed a resource */
        if (sent && coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter))
        {
            debug("observation relationship established, set timeout to %d\n", obs_seconds);
            set_timeout(&obs_wait, obs_seconds);
        }

        /* Got some data, check if block option is set. Behavior is undefined if
         * both, Block1 and Block2 are present. */
        block_opt = get_block(received, &opt_iter);
        if (!block_opt)
        {
            /* There is no block option set, just read the data and we are done. */
        }
        else
        {
            unsigned short blktype = opt_iter.type;

            /* TODO: check if we are looking at the correct block number */

            if (COAP_OPT_BLOCK_MORE(block_opt))
            {
                /* more bit is set */
                debug(
                        "found the M bit, block size is %u, block nr. %u\n", COAP_OPT_BLOCK_SZX(block_opt), coap_opt_block_num(block_opt));

                /* create pdu with request for next block */
                pdu = coap_new_request(ctx, method, NULL); /* first, create bare PDU w/o any option  */
                if (pdu)
                {
                    /* add URI components from optlist */
                    for (option = optlist; option; option = option->next)
                    {
                        switch (COAP_OPTION_KEY(*(coap_option *)option->data))
                        {
                            case COAP_OPTION_URI_HOST:
                            case COAP_OPTION_URI_PORT:
                            case COAP_OPTION_URI_PATH:
                            case COAP_OPTION_URI_QUERY:
                                coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *)option->data),
                                        COAP_OPTION_LENGTH(*(coap_option *)option->data),
                                        COAP_OPTION_DATA(*(coap_option *)option->data));
                                break;
                            default:
                                ; /* skip other options */
                        }
                    }

                    /* finally add updated block option from response, clear M bit */
                    /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
                    debug("query block %d\n", (coap_opt_block_num(block_opt) + 1));
                    coap_add_option(pdu, blktype,
                            coap_encode_var_bytes(buf,
                                    ((coap_opt_block_num(block_opt) + 1) << 4)
                                            | COAP_OPT_BLOCK_SZX(block_opt)), buf);

                    if (received->hdr->type == COAP_MESSAGE_CON)
                        tid = coap_send_confirmed(ctx, remote, pdu);
                    else
                        tid = coap_send(ctx, remote, pdu);

                    if (tid == COAP_INVALID_TID)
                    {
                        debug("message_handler: error sending new request");
                        coap_delete_pdu(pdu);
                    }
                    else
                    {
                        set_timeout(&max_wait, wait_seconds);
                        if (received->hdr->type != COAP_MESSAGE_CON)
                            coap_delete_pdu(pdu);
                    }

                    return;
                }
            }
        }
    }
    else
    { /* no 2.05 */

        /* check if an error was signaled and output payload if so */
        if (COAP_RESPONSE_CLASS(received->hdr->code) >= 4)
        {
            printf("%d.%02d", (received->hdr->code >> 5), received->hdr->code & 0x1F);
            if (coap_get_data(received, &len, &databuf))
            {
                printf(" ");
                while (len--)
                    printf("%c", *databuf++);
            }
            printf("\n");
        }

    }

    /* finally send new request, if needed */
    if (pdu && coap_send(ctx, remote, pdu) == COAP_INVALID_TID)
    {
        debug("message_handler: error sending response");
    }
    coap_delete_pdu(pdu);
    /* our job is done, we can exit at any time */
    ready = coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter) == NULL;
    if(ready && COAP_SUBSCRIBE)
      ready = 0;
}



extern err_t  mld6_joingroup(const ip6_addr_t *srcaddr, const ip6_addr_t *groupaddr);
int join(coap_context_t *ctx, char *group_name)
{
    //Register to multicast group membership
    ip6_addr_t mcast_addr;
    
    inet_pton(AF_INET6, group_name, &(mcast_addr.addr));
    if(mld6_joingroup((const ip6_addr_t *)IP6_ADDR_ANY , &mcast_addr) != 0){
        printf("\n\r[ERROR] Register to ipv6 multicast group failed\n");
        return -1;
    }
    return 0;
}

int order_opts(void *a, void *b)
{
    if (!a || !b)
        return a < b ? -1 : 1;

    if (COAP_OPTION_KEY(*(coap_option *)a) < COAP_OPTION_KEY(*(coap_option *)b))
        return -1;

    return COAP_OPTION_KEY(*(coap_option *)a) == COAP_OPTION_KEY(*(coap_option *)b);
}

coap_list_t *
new_option_node(unsigned short key, unsigned int length, unsigned char *data)
{
    coap_option *option;
    coap_list_t *node;

    option = coap_malloc(sizeof(coap_option) + length);
    if (!option)
        goto error;

    COAP_OPTION_KEY(*option) = key;
    COAP_OPTION_LENGTH(*option) = length;
    memcpy(COAP_OPTION_DATA(*option), data, length);

    /* we can pass NULL here as delete function since option is released automatically  */
    node = coap_new_listnode(option, NULL);

    if (node)
        return node;

    error: printf("new_option_node: malloc");
    coap_free( option);
    return NULL;
}

typedef struct
{
    unsigned char code;
    char *media_type;
} content_type_t;

void cmdline_content_type(char *arg, unsigned short key)
{
    static content_type_t content_types[] =
    {
    { 0, "plain" },
    { 0, "text/plain" },
    { 40, "link" },
    { 40, "link-format" },
    { 40, "application/link-format" },
    { 41, "xml" },
    { 42, "binary" },
    { 42, "octet-stream" },
    { 42, "application/octet-stream" },
    { 47, "exi" },
    { 47, "application/exi" },
    { 50, "json" },
    { 50, "application/json" },
    { 255, NULL } };
    coap_list_t *node;
    unsigned char i, value[10];
    int valcnt = 0;
    unsigned char buf[2];
    char *p, *q = arg;

    while (q && *q)
    {
        p = strchr(q, ',');

        if (isdigit(*q))
        {
            if (p)
                *p = '\0';
            value[valcnt++] = atoi(q);
        }
        else
        {
            for (i = 0;
                    content_types[i].media_type
                            && strncmp(q, content_types[i].media_type, p ? p - q : strlen(q)) != 0;
                    ++i)
                ;

            if (content_types[i].media_type)
            {
                value[valcnt] = content_types[i].code;
                valcnt++;
            }
            else
            {
                warn("W: unknown content-type '%s'\n", arg);
            }
        }

        if (!p || key == COAP_OPTION_CONTENT_TYPE)
            break;

        q = p + 1;
    }

    for (i = 0; i < valcnt; ++i)
    {
        node = new_option_node(key, coap_encode_var_bytes(buf, value[i]), buf);
        if (node)
            coap_insert(&optlist, node, order_opts);
    }
}

void cmdline_uri(char *arg)
{
    unsigned char portbuf[2];
#define BUFSIZE 40
    unsigned char _buf[BUFSIZE];
    unsigned char *buf = _buf;
    size_t buflen;
    int res;

    if (proxy.length)
    { /* create Proxy-Uri from argument */
        size_t len = strlen(arg);
        while (len > 270)
        {
            coap_insert(&optlist,
                    new_option_node(COAP_OPTION_PROXY_URI, 270, (unsigned char *) arg), order_opts);
            len -= 270;
            arg += 270;
        }

        coap_insert(&optlist, new_option_node(COAP_OPTION_PROXY_URI, len, (unsigned char *) arg),
                order_opts);
    }
    else
    { /* split arg into Uri-* options */
        coap_split_uri((unsigned char *) arg, strlen(arg), &uri);

        if (uri.port != COAP_DEFAULT_PORT)
        {
            coap_insert(&optlist,
                    new_option_node(COAP_OPTION_URI_PORT, coap_encode_var_bytes(portbuf, uri.port),
                            portbuf), order_opts);
        }

        if (uri.path.length)
        {
            buflen = BUFSIZE;
            res = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);

            while (res--)
            {
                coap_insert(&optlist,
                        new_option_node(COAP_OPTION_URI_PATH, COAP_OPT_LENGTH(buf),
                                COAP_OPT_VALUE(buf)), order_opts);

                buf += COAP_OPT_SIZE(buf);
            }
        }

        if (uri.query.length)
        {
            buflen = BUFSIZE;
            buf = _buf;
            res = coap_split_query(uri.query.s, uri.query.length, buf, &buflen);

            while (res--)
            {
                coap_insert(&optlist,
                        new_option_node(COAP_OPTION_URI_QUERY, COAP_OPT_LENGTH(buf),
                                COAP_OPT_VALUE(buf)), order_opts);

                buf += COAP_OPT_SIZE(buf);
            }
        }
    }
}

int cmdline_blocksize(char *arg)
{
    unsigned short size;

    again: size = 0;
    while (*arg && *arg != ',')
        size = size * 10 + (*arg++ - '0');

    if (*arg == ',')
    {
        arg++;
        block.num = size;
        goto again;
    }

    if (size)
        block.szx = (coap_fls(size >> 4) - 1) & 0x07;

    flags |= FLAGS_BLOCK;
    return 1;
}

/* Called after processing the options from the commandline to set
 * Block1 or Block2 depending on method. */
void set_blocksize(void)
{
    static unsigned char buf[4]; /* hack: temporarily take encoded bytes */
    unsigned short opt;

    if (method != COAP_REQUEST_DELETE)
    {
        opt = method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 : COAP_OPTION_BLOCK1;

        coap_insert(&optlist,
                new_option_node(opt, coap_encode_var_bytes(buf, (block.num << 4 | block.szx)), buf),
                order_opts);
    }
}

void cmdline_subscribe(void)
{
#if COAP_SUBSCRIBE
    obs_seconds = atoi(SUBSCRIBE_TIME);
#endif
    coap_insert(&optlist, new_option_node(COAP_OPTION_SUBSCRIPTION, 0, NULL), order_opts);
}

int cmdline_proxy(char *arg)
{
    char *proxy_port_str = strrchr((const char *) arg, ':'); /* explicit port ? */
    if (proxy_port_str)
    {
        char *ipv6_delimiter = strrchr((const char *) arg, ']');
        if (!ipv6_delimiter)
        {
            if (proxy_port_str == strchr((const char *) arg, ':'))
            {
                /* host:port format - host not in ipv6 hexadecimal string format */
                *proxy_port_str++ = '\0'; /* split */
            }
        }
        else
        {
            arg = strchr((const char *) arg, '[');
            if (!arg)
                return 0;
            arg++;
            *ipv6_delimiter = '\0'; /* split */
            if (ipv6_delimiter + 1 == proxy_port_str++)
            {
                /* [ipv6 address]:port */
            }
        }
    }

    proxy.length = strlen(arg);
    if ((proxy.s = coap_malloc(proxy.length + 1)) == NULL)
    {
        proxy.length = 0;
        return 0;
    }

    memcpy(proxy.s, arg, proxy.length + 1);
    return 1;
}

inline void cmdline_token(char *arg)
{
    strncpy((char *) the_token.s, arg, min(sizeof(_token_data), strlen(arg)));
    the_token.length = strlen(arg);
}

void cmdline_option(char *arg)
{
    unsigned int num = 0;

    while (*arg && *arg != ',')
    {
        num = num * 10 + (*arg - '0');
        ++arg;
    }
    if (*arg == ',')
        ++arg;

    coap_insert(&optlist, new_option_node(num, strlen(arg), (unsigned char *) arg), order_opts);
}

extern int check_segment(const unsigned char *s, size_t length);
extern void decode_segment(const unsigned char *seg, size_t length, unsigned char *buf);

int cmdline_input(char *text, str *buf)
{
    int len;
    len = check_segment((unsigned char *) text, strlen(text));

    if (len <= 0)
        return 0;

    buf->s = (unsigned char *) coap_malloc(len);
    if (!buf->s)
        return 0;

    buf->length = len;
    decode_segment((unsigned char *) text, strlen(text), buf->s);
    return 1;
}

method_t cmdline_method(char *arg)
{
    static char *methods[] =
    { 0, "get", "post", "put", "delete", 0 };
    unsigned char i;

    for (i = 1; methods[i] && strcasecmp(arg, methods[i]) != 0; ++i)
        ;

    return i; /* note that we do not prevent illegal methods */
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
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;

    s = getaddrinfo(node, port, &hints, &result);
    if (s != 0)
    {
        printf("getaddrinfo failed.\n");
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

extern int coap_read_len;
static void example_coap_client_thread(void *para){
    while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS){
        printf("Wait for WIFI connection ...\n");
        vTaskDelay(1000);
    }
    printf("\nCoAP Client Example starts...\n");
    
    coap_context_t *ctx = NULL;
    coap_address_t dst;
    static char addr[INET6_ADDRSTRLEN];
    void *addrptr = NULL;
    coap_tick_t now;
    coap_queue_t *nextpdu;
    coap_pdu_t *pdu;
    static str server;
    int res;
    char *group = NULL;
    coap_log_t log_level = LOG_WARNING;
    coap_tid_t tid = COAP_INVALID_TID;
 
#if COAP_BLOCK
    cmdline_blocksize(COAP_BLOCK_SIZE);
#endif

    if (!cmdline_input(PAYLOAD, &payload))
      payload.length = 0;
   
    method = cmdline_method(CMD);

#if COAP_MESSAGE_NONCON
    msgtype = COAP_MESSAGE_NON;
#endif

#if COAP_SUBSCRIBE
    cmdline_subscribe();
#endif

#if COAP_MEDIA_TYPE
    cmdline_content_type(COAP_MEDIA_TYPE_ITEM, COAP_OPTION_ACCEPT);    
#endif
    
#if COAP_CONTENT_TYPE
    cmdline_content_type(COAP_CONTENT_TYPE_ITEM, COAP_OPTION_CONTENT_TYPE);
#endif
                
#if COAP_ADD_OPTION
    cmdline_option(COAP_ADD_OPTION_CONTENT);
#endif

#if COAP_PROXY
    if (!cmdline_proxy(COAP_PROXY_URI))
    {
      printf("error specifying proxy address\n");
      goto exit;
    }    
#endif

#if COAP_DELETE_LIST
    coap_delete_list(optlist);
    optlist = NULL;
#endif
    
    cmdline_token(TOKEN);
      
    coap_set_log_level(log_level);
    
    cmdline_uri(SERVER_HOST); 

    if (proxy.length)
    {
        server = proxy;
    }
    else
    {
        server = uri.host;
    }

    /* resolve destination address where server should be sent */
    memset(&dst,0,sizeof(coap_address_t));
    res = resolve_address(&server, &dst.addr);
    if (res < 0)
    {
      printf("failed to resolve address\n");
      goto exit;
    }
    
    dst.port = atoi(SERVER_PORT);
    
   /* add Uri-Host if server address differs from uri.host */

    switch (dst.addr.type)
    {
        case IPADDR_TYPE_V4:
            addrptr = &dst.addr.u_addr.ip4;

            /* create context for IPv4 */
            ctx = get_context(NULL, SERVER_PORT);
            break;
        case IPADDR_TYPE_V6:
            addrptr = &dst.addr.u_addr.ip6;

            /* create context for IPv6 */
            ctx = get_context(NULL, SERVER_PORT);
            break;
        default:
            ;
    }
    
    if (!ctx)
    {
      coap_log(LOG_EMERG, "cannot create context\n");
      goto exit;
    }
   
    coap_register_option(ctx, COAP_OPTION_BLOCK2);
    coap_register_response_handler(ctx, message_handler);

    /* join multicast group if requested at command line */
    if (group)
        join(ctx, group);
    
    /* construct CoAP message */

    if (!proxy.length && addrptr
            && (ipaddr_ntoa_r(&dst.addr, addr, sizeof(addr)) != 0)
            && (strlen(addr) != uri.host.length || memcmp(addr, uri.host.s, uri.host.length) != 0))
    {
        /* add Uri-Host */
        coap_insert(&optlist, new_option_node(COAP_OPTION_URI_HOST, uri.host.length, uri.host.s),
                order_opts);
    }

    /* set block option if requested at commandline */
    if (flags & FLAGS_BLOCK)
        set_blocksize();

    if (!(pdu = coap_new_request(ctx, method, optlist)))
        goto exit;

#ifndef NDEBUG
    if (LOG_DEBUG <= coap_get_log_level())
    {
        debug("sending CoAP request:\n");
        coap_show_pdu(pdu);
    }
#endif

    if (pdu->hdr->type == COAP_MESSAGE_CON)
        tid = coap_send_confirmed(ctx, &dst, pdu);
    else
        tid = coap_send(ctx, &dst, pdu);

    if (pdu->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)
        coap_delete_pdu(pdu);

    set_timeout(&max_wait, wait_seconds);
    debug("timeout is set to %d seconds\n", wait_seconds);

    while (!(ready && coap_can_exit(ctx)))
    {
        nextpdu = coap_peek_next(ctx);

        coap_ticks(&now);
        while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime)
        {
            coap_retransmit(ctx, coap_pop_next(ctx));
            nextpdu = coap_peek_next(ctx);
        }

        /* read from socket */
        if(coap_read_len > 0)
        {
          coap_read_len = 0;
          printf("\n%d bytes read\n\r%s\n", ctx->recvqueue->pdu->length, ctx->recvqueue->pdu->data);
          coap_dispatch(ctx,(const char*)&response); /* and dispatch PDUs from receivequeue */
        }
    }
    
exit:
    coap_free_context(ctx);
    printf("\nCoAP Client Example end.\n");
    
    vTaskDelete(NULL);
}

void example_coap_client(void)
{
  if(xTaskCreate(example_coap_client_thread, ((const char*)"example_coap_client_thread"), 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    printf("\n\r%s xTaskCreate(init_thread) failed", __FUNCTION__);
}

#endif