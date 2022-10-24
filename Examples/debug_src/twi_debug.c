
/****************************************************************************/
/* Copyright (c) 2014 Thirdwayv, Inc. All Rights Reserved. */

/****************************************************************************/
/**
 ** @file					twi_debug.c
 ** @brief					This file contains implements the LOGGER module.
 **
 */

/*----------------------------------------------------------*/
/*- INCLUDES -----------------------------------------------*/
/*----------------------------------------------------------*/
 
#ifdef DEBUGGING_ENABLE

	#include "twi_debug.h"
	//lint -save -e451
	#include <string.h>
	//lint -restore
	#define __VALIST __gnuc_va_list
	#define __need___va_list

	#ifdef DEBUG_TIME_ENABLE
	#include "timer_mgmt.h"
	#endif

	#if defined(TWI_CEEDLING_TEST)
		#include <stdio.h>
		#include <string.h>    

	#elif (defined(NRF51) || defined(NRF52) || defined(NXP_QN908X) || defined(NXP_NHS31xx) || defined(TWI_CYPRESS)||defined(ST_BLUENRG_2)||defined(ESP32)||defined(LINUX))&& (!defined(WIN32))

		#include "string.h"

		#ifdef I2C_DEBUG
			#include "twi_i2c_master.h"
		#elif defined SOFT_UART_DEBUG
			#include "twi_soft_uart.h"
		#elif defined STEDIO_DEBUG

		#elif defined ITM_DEBUG
			#include "twi_itm.h"
		
		#else
			#include "twi_uart.h"
		#endif

		#ifndef LINUX
			#define TWI_CFG_PRINTF
		#endif
		
	#elif defined(GALILEO)

		#include <string.h>
		#include <stdio.h>
		#include "twi_uart.h"
			
	#elif defined(EM9304)
		#include <string.h>
		#include "twi_uart.h"

	#elif defined(WIN32)

		#include <stdio.h>
		#include <stdarg.h>
		#include <Windows.h>

		FILE*  pFile;
		SYSTEMTIME lt;

		#define FILE_PATH	".\\ApplicationLogs.txt"

	#elif defined (DIALOG)

		#include "twi_uart.h"

	#endif
#endif

/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS PROTOTYPES ----------------------------*/
/*---------------------------------------------------------*/
#if defined STEDIO_DEBUG
extern void twi_outch(char c);
#endif
/*---------------------------------------------------------*/
/*- GLOBAL STATIC VARIABLES -------------------------------*/
/*---------------------------------------------------------*/

#ifdef DEBUGGING_ENABLE
	static twi_bool 	gb_logger_init = TWI_FALSE;
	#ifdef DEBUG_TIME_ENABLE
	volatile twi_u32      gu32_Jiff = 0;
	tstr_timer_mgmt_timer gstr_debug_timer;
	#endif
#endif
/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/
#ifdef DEBUGGING_ENABLE
	#ifdef DEBUG_TIME_ENABLE
void debug_timer_cb(void *pv)
{
    gu32_Jiff++;
}
	#endif
 
	#if defined(TWI_CEEDLING_TEST) || defined (STDLIB)
		#define TWI_LOG_VAR(pu8_prnt_msg, argp, u8_strlen)      printf_var(pu8_prnt_msg, argp, u8_strlen)
	#else
		#define TWI_LOG_VAR(pu8_prnt_msg, argp, u8_strlen)      twi_logger_var(pu8_prnt_msg, argp, u8_strlen)
	#endif

	#ifdef WIN32
// @brief This function will be called when application exits
void atexit_handler(void)
{
	twi_logger_deinit();
}
	#endif

/** @brief  This function initializes the logger module
*/
void twi_logger_init(void *uart_rx_cb)
{
	#ifdef DEBUG_TIME_ENABLE
	/* The following condition is added if the logger is being de-initialized and re-initialized for powerdown or sleep conditions, i.e. re-initializing the UART after sleep then wakeup */
    if(gstr_debug_timer.b_is_active != TWI_TRUE)
    {
        timer_mgmt_init();
        start_timer(&gstr_debug_timer,(twi_s8*) "Debug Timer",TWI_TIMER_TYPE_PERIODIC,1000,debug_timer_cb,NULL);
    }
	#endif
	#if defined STEDIO_DEBUG
	// Check on previous initialization to prevent re-opening the file
	if(TWI_FALSE == gb_logger_init)
	{
		gb_logger_init = TWI_TRUE;
	}
	
	#elif defined(TWI_CEEDLING_TEST)

	#elif (defined(NRF51) || defined(NRF52) || defined(GALILEO) || defined(DIALOG) || defined(NXP_QN908X) || defined(NXP_NHS31xx) || defined(EM9304) || defined(TWI_CYPRESS)||defined(ST_BLUENRG_2)||defined(ESP32)||defined(LINUX))&& (!defined (WIN32))
	// Check on previous initialization to prevent re-opening the file
	if(TWI_FALSE == gb_logger_init)
	{
		#ifdef I2C_DEBUG

		twi_s32 s32_retval = TWI_SUCCESS;
		tstr_twi_i2c_master_config str_i2c_master_cfg;

		str_i2c_master_cfg.u32_clock = I2C_SCL_400K;
		str_i2c_master_cfg.u8_addr_size = SEVEN_BITS;
		str_i2c_master_cfg.u8_pull_up = GET_I2C_PULL_CFG(I2C_DEBUG_CHANNEL);

		s32_retval = twi_i2c_master_init(I2C_DEBUG_CHANNEL,&str_i2c_master_cfg);
		if(TWI_SUCCESS == s32_retval)
		{
			gb_logger_init = TWI_TRUE;
		}

		#elif defined SOFT_UART_DEBUG
		twi_s32 s32_retval = TWI_SUCCESS;
		tstr_twi_uart_config str_uart_cfg;

		str_uart_cfg.u32_baud_rate = UART_BAUD_115200;
		str_uart_cfg.u8_flags = TWI_UART_PARITY_DISABLE|TWI_UART_HALF_DUPLEX_TX;
		str_uart_cfg.pf_uart_recv_cb = NULL;

		s32_retval = twi_soft_uart_init(UART_DEBUG_CHANNEL, &str_uart_cfg);
		if(TWI_SUCCESS == s32_retval)
		{
			gb_logger_init = TWI_TRUE;
		}

		#elif defined ITM_DEBUG
		twi_itm_init();
		gb_logger_init = TWI_TRUE;
		#else
		twi_s32 s32_retval = TWI_SUCCESS;
		tstr_twi_uart_config str_uart_cfg;

		str_uart_cfg.u32_baud_rate = UART_BAUD_115200;
		if (uart_rx_cb == NULL)
		{
			str_uart_cfg.u8_flags = TWI_UART_PARITY_DISABLE|TWI_UART_HALF_DUPLEX_TX;
		}
		else
		{
			str_uart_cfg.u8_flags = TWI_UART_PARITY_DISABLE|TWI_UART_FULL_DUPLEX;
		}
		str_uart_cfg.pf_uart_recv_cb = (tpf_twi_uart_cb)(uart_rx_cb);

		s32_retval = twi_uart_init(UART_DEBUG_CHANNEL, &str_uart_cfg);
		if(TWI_SUCCESS == s32_retval)
		{
			gb_logger_init = TWI_TRUE;
		}
		#endif
	}

	#elif defined(WIN32)

	// Check on previous initialization to prevent re-opening the file
	if (TWI_FALSE == gb_logger_init)
	{
		// Open the logs file

		if (fopen_s(&pFile, FILE_PATH, "a+") == 0)
		{
			gb_logger_init = TWI_TRUE;

			atexit(atexit_handler);

			GetLocalTime(&lt);

			fprintf(pFile, "\n\n\n\t\tNew Log session has been started [%02d-%02d-%04d, %02d:%02d:%02d.%03d]\n\n",
				lt.wDay, lt.wMonth,lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
		}
		else
		{
			printf("ERROR in open file to start logging!!!!!!!!!!!!!\n");
			getchar();
			exit(EXIT_FAILURE);
		}
	}
	#endif	
}


void twi_logger_deinit(void)
{
	#if defined(TWI_CEEDLING_TEST)

	#elif (defined(NRF51) || defined(NRF52) || defined(GALILEO) || defined(DIALOG) || defined(NXP_QN908X)|| defined(NXP_NHS31xx)||defined(ST_BLUENRG_2)||defined(ESP32))&& (!defined(WIN32))
		#ifdef I2C_DEBUG
			#if 0  /*not tested yet*/
    
        /*We need to deinit I2C ONLY not the whole logger context*/
    TWI_ASSERT(TWI_SUCCESS == twi_i2c_master_deinit(I2C_DEBUG_CHANNEL));
    gb_logger_init = TWI_FALSE;
			#endif
		#elif defined SOFT_UART_DEBUG
			#if 0 /*not tested yet*/
    TWI_ASSERT(TWI_SUCCESS == twi_soft_uart_deinit(UART_DEBUG_CHANNEL));
    gb_logger_init = TWI_FLASE;
			#endif
		#elif defined STEDIO_DEBUG
	gb_logger_init = TWI_FALSE;

		#elif defined ITM_DEBUG
	gb_logger_init = TWI_FALSE;
		#else
    /*We need to deinit UART ONLY not the whole logger context*/
    TWI_ASSERT(TWI_SUCCESS == twi_uart_deinit(UART_DEBUG_CHANNEL));
    gb_logger_init = TWI_FALSE;
		#endif
	#elif defined(WIN32)
    fclose(pFile);
    gb_logger_init = TWI_FALSE;
	#endif
}

/** @brief Function to print a logging message via Uart or I2C
*/
	#ifdef TWI_CFG_PRINTF


static void outch(char c)
{
		#ifdef I2C_DEBUG
	/* sending the string over i2c */
	twi_i2c_master_send_n(I2C_DEBUG_CHANNEL, (const twi_u8*)&c, 1, I2C_DEBUG_SLAVE_ADD, TWI_TRUE);

		#elif defined SOFT_UART_DEBUG
	(void)twi_soft_uart_send_char(UART_DEBUG_CHANNEL,c);
	
		#elif defined ITM_DEBUG

	(void)twi_itm_send_char(c);

		#elif defined STEDIO_DEBUG
	twi_outch(c);

		#elif defined STDLIB

		#else
	/* sending the string over uart */
	(void)twi_uart_send_n(UART_DEBUG_CHANNEL, (const twi_u8*)&c, 1);
		#endif

}

static void utod(char* buffer, twi_u64 x)
{
    twi_u64 au64_q[20];
    /*Intialize the array to be non-complianed with pc-lint*/
    {
        /*Local scope*/
        twi_u8 u8_i = 0;
        au64_q[u8_i++] = 0x7FFFFFFFFFFFFFFFL;
        au64_q[u8_i++] = 1000000000000000000L;
        au64_q[u8_i++] = 100000000000000000L;
        au64_q[u8_i++] = 10000000000000000L;
        au64_q[u8_i++] = 1000000000000000L;
        au64_q[u8_i++] = 100000000000000L;
        au64_q[u8_i++] = 10000000000000L;
        au64_q[u8_i++] = 1000000000000L;
        au64_q[u8_i++] = 100000000000L;
        au64_q[u8_i++] = 10000000000L;
        au64_q[u8_i++] = 1000000000L;
        au64_q[u8_i++] = 100000000L;
        au64_q[u8_i++] = 10000000L;
        au64_q[u8_i++] = 1000000L;
        au64_q[u8_i++] = 100000L;
        au64_q[u8_i++] = 10000L;
        au64_q[u8_i++] = 1000L;
        au64_q[u8_i++] = 100L;
        au64_q[u8_i++] = 10L;
        au64_q[u8_i++] = 1L;
    }
        
    if (x == 0) {
        *buffer++ = '0';
    } else {
        unsigned int i;
        int significant = 0;

		for (i = 0; i < (sizeof(au64_q)/sizeof(au64_q[0])); i++) {
			twi_u64 qq = au64_q[i];
			char digit = 0;
			while (x >= qq) {
				x -= qq;
				digit++;
			}
			if (significant || digit != 0) {
				significant = 1;
				*buffer++ = digit + '0';
			}
		}
	}
	*buffer = 0;
}
static void utoh(char* buffer, twi_u64 x, twi_s32 upper)
{
    if (x == 0) {
        *buffer++ = '0';
    } else {
        int i;
        int significant = 0;

		for (i = 0; i < 64; i += 4) {
			char digit = (x >> (64-4-i)) & 15;
			if (significant || digit != 0) {
				unsigned char ch;
				significant = 1;
				if (digit > 9) {
					ch = digit-10 + (upper ? 'A' : 'a');
				} else {
					ch = digit + '0';
				}
				*buffer++ = ch;
			}
		}
	}
	*buffer = 0;
}
static twi_u32 scandec (const char** s,int * limit_len)
{
  twi_u32 result = 0;
  twi_u32 c;

  c = **s;
  while ('0' <= c && c <= '9') {
    (*s)++;
    (*limit_len)--;
    /* Be careful about evaluation order to avoid overflow if
       at all possible */
    result *= 10;
    c -= '0';
    result += c;
    c = **s;
  }
  return result;
}
		#ifdef FLOATING_POINT_ENABLED
static int normalize(double *val) {
    int exponent = 0;
    double value = *val;

    while (value >= 1.0) {
        value /= 10.0;
        ++exponent;
    }

    while (value < 0.1) {
        value *= 10.0;
        --exponent;
    }
    *val = value;
    return exponent;
}

static void ftoa_fixed(char *buffer, double value,int width) {
    /* carry out a fixed conversion of a double value to a string, with a precision of 5 decimal digits.
     * Values with absolute values less than 0.000001 are rounded to 0.0
     * Note: this blindly assumes that the buffer will be large enough to hold the largest possible result.
     * The largest value we expect is an IEEE 754 double precision real, with maximum magnitude of approximately
     * e+308. The C standard requires an implementation to allow a single conversion to produce up to 512
     * characters, so that's what we really expect as the buffer size.
     */

    int exponent = 0;
    int lplaces = 0;
    int rplaces = 0;
    
    if(width == 0) {
        width = 4;
    }

    if (value == 0.0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0.0) {
        *buffer++ = '-';
        value = -value;
    }

    exponent = normalize(&value);

    while (exponent > 0) {
        int digit = value * 10;
        *buffer++ = digit + '0';
        value = value * 10 - digit;
        ++lplaces;
        --exponent;
    }

    if (lplaces == 0)
        *buffer++ = '0';

    *buffer++ = '.';

    while (exponent < 0 && rplaces < width) {
        *buffer++ = '0';
        --exponent;
        ++rplaces;
    }

    while (rplaces < width) {
        int digit = value * 10.0;
        *buffer++ = digit + '0';
        value = value * 10.0 - digit;
        ++rplaces;
    }
    *buffer = '\0';
}
		#endif


static int twifprintf(const char* format, va_list argp,int limit_len)
{
    int count = 0; /* count of characters transferred */
    const char* fp;

    enum Flags {
        flag_left = 1,
        flag_plus = 2,
        flag_blank = 4,
        flag_zero = 8,
    };

    for (fp = format; (*fp != 0)&&(limit_len--); fp++) {
        if (fp[0] == '%' && fp[1] != 0) {
            int len;
            int flags = 0;
            int width = 0;
            int precision = -1;
            int lsz = 0;

            /* Sign character to be output for conversion */
            char sign = 0;

            /* Fill character to use */
            char fill = ' ';

		#ifdef FLOATING_POINT_ENABLED
			/* Buffer for conversions */
			char buffer[512];
		#else
            char buffer[64];
		#endif            

            /* Start of converted string to be output */
            char* s = buffer;

            /* Process flags */
            int valid = 1;
            
            
            while (valid) {
                char c = *++fp;
                limit_len--;
                /* Process flags */
                switch (c) {
                    case '-':
                        flags |= flag_left;
                        break;
                    case '+':
                        flags |= flag_plus;
                        sign = '+';
                        break;
                    case ' ':
                        flags |= flag_blank;
                        sign = ' ';
                        break;
                    case '0':
                        flags |= flag_zero;
                        fill = '0';
                        break;
                    case '.':
                        break;
                    default:
                        valid = 0;
                        break;
                }
            }

            /* Process width */
            if (*fp == '*') {
                fp++;
				limit_len--;
                /*lint -save -e586 */
                /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
                width = va_arg(argp,int);
                /*lint -restore */
            } else {
                width = scandec(&fp,&limit_len);
            }
            if (width < 0) {
                width = -width;
                flags |= flag_left;
            }

            /* Process precision (only for fixed point and strings) */
            if (*fp == '.') {
                fp++;
				limit_len--;
                if (*fp == '*') {
                    fp++;
					limit_len--;
                    /*lint -save -e586 */
                    /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
                    precision = va_arg(argp,int);
                    /*lint -restore */
                } else {
                    precision = scandec(&fp,&limit_len);
                }
            }
                /* skip over size */
            if ((*fp == 'l') || (*fp == 'h')) {limit_len--; fp++; lsz = 1;}

            /* Process type */
            switch (*fp) {
                case '%': {
                    buffer[0] = '%';
                    buffer[1] = 0;
                }
                break;

                case 'c': {
                    /* char is promoted to int */
                    /*lint -save -e586 */ 
                    /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
                    char ch = va_arg(argp,int);
                    /*lint -restore */
                    buffer[0] = ch;
                    buffer[1] = 0;
                }
                break;

                case 's': {
                    /*lint -save -e586 -e64*/
                    /*Skip is deprecated[MISRA 2012 Rule 17.1]& Type mismatch[Rule 1.3] lint warning*/
                    s = va_arg(argp,char*);
                    /*lint -restore */
                }
                break;

				case 'i':
				case 'd': {

                    /*lint -save -e586 */ 
                    /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
                    twi_s64 x;
                    if(0 != lsz)
                    {
                        x = va_arg(argp, twi_s64);
                    }
                    else
                    {
                        x = (twi_s64)va_arg(argp, twi_s32);
                    }
                    
                    /*lint -restore */
					if (x < 0) {
						sign = '-';
						x = -x;
					}

					utod(buffer,x);
					break;
				}
				case 'u': {
                    /*lint -save -e586 -e571*/ 
                    /*Skip is deprecated[MISRA 2012 Rule 17.1]& Suspicious cast lint warning*/
                    twi_u64 x;
                    if(0 != lsz)
                    {
                        x = va_arg(argp, twi_u64);
                    }
                    else
                    {
                        x = (twi_u64)va_arg(argp, twi_u32);
                    }
                    /*lint -restore */
					utod(buffer,x);
					break;
				}

				case 'p':
				{
					width = 4;
					fill = '0';
				}
				break;
				case 'x': {
                    /*lint -save -e586 -e571*/ 
                    /*Skip is deprecated[MISRA 2012 Rule 17.1] & Suspicious cast lint warning*/
                    twi_u64 x;
                    if(0 != lsz)
                    {
                        x = va_arg(argp, twi_u64);
                    }
                    else
                    {
                        x = (twi_u64)va_arg(argp, twi_u32);
                    }
                    /*lint -restore */
					utoh(buffer,x,0);
				}
				break;

				case 'X': {
                    /*lint -save -e586 -e571*/ 
                    /*Skip is deprecated[MISRA 2012 Rule 17.1] & Suspicious cast lint warning*/
                    twi_u64 x;
                    if(0 != lsz)
                    {
                        x = va_arg(argp, twi_u64);
                    }
                    else
                    {
                        x = (twi_u64)va_arg(argp, twi_u32);
                    }
                    /*lint -restore */
					utoh(buffer,x,1);
				}
				break;
		#ifdef FLOATING_POINT_ENABLED
                case 'f': {
                    double x = (double)va_arg(argp, double);
                    ftoa_fixed(buffer, x, width);
                }
                break;
		#endif
                default: {
                    buffer[0] = *fp;
                    buffer[1] = 0;
                    break;
                }
            }

            len = strlen(s);
            if (*fp == 's' && precision >= 0) {
                if (len > precision) len = precision;
            }
            if (sign) width--;

            if (flags&flag_left) {
                /* Pad on right */
                int i;
                fill = ' ';
                if (sign) {
                    outch(sign);
                    count++;
                }
                for (i = 0; i < len; i++) {
                	outch(*s++);
                    count++;
                }
                for ( ; i < width; i++) {
                	outch(fill);
                    count++;
                }
            } else {
                int i;
                /* Pad on left */
                if (fill != ' ' && sign) {
                	outch(sign);
                    count++;
                }
                for (i = len; i < width; i++) {
                    outch(fill);
                    count++;
                }
                if (fill == ' ' && sign) {
                   outch(sign);
                    count++;
                }
                for (i = 0; i < len; i++) {
                	outch(*s++);
                    count++;
                }
                
            }
        } else {
        	outch(*fp);
            count++;
        }
    }
    return count;
}
	#endif

	#if defined(TWI_CEEDLING_TEST) || defined (STDLIB)
static void printf_var(const char* pu8_prnt_msg, va_list argp,int u8_strlen)
{
    (void)vfprintf(stdout, pu8_prnt_msg, argp);
}
	#else
void twi_logger_var(const char* pu8_prnt_msg, va_list argp,int u8_strlen)
{
		#if defined(TWI_CEEDLING_TEST)

		#elif (defined(NRF51) || defined(NRF52) || defined(GALILEO) || defined(DIALOG) || defined(TWI_CYPRESS)|| defined(NXP_QN908X)|| defined(NXP_NHS31xx)||defined(ST_BLUENRG_2)||defined(ESP32))&& (!defined(WIN32))

	if(TWI_TRUE == gb_logger_init)
	{
			#ifdef TWI_CFG_PRINTF
        twi_bool b_lf = TWI_FALSE;
        twi_bool b_cr = TWI_FALSE;

        if(pu8_prnt_msg[u8_strlen - 1] == '\n')
        {
            if((u8_strlen >= 2)&&(pu8_prnt_msg[u8_strlen - 2] == '\r'))
            {
                /*wrong should be \r\n -2*/
                u8_strlen-=2;
            } else{
                /*missing "\r" before \n - 1*/
                u8_strlen-=1;
            }
            b_cr = TWI_TRUE;
            b_lf = TWI_TRUE;
        }
        else if(pu8_prnt_msg[u8_strlen - 1] == '\r')
        {
            if((u8_strlen >= 2)&&(pu8_prnt_msg[u8_strlen - 2] == '\n'))
            {
                /*correct*/
            } else{
                /*missing "\r\n" after*/
                b_lf = TWI_TRUE;
            }
        }
		(void)twifprintf((const char*)pu8_prnt_msg,argp,u8_strlen);
        
        if(b_cr == TWI_TRUE)
		{
			(void)twifprintf((const char*)"\r", argp,1);
		}
        if(b_lf == TWI_TRUE)
		{
			(void)twifprintf((const char*)"\n", argp,1);
		}
			#elif defined I2C_DEBUG

		/* sending the string over i2c */
		twi_i2c_master_send_n(I2C_DEBUG_CHANNEL, (const twi_u8*)pu8_prnt_msg, u8_strlen, I2C_DEBUG_SLAVE_ADD, TWI_TRUE);

			#elif defined SOFT_UART_DEBUG
		/* sending the string over soft uart */
		twi_soft_uart_send_n(UART_DEBUG_CHANNEL, (const twi_u8*)pu8_prnt_msg, u8_strlen);

		if(pu8_prnt_msg[u8_strlen - 1] == '\n')
		{
			twi_soft_uart_send_char(UART_DEBUG_CHANNEL, '\r');
		}
			#else
		/* sending the string over uart */
		twi_uart_send_n(UART_DEBUG_CHANNEL, (const twi_u8*)pu8_prnt_msg, u8_strlen);

		if(pu8_prnt_msg[u8_strlen - 1] == '\n')
		{
			twi_uart_send_char(UART_DEBUG_CHANNEL, '\r');
		}
			#endif
	}
		#elif defined(WIN32)
	if (TWI_TRUE == gb_logger_init)
	{
		twi_u16	u16_strlen;
		va_list args;
		u16_strlen = (twi_u16) strlen((const char*)pu8_prnt_msg);
		va_start(args, pu8_prnt_msg);
		
		if (NULL != pFile)
		{
			/* Print on file */
			if ((pu8_prnt_msg[u16_strlen - 1] == '\n') && (u16_strlen > 1))
			{
				GetLocalTime(&lt);
				fprintf(pFile, "[%02d:%02d:%02d.%03d] : ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
			}
	
			vfprintf(pFile, pu8_prnt_msg, args);
			
			fflush(pFile);
		}
		
		/* print on console */
		printf(pu8_prnt_msg, args);

		va_end(args);
	}
		#endif
}
	#endif


void twi_logger(const twi_u8* pu8_prnt_msg, ...)
{
	#if defined(TWI_CEEDLING_TEST)

	#elif (defined(NRF51) || defined(NRF52) || defined(GALILEO) || defined(DIALOG) || defined(NXP_QN908X)|| defined(NXP_NHS31xx) || defined(EM9304) || defined(TWI_CYPRESS)||defined(ST_BLUENRG_2)||defined(ESP32)) && (!defined(WIN32)) 

	if(TWI_TRUE == gb_logger_init)
	{
		volatile twi_u8 u8_strlen;
		u8_strlen = strlen((const char*)pu8_prnt_msg); 		/* getting the length of the input string */

		#ifdef TWI_CFG_PRINTF
		va_list argp;
        twi_bool b_lf = TWI_FALSE;
        twi_bool b_cr = TWI_FALSE;
        /*lint -save -e586 -e530*/ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]& Symbol 'argp'not initialized [ Rule 9.1] lint warning*/
		va_start(argp,pu8_prnt_msg);
        /*lint -restore */
        if(pu8_prnt_msg[u8_strlen - 1] == '\n')
        {
            if((u8_strlen >= 2)&&(pu8_prnt_msg[u8_strlen - 2] == '\r'))
            {
                /*wrong should be \r\n -2*/
                u8_strlen-=2;
            } else{
                /*missing "\r" before \n - 1*/
                u8_strlen-=1;
            }
            b_cr = TWI_TRUE;
            b_lf = TWI_TRUE;
        }
        else if(pu8_prnt_msg[u8_strlen - 1] == '\r')
        {
            if((u8_strlen >= 2)&&(pu8_prnt_msg[u8_strlen - 2] == '\n'))
            {
                /*correct*/
            } else{
                /*missing "\r\n" after*/
                b_lf = TWI_TRUE;
            }
        }
		(void)twifprintf((const char*)pu8_prnt_msg,argp,u8_strlen);
        /*lint -save -e586 */ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
		va_end(argp);
        /*lint -restore */

		if(b_cr == TWI_TRUE)
		{
			(void)twifprintf((const char*)"\r", argp,1);
		}
        if(b_lf == TWI_TRUE)
		{
			(void)twifprintf((const char*)"\n", argp,1);
		}
		#elif defined I2C_DEBUG

		/* sending the string over i2c */
        twi_i2c_master_send_n(I2C_DEBUG_CHANNEL, (const twi_u8*)pu8_prnt_msg, u8_strlen, I2C_DEBUG_SLAVE_ADD, TWI_TRUE);

		#elif defined SOFT_UART_DEBUG
		/* sending the string over soft uart */
		twi_soft_uart_send_n(UART_DEBUG_CHANNEL, (const twi_u8*)pu8_prnt_msg, u8_strlen);

		if(pu8_prnt_msg[u8_strlen - 1] == '\n')
		{
			twi_soft_uart_send_char(UART_DEBUG_CHANNEL, '\r');
		}

		#elif defined ITM_DEBUG
		/* sending the string over itm */
		TWI_ASSERT(TWI_SUCCESS == twi_itm_send_n( (const twi_u8*)pu8_prnt_msg, u8_strlen));

		if(pu8_prnt_msg[u8_strlen - 1] == '\n')
		{
			twi_itm_send_char('\r');
		}
		#else

		/* sending the string over uart */
		twi_uart_send_n(UART_DEBUG_CHANNEL, (const twi_u8*)pu8_prnt_msg, u8_strlen);

		if(pu8_prnt_msg[u8_strlen - 1] == '\n')
		{
			twi_uart_send_char(UART_DEBUG_CHANNEL, '\r');
		}
		#endif
	}
	#elif defined(WIN32)
	if (TWI_TRUE == gb_logger_init)
	{
		twi_u16	u16_strlen;
		va_list args;
		u16_strlen = (twi_u16) strlen((const char*)pu8_prnt_msg);
		va_start(args, pu8_prnt_msg);
		
		if (NULL != pFile)
		{
			/* Print on file */
			if ((pu8_prnt_msg[u16_strlen - 1] == '\n') && (u16_strlen > 1))
			{
				GetLocalTime(&lt);
				fprintf(pFile, "[%02d:%02d:%02d.%03d] : ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
			}
	
			vfprintf(pFile, pu8_prnt_msg, args);
			
			fflush(pFile);
		}
		
		/* print on console */
		vprintf(pu8_prnt_msg, args);

		va_end(args);
	}
	#endif
}

void twi_logger_debug(const twi_u8* pu8_prnt_msg,...)
{
    do{
        va_list argp;
        volatile twi_u8 u8_strlen;
           
        u8_strlen = strlen((const char*)pu8_prnt_msg); 		/* getting the length of the input string */
        /*lint -save -e586 -e530*/ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]& Symbol 'argp'not initialized [ Rule 9.1] lint warning*/
        va_start(argp,pu8_prnt_msg);
        /*lint -restore */
        TWI_LOG_TIME;
        TWI_LOG_VAR((const char*)pu8_prnt_msg, argp, u8_strlen);
        /*lint -save -e586 */ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
        va_end(argp);
        /*lint -restore */
    }while(0);
}

void twi_logger_info(const twi_u8* pu8_prnt_msg,...)
{
   do{
        va_list argp;
        volatile twi_u8 u8_strlen;
           
        u8_strlen = strlen((const char*)pu8_prnt_msg); 		/* getting the length of the input string */
        /*lint -save -e586 -e530 */ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]& Symbol 'argp'not initialized [ Rule 9.1] lint warning*/
        va_start(argp,pu8_prnt_msg);
        /*lint -restore */
        TWI_LOG_TIME;
        TWI_LOG("%s", (const twi_u8*)"(INFO)");
        TWI_LOG_VAR((const char*)pu8_prnt_msg, argp, u8_strlen);
        /*lint -save -e586 */ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
        va_end(argp);
       /*lint -restore */
   }while(0);
}

void twi_logger_err(const twi_u8* pu8_file_name, const twi_u32 u32_line, const twi_u8* pu8_prnt_msg,...)
{
    do{
        va_list argp;
        volatile twi_u8 u8_strlen;
           
        u8_strlen = strlen((const char*)pu8_prnt_msg); 		/* getting the length of the input string */
        /*lint -save -e586 -e530*/ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]& Symbol 'argp'not initialized [ Rule 9.1] lint warning*/
        va_start(argp,pu8_prnt_msg);
        /*lint -restore */
        TWI_LOG_PRINT("%s", "\033[1;31m");
        TWI_LOG_TIME;
	#ifdef LINUX
        TWI_LOG((const twi_u8*)"(ERR)(%s)(%d)",pu8_file_name,u32_line);
	#else
        TWI_LOG((const twi_u8*)"(ERR)(%s)(%ld)",pu8_file_name,u32_line);
	#endif
        TWI_LOG_VAR((const char*)pu8_prnt_msg, argp, u8_strlen);
        /*lint -save -e586 */ 
        /*Skip is deprecated[MISRA 2012 Rule 17.1]. lint warning*/
        va_end(argp);
        /*lint -restore */
        TWI_LOG("%s", (const twi_u8*)"\033[0m\r\n");
    }while(0);
}

void twi_logger_dump_buf(const twi_u8* name, const twi_u8* Buffer, const twi_u32 size)
{
    do																	
    {																	
        twi_u32 k;													    
                            
        twi_logger_debug((const twi_u8*)"%s (addr: %08X) (sz: %d)",name,(twi_uptr)Buffer,size);	
        for (k = 0; k < size; k++)										
        {																
            if (!(k % 16))				
	#ifdef LINUX
            TWI_LOG_PRINT("\r\n 0x%04X)\t",k);
	#else
            TWI_LOG_PRINT("\r\n 0x%04lX)\t",k);
	#endif
            TWI_LOG_PRINT("%02X ", Buffer[k]);								
        }																
        TWI_LOG_PRINT("%s", "\r\n");											
    }while(0);
}

void twi_logger_dump_hex(const twi_u8* Buffer, const twi_u32 size)
{
    do																	
    {																	
        twi_u32 k;													    
        for (k = 0; k < size; k++)										
        {																
            if (!(k % 16)&&(k != 0))									
            TWI_LOG_PRINT("%s", "\r\n");										
            TWI_LOG_PRINT("%02X ", Buffer[k]);								
        }																
        TWI_LOG_PRINT("%s", "\r\n");											
    }while(0);  
}

void twi_logger_dump_bufc(const twi_u8* name, const twi_u8* Buffer, const twi_u32 size)
{
	do																	
    {																	
        twi_u32 k;
	#ifdef LINUX
		TWI_LOGGER("%s(%08lX)(%u)",name,(twi_uptr)Buffer, size);
	#else
		TWI_LOGGER("%s(%08X)(%lu)",name,(twi_uptr)Buffer, size);
	#endif
        for (k = 0; k < size; k++)										
        {																
            TWI_LOG_PRINT("%c", Buffer[k]); 								
        }																
        TWI_LOG_PRINT("%s", "\r\n");											
    }while(0);
}
#endif
