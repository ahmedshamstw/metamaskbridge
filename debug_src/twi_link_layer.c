/****************************************************************************/
/* Copyright (c) 2022 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/

/**
 * @file:	twi_link_layer.c
 * @brief:	This file implements the link layer interface in the communication stack.
 */

#include "twi_link_layer.h"
#include "twi_common.h"

/*---------------------------------------------------------*/
/*- MODULE LOCAL MACROS DEFINITION-------------------------*/
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

/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/

/*
*	@brief		This is the init function. It shall be called before any other API.
*	@param [in]	enu_ll_type			Link Layer Type.
*	@param [in]	puni_ctx			Pointer to structure of the whole layer context.
*	@param [in] pv_args				Void pointer fot the user argument
*	@param [in]	pf_evt_cb		    Pointer to Function that is called by the stack to pass events to the application.
*	@param [in]	pstr_helpers		Pointer to structure of the whole layer helper APIs.
*	@param [in]	pv_helpers		    Pointer to void to be passed to helpers Apis.
*/
twi_s32 twi_ll_init(	tenu_twi_ll_type enu_ll_type,
						tuni_ll_ctx * puni_ctx,
						void * pv_args,
						tpf_ll_cb pf_evt_cb,
						tstr_stack_helpers * pstr_helpers, void* pv_helpers)
{
	twi_s32 s32_retval = TWI_ERROR_NOT_SUPPORTED_FEATURE;
	if(TWI_USB_LL == enu_ll_type)
	{

#if defined (TWI_USB_STACK_ENABLED)
		s32_retval = twi_usb_ll_init(&(puni_ctx->str_usb), pv_args, pf_evt_cb, pstr_helpers, pv_helpers);
#endif

	}
	else if(TWI_BLE_LL == enu_ll_type)
	{

#if defined (TWI_BLE_STACK_ENABLED)
		s32_retval = twi_ble_ll_init(&(puni_ctx->str_ble), pv_args, pf_evt_cb, pstr_helpers, pv_helpers);
#endif

	}
	return s32_retval;
}

/*
*	@brief		This is a function that called to pass the different events to the link layer.
*	@param [in]	enu_ll_type		Link Layer Type.
*	@param [in]	puni_ctx		Pointer to structure of the whole layer context.
*	@param [in]	pstr_ble_evt	Pointer to Structure holds the event data.
*/
void twi_ll_handle_evt(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx, void* pstr_evt)
{
	if(TWI_USB_LL == enu_ll_type)
	{
#if defined (TWI_USB_STACK_ENABLED)
		twi_usb_ll_handle_usb_evt(&(puni_ctx->str_usb), ((tstr_twi_usb_evt*)pstr_evt)->enu_usbd_evt);
#endif	
	}
	else if(TWI_BLE_LL == enu_ll_type)
	{
#if defined (TWI_BLE_STACK_ENABLED)	
		twi_ble_ll_handle_ble_evt(&(puni_ctx->str_ble), (tstr_twi_ble_evt*)pstr_evt);
#endif
	}	
}

/*
*	@brief		This is the Link Layer send data function
*	@param [in]	enu_ll_type		Link Layer Type.
*	@param [in]	puni_ctx		Pointer to structure of the whole layer context.
*	@param [in]	pu8_data		Pointer to data to be sent.
*                               This buffer shall remain untouched by the application till a TWI_LL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_data_len    Data Buffer Length.
*	@param [in]	pv_arg    		User argument.
*/
twi_s32 twi_ll_send_data(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx, twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg)
{
	twi_s32 s32_retval = TWI_ERROR_NOT_SUPPORTED_FEATURE;
	if(TWI_USB_LL == enu_ll_type)
	{

#if defined (TWI_USB_STACK_ENABLED)
		s32_retval = twi_usb_ll_send_data(&(puni_ctx->str_usb), pu8_data, u16_data_len, pv_arg);
#endif

	}
	else if(TWI_BLE_LL == enu_ll_type)
	{

#if defined (TWI_BLE_STACK_ENABLED)
		s32_retval = twi_ble_ll_send_data(&(puni_ctx->str_ble), pu8_data, u16_data_len, pv_arg);
#endif

	}
	return s32_retval;
}

/*
*	@brief		This is the Link Layer send error function
*	@param [in]	enu_ll_type		Link Layer Type.
*	@param [in]	puni_ctx		Pointer to structure of the whole layer context.
*	@param [in]	enu_err_code	    Error code to be sent.
*	@param [in]	pu8_error_data		Pointer to error data to be sent.
*                                   This buffer shall remain untouched by the application till a TWI_LL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_error_len       Error Data Buffer length.
*/
twi_s32 twi_ll_send_error(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx, tenu_stack_err_code enu_err_code, twi_u8* pu8_error_data, twi_u16 u16_error_len)
{
	twi_s32 s32_retval = TWI_ERROR_NOT_SUPPORTED_FEATURE;
	if(TWI_USB_LL == enu_ll_type)
	{

#if defined (TWI_USB_STACK_ENABLED)
		s32_retval = twi_usb_ll_send_error(&(puni_ctx->str_usb), enu_err_code, pu8_error_data, u16_error_len);
#endif

	}
	else if(TWI_BLE_LL == enu_ll_type)
	{

#if defined (TWI_BLE_STACK_ENABLED)
		s32_retval = twi_ble_ll_send_error(&(puni_ctx->str_ble), enu_err_code, pu8_error_data, u16_error_len);
#endif

	}
	return s32_retval;
}

/*
*	@brief		This is the Link Layer dispatcher. This is used to handle the periodic events of the link layer.
*	@param [in]	enu_ll_type		Link Layer Type.
*	@param [in]	puni_ctx		Pointer to structure of the whole layer context.
*/
void twi_ll_dispatcher(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx)
{
	if(TWI_USB_LL == enu_ll_type)
	{

#if defined (TWI_USB_STACK_ENABLED)
		twi_usb_ll_dispatcher(&(puni_ctx->str_usb));
#endif

	}
	else if(TWI_BLE_LL == enu_ll_type)
	{

#if defined (TWI_BLE_STACK_ENABLED)
		twi_ble_ll_dispatcher(&(puni_ctx->str_ble));
#endif

	}
}

/*
*	@brief		This is an API to get the MTU size agreed upon in the connection.
*	@param [in]	enu_ll_type		Link Layer Type.
*	@param [in]	puni_ctx		Pointer to structure of the whole layer context.
*	@return	    The MTU size.
*/
twi_u16 twi_ll_get_mtu_size(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx)
{
	twi_u16 u16_retval = 0;

	if(TWI_USB_LL == enu_ll_type)
	{
#if defined (TWI_USB_STACK_ENABLED)
		u16_retval = twi_usb_ll_get_mtu_size();
#endif
	}
	else if(TWI_BLE_LL == enu_ll_type)
	{
#if defined (TWI_BLE_STACK_ENABLED)
		u16_retval = twi_ble_ll_get_mtu_size(&(puni_ctx->str_ble));
#endif

	}
	return u16_retval;
}


void twi_ll_is_ready_to_send(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx, twi_bool* pb_is_ready)
{
	TWI_ASSERT(puni_ctx != NULL);		
	if(TWI_USB_LL == enu_ll_type)
	{
#if defined (TWI_USB_STACK_ENABLED)		
		twi_usb_ll_is_ready_to_send(&(puni_ctx->str_usb), pb_is_ready);
#endif		
	}
	else
	{
#if defined (TWI_BLE_STACK_ENABLED)		
		twi_ble_ll_is_ready_to_send(&(puni_ctx->str_ble), pb_is_ready);
#endif
	}	
}

twi_bool twi_ll_is_idle(tenu_twi_ll_type enu_ll_type, tuni_ll_ctx * puni_ctx)
{
	twi_bool b_is_idle = TWI_TRUE;

	TWI_ASSERT(puni_ctx != NULL);

	if(TWI_USB_LL == enu_ll_type)
	{
#if defined (TWI_USB_STACK_ENABLED)		
		b_is_idle = twi_usb_ll_is_idle(&(puni_ctx->str_usb));
#endif		
		return 0;
	}
	else
	{
#if defined (TWI_BLE_STACK_ENABLED)		
		b_is_idle = twi_ble_ll_is_idle(&(puni_ctx->str_ble));
#endif	
	}

	return b_is_idle;
}
