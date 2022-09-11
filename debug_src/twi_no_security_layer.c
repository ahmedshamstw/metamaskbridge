/****************************************************************************/
/* Copyright (c) 2022 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/

/**
 * @file:	twi_no_security_layer.c
 * @brief:	This file implements the no security manager layer in the communication stack.
 */

#include "twi_security_layer.h"
#include "twi_retval.h"

#define NO_SEC_LOG(...)
#define NO_SEC_LOG_ERR(...)
#define NO_SEC_LOG_HEX(MSG, HEX_BUFFER, LEN)

//#define	NO_SEC_LOG_ENABLE

#if defined (NO_SEC_LOG_ENABLE)
#define NO_SEC_LOG_ERR_LEVEL
#endif

#ifdef	NO_SEC_LOG_ENABLE
	#include "twi_debug.h"
#if defined(NO_SEC_LOG_DEBUG_LEVEL)
	#undef NO_SEC_LOG
	#define NO_SEC_LOG(...)								TWI_LOGGER("[NO_SEC]: "__VA_ARGS__)

	#undef  NO_SEC_LOG_HEX
    #define NO_SEC_LOG_HEX(MSG, HEX_BUFFER, LEN)  		TWI_DUMP_BUF("[NO_SEC]: "MSG,HEX_BUFFER,LEN)
#endif
#if defined(NO_SEC_LOG_DEBUG_LEVEL) || defined(NO_SEC_LOG_ERR_LEVEL)
	#undef NO_SEC_LOG_ERR
	#define	NO_SEC_LOG_ERR(...)							do{															\
																TWI_LOGGER("\033[1;31m[NO_SEC]: "__VA_ARGS__);	\
																TWI_LOGGER("\033[0m");									\
															}while(0)
	#undef  NO_SEC_LOG_HEX
    #define NO_SEC_LOG_HEX(MSG, HEX_BUFFER, LEN)  		TWI_DUMP_BUF("[NO_SEC]: "MSG,HEX_BUFFER,LEN)
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
static void twi_nl_cb(tstr_twi_nl_evt* pstr_evt);
static void twi_init_ns_global_variables(tstr_sl_ctx *pstr_ctx);
/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/

/**
 *	@brief: This is the callback function that is used to notify the security manager layer with the link layer events.
 *	@param[in]  pstr_evt: 		a pointer to the link layer structure that contains all the needed event(s) data.
*/
static void twi_nl_cb(tstr_twi_nl_evt* pstr_evt)
{
	TWI_ASSERT((pstr_evt != NULL) && (pstr_evt->pv_args != NULL));
	tstr_twi_sl_evt str_sl_evt;
	tstr_sl_ctx * pstr_ctx = (tstr_sl_ctx *)pstr_evt->pv_args;

	TWI_MEMSET(&str_sl_evt, 0, sizeof(tstr_twi_sl_evt));

	switch(pstr_evt->enu_event)
	{
		case TWI_NL_SEND_STATUS_EVT:
		{
			NO_SEC_LOG("TWI_NL_SEND_STATUS_EVT\r\n");
			str_sl_evt.enu_event								= TWI_SL_SEND_STATUS_EVT;
			str_sl_evt.uni_data.str_send_stts_evt.b_is_success 	= pstr_evt->uni_data.str_send_stts_evt.b_is_success;
			str_sl_evt.uni_data.str_send_stts_evt.pv_user_arg 	= pstr_evt->uni_data.str_send_stts_evt.pv_user_arg;
			break;
		}
			
		case TWI_NL_RCV_DATA_EVT:
		{
			NO_SEC_LOG("TWI_NL_RCV_DATA_EVT\r\n");
			str_sl_evt.enu_event 								= TWI_SL_RCV_DATA_EVT;
			str_sl_evt.uni_data.str_rcv_data_evt.enu_msg_type 	= TWI_STACK_CLR_MSG;
			str_sl_evt.uni_data.str_rcv_data_evt.pu8_data 		= pstr_evt->uni_data.str_rcv_data_evt.pu8_data;
			str_sl_evt.uni_data.str_rcv_data_evt.u16_data_len 	= pstr_evt->uni_data.str_rcv_data_evt.u16_data_len;
			break;
		}

		case TWI_NL_RCV_ERROR_EVT:
		{
			NO_SEC_LOG("TWI_NL_RCV_ERROR_EVT\r\n");
			NO_SEC_LOG_HEX("Received Error Data:", pstr_evt->uni_data.str_rcv_err_evt.pu8_err_data, pstr_evt->uni_data.str_rcv_err_evt.u16_err_data_len);
			str_sl_evt.enu_event = TWI_SL_INVALID_EVT;
			break;
		}

		default:
		{
			NO_SEC_LOG_ERR("Unhandled Event = %d\r\n", pstr_evt->enu_event);
			str_sl_evt.enu_event = TWI_SL_INVALID_EVT;
		}
			
	}

	str_sl_evt.pv_args = pstr_ctx->str_global.pv_args;

	if(TWI_SL_INVALID_EVT != str_sl_evt.enu_event)
	{
		TWI_ASSERT(NULL != pstr_ctx->str_global.pf_sl_cb);
		pstr_ctx->str_global.pf_sl_cb(&str_sl_evt);
	}
}

/**
 *	@brief:	This function initializes all the security manager layer global variables used.
*/
static void twi_init_ns_global_variables(tstr_sl_ctx *pstr_ctx)
{
	pstr_ctx->str_global.b_is_initialized 	= TWI_FALSE;
	pstr_ctx->str_global.pf_sl_cb	 		= NULL;
}
/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/

/**
*	@brief		This is the init function. It shall be called before any other API.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in] pv_args				Void pointer for the user argument
*	@param [in]	pf_evt_cb		    Pointer to Function that is called by the stack to pass events to the application.
*	@param [in]	enu_ll_type		    Type of the Link Layer.
*	@param [in]	pstr_helpers		Pointer to structure of the whole layer helper APIs.
*	@param [in]	pv_helpers		    Pointer to void to be passed to helpers Apis.
*/
twi_s32 twi_sl_init(tstr_sl_ctx *pstr_ctx,
						void * pv_args, 
						tpf_sl_cb pf_evt_cb, 
                        tenu_twi_ll_type enu_ll_type,
						tstr_stack_helpers * pstr_helpers, void* pv_helpers)
{	
	twi_s32 s32_retval;
	if((pf_evt_cb != NULL) && (pstr_ctx != NULL))
	{
		if(TWI_FALSE == pstr_ctx->str_global.b_is_initialized)
		{
			twi_init_ns_global_variables(pstr_ctx);
			s32_retval = twi_nl_init(	&(pstr_ctx->str_nl_ctx),
											pstr_ctx, 
											twi_nl_cb,
											enu_ll_type,
											pstr_helpers, pv_helpers);
			if(TWI_SUCCESS == s32_retval)
			{
				pstr_ctx->str_global.b_is_initialized 	= TWI_TRUE;
				pstr_ctx->str_global.pf_sl_cb			= pf_evt_cb;
				pstr_ctx->str_global.pv_args			= pv_args;
			}
			else
			{
				NO_SEC_LOG_ERR("Failed to Send Data To Security Layer With Error = %d\r\n", s32_retval);
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
*	@brief		This is a function just pass the event to the Link Layer to handle it.
*	@param [in]	pstr_ctx			Pointer that holds the upper layer context.
*	@param [in]	pstr_ble_evt		Pointer to Structure holds the BLE event data.
*/
void twi_sl_handle_ble_evt(tstr_sl_ctx *pstr_ctx , tstr_twi_ble_evt* pstr_ble_evt)
{
	TWI_ASSERT((pstr_ble_evt != NULL) && (pstr_ctx != NULL));
	twi_nl_handle_ble_evt(&(pstr_ctx->str_nl_ctx) , pstr_ble_evt);
}
#endif

#if defined (TWI_USB_STACK_ENABLED)
/**
*	@brief		This is a function just pass the event to the Link Layer to handle it.
*	@param [in]	pstr_usb_evt		Pointer to Structure holds the USB event data.
*/
void twi_sl_handle_usb_evt(tstr_sl_ctx *pstr_ctx , tstr_twi_usb_evt* pstr_usb_evt)
{
	TWI_ASSERT((pstr_usb_evt != NULL) && (pstr_ctx != NULL));
	twi_nl_handle_usb_evt(&(pstr_ctx->str_nl_ctx) , pstr_usb_evt);
}
#endif

/**
*	@brief		This is the Network Layer send function
*	@param [in]	enu_msg_type	type of message that will be sent.
*	@param [in]	pu8_data		Pointer to data to be sent. The data will be fragmented if needed to.
*                               This buffer shall remain untouched by the application till a TWI_SL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_data_len    Data length.
*	@param [in]	pv_arg    		User argument.
*/
twi_s32 twi_sl_send_data( tstr_sl_ctx *pstr_ctx , tenu_twi_stack_msg_type enu_msg_type, twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg)
{
	twi_s32 s32_retval;
	if((enu_msg_type < TWI_STACK_INVLD_MSG) && (pu8_data != NULL) && (u16_data_len > 0)&& (pstr_ctx != NULL) )
	{
		if(TWI_TRUE == pstr_ctx->str_global.b_is_initialized)
		{
			if(TWI_STACK_CLR_MSG == enu_msg_type)
			{
				s32_retval = twi_nl_send_data(&(pstr_ctx->str_nl_ctx) , pu8_data, u16_data_len, pv_arg);
				if(s32_retval != TWI_SUCCESS)
				{
					NO_SEC_LOG_ERR("Failed to Send Data To Network Layer With Error = %d\r\n", s32_retval);
				}
			}
			else
			{
				s32_retval = TWI_ERROR_NOT_SUPPORTED_FEATURE;
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
 *	@brief:	This is the Security Manager Layer dispatcher. This is used to handle the periodic events of the security manager.
*/
void twi_sl_dispatcher(tstr_sl_ctx *pstr_ctx)
{
	TWI_ASSERT(pstr_ctx != NULL);
	twi_nl_dispatcher(&(pstr_ctx->str_nl_ctx));
}

/**
*	@brief		This is an API to unlock the BLE receive buffer in the Network Layer.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pu8_buffer_to_unlock		Pointer to buffer to unlock.
*
*	@return	    None.
*/
void twi_sl_unlock_rcv_buf( tstr_sl_ctx *pstr_ctx , twi_u8* pu8_buffer_to_unlock)
{
	twi_nl_unlock_rcv_buf( &(pstr_ctx->str_nl_ctx) , pu8_buffer_to_unlock);
}

void twi_sl_is_ready_to_send(tstr_sl_ctx* pstr_cntxt, twi_bool* pb_is_ready)
{
	twi_nl_is_ready_to_send(&pstr_cntxt->str_nl_ctx, pb_is_ready);
}

twi_bool twi_sl_is_idle(tstr_sl_ctx* pstr_cntxt)
{
	return twi_nl_is_idle(&pstr_cntxt->str_nl_ctx);
}

