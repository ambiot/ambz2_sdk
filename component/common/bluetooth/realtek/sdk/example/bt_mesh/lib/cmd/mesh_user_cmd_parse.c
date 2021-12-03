/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     user_cmd_parse.c
  * @brief    Source file for command parse.
  * @details  Parse user commands and execute right commands.
  * @author   bill
  * @date     2017-05-16
  * @version  v2.0
  * *************************************************************************************
  */

/* Add Includes here */
#include <string.h>
#include "mesh_data_uart.h"
#include "mesh_user_cmd_parse.h"
#include "platform_os.h"

/* User command interface data, used to parse the commands from Data UART. */
user_cmd_if_t *puser_cmd_if;

/**
 * @brief  Check, if a character is a white space.
 *
 * @param c         char data to check.
 * @return check result.
 * @retval 1  TRUE.
 * @retval 1  FALSE.
*/
static bool user_cmd_is_white_space(char c)
{
    return (((c >= 9) && (c <= 13)) || (c == 32));
}

/**
 * @brief  Skip white spaces in buffer.
 *
 * @param buffer
 * @return pointer to skipped white spaces' new buffer.
*/
char *user_cmd_skip_spaces(char *buffer)
{
    char *p = buffer;

    while (user_cmd_is_white_space(*p)) /* white space */
    {
        p++;
    }
    return p;
}

/**
 * @brief  Find end of a word.
 *
 * @param buffer
 * @return
*/
char *user_cmd_find_end_of_word(char *buffer)
{
    char *p = buffer;

    while (!user_cmd_is_white_space(*p) && (*p != '\0'))
    {
        p++;
    }
    return p;
}

/**
 * @brief  Read ASCII string and convert to uint32_t.
 *
 * @param p
 * @return
*/
uint32_t user_cmd_string2uint32(char *p)
{
    uint32_t result = 0;
    char     ch;
    bool     hex = false;

    /* check if value is dec */
    if (p[0] == 'x' || p[0] == 'X')
    {
        hex = true;
        p = &p[1];
    }
    else if ((p[0] == '0') && (p[1] == 'x' || p[1] == 'X'))
    {
        hex = true;
        p = &p[2];
    }

    for (;;)
    {
        ch = *(p++) | 0x20;                 /* convert to lower case */

        if (hex)                            /* dec value */
        {
            /* hex value */
            if ((ch >= 'a') && (ch <= 'f'))
            {
                ch -= ('a' - 10);
            }
            else if ((ch >= '0') && (ch <= '9'))
            {
                ch -= '0';
            }
            else
            {
                break;
            }
            result = (result << 4);
            result += (ch & 0x0f);
        }
        else
        {
            if (ch < '0' || ch > '9')
            {
                break;    /* end of string reached */
            }
            result = 10 * result + ch - '0';
        }
    }
    return (result);
}

/**
 * @brief  Send result, display in UART Assitant.
 *
 * @param pUserCmdIF command parsed.
 * @param Result                command parse result.
 * @return none
*/
static void user_cmd_send_result(user_cmd_parse_result_t result)
{
    switch (result)
    {
    case USER_CMD_RESULT_ERROR:
        data_uart_debug("Error\n\r");
        break;
    case USER_CMD_RESULT_CMD_NOT_FOUND:
        data_uart_debug("Command not found\n\r");
        break;
    case USER_CMD_RESULT_WRONG_NUM_OF_PARAMETERS:
        data_uart_debug("Wrong number of parameters\n\r");
        break;
    case USER_CMD_RESULT_WRONG_PARAMETER:
        data_uart_debug("Wrong parameter\n\r");
        break;
    case USER_CMD_RESULT_VALUE_OUT_OF_RANGE:
        data_uart_debug("Value out of range\n\r");
        break;
    default:
        break;
    }
}

/**
 * @brief  execute cmd.
 *
 * @param pUserCmdIF command parsed.
 * @param pparse_value       command parse value.
 * @param pCmdTable         command table, include user self-definition command function.
 * @return command execute result.
*/
static user_cmd_parse_result_t user_cmd_list(user_cmd_parse_value_t *pparse_value,
                                             const user_cmd_table_entry_t *pcmd_table)
{
    /* avoid gcc compile warning */
    (void)pparse_value;
    int32_t              i = 0;
    user_cmd_parse_result_t  Result = USER_CMD_RESULT_CMD_NOT_FOUND;

    /* find command in table */
    while ((pcmd_table + i)->pcommand != NULL)
    {
        data_uart_debug((pcmd_table + i)->poption);
        data_uart_debug("  *");
        data_uart_debug((pcmd_table + i)->phelp);
        Result = USER_CMD_RESULT_OK;
        i++;
    };

    data_uart_debug(",.\r\n  *up down\r\n");
    data_uart_debug("[]\r\n  *left right\r\n");
    data_uart_debug("/\\\r\n  *home end\r\n");
    data_uart_debug("backspace\r\n  *delete\r\n");

    return (Result);
}

static user_cmd_parse_result_t user_cmd_execute(user_cmd_parse_value_t *pparse_value,
                                                const user_cmd_table_entry_t *pcmd_table)
{
    int32_t i = 0;
    user_cmd_parse_result_t  Result = USER_CMD_RESULT_CMD_NOT_FOUND;

    if (strcmp((const char *)pparse_value->pcommand, (const char *)"?") == 0)
    {
        user_cmd_list(pparse_value, pcmd_table);
        return USER_CMD_RESULT_OK;
    }

    /* find command in table */
    while ((pcmd_table + i)->pcommand != NULL)
    {
        if (strcmp((const char *)(pcmd_table + i)->pcommand, (const char *)pparse_value->pcommand) == 0)
        {
            /* command found */
            /* check if user wants help */
            if (pparse_value->para_count && *pparse_value->pparameter[0] == '?')
            {
                data_uart_debug((pcmd_table + i)->poption);
                data_uart_debug("  *");
                data_uart_debug((pcmd_table + i)->phelp);
                Result = USER_CMD_RESULT_OK;
            }
            else
            {
                /* execute command function */
                Result = (pcmd_table + i)->user_cmd_func(pparse_value);
            }
            /* exit while */
            break;
        }
        i++;
    };

    return (Result);
}

/**
 * @brief  Parse a command line and return the found
 *       command and parameters in "pparse_value"
 *
 * @param pUserCmdIF command parsed.
 * @param pparse_value       command parse value.
 * @return command parse result.
*/
static user_cmd_parse_result_t user_cmd_parse(user_cmd_parse_value_t *pparse_value)
{
    int32_t              i;
    user_cmd_parse_result_t  Result;
    char            *p, *q;

    /* clear all results */
    Result = USER_CMD_RESULT_OK;
    pparse_value->pcommand            = NULL;
    pparse_value->para_count     = 0;
    for (i = 0 ; i < USER_CMD_MAX_PARAMETERS; i++)
    {
        pparse_value->pparameter[i]     = NULL;
        pparse_value->dw_parameter[i]    = 0;
    }

    /* Parse line */
    p = puser_cmd_if->cmd_line;

    /*ignore leading spaces */
    p = user_cmd_skip_spaces(p);
    if (*p == '\0')                      /* empty command line ? */
    {
        Result = USER_CMD_RESULT_EMPTY_CMD_LINE;
    }
    else
    {
        /* find end of word */
        q = user_cmd_find_end_of_word(p);
        if (p == q)                        /* empty command line ? */
        {
            Result = USER_CMD_RESULT_EMPTY_CMD_LINE;
        }
        else                                /* command found */
        {
            pparse_value->pcommand = p;
            *q = '\0';                        /* mark end of command */
            p = q + 1;
            /* parse parameters */
            if (*p != '\0')                   /* end of line ? */
            {
                uint8_t j = 0;

                do
                {
                    uint32_t d;
                    /* ignore leading spaces */
                    p = user_cmd_skip_spaces(p);
                    d = user_cmd_string2uint32(p);

                    pparse_value->pparameter[j]    = p;
                    pparse_value->dw_parameter[j++] = d;

                    if (j >= USER_CMD_MAX_PARAMETERS)
                    {
                        break;
                    }

                    /* find next parameter */
                    p  = user_cmd_find_end_of_word(p);
                    *p++ = '\0';                        /* mark end of parameter */
                }
                while (*p != '\0');
                pparse_value->para_count = j;
            }
        }
    }

    return (Result);
}

static void user_cmd_move_back(void)
{
    for (uint8_t loop = 0; loop < puser_cmd_if->cmd_len - puser_cmd_if->cmd_cur; loop ++)
    {
        puser_cmd_if->cmd_line[puser_cmd_if->cmd_len - loop] = puser_cmd_if->cmd_line[puser_cmd_if->cmd_len
                                                                                      - loop - 1];
    }
}

static void user_cmd_move_forward(void)
{
    for (uint8_t loop = 0; loop < puser_cmd_if->cmd_len - puser_cmd_if->cmd_cur; loop ++)
    {
        puser_cmd_if->cmd_line[puser_cmd_if->cmd_cur + loop - 1] =
            puser_cmd_if->cmd_line[puser_cmd_if->cmd_cur + loop];
    }
}

/**
  * @brief clear command line buffer
  *
  * @param none
  * @return none
  */
static void user_cmd_clear_command(void)
{
    puser_cmd_if->cmd_len = 0;
    puser_cmd_if->cmd_cur = 0;
    memset(puser_cmd_if->cmd_line, 0, sizeof(puser_cmd_if->cmd_line));
}

/**
  * @brief clear screen from cursor
  *
  * @param none
  * @return none
  */
static void user_cmd_clear_screen(void)
{
    if (puser_cmd_if->cmd_cur < puser_cmd_if->cmd_len)
    {
        data_uart_debug("%s", puser_cmd_if->cmd_line + puser_cmd_if->cmd_cur);
    }

    while (puser_cmd_if->cmd_len != 0)
    {
        puser_cmd_if->cmd_len--;
        puser_cmd_if->cmd_line[puser_cmd_if->cmd_len] = '\0';
        data_uart_debug("\b \b");
    }
    puser_cmd_if->cmd_cur = 0;
}

/**
  * @brief  collect cmd characters.
  *
  * @param pUserCmdIF store parsed commands.
  * @param pdata             data to be parsed.
  * @param len               length of data to be command parsed.
  * @param pcmd_table         command table to execute function.
  * @return command collect result.
  * @retval 1 TRUE.
  * @retval 0 FALSE.
  */
bool mesh_user_cmd_collect(uint8_t *pdata, uint8_t len, const user_cmd_table_entry_t *pcmd_table)
{
    user_cmd_parse_value_t parse_value;
    char c;

    /* discard rx data as long help text output is in progress */
    while (len--)
    {
        c = *pdata++;
        if (c != 0x0)                       /* not ESC character received */
        {
            switch (c)                        /* Normal handling */
            {
            case '\n':
            case '\r':                          /* end of line */
                data_uart_debug("%s", puser_cmd_if->cmd_crlf);
                puser_cmd_if->history_cur = USER_CMD_MAX_HISTORY_LINE;
                if (puser_cmd_if->cmd_len > 0)  /* at least one character in command line ? */
                {
                    // save cmd first
                    if (puser_cmd_if->history_head == USER_CMD_MAX_HISTORY_LINE)
                    {
                        puser_cmd_if->history_head = 0;
                        puser_cmd_if->history_tail = 0;
                    }
                    else
                    {
                        puser_cmd_if->history_tail = (puser_cmd_if->history_tail + 1) % USER_CMD_MAX_HISTORY_LINE;
                        if (puser_cmd_if->history_tail == puser_cmd_if->history_head)
                        {
                            puser_cmd_if->history_head = (puser_cmd_if->history_head + 1) % USER_CMD_MAX_HISTORY_LINE;
                        }
                    }
                    puser_cmd_if->cmd_history_len[puser_cmd_if->history_tail] = puser_cmd_if->cmd_len;
                    memcpy(puser_cmd_if->cmd_history[puser_cmd_if->history_tail], puser_cmd_if->cmd_line,
                           puser_cmd_if->cmd_len);

                    user_cmd_parse_result_t Result;
                    //puser_cmd_if->cmd_line[puser_cmd_if->cmd_len] = '\0';
                    Result = user_cmd_parse(&parse_value);
                    if (Result == USER_CMD_RESULT_OK)
                    {
                        Result = user_cmd_execute(&parse_value, pcmd_table);
                    }

                    if (Result != USER_CMD_RESULT_OK)
                    {
                        user_cmd_send_result(Result);
                    }
                }

                data_uart_debug("%s", puser_cmd_if->cmd_prompt);

                /* maybe more than 1 cmd in pData: */
                user_cmd_clear_command();
                break;
            case '\b':                          /* backspace */
                if (puser_cmd_if->cmd_len > 0 && puser_cmd_if->cmd_cur > 0)
                {
                    //memcpy(puser_cmd_if->cmd_line + puser_cmd_if->cmd_cur - 1, puser_cmd_if->cmd_line + puser_cmd_if->cmd_cur, puser_cmd_if->cmd_len - puser_cmd_if->cmd_cur);
                    user_cmd_move_forward();
                    puser_cmd_if->cmd_len--;
                    puser_cmd_if->cmd_cur--;
                    puser_cmd_if->cmd_line[puser_cmd_if->cmd_len] = '\0';
                    data_uart_debug("\b%s", puser_cmd_if->cmd_line + puser_cmd_if->cmd_cur);
                    data_uart_debug(" \b");
                    for (uint8_t loop = 0; loop < puser_cmd_if->cmd_len - puser_cmd_if->cmd_cur; loop++)
                    {
                        data_uart_debug("\b");
                    }
                }
                break;
            case 44:                            /* up: , */
                if (puser_cmd_if->history_head != USER_CMD_MAX_HISTORY_LINE)
                {
                    user_cmd_clear_screen();
                    if (puser_cmd_if->history_cur == USER_CMD_MAX_HISTORY_LINE)
                    {
                        puser_cmd_if->history_cur = puser_cmd_if->history_tail;
                    }
                    else
                    {
                        if (puser_cmd_if->history_cur != puser_cmd_if->history_head)
                        {
                            puser_cmd_if->history_cur = (puser_cmd_if->history_cur + USER_CMD_MAX_HISTORY_LINE - 1) %
                                                        USER_CMD_MAX_HISTORY_LINE;
                        }
                        else
                        {
                            puser_cmd_if->history_cur = USER_CMD_MAX_HISTORY_LINE;
                            break;
                        }
                    }
                    puser_cmd_if->cmd_len = puser_cmd_if->cmd_history_len[puser_cmd_if->history_cur];
                    puser_cmd_if->cmd_cur = puser_cmd_if->cmd_len;
                    memcpy(puser_cmd_if->cmd_line, puser_cmd_if->cmd_history[puser_cmd_if->history_cur],
                           puser_cmd_if->cmd_len);
                    data_uart_debug("%s", puser_cmd_if->cmd_line);
                }
                break;
            case 46:                            /* down: . */
                if (puser_cmd_if->history_head != USER_CMD_MAX_HISTORY_LINE)
                {
                    user_cmd_clear_screen();
                    if (puser_cmd_if->history_cur == USER_CMD_MAX_HISTORY_LINE)
                    {
                        puser_cmd_if->history_cur = puser_cmd_if->history_head;
                    }
                    else
                    {
                        if (puser_cmd_if->history_cur != puser_cmd_if->history_tail)
                        {
                            puser_cmd_if->history_cur = (puser_cmd_if->history_cur + 1) % USER_CMD_MAX_HISTORY_LINE;
                        }
                        else
                        {
                            puser_cmd_if->history_cur = USER_CMD_MAX_HISTORY_LINE;
                            break;
                        }
                    }
                    puser_cmd_if->cmd_len = puser_cmd_if->cmd_history_len[puser_cmd_if->history_cur];
                    puser_cmd_if->cmd_cur = puser_cmd_if->cmd_len;
                    memcpy(puser_cmd_if->cmd_line, puser_cmd_if->cmd_history[puser_cmd_if->history_cur],
                           puser_cmd_if->cmd_len);
                    data_uart_debug("%s", puser_cmd_if->cmd_line);
                }
                break;
            case 91:                           /* left: [ */
                if (puser_cmd_if->cmd_cur > 0)
                {
                    data_uart_debug("\b");
                    puser_cmd_if->cmd_cur--;
                }
                break;
            case 93:                           /* right: ] */
                if (puser_cmd_if->cmd_cur < puser_cmd_if->cmd_len)
                {
                    data_uart_debug("%c", puser_cmd_if->cmd_line[puser_cmd_if->cmd_cur]);
                    puser_cmd_if->cmd_cur++;
                }
                break;
            case 92:                            /* end: \ */
                if (puser_cmd_if->cmd_cur < puser_cmd_if->cmd_len)
                {
                    data_uart_debug("%s", puser_cmd_if->cmd_line + puser_cmd_if->cmd_cur);
                    puser_cmd_if->cmd_cur = puser_cmd_if->cmd_len;
                }
                break;
            case 47:                            /* begin: / */
                while (puser_cmd_if->cmd_cur > 0)
                {
                    data_uart_debug("\b");
                    puser_cmd_if->cmd_cur--;
                }
                break;
            default:
                /* Put character in command buffer */
                if (puser_cmd_if->cmd_len < USER_CMD_MAX_COMMAND_LINE)
                {
                    user_cmd_move_back();
                    puser_cmd_if->cmd_line[puser_cmd_if->cmd_cur] = c;
                    data_uart_debug("%s", puser_cmd_if->cmd_line + puser_cmd_if->cmd_cur);
                    puser_cmd_if->cmd_len++;
                    puser_cmd_if->cmd_cur++;
                    for (uint8_t loop = 0; loop < puser_cmd_if->cmd_len - puser_cmd_if->cmd_cur; loop++)
                    {
                        data_uart_debug("\b");
                    }
                }
                break;
            }
        }
        else
        {
            //clear

        }
    }

    return (true);
}

void mesh_user_cmd_init(char *s)
{
    puser_cmd_if = (user_cmd_if_t *)plt_malloc(sizeof(user_cmd_if_t), RAM_TYPE_DATA_OFF);
    memset((void *)puser_cmd_if, 0, sizeof(user_cmd_if_t));
    puser_cmd_if->cmd_crlf[0]   = '\r';
    puser_cmd_if->cmd_crlf[1]   = '\n';
    puser_cmd_if->cmd_prompt[0] = '>';
    puser_cmd_if->history_head = USER_CMD_MAX_HISTORY_LINE;
    puser_cmd_if->history_tail = USER_CMD_MAX_HISTORY_LINE;
    puser_cmd_if->history_cur = USER_CMD_MAX_HISTORY_LINE;

    data_uart_debug(">> Hello %s <<\r\n%s", s, puser_cmd_if->cmd_prompt);
}

void mesh_user_cmd_deinit(char *s)
{
    if (puser_cmd_if) {
        data_uart_debug(">> Goodbye %s <<\r\n", s);
        plt_free(puser_cmd_if, RAM_TYPE_DATA_OFF);
        puser_cmd_if = NULL;
    }
}

