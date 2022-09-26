/****************************************************************************/
/* Copyright (c) 2022 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/

/**
 * @file:	twi_usb_link_layer.c
 * @brief:	This file implements the link layer interface in the communication stack.
 */

#include "twi_usb_link_layer.h"
#include "twi_link_layer.h"
#include "twi_common.h"
#include "timer_mgmt.h"
#include "twi_system.h"

/*---------------------------------------------------------*/
/*- MODULE LOGGER CONFIGURATIONS----------------------------*/
/*---------------------------------------------------------*/
#define	USB_LINK_LAYER_LOG_ENABLE

#if defined (USB_LINK_LAYER_LOG_ENABLE)
#define USB_LINK_LAYER_LOG_DEBUG_LEVEL
#endif

/*---------------------------------------------------------*/
/*- MODULE LOGGER DEFINITION-------------------------------*/
/*---------------------------------------------------------*/
#define USB_LINK_LAYER_LOG(...)
#define USB_LINK_LAYER_LOG_ERR(...)
#define USB_LINK_LAYER_LOG_HEX(MSG, HEX_BUFFER, LEN)


#ifdef	USB_LINK_LAYER_LOG_ENABLE
	#include "twi_debug.h"
#if defined(USB_LINK_LAYER_LOG_DEBUG_LEVEL)
	#undef USB_LINK_LAYER_LOG
	#define USB_LINK_LAYER_LOG(...)								TWI_LOGGER("[_LINK_]: "__VA_ARGS__)

	#undef  USB_LINK_LAYER_LOG_HEX
    #define USB_LINK_LAYER_LOG_HEX(MSG, HEX_BUFFER, LEN)  		TWI_DUMP_BUF("[_LINK_]: "MSG,HEX_BUFFER,LEN)
#endif
#if defined(USB_LINK_LAYER_LOG_DEBUG_LEVEL) || defined(USB_LINK_LAYER_LOG_ERR_LEVEL)
	#undef USB_LINK_LAYER_LOG_ERR
	#define	USB_LINK_LAYER_LOG_ERR(...)							do{												\
																TWI_LOGGER_ERR("[_LINK_]: "__VA_ARGS__);		\
															}while(0)
	#undef  USB_LINK_LAYER_LOG_HEX
    #define USB_LINK_LAYER_LOG_HEX(MSG, HEX_BUFFER, LEN)  		TWI_DUMP_BUF("[_LINK_]: "MSG,HEX_BUFFER,LEN)
#endif
#endif

/*---------------------------------------------------------*/
/*- MODULE LOCAL MACROS DEFINITION-------------------------*/
/*---------------------------------------------------------*/
#define TWI_SEND_TIMEOUT_MS								(10000)			/** @brief: 1 Second. This is the maximum allowed timeout for the data/error send. It's used to propagate send error to upper layers if the @ref: TWI_USBD_TX_DONE is not received in less than this time window*/
#define TWI_STACK_SPECS_COMMANDS_TIMEOUT_MS				(10000)			/** @brief: 1 Seconds. This is the time window allowed For the Mobile side and the Firmware Side to exchange their stack specification info.*/

#define CONTROL_MESSAGE_MARKER							(1)				
#define DATA_MESSAGE_MARKER								(0)

#define MESSAGE_TYPE_MARKER_INDEX						(0)
#define DATA_MESSAGE_ERR_CODE_INDEX						(1)
#define DATA_MESSAGE_INDEX								(1)
#define DATA_MESSAGE_ERR_DATA_INDEX						(2)

#define MY_STACK_SPECS_MAJOR_VER						(2)
#define MY_STACK_SPECS_MINOR_VER						(0)
#define FW_STACK_SPECS_VERSION_SIZE						(2)	/*1 Byte For Major, 1 Byte For Minor*/
#define FW_STACK_SPECS_MAX_CTU_SIZE						(4)	/*4 Bytes for the Max CTU*/
#define FW_STACK_SPECS_DATA_SIZE						(FW_STACK_SPECS_VERSION_SIZE + FW_STACK_SPECS_MAX_CTU_SIZE)	/*1 For Major Version, 1 For Minor Version, 4 For The Supported MAX CTU*/
#define FIND_CURRENT_STACK_SEPCS_BUFF_IDX(IDX)			(FW_STACK_SPECS_VERSION_SIZE + IDX)

#define DATA_MESSAGE_MARKER_SIZE						(sizeof(twi_u8))
#define DATA_MESSAGE_ERR_CODE_SIZE						(sizeof(twi_u8))

#define REPORT_ID_ELEMENT_SIZE							(sizeof(twi_u8))
#define DATA_LENGTH_ELEMENT_SIZE						(sizeof(twi_u8))

#if defined (TWI_USE_USB_AS_HID) && defined (TWI_USE_USB_AS_CDC)
#error "Define Only One Type of USB HAL"
#endif

#if defined (TWI_USE_USB_AS_HID)
#define TWI_LL_USB_MAX_TRANSMIT_BUFF_LEN				(TWI_LL_USB_MAX_BUFF_SIZE - TWI_LL_MESSAGE_MARKER_SIZE - REPORT_ID_ELEMENT_SIZE - DATA_LENGTH_ELEMENT_SIZE)
#elif defined (TWI_USE_USB_AS_CDC)
#define TWI_LL_USB_MAX_TRANSMIT_BUFF_LEN				(TWI_LL_USB_MAX_BUFF_SIZE - TWI_LL_MESSAGE_MARKER_SIZE)
#else
#error "Not Supported USB HAL Type!"
#endif
/**
 * 	@brief: The Formation of the Buffers to Send/ Receive over the USB.
 *	@ref: 	TWI_USBD_RX_DONE_BYTES_COUNT_TRIGGER. 
 * 	@note: 	The Macro that controls the Maximum Buffer Size to be sent/received Shall be defined in the above macro. 
 * 
 */
/*
 *				Error Data Receive Buffer Format (33 Bytes)
 * |--------------------|-------------------|---------------------------|
 * |					|					|							|
 * |					|					|							|
 * |--<- MSG MARKER	->--|--<- ERR CODE ->---|--<- ERR MESSAGE DATA ->---|
 * |		[1 Byte]	|		[1 Byte]	|			[n Bytes]		|
 * |					|					|							|
 * |--------------------|-------------------|---------------------------|
 * 
 * 				 Data Receive Buffer Format (64 Bytes)
 * |--------------------|---------------------------|
 * |					|							|
 * |					|							|
 * |--<- MSG MARKER	->--|----<- MESSAGE DATA ->-----|
 * |		[1 Byte]	|			[n Bytes]		|
 * |					|							|
 * |--------------------|---------------------------|
 * 
 * 				 Data To Send Buffer Format (64 Bytes)
 * |--------------------|---------------------------|
 * |					|							|
 * |					|							|
 * |--<- MSG MARKER	->--|----<- MESSAGE DATA ->-----|
 * |		[1 Byte]	|			[n Bytes]		|
 * |					|							|
 * |--------------------|---------------------------|
*/


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

/**
 *	@brief			                This function initializes all the link layer global variables used.
 *	@param[in]  pstr_ctx: 			a pointer to the usb context.
*/
static void twi_init_ll_global_variables(tstr_usb_ll_ctx * pstr_ctx, tstr_stack_helpers * pstr_helper, void* pv_helpers);

/**
 *	@brief			            			This function is used to parse the stack specs command data.
 *	@param[in]	pu8_data: 					The Stack Specifications Data.
 *	@param[in]	u16_data_length: 			The Length Of Stack Specification Data.
 *	@param[out]	pu8_formatted_data: 		Pointer to the Formatted Data which we will send to the mobile.
  *	@param[out]	pu16_formatted_data_length: Pointer to the Formatted Data Length.
 *	@return : ::TWI_TRUE if Data is Valid, TWI_FALSE If The passed array length is less than the expected formatted data size
*/
static twi_bool parse_compose_stack_specs(void* pv, twi_u8* pu8_data, twi_u16 u16_data_length, twi_u8* pu8_formatted_data, twi_u16* pu16_formatted_data_length);

/**
 *	@brief			            	This function is used as the timer callback for the @ref: gstr_stack_event_timeout.
 *	@param[in]  pv: 				a pointer to the needed user data. This is reserved for future development needs
*/
static void twi_stack_specs_timer_timeout_cb (void* pv);
/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/

/**
 *	@brief			                This function initializes all the link layer global variables used.
*/
static void twi_init_ll_global_variables(tstr_usb_ll_ctx * pstr_ctx, tstr_stack_helpers * pstr_helper, void* pv_helpers)
{
	pstr_ctx->str_global.b_need_to_disconnect					= TWI_FALSE;
	pstr_ctx->str_global.b_is_initialized						= TWI_FALSE;
	pstr_ctx->str_global.pf_ll_cb	 							= NULL;	
	pstr_ctx->str_global.pv_user_arg							= NULL;
	pstr_ctx->str_global.u16_err_buf_length						= 0;
	pstr_ctx->str_global.u16_data_buf_length					= 0;
	pstr_ctx->str_global.u16_data_to_send_buf_length			= 0;
	pstr_ctx->str_global.enu_link_layer_state					= USB_LINK_LAYER_STATE_WAITING_TO_CONNECT;
	pstr_ctx->str_global.b_is_stack_specs_received 				= TWI_FALSE;
	pstr_ctx->str_global.b_is_sending_stack_specs				= TWI_FALSE; 
	pstr_ctx->pstr_stack_helpers								= pstr_helper;
	pstr_ctx->pv_stack_helpers									= pv_helpers;

	TWI_MEMSET(pstr_ctx->str_global.au8_err_send_buff,  			0, 	sizeof(pstr_ctx->str_global.au8_err_send_buff));
	TWI_MEMSET(pstr_ctx->str_global.au8_data_rcv_buff, 				0, 	sizeof(pstr_ctx->str_global.au8_data_rcv_buff));
	TWI_MEMSET(pstr_ctx->str_global.au8_data_send_buf, 				0, 	sizeof(pstr_ctx->str_global.au8_data_send_buf));
	TWI_MEMSET(&(pstr_ctx->str_global.str_stack_event_timeout), 	0, 	sizeof(pstr_ctx->str_global.str_stack_event_timeout));
}

/**
 *	@brief			            			This function is used to parse the stack specs command data.
 *	@param[in]	pu8_data: 					The Stack Specifications Data.
 *	@param[in]	u16_data_length: 			The Length Of Stack Specification Data.
 *	@param[out]	pu8_formatted_data: 		Pointer to the Formatted Data which we will send to the mobile.
  *	@param[out]	pu16_formatted_data_length: Pointer to the Formatted Data Length.
 *	@return : ::TWI_TRUE if Data is Valid, TWI_FALSE If The passed array length is less than the expected formatted data size
*/
static twi_bool parse_compose_stack_specs(void* pv, twi_u8* pu8_data, twi_u16 u16_data_length, twi_u8* pu8_formatted_data, twi_u16* pu16_formatted_data_length)
{
	/*
		Stack Specifications (STACK_SPECS) Data Format:
		|-------------------|-------------------|-----------------------------------------------------------------------------------|
		| 		1 Byte		|		1 Byte		|									4 Bytes											|
		|	Major Version 	|	Minor Version	|	Maximum Supported CTU "Maximum Size of Unfragmented Packet to send or receive"	|
		|-------------------|-------------------|-----------------------------------------------------------------------------------|
	*/

	twi_bool b_retval							= TWI_TRUE;
	twi_u8	u8_looping_index;
	twi_u16 u16_current_data_index 				= 0;
	twi_u16 u16_least_significant_ctu_peer 		= 0;
	twi_u16 u16_most_significant_ctu_peer 		= 0;
	twi_u32 u32_ctu_for_peer 					= 0;
	twi_u32 u32_ctu_for_mine 					= 0;
	twi_u32 u32_min_ctu_from_both_ctu_values 	= 0;

	/*Input Parameters Validation*/
	if ((pu8_data != NULL) && (u16_data_length != 0)) 
	{
		/*this is the parsing section. This means that we received a STACK SPECS command from the other side.*/
		if ((pu8_data[0] == MY_STACK_SPECS_MAJOR_VER) && (pu8_data[1] == MY_STACK_SPECS_MINOR_VER))
		{
			u16_least_significant_ctu_peer = TWO_BYTE_CONCAT(pu8_data[FIND_CURRENT_STACK_SEPCS_BUFF_IDX(1)], pu8_data[FIND_CURRENT_STACK_SEPCS_BUFF_IDX(0)]);
			u16_most_significant_ctu_peer = TWO_BYTE_CONCAT(pu8_data[FIND_CURRENT_STACK_SEPCS_BUFF_IDX(3)], pu8_data[FIND_CURRENT_STACK_SEPCS_BUFF_IDX(2)]);
			u32_ctu_for_peer = TWO_16BITS_CONCAT(u16_most_significant_ctu_peer, u16_least_significant_ctu_peer);
			u32_ctu_for_mine = MAX_PKT_SZ;

			USB_LINK_LAYER_LOG("STACK_SPECS COMMAND DATA: Major = %d, Minor = %d, CTU Mine = %d, CTU Peer = %d\r\n", pu8_data[0], pu8_data[1], u32_ctu_for_mine, u32_ctu_for_peer);

			/*Getting the Minimum of Both CTU Values for Mobile and FW Side*/
			u32_min_ctu_from_both_ctu_values = (u32_ctu_for_mine < u32_ctu_for_peer) ? (u32_ctu_for_mine) : (u32_ctu_for_peer);
		}
		else
		{
			/*Invalid Data Length!*/
			USB_LINK_LAYER_LOG_ERR("Invalid Major Or Minor Versions! Major = %d, Minor = %d\r\n", pu8_data[0], pu8_data[1]);
			b_retval = TWI_FALSE;
		}
	}
	else 
	{
		/*this is the composing section. This means that we send a STACK SPECS command to the other side.*/
		USB_LINK_LAYER_LOG("Composing Stack Specs Command!\r\n");
		u32_min_ctu_from_both_ctu_values = MAX_PKT_SZ;
	}

	if ((pu8_formatted_data != NULL) && (pu16_formatted_data_length != NULL))
	{
		twi_u16 u16_max_buff_length = *pu16_formatted_data_length;

		if (u16_max_buff_length >= FW_STACK_SPECS_DATA_SIZE)
		{
			/*Format Both Major and Minor Versions.*/
			pu8_formatted_data[u16_current_data_index++] = MY_STACK_SPECS_MAJOR_VER;
			pu8_formatted_data[u16_current_data_index++] = MY_STACK_SPECS_MINOR_VER;

			for (u8_looping_index = 0; u8_looping_index < FW_STACK_SPECS_MAX_CTU_SIZE; u8_looping_index++)
			{
				/*Copy The Minimum of both Firmware & Mobile CTU Size. With taking into consideration of bytes flipping over BLE*/
				pu8_formatted_data[u8_looping_index + u16_current_data_index] = GET_BYTE_STATUS(u32_min_ctu_from_both_ctu_values, u8_looping_index);
			}
			/*Updating The Current Data Index to be equal to the total data length. And Store its value inside the pointer passed to the function*/
			u16_current_data_index += sizeof(u32_min_ctu_from_both_ctu_values);
			*pu16_formatted_data_length = u16_current_data_index;
		}
	}
	
	return b_retval;
}

/**
 *	@brief			            	This function is used as the timer callback for the @ref: gstr_stack_event_timeout.
 *	@param[in]  pv: 				a pointer to the needed user data. This is reserved for future development needs
*/
static void twi_stack_specs_timer_timeout_cb (void* pv)
{
	tstr_usb_ll_ctx * pstr_ctx = (tstr_usb_ll_ctx*) pv;
	USB_LINK_LAYER_LOG_ERR("Stack Specs Timeout Fired!\r\n");
	pstr_ctx->pstr_stack_helpers->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);
	/*pstr_ctx->str_global.b_need_to_disconnect = TWI_TRUE;*/
}
/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/

/**
*	@brief		This is the init function. It shall be called before any other API.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in] pv_args				Void pointer fot the user argument
*	@param [in]	pf_evt_cb		    Pointer to Function that is called by the usb stack to pass events to the application.
*	@param [in]	pstr_helpers		Pointer to structure of the whole layer helper APIs.
*	@param [in]	pv_helpers		    Pointer to void to be passed to helpers Apis.
*/
twi_s32 twi_usb_ll_init(tstr_usb_ll_ctx * pstr_ctx, void * pv_args, tpf_ll_cb pf_evt_cb, tstr_stack_helpers * pstr_helpers, void* pv_helpers)
{
	twi_s32 s32_retval = TWI_SUCCESS;
	if((pf_evt_cb != NULL) && (pstr_ctx != NULL) && (pf_evt_cb != NULL) && (pstr_helpers != NULL))
	{
		if(TWI_FALSE == pstr_ctx->str_global.b_is_initialized)
		{
			twi_init_ll_global_variables(pstr_ctx, pstr_helpers, pv_helpers);
	
			pstr_ctx->str_global.b_is_initialized 			= TWI_TRUE;
			pstr_ctx->str_global.pf_ll_cb					= pf_evt_cb;
			pstr_ctx->str_global.pv_args					= pv_args;		
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

/**
*	@brief		This is a function that called to pass the different USB events to the link layer.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	enu_usbd_evt		Enumeration of the Current USB Event.
*/
void twi_usb_ll_handle_usb_evt(tstr_usb_ll_ctx *pstr_ctx, twi_usbd_events_t enu_usbd_evt)
{
	twi_s32 			s32_retval 					= TWI_SUCCESS;
	twi_bool			b_is_need_to_propagate_evt 	= TWI_FALSE;
	twi_u32				u32_receive_buff_length		= sizeof(pstr_ctx->str_global.au8_data_rcv_buff);
	tstr_twi_ll_evt str_notify_ll_evt;

	TWI_MEMSET(&str_notify_ll_evt, 0, sizeof(tstr_twi_ll_evt));

    switch(enu_usbd_evt)
	{
		case TWI_USBD_RX_DONE:
		{
			USB_LINK_LAYER_LOG("TWI_USBD_RX_DONE\r\n");
			s32_retval = pstr_ctx->pstr_stack_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_receive(pstr_ctx->pv_stack_helpers, pstr_ctx->str_global.au8_data_rcv_buff, &u32_receive_buff_length);
			USB_LINK_LAYER_LOG("u32_receive_buff_length = %d, s32_retval = %d \r\n", u32_receive_buff_length, s32_retval);
			if ((u32_receive_buff_length > 0) && (s32_retval == TWI_SUCCESS))
			{
				if (pstr_ctx->str_global.b_is_stack_specs_received == TWI_FALSE)
				{
					USB_LINK_LAYER_LOG("Stack Specs Not Received!\r\n");
					twi_bool b_retval = TWI_FALSE;
#if defined (TWI_USB_DEVICE)
					twi_u8 	au8_formatted_data[FW_STACK_SPECS_DATA_SIZE]; /*1 Byte For Major Version, 1 Byte For Minor Version, 4 Bytes For The Max CTU*/
					twi_u16 u16_formatted_data_length = sizeof(au8_formatted_data);
#endif			
					if ((CONTROL_MESSAGE_MARKER == pstr_ctx->str_global.au8_data_rcv_buff[MESSAGE_TYPE_MARKER_INDEX]) && (TWI_STACK_SPECS_CMD_ERR_CODE == pstr_ctx->str_global.au8_data_rcv_buff[DATA_MESSAGE_ERR_CODE_INDEX]))
					{
						USB_LINK_LAYER_LOG("Start Parse/ Compose Stack Specs!\r\n");
#if defined (TWI_USB_DEVICE)
						b_retval = parse_compose_stack_specs(pstr_ctx->str_global.pv_args, &(pstr_ctx->str_global.au8_data_rcv_buff[2]), (u32_receive_buff_length - 2), au8_formatted_data, &u16_formatted_data_length);
#elif defined(TWI_USB_HOST)
						b_retval = parse_compose_stack_specs(pstr_ctx->str_global.pv_args, &(pstr_ctx->str_global.au8_data_rcv_buff[2]), (u32_receive_buff_length - 2), NULL, NULL);
#endif
						if (b_retval == TWI_TRUE)
						{
							TWI_ASSERT(TWI_SUCCESS == pstr_ctx->pstr_stack_helpers->pf_stop_timer(pstr_ctx->pv_stack_helpers, &(pstr_ctx->str_global.str_stack_event_timeout)));
#if defined (TWI_USB_HOST)
							pstr_ctx->str_global.enu_link_layer_state = USB_LINK_LAYER_STATE_READY;
#elif defined (TWI_USB_DEVICE)
							USB_LINK_LAYER_LOG_HEX("FORMATTED ARRAY: ", au8_formatted_data, u16_formatted_data_length);
							pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_TRUE;
							TWI_ASSERT(TWI_SUCCESS == twi_usb_ll_send_error(pstr_ctx, TWI_STACK_SPECS_CMD_ERR_CODE, au8_formatted_data, u16_formatted_data_length));
#endif
							pstr_ctx->str_global.b_is_stack_specs_received = TWI_TRUE;
						}
					}
				}
				else
				{
					if (pstr_ctx->str_global.au8_data_rcv_buff[MESSAGE_TYPE_MARKER_INDEX] == CONTROL_MESSAGE_MARKER)
					{
						USB_LINK_LAYER_LOG("Control Message Received!\r\n");
						if (pstr_ctx->str_global.au8_data_rcv_buff[DATA_MESSAGE_ERR_CODE_INDEX] == TWI_STACK_SPECS_CMD_ERR_CODE)
						{
							USB_LINK_LAYER_LOG_ERR("Can't Parse The Stack Specs Command Twice! Need to Disconnect Now!!\r\n");
							pstr_ctx->str_global.b_need_to_disconnect = TWI_TRUE;
						}
						else
						{
							USB_LINK_LAYER_LOG("Received Error Data With Error Code = %d, Buffer Length = %d, Overhead Size = %d!\r\n", (tenu_stack_err_code)(pstr_ctx->str_global.au8_data_rcv_buff[DATA_MESSAGE_ERR_CODE_INDEX]), (u32_receive_buff_length), (DATA_MESSAGE_MARKER_SIZE + DATA_MESSAGE_ERR_CODE_SIZE));
							str_notify_ll_evt.enu_event										= TWI_LL_RCV_ERROR_EVT;
							str_notify_ll_evt.uni_data.str_rcv_error_evt.enu_err_code		= (tenu_stack_err_code)(pstr_ctx->str_global.au8_data_rcv_buff[DATA_MESSAGE_ERR_CODE_INDEX]);
							str_notify_ll_evt.uni_data.str_rcv_error_evt.pu8_err_data		= &(pstr_ctx->str_global.au8_data_rcv_buff[DATA_MESSAGE_ERR_DATA_INDEX]);
							str_notify_ll_evt.uni_data.str_rcv_error_evt.u16_err_data_len	= (u32_receive_buff_length)-(DATA_MESSAGE_MARKER_SIZE + DATA_MESSAGE_ERR_CODE_SIZE);
							b_is_need_to_propagate_evt 										= TWI_TRUE;
						}
					}
					else if (pstr_ctx->str_global.au8_data_rcv_buff[MESSAGE_TYPE_MARKER_INDEX] == DATA_MESSAGE_MARKER)
					{
						USB_LINK_LAYER_LOG("Data Message Received!\r\n");
						str_notify_ll_evt.enu_event									= TWI_LL_RCV_DATA_EVT;
						str_notify_ll_evt.uni_data.str_rcv_data_evt.pu8_data		= &(pstr_ctx->str_global.au8_data_rcv_buff[DATA_MESSAGE_INDEX]);
						str_notify_ll_evt.uni_data.str_rcv_data_evt.u16_data_len	= (u32_receive_buff_length)-(DATA_MESSAGE_MARKER_SIZE);
						b_is_need_to_propagate_evt 									= TWI_TRUE;
					}
					else
					{
						/*Log inidicates unhandled*/
						USB_LINK_LAYER_LOG_ERR("Invalid Message Marker = %d\r\n", pstr_ctx->str_global.au8_data_rcv_buff[MESSAGE_TYPE_MARKER_INDEX]);
					}
				}
			}
			else
			{
				USB_LINK_LAYER_LOG_ERR("Failed To Receive Data Over USB! Error Code = %d, Data Length = %d\r\n", s32_retval, u32_receive_buff_length);
			}

			break;
		}

		case TWI_USBD_TX_DONE:
		{
			USB_LINK_LAYER_LOG("TWI_USBD_TX_DONE\r\n");

#if defined(TWI_USB_HOST)
			if (pstr_ctx->str_global.b_is_stack_specs_received == TWI_FALSE)
			{
				USB_LINK_LAYER_LOG("Stack specs is sent 1\r\n");
				pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_FALSE;
				pstr_ctx->str_global.enu_link_layer_state 		= USB_LINK_LAYER_STATE_CONNECTED;
				USB_LINK_LAYER_LOG("Waiting for stack specs response\r\n");
				/*Start the STACK_SPECS Command Timer.*/
				TWI_ASSERT(TWI_SUCCESS == pstr_ctx->pstr_stack_helpers->pf_start_timer(pstr_ctx->pv_stack_helpers, &(pstr_ctx->str_global.str_stack_event_timeout), (twi_s8*)"Stack Specs Exchange", TWI_TIMER_TYPE_ONE_SHOT, TWI_STACK_SPECS_COMMANDS_TIMEOUT_MS, twi_stack_specs_timer_timeout_cb, (void*) pstr_ctx));
			}
			else 
#endif
			{
				if(pstr_ctx->str_global.b_is_sending_stack_specs == TWI_FALSE)
				{
					USB_LINK_LAYER_LOG("Sending for stack specs response\r\n");

					pstr_ctx->str_global.enu_link_layer_state					= USB_LINK_LAYER_STATE_READY;

					str_notify_ll_evt.enu_event									= TWI_LL_SEND_STATUS_EVT;
					str_notify_ll_evt.uni_data.str_send_stts_evt.b_is_success	= TWI_TRUE;
					str_notify_ll_evt.uni_data.str_send_stts_evt.pv_user_arg	= pstr_ctx->str_global.pv_user_arg;
					b_is_need_to_propagate_evt									= TWI_TRUE;
				}
				else
				{
					USB_LINK_LAYER_LOG("Stack specs is sent 2\r\n");
					pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_FALSE;
#if defined (TWI_USB_DEVICE)
					pstr_ctx->str_global.enu_link_layer_state		= USB_LINK_LAYER_STATE_READY;
#endif
				}
			}
			break;
		}

		case TWI_USBD_TX_FAIL:
		{
			USB_LINK_LAYER_LOG("TWI_USBD_TX_FAIL\r\n");
#if defined(TWI_USB_HOST)
			if (pstr_ctx->str_global.b_is_stack_specs_received == TWI_FALSE)
			{
				pstr_ctx->pstr_stack_helpers->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);
				pstr_ctx->str_global.b_need_to_disconnect 		= TWI_TRUE;
				pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_FALSE;
			}
			else
#endif
			{
				if(pstr_ctx->str_global.b_is_sending_stack_specs == TWI_FALSE)
				{
					pstr_ctx->str_global.enu_link_layer_state					= USB_LINK_LAYER_STATE_READY;

					str_notify_ll_evt.enu_event									= TWI_LL_SEND_STATUS_EVT;
					str_notify_ll_evt.uni_data.str_send_stts_evt.b_is_success	= TWI_FALSE;
					str_notify_ll_evt.uni_data.str_send_stts_evt.pv_user_arg	= pstr_ctx->str_global.pv_user_arg;
					b_is_need_to_propagate_evt									= TWI_TRUE;
				}
				else
				{
#if defined (TWI_USB_DEVICE)
					pstr_ctx->str_global.enu_link_layer_state		= USB_LINK_LAYER_STATE_CONNECTED;
#endif
					pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_FALSE;
					pstr_ctx->str_global.b_need_to_disconnect		= TWI_TRUE;
				}
			}
			break;
		}
		
		case TWI_USBD_PORT_OPEN:
		{
			USB_LINK_LAYER_LOG("TWI_USBD_PORT_OPEN\r\n");
			pstr_ctx->str_global.b_is_stack_specs_received 	= TWI_FALSE;
			pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_FALSE;
			pstr_ctx->str_global.enu_link_layer_state 		= USB_LINK_LAYER_STATE_CONNECTED;

#if defined (TWI_USB_DEVICE)
			/*Start the STACK_SPECS Command Timer.*/
			TWI_ASSERT(TWI_SUCCESS == pstr_ctx->pstr_stack_helpers->pf_start_timer(pstr_ctx->pv_stack_helpers, &(pstr_ctx->str_global.str_stack_event_timeout), (twi_s8*)"Stack Specs Exchange", TWI_TIMER_TYPE_ONE_SHOT, TWI_STACK_SPECS_COMMANDS_TIMEOUT_MS, twi_stack_specs_timer_timeout_cb, (void*) pstr_ctx));
#elif defined (TWI_USB_HOST)
			twi_u8 	au8_formatted_data[FW_STACK_SPECS_DATA_SIZE]; /*1 For Major Version, 1 For Minor Version, 4 For The Max CTU*/
			twi_u16 u16_formatted_data_length 	= sizeof(au8_formatted_data);

			TWI_ASSERT(TWI_TRUE == parse_compose_stack_specs(pstr_ctx->str_global.pv_args, NULL, 0, au8_formatted_data, &u16_formatted_data_length));
			pstr_ctx->str_global.b_is_sending_stack_specs 	= TWI_TRUE;
			USB_LINK_LAYER_LOG("STACK SPECS BUFFER FORMATTEED!\r\n");
			for(int index = 0; index < u16_formatted_data_length; index++)
			{
				USB_LINK_LAYER_LOG("0x%x ", au8_formatted_data[index]);
			}
			USB_LINK_LAYER_LOG("\r\n");
			TWI_ASSERT(TWI_SUCCESS == twi_usb_ll_send_error(pstr_ctx, TWI_STACK_SPECS_CMD_ERR_CODE, au8_formatted_data, u16_formatted_data_length));	
#endif
			break;
		}

		case TWI_USBD_PORT_CLOSE:
		{
			USB_LINK_LAYER_LOG("TWI_USBD_PORT_CLOSE\r\n");
			pstr_ctx->str_global.enu_link_layer_state = USB_LINK_LAYER_STATE_WAITING_TO_CONNECT;
			break;
		}

		default:
		{
			USB_LINK_LAYER_LOG_ERR("Unhandled Event = %d\r\n", enu_usbd_evt);
			break;
		}	
	}
	
	if(b_is_need_to_propagate_evt == TWI_TRUE)
	{
		str_notify_ll_evt.pv_args = (void*) pstr_ctx->str_global.pv_args;

		TWI_ASSERT(pstr_ctx->str_global.pf_ll_cb != NULL);
		
		pstr_ctx->str_global.pf_ll_cb(&str_notify_ll_evt);
	}
}
/**
*	@brief		This is the Link Layer send data function
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pu8_data		Pointer to data to be sent.
*                               This buffer shall remain untouched by the application till a TWI_LL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_data_len    Data Buffer Length.
*	@param [in]	pv_arg    		User argument.
*/
twi_s32 twi_usb_ll_send_data(tstr_usb_ll_ctx * pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg)
{
	USB_LINK_LAYER_LOG("***** twi_usb_ll_send_data ***** With Length = %d\r\n", u16_data_len);
	twi_s32 s32_retval = TWI_SUCCESS;
	twi_u16 u16_idx;
	if((pstr_ctx != NULL) && (pu8_data != NULL) && (u16_data_len > 0)&& (u16_data_len <= TWI_LL_USB_MAX_TRANSMIT_BUFF_LEN))
	{
		if(TWI_TRUE == pstr_ctx->str_global.b_is_initialized)
		{
			if(pstr_ctx->str_global.enu_link_layer_state == USB_LINK_LAYER_STATE_READY)
			{
				pstr_ctx->str_global.enu_link_layer_state 					= USB_LINK_LAYER_STATE_SEND_IN_PROGRESS;
				u16_idx 													= MESSAGE_TYPE_MARKER_INDEX;
				pstr_ctx->str_global.au8_data_send_buf[u16_idx++] 			= (twi_u8) DATA_MESSAGE_MARKER;					
				pstr_ctx->str_global.u16_data_buf_length					= (u16_data_len + DATA_MESSAGE_MARKER_SIZE);
				pstr_ctx->str_global.pv_user_arg							= pv_arg;

				TWI_MEMCPY(&(pstr_ctx->str_global.au8_data_send_buf[u16_idx]), pu8_data, u16_data_len);
#if defined (TWI_USB_HOST)
				USB_LINK_LAYER_LOG("Dump Buffer To Send in Link Layer With Size = %d\r\n", u16_data_len);
				for(int index = 0; index < u16_data_len; index++)
				{
					USB_LINK_LAYER_LOG("%d ", pu8_data[index]);
				}
				USB_LINK_LAYER_LOG("\r\n");
#endif
				s32_retval = pstr_ctx->pstr_stack_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_send((void*) pstr_ctx->pv_stack_helpers, (const void*) (pstr_ctx->str_global.au8_data_send_buf), (twi_u32) (pstr_ctx->str_global.u16_data_buf_length));		/*1 Byte for the Message Marker*/
				if(TWI_SUCCESS != s32_retval)
				{
					pstr_ctx->str_global.enu_link_layer_state 	= USB_LINK_LAYER_STATE_READY;
					USB_LINK_LAYER_LOG("Failed to write Data On USB With Error = %d\r\n", s32_retval);
				}
			}
			else
			{
				USB_LINK_LAYER_LOG("Trying To Send Data In Invalid State = %d\r\n", pstr_ctx->str_global.enu_link_layer_state);
				s32_retval = TWI_ERROR_INVALID_STATE;
			}
		}
		else
		{
			USB_LINK_LAYER_LOG("TWI_ERROR_NOT_INITIALIZED\r\n");
			s32_retval = TWI_ERROR_NOT_INITIALIZED;
		}
	}
	else
	{
		USB_LINK_LAYER_LOG("TWI_ERROR_INVALID_ARGUMENTS\r\n");
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}
	return s32_retval;
}

/**
*	@brief		This is the Link Layer send error function
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	enu_err_code	    Error code to be sent.
*	@param [in]	pu8_error_data		Pointer to error data to be sent.
*                                   This buffer shall remain untouched by the application till a TWI_LL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_error_len       Error Data Buffer length.
*/
twi_s32 twi_usb_ll_send_error(tstr_usb_ll_ctx * pstr_ctx ,tenu_stack_err_code enu_err_code, twi_u8* pu8_error_data, twi_u16 u16_error_len)
{
	USB_LINK_LAYER_LOG("***** twi_usb_ll_send_error *****\r\n");
	twi_s32 s32_retval = TWI_SUCCESS;
	twi_u16 u16_idx;
	if((pstr_ctx != NULL) && (enu_err_code < TWI_STACK_INVALID_ERR_CODE) && (pu8_error_data != NULL) && (u16_error_len > 0) )
	{
		if(TWI_TRUE == pstr_ctx->str_global.b_is_initialized)
		{
			/*Check that the Current State is Connected and wait to send stack specs || we finalized the stack specs hand shake command and the data flow is going.*/
			if((pstr_ctx->str_global.enu_link_layer_state == USB_LINK_LAYER_STATE_CONNECTED) || (pstr_ctx->str_global.enu_link_layer_state == USB_LINK_LAYER_STATE_READY))
			{
				pstr_ctx->str_global.enu_link_layer_state 			= USB_LINK_LAYER_STATE_SEND_IN_PROGRESS;
				u16_idx 											= MESSAGE_TYPE_MARKER_INDEX;
				pstr_ctx->str_global.au8_err_send_buff[u16_idx++] 	= (twi_u8) CONTROL_MESSAGE_MARKER;						
				pstr_ctx->str_global.au8_err_send_buff[u16_idx++] 	= (twi_u8) enu_err_code;		
				pstr_ctx->str_global.u16_data_to_send_buf_length	= (u16_error_len + DATA_MESSAGE_MARKER_SIZE + DATA_MESSAGE_ERR_CODE_SIZE);

				TWI_MEMCPY(&(pstr_ctx->str_global.au8_err_send_buff[u16_idx]), pu8_error_data, u16_error_len);

				s32_retval = pstr_ctx->pstr_stack_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_send((void*)pstr_ctx->pv_stack_helpers, (const void*) (pstr_ctx->str_global.au8_err_send_buff), (twi_u32) (pstr_ctx->str_global.u16_data_to_send_buf_length));	/*1 Byte for the Message Marker & 1 Byte for the Error Code.*/
				if(TWI_SUCCESS != s32_retval)
				{
					USB_LINK_LAYER_LOG_ERR("Failed To Write on CC With Error = %d\r\n", s32_retval);
					if(pstr_ctx->str_global.b_is_sending_stack_specs == TWI_TRUE)
					{
						pstr_ctx->str_global.enu_link_layer_state = USB_LINK_LAYER_STATE_CONNECTED;
					}
					else
					{
						pstr_ctx->str_global.enu_link_layer_state = USB_LINK_LAYER_STATE_READY;
					}
				}
			}
			else
			{
				s32_retval = TWI_ERROR_INVALID_STATE;
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
*	@brief		This is the Link Layer dispatcher. This is used to handle the periodic events of the link layer.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*/
void twi_usb_ll_dispatcher(tstr_usb_ll_ctx * pstr_ctx)
{
	pstr_ctx->pstr_stack_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_dispatch(pstr_ctx->pv_stack_helpers);
	if(pstr_ctx->str_global.b_need_to_disconnect == TWI_TRUE)
	{
		pstr_ctx->str_global.b_need_to_disconnect = TWI_FALSE;
		pstr_ctx->pstr_stack_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_stop(pstr_ctx->pv_stack_helpers);
		pstr_ctx->pstr_stack_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_dispatch(pstr_ctx->pv_stack_helpers);
	}
}

/**
*	@brief		This is an API to get the MTU size agreed upon in the connection.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@return	    The MTU size.
*/
twi_u16 twi_usb_ll_get_mtu_size(void)
{
	return (TWI_LL_USB_MAX_TRANSMIT_BUFF_LEN);
}

void twi_usb_ll_is_ready_to_send(tstr_usb_ll_ctx* pstr_cntxt, twi_bool* pb_is_ready)
{
	if(pstr_cntxt->str_global.enu_link_layer_state == USB_LINK_LAYER_STATE_READY)
	{
		*pb_is_ready = TWI_TRUE;
	}
}

twi_bool twi_usb_ll_is_idle(tstr_usb_ll_ctx* pstr_cntxt)
{	
	twi_bool b_is_idle = TWI_FALSE;
	
	if(pstr_cntxt->str_global.enu_link_layer_state == (tenu_usb_link_layer_status)USB_LINK_LAYER_STATE_WAITING_TO_CONNECT)
	{
		b_is_idle = TWI_TRUE;
	}

	return b_is_idle;
}

