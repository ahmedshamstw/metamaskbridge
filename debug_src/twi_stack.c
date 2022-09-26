/****************************************************************************/
/* Copyright (c) 2022 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/

/**
 * @file:	twi_stack.c
 * @brief:	This file implements the stack interface layer in the communication stack.
 */

#include "twi_stack.h"
#include "twi_retval.h"

#define STACK_LOG(...)
#define STACK_LOG_ERR(...)
#define STACK_LOG_HEX(MSG, HEX_BUFFER, LEN)

#define	STACK_LOG_ENABLE

#if defined (STACK_LOG_ENABLE)
#define STACK_LOG_ERR_LEVEL
#endif

#ifdef	STACK_LOG_ENABLE
	#include "twi_debug.h"
#if defined(STACK_LOG_DEBUG_LEVEL)
	#undef STACK_LOG
	#define STACK_LOG(...)								TWI_LOGGER("[TWI_STACK]: "__VA_ARGS__)

	#undef  STACK_LOG_HEX
    #define STACK_LOG_HEX(MSG, HEX_BUFFER, LEN)  		TWI_DUMP_BUF("[TWI_STACK]: "MSG,HEX_BUFFER,LEN)
#endif
#if defined(STACK_LOG_DEBUG_LEVEL) || defined(STACK_LOG_ERR_LEVEL)
	#undef STACK_LOG_ERR
	#define	STACK_LOG_ERR(...)							do{															\
																TWI_LOGGER("\033[1;31m[TWI_STACK]: "__VA_ARGS__);	\
																TWI_LOGGER("\033[0m");									\
															}while(0)
	#undef  STACK_LOG_HEX
    #define STACK_LOG_HEX(MSG, HEX_BUFFER, LEN)  		TWI_DUMP_BUF("[TWI_STACK]: "MSG,HEX_BUFFER,LEN)
#endif
#endif

/*---------------------------------------------------------*/
/*- STRUCTS AND UNIONS AND ENUM----------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- GLOBAL CONST VARIABLES --------------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- GLOBAL STATIC VARIABLES -------------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- GLOBAL EXTERN VARIABLES -------------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS PROTOTYPES ----------------------------*/
/*---------------------------------------------------------*/
static void twi_sl_cb(tstr_twi_sl_evt* pstr_evt);
static void twi_init_si_global_variables(tstr_stack_ctx * pstr_ctx);
/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/
static void twi_sl_cb(tstr_twi_sl_evt* pstr_evt)
{	
	TWI_ASSERT(pstr_evt != NULL);
	TWI_ASSERT(pstr_evt->pv_args != NULL);

	tstr_twi_stack_evt str_stack_evt;
	tstr_stack_ctx * pstr_ctx = (tstr_stack_ctx *) pstr_evt->pv_args;

	TWI_MEMSET(&str_stack_evt, 0, sizeof(tstr_twi_stack_evt));

	STACK_LOG_ERR("EVENT = %d\r\n", pstr_evt->enu_event);
	switch(pstr_evt->enu_event)
	{
		case TWI_SL_SEND_STATUS_EVT:
		{
			str_stack_evt.enu_event 								= TWI_STACK_SEND_STATUS_EVT;
			str_stack_evt.uni_data.str_send_stts_evt.b_is_success 	= pstr_evt->uni_data.str_send_stts_evt.b_is_success;
			str_stack_evt.uni_data.str_send_stts_evt.pv_user_arg 	= pstr_evt->uni_data.str_send_stts_evt.pv_user_arg;
			break;
		}
			
		case TWI_SL_RCV_DATA_EVT:
		{
			str_stack_evt.enu_event 								= TWI_STACK_RCV_DATA_EVT;
			str_stack_evt.uni_data.str_rcv_data_evt.enu_msg_type 	= pstr_evt->uni_data.str_rcv_data_evt.enu_msg_type;
			str_stack_evt.uni_data.str_rcv_data_evt.pu8_data 		= pstr_evt->uni_data.str_rcv_data_evt.pu8_data;
			str_stack_evt.uni_data.str_rcv_data_evt.u16_data_len 	= pstr_evt->uni_data.str_rcv_data_evt.u16_data_len;
			str_stack_evt.uni_data.str_rcv_data_evt.pv_user_arg 	= (void *) pstr_ctx;
			break;
		}
			
		default:
		{
			STACK_LOG_ERR("Unhandled Event = %d\r\n", pstr_evt->enu_event);
			str_stack_evt.enu_event = TWI_STACK_INVALID_EVT;
			break;
		}
			
	}

	if(TWI_STACK_INVALID_EVT != str_stack_evt.enu_event)
	{
		TWI_ASSERT(NULL != (pstr_ctx->str_global.pf_stack_cb));
		pstr_ctx->str_global.pf_stack_cb(&str_stack_evt, pstr_ctx->str_global.pv);
	}
}
static void twi_init_si_global_variables(tstr_stack_ctx * pstr_ctx)
{
	pstr_ctx->str_global.b_is_initialized 	= TWI_FALSE;
	pstr_ctx->str_global.pf_stack_cb 		= NULL;
	pstr_ctx->str_global.pv					= NULL;
}
/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/

/**
*	@brief		This is the init function. It shall be called before any other API.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pf_evt_cb		    Pointer to Function that is called by the ble stack to pass events to the application.
*	@param [in]	enu_ll_type		    Type of the Link Layer.
*   @param [in] pv                  pointer to void    
*	@param [in]	pstr_helpers		Pointer to structure of the whole layer helper APIs. 
*/
twi_s32 twi_stack_init(	tstr_stack_ctx * pstr_ctx , 
							tpf_stack_cb pf_evt_cb , void* pv, 
                            tenu_twi_ll_type enu_ll_type,
							tstr_stack_helpers * pstr_helpers)
{
	twi_s32 s32_retval;
	if((pf_evt_cb != NULL) && (pstr_ctx != NULL))
	{
		if(TWI_FALSE == pstr_ctx->str_global.b_is_initialized)
		{
			twi_init_si_global_variables(pstr_ctx);
			s32_retval = twi_sl_init(	&(pstr_ctx->str_sl_ctx), 
											pstr_ctx, 
											twi_sl_cb, 
											enu_ll_type,
											pstr_helpers, pv);
			if(TWI_SUCCESS == s32_retval)
			{
				pstr_ctx->str_global.b_is_initialized 	= TWI_TRUE;
				pstr_ctx->str_global.pf_stack_cb		= pf_evt_cb;
				pstr_ctx->str_global.pv					= pv;
			}
			else
			{
				STACK_LOG_ERR("Failed to Init Security Layer With Error = %d\r\n", s32_retval);
			}
		}
		else
		{
			s32_retval = TWI_ERROR_ALREADY_INITIALIZED;
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}
	return s32_retval;
}

#if defined (TWI_BLE_STACK_ENABLED)
/**
*	@brief		This is a function that called by the application to pass the different BLE events to the stack to handle them
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pstr_ble_evt		Pointer to Structure holds the BLE event data.
*/
void twi_stack_handle_ble_evt(tstr_stack_ctx * pstr_ctx , tstr_twi_ble_evt * pstr_ble_evt)
{
	TWI_ASSERT(pstr_ble_evt != NULL);
	TWI_ASSERT(pstr_ctx != NULL);
	twi_sl_handle_ble_evt(&(pstr_ctx->str_sl_ctx) , pstr_ble_evt);
}
#endif

#if defined (TWI_USB_STACK_ENABLED)
/**
*	@brief		This is a function that called by the application to pass the different USB events to the stack to handle them
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pstr_usb_evt		Pointer to Structure holds the USB event data.
*/
void twi_stack_handle_usb_evt(tstr_stack_ctx * pstr_ctx , tstr_twi_usb_evt * pstr_usb_evt)
{
	TWI_ASSERT(pstr_usb_evt != NULL);
	TWI_ASSERT(pstr_ctx != NULL);
	twi_sl_handle_usb_evt(&(pstr_ctx->str_sl_ctx) , pstr_usb_evt);
}
#endif

/**
*	@brief		This is the stack send function
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	enu_msg_type	type of message that will be sent.
*	@param [in]	pu8_data		Pointer to data to be sent.
*                               This buffer shall remain untouched by the application till a TWI_STACK_SEND_STATUS_EVT is passed from the stack.
*	@param [in]	u16_data_len    Data length.
*	@param [in]	pv_arg    		User argument.
*/
twi_s32 twi_stack_send_data(tstr_stack_ctx * pstr_ctx , tenu_twi_stack_msg_type enu_msg_type, twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg)
{
	twi_s32 s32_retval;
	if((enu_msg_type < TWI_STACK_INVLD_MSG) && (pu8_data != NULL) && (u16_data_len > 0)  && (pstr_ctx != NULL))
	{
		if(TWI_TRUE == pstr_ctx->str_global.b_is_initialized)
		{
			s32_retval = twi_sl_send_data(&(pstr_ctx->str_sl_ctx) , enu_msg_type, pu8_data, u16_data_len, pv_arg);
			if(s32_retval != TWI_SUCCESS)
			{
				STACK_LOG_ERR("Failed to Send Data To Security Layer With Error = %d\r\n", s32_retval);
			}
		}
		else
		{
			s32_retval = TWI_ERROR_NOT_INITIALIZED;
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}
	return s32_retval;
}

/**
*	@brief:	This is the Stack Interface Layer dispatcher. This is used to handle the periodic events of the stack interface.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*/
void twi_stack_dispatcher(tstr_stack_ctx * pstr_ctx)
{
	TWI_ASSERT(pstr_ctx != NULL);
	twi_sl_dispatcher(&(pstr_ctx->str_sl_ctx));
}

/**
*	@brief		This is an API to unlock the BLE receive buffer in the Network Layer.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pu8_buffer_to_unlock		Pointer to buffer to unlock.
*
*	@return	    None.
*/
void twi_stack_unlock_rcv_buf( tstr_stack_ctx * pstr_ctx , twi_u8* pu8_buffer_to_unlock)
{
	twi_sl_unlock_rcv_buf( &(pstr_ctx->str_sl_ctx) , pu8_buffer_to_unlock);
}

void twi_stack_is_ready_to_send(tstr_stack_ctx* pstr_cntxt, twi_bool* pb_is_ready)
{
	FUN_IN;
	twi_sl_is_ready_to_send(&pstr_cntxt->str_sl_ctx, pb_is_ready);
	TWI_LOGGER("*pb_is_ready = %d\r\n", *pb_is_ready);
}

twi_bool twi_stack_is_idle(tstr_stack_ctx* pstr_cntxt)
{
	return twi_sl_is_idle(&pstr_cntxt->str_sl_ctx);
}

