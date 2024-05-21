#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sys/random.h>
#include <time.h>

#define RAND_LENGTH (10)
#define UUID_T_LENGTH (16)
#define UNIX_TS_LENGTH (6)

static uint8_t entrpy[RAND_LENGTH];

static char *ngx_http_uuid7(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_http_uuid7_commands[] = {
    {ngx_string("uuid7"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
     ngx_http_uuid7,
     NGX_HTTP_LOC_CONF_OFFSET,
     0,
     NULL},
    ngx_null_command};

static ngx_http_module_t ngx_http_uuid7_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL  /* merge location configuration */
};

ngx_module_t ngx_http_uuid7_module = {
    NGX_MODULE_V1,
    &ngx_http_uuid7_module_ctx, /* module context */
    ngx_http_uuid7_commands,    /* module directives */
    NGX_HTTP_MODULE,            /* module type */
    NULL,                       /* init master */
    NULL,                       /* init module */
    NULL,                       /* init process */
    NULL,                       /* init thread */
    NULL,                       /* exit thread */
    NULL,                       /* exit process */
    NULL,                       /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_int_t initialize_mt(ngx_http_request_t *r)
{
    if (getentropy(entrpy, RAND_LENGTH) != 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to get random entropy");
        return NGX_ERROR;
    }
    return NGX_OK;
}

static ngx_int_t get_milliseconds(ngx_http_request_t *r, uint64_t *unix_ts)
{
    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, &tp) != 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "failed to get current time");
        return NGX_ERROR;
    }
    *unix_ts = (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000);
    return NGX_OK;
}

static ngx_int_t ngx_http_uuid7_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v,
                                         uintptr_t data)
{
    static const size_t UUID_STR_LENGTH = 36;
    uint8_t uuid[UUID_T_LENGTH];
    char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    uint64_t unix_ts;

    if (initialize_mt(r) != NGX_OK || get_milliseconds(r, &unix_ts) != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (int i = 0; i < UNIX_TS_LENGTH; i++)
    {
        uuid[UNIX_TS_LENGTH - 1 - i] = (uint8_t)(unix_ts >> (8 * i));
    }

    for (int i = 0; i < RAND_LENGTH; i++)
    {
        uuid[UNIX_TS_LENGTH + i] = entrpy[i];
    }

    uuid[6] = 0x70 | (uuid[6] & 0x0f); // Set UUID version to 7
    uuid[8] = 0x80 | (uuid[8] & 0x3f); // Set UUID variant to DCE 1.1

    char str[UUID_STR_LENGTH];
    int s = 0;

    for (int i = 0; i < UUID_T_LENGTH; i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
        {
            str[s++] = '-';
        }
        str[s++] = hex[uuid[i] >> 4];
        str[s++] = hex[uuid[i] & 0x0f];
    }
    str[s] = '\0';

    v->len = UUID_STR_LENGTH;
    v->data = ngx_palloc(r->pool, UUID_STR_LENGTH);
    if (v->data == NULL)
    {
        *v = ngx_http_variable_null_value;
        return NGX_OK;
    }
    ngx_memcpy(v->data, str, UUID_STR_LENGTH);
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}

static char *ngx_http_uuid7(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t *value;
    ngx_http_variable_t *v;
    ngx_int_t index;

    value = cf->args->elts;
    if (value[1].data[0] != '$')
    {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;

    v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (v == NULL)
    {
        return NGX_CONF_ERROR;
    }

    index = ngx_http_get_variable_index(cf, &value[1]);
    if (index == NGX_ERROR)
    {
        return NGX_CONF_ERROR;
    }

    v->get_handler = ngx_http_uuid7_variable;

    return NGX_CONF_OK;
}
