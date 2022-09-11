/****************************************************************************/
/* Copyright (c) 2022 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/

/**
@file		    twi_usb_wallet_if.c
@brief		    this file holds the interface of USB wallet core 
*/

//***********************************************************
/*- INCLUDES ----------------------------------------------*/
//***********************************************************

#include "twi_common.h"
#include "twi_usb_wallet_if.h"
#include "twi_apdu_parser_composer.h"
#include<stdlib.h>

/*---------------------------------------------------------*/
/*- LOCAL MACROS ------------------------------------------*/
/*---------------------------------------------------------*/

#define TWI_USB_WALLET_IF_ERR(...)
#define TWI_USB_WALLET_IF_INFO(...)
#define TWI_USB_WALLET_IF_DBG(...)

#define TWI_USB_WALLET_IF_LOG_LEVEL_NONE  (0)
#define TWI_USB_WALLET_IF_LOG_LEVEL_ERR   (1)
#define TWI_USB_WALLET_IF_LOG_LEVEL_INFO  (2)
#define TWI_USB_WALLET_IF_LOG_LEVEL_DBG   (3)

#define TWI_USB_WALLET_IF_LOG_LEVEL   TWI_USB_WALLET_IF_LOG_LEVEL_DBG

#define TWI_USB_WALLET_IF_PRINT(...) 		do{TWI_LOGGER(__VA_ARGS__)}while(0)

#if (TWI_USB_WALLET_IF_LOG_LEVEL >= TWI_USB_WALLET_IF_LOG_LEVEL_ERR)

	#undef TWI_USB_WALLET_IF_ERR
	#define TWI_USB_WALLET_IF_ERR(...) 	do{TWI_LOGGER("[ERR][%s][%d]",__FUNCTION__,__LINE__)TWI_LOGGER(__VA_ARGS__)}while(0)

	#if (TWI_USB_WALLET_IF_LOG_LEVEL >= TWI_USB_WALLET_IF_LOG_LEVEL_INFO)

		#undef TWI_USB_WALLET_IF_INFO
		#define TWI_USB_WALLET_IF_INFO(...) 	do{TWI_LOGGER("[SSS_INTFC]:[INFO]")TWI_LOGGER(__VA_ARGS__)}while(0)

		#if (TWI_USB_WALLET_IF_LOG_LEVEL >= TWI_USB_WALLET_IF_LOG_LEVEL_DBG)

			#undef TWI_USB_WALLET_IF_DBG
			#define TWI_USB_WALLET_IF_DBG(...) 	do{TWI_LOGGER("[SSS_INTFC]:[DBG][%s][%d]",__FUNCTION__,__LINE__)TWI_LOGGER(__VA_ARGS__)}while(0)
		#endif
	#endif
#endif

#define BITCOIN_APP_NAME						"Bitcoin" 
#define TEST_BITCOIN_APP_NAME					"TestBitcoin"
#define ETHEREUM_APP_NAME						"Ethereum"
#define TEST_ETHEREUM_APP_NAME					"Ethereum"

#define USB_WALLET_APP_NAME_MAX_LEN				(15)
/************************************************/
/*********** Internal Commnds Class *************/
/************************************************/
#define INTERNAL_COMMANDS_CLASS 				0xFF
/***************** commands *********************/
#define REQUEST_OPEN_APP_INS					0x04		
#define CONFIRM_OPEN_APP_INS					0x05
#define GET_WALLET_ID_INS						0x0C

/************************************************/
/*********** BITCOIN Commnds Class **************/
/************************************************/
#define BITCOIN_APP_COMMANDS_CLASS				0x00
/***************** commands *********************/
#define BITCOIN_GET_EXTENDED_PUBKEY_INS			0x00
#define BITCOIN_START_SIGN_TX_INS				0x01
#define BITCOIN_CONTINUE_SIGN_TX_INS			0x02
#define BITCOIN_REQUEST_SIGN_TX_INS 			0x03
#define BITCOIN_FINISH_SIGN_TX_INS				0x04

#define BITCOIN_START_SIGN_MSG_INS				0x05
#define BITCOIN_CONTINUE_SIGN_MSG_INS			0x06
#define BITCOIN_REQUEST_SIGN_MSG_INS 			0x07
#define BITCOIN_FINISH_SIGN_MSG_INS				0x08

/************************************************/
/*********** ETHEREUM Commnds Class *************/
/************************************************/
#define ETHEREUM_APP_COMMANDS_CLASS				0x01
/***************** commands *********************/
#define ETHEREUM_GET_EXTENDED_PUBKEY_INS		0x00
#define ETHEREUM_REQUEST_SIGN_TX_INS 			0x01
#define ETHEREUM_CONFIRM_SIGN_TX_INS 			0x02
#define ETHEREUM_FINISH_SIGN_TX_INS				0x03

#define ETHEREUM_START_SIGN_MSG_INS				0x04
#define ETHEREUM_CONTINUE_SIGN_MSG_INS			0x05
#define ETHEREUM_REQUEST_SIGN_MSG_INS 			0x06
#define ETHEREUM_FINISH_SIGN_MSG_INS			0x07

#define DVC_ID_IDX								(3)
#define DVC_ID_LEN								(4)	

#define USB_WALLET_PUBKEY_MAX_LEN				(255)

#define TWI_ETHEREUM_SIGNATURE_TOTAL_LEN		(TWI_USB_ETHEREUM_SIGNATURE_V_LEN + TWI_USB_ETHEREUM_SIGNATURE_R_LEN + TWI_USB_ETHEREUM_SIGNATURE_S_LEN)
/*---------------------------------------------------------*/
/*- GLOBAL CONSTANT VARIABLES -----------------------------*/
/*---------------------------------------------------------*/

/************************************************************/
/*- STRUCTS AND UNIONS -------------------------------------*/
/************************************************************/
typedef struct
{
	twi_u8* pu8_rx_buf;
	twi_u16 u16_rx_buf_len;

}tstr_usb_rx_info;

typedef struct 
{
	twi_u8 au8_pubkey[USB_WALLET_PUBKEY_MAX_LEN];
	twi_u8 u8_pubkey_len;

}tstr_usb_pubkey_info;

typedef struct 
{
	tstr_usb_crypto_path str_path;	
	tstr_usb_pubkey_info str_extended_pubkey;

}tstr_usb_get_extended_pubkey_info;

typedef struct 
{
    union sign_tx_info
    {
        struct bitcoin_sign_tx
        {
			tstr_usb_bitcoin_tx		str_tx_info;
			tstr_usb_bitcoin_signed_tx	str_signed_tx;	

        }str_bitcoin_sign_tx;

        struct ethereum_sign_tx
        {
			tstr_usb_ethereum_tx		str_tx_info;
			tstr_usb_ethereum_signed_tx	str_signed_tx;	

        }str_ethereum_sign_tx;  

    }uni_sign_tx_info;			 		

}tstr_usb_sign_tx_info;

typedef struct 
{		 		
    union sign_msg_info
    {
        struct bitcoin_sign_msg
        {
			twi_u16 						u16_total_signed_sz;
			twi_u16							u16_signing_sz;
			tstr_usb_bitcoin_msg			str_msg_info;
			tstr_usb_bitcoin_signed_msg		str_signed_msg;	

        }str_bitcoin_sign_msg;

        struct ethereum_sign_msg
        {
			twi_u16 						u16_total_signed_sz;			
			twi_u16							u16_signing_sz;			
			tstr_usb_ethereum_msg			str_msg_info;
			tstr_usb_ethereum_signed_msg	str_signed_msg;	

        }str_ethereum_sign_msg;  

    }uni_sign_msg_info;

}tstr_usb_sign_msg_info;

typedef struct 
{
	twi_u8 au8_wallet_id[USB_WALLET_ID_LEN];
	twi_u8 u8_wallet_id_len;

}tstr_usb_get_wallet_id_info;

typedef enum
{
	USB_WALLET_OP_STATE_CONNECTION_EVENT = 0,
	USB_WALLET_OP_STATE_CONNECTION_FAILED_EVENT,
	USB_WALLET_OP_STATE_DISCONNECTION_EVENT,
	USB_WALLET_OP_STATE_DATA_RCVD_EVENT,
	USB_WALLET_OP_STATE_SEND_FAILED_EVENT,
	USB_WALLET_OP_STATE_TIMER_FIRED_EVENT,
	USB_WALLET_OP_STATE_USER_CONFIRMATION_EVENT,

}tenu_usb_op_state_event;


/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS PROTOTYPES ----------------------------*/
/*---------------------------------------------------------*/
static void usb_stack_cb(tstr_twi_stack_evt* pstr_evt, void* pv);
static void current_operation_finalize(tstr_usb_if_context* pstr_cntxt, twi_u8* pu8_data_buf, twi_u32 u32_data_len, twi_s32 s32_err, twi_bool b_disconnected);
static twi_s32 signed_tx_parse(tstr_usb_if_context* pstr_cntxt, twi_u8* pu8_sign_buf, twi_u16 u16_sign_len, void* pstr_signed_tx);
static void wait_to_connect_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event);
static void get_wallet_id_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void request_open_app_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void confirm_open_app_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void get_extended_pubkey_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void sign_tx_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void sign_msg_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static void get_wallet_id_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv);
static twi_s32 apdu_cmd_send(tstr_usb_if_context* pstr_cntxt, tenu_twi_usb_apdu_cmds enu_apdu_cmd);

/*---------------------------------------------------------*/
/*- GLOBAL STATIC VARIABLES -------------------------------*/
/*---------------------------------------------------------*/


/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/
static void usb_stack_cb(tstr_twi_stack_evt* pstr_evt, void* pv)
{
	TWI_ASSERT((NULL != pstr_evt) && (NULL != pv));

	tstr_usb_if_context* pstr_cntxt = (tstr_usb_if_context*)pv;

	switch (pstr_evt->enu_event)
	{
		case TWI_STACK_SEND_STATUS_EVT:
		{
			/* code */
			if(TWI_FALSE == pstr_evt->uni_data.str_send_stts_evt.b_is_success)
			{ 
				op_state_update(pstr_cntxt, USB_WALLET_OP_STATE_SEND_FAILED_EVENT , NULL);
			}

			break;
		}

		case TWI_STACK_RCV_DATA_EVT:
		{
			//TODO: check msg type 
			tstr_usb_rx_info str_rx_info;
			TWI_MEMSET(&str_rx_info, 0x0, sizeof(tstr_usb_rx_info));
			str_rx_info.pu8_rx_buf = pstr_evt->uni_data.str_rcv_data_evt.pu8_data;
			str_rx_info.u16_rx_buf_len = pstr_evt->uni_data.str_rcv_data_evt.u16_data_len;
			twi_stack_unlock_rcv_buf(pstr_evt->uni_data.str_rcv_data_evt.pv_user_arg , pstr_evt->uni_data.str_rcv_data_evt.pu8_data);
			op_state_update(pstr_cntxt, USB_WALLET_OP_STATE_DATA_RCVD_EVENT , &str_rx_info);
			break;
		}

		default:
		{
			break;
		}	
	}	
}	

static void current_operation_finalize(tstr_usb_if_context* pstr_cntxt, twi_u8* pu8_data_buf, twi_u32 u32_data_len, twi_s32 s32_err, twi_bool b_disconnected)
{
	TWI_LOGGER_ERR("pstr_cntxt->str_cur_op.b_skip_disconnection:%d:s32_err:%d:b_disconnected:%d\r\n", pstr_cntxt->str_cur_op.b_skip_disconnection, s32_err, b_disconnected);
		
	if(((TWI_TRUE == pstr_cntxt->str_cur_op.b_skip_disconnection) && (s32_err == USB_IF_NO_ERR)) || (b_disconnected == TWI_TRUE))
	{
		pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_FALSE;
	
		switch(pstr_cntxt->str_cur_op.enu_cur_op)
		{
			case USB_WALLET_APP_GET_EXTENDED_PUBKEY_OP:
			{
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onGetExtendedPubKeyResult);
				pstr_cntxt->str_in_param.__onGetExtendedPubKeyResult(pstr_cntxt->pv_device_info, pu8_data_buf, u32_data_len, s32_err);				
				break;
			}	

			case USB_WALLET_APP_SIGN_TX_OP:
			{
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onSignTransactionResult);
				pstr_cntxt->str_in_param.__onSignTransactionResult(pstr_cntxt->pv_device_info, (void*)pu8_data_buf, s32_err);				
				break;
			}

			case USB_WALLET_APP_SIGN_MSG_OP:
			{
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onSignMessageResult);
				pstr_cntxt->str_in_param.__onSignMessageResult(pstr_cntxt->pv_device_info, (void*)pu8_data_buf, s32_err);				
				break;
			}

			case USB_WALLET_APP_GET_ID_OP:
			{
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onGetWalletIDResult);
				pstr_cntxt->str_in_param.__onGetWalletIDResult(pstr_cntxt->pv_device_info, pu8_data_buf, (twi_u8)u32_data_len, s32_err);				
				break;
			}	

			default:
			{
				break;
			}				
		}

		free(pstr_cntxt->str_cur_op.pv);
		
		pstr_cntxt->str_cur_op.enu_cur_op = USB_WALLET_APP_IDLE_OP;							
		pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_INVALID;				
	}
	else
	{						
		pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_FALSE;	
		pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_WAITING_TO_DISCONNECT;		
		pstr_cntxt->str_cur_op.enu_err_code = s32_err;
		TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_disconnect);
		pstr_cntxt->str_in_param.__usb_disconnect(pstr_cntxt->pv_device_info);
	}	
}

static twi_s32 signed_tx_parse(tstr_usb_if_context* pstr_cntxt, twi_u8* pu8_sign_buf, twi_u16 u16_sign_len, void* pstr_signed_tx)
{
	twi_s32 s32_retval = TWI_SUCCESS;

	if((NULL != pu8_sign_buf)&&(NULL != pstr_signed_tx))
	{
		twi_bool b_vaild_len = TWI_TRUE;
		twi_u16 u16_idx = 0;

		switch (pstr_cntxt->str_cur_op.enu_coin_type)
		{
			case USB_WALLET_COIN_BITCOIN:
			case USB_WALLET_COIN_TEST_BITCOIN:
			{
				tstr_usb_bitcoin_signed_tx* pstr_bitcoin_signed_inputs = (tstr_usb_bitcoin_signed_tx*)pstr_signed_tx;
				TWI_MEMSET(pstr_bitcoin_signed_inputs, 0x0, sizeof(tstr_usb_bitcoin_signed_tx));				
		
				do
				{
					twi_u8 u8_sign_len;
					u16_idx += 4; /* we don't need input idx currently */ 
					u8_sign_len = pu8_sign_buf[u16_idx];
					u16_idx += 1;
					
					if(((u8_sign_len + u16_idx) <= u16_sign_len) && (u8_sign_len <= USB_WALLET_SIGNED_INPUT_MAX_LEN))
					{
						pstr_bitcoin_signed_inputs->astr_signed_inputs[pstr_bitcoin_signed_inputs->u8_signed_inputs_num].u8_sign_len  = u8_sign_len;
						TWI_MEMCPY(pstr_bitcoin_signed_inputs->astr_signed_inputs[pstr_bitcoin_signed_inputs->u8_signed_inputs_num].au8_sign_buf, &pu8_sign_buf[u16_idx], u8_sign_len);
						u16_idx += u8_sign_len;
						pstr_bitcoin_signed_inputs->u8_signed_inputs_num += 1;

						if(pstr_bitcoin_signed_inputs->u8_signed_inputs_num > USB_WALLET_TX_INPUTS_MAX_NUM)
						{
							b_vaild_len = TWI_FALSE;
							break;					
						}
					}
					else
					{
						b_vaild_len = TWI_FALSE;
						break;
					}

				} while(u16_idx < u16_sign_len);
				
				if(TWI_FALSE == b_vaild_len)
				{
					s32_retval = TWI_ERROR_INVALID_LEN;
				}
				else
				{
					/* do nothing */
				}

				break;
			}

			case USB_WALLET_COIN_ETHEREUM:
			case USB_WALLET_COIN_TEST_ETHEREUM:
			{
				if(u16_sign_len == TWI_ETHEREUM_SIGNATURE_TOTAL_LEN)
				{
					tstr_usb_ethereum_signed_tx* pstr_ethereum_signed_tx = (tstr_usb_ethereum_signed_tx*)pstr_signed_tx;
					TWI_MEMSET(pstr_ethereum_signed_tx, 0x0, sizeof(tstr_usb_ethereum_signed_tx));		

					pstr_ethereum_signed_tx->u8_sig_v = pu8_sign_buf[0];

					TWI_MEMCPY(pstr_ethereum_signed_tx->au8_sig_r, &pu8_sign_buf[TWI_USB_ETHEREUM_SIGNATURE_V_LEN], TWI_USB_ETHEREUM_SIGNATURE_R_LEN);
					TWI_MEMCPY(pstr_ethereum_signed_tx->au8_sig_s, &pu8_sign_buf[TWI_USB_ETHEREUM_SIGNATURE_R_LEN + TWI_USB_ETHEREUM_SIGNATURE_V_LEN], TWI_USB_ETHEREUM_SIGNATURE_S_LEN);					
				}
				else
				{
					s32_retval = TWI_ERROR_INVALID_LEN;
				}

				break;
			}

			default:
			{
				break;
			}
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}	

	return s32_retval;
}

static void wait_to_connect_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event)
{
	switch(enu_event)
	{
		case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
			break;
		}

		case USB_WALLET_OP_STATE_CONNECTION_FAILED_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_CON_FAILED, TWI_TRUE);	
			break;
		}

		case USB_WALLET_OP_STATE_CONNECTION_EVENT:
		{
			if(NULL != pstr_cntxt->str_in_param.__onConnectionDone)
			{
				pstr_cntxt->str_in_param.__onConnectionDone(pstr_cntxt->pv_device_info);
			}

			pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_ID;
			
			if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_WALLET_ID_CMD))
			{
				current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
			}							
			
			break;
		}

		default:
		{
			break;
		}
	}	
}

static void get_wallet_id_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch(enu_event)
	{
		case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
			break;
		}

		case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
			break;
		}

		case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
		{
			tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
			TWI_ASSERT(pstr_rx != NULL);
			tstr_twi_apdu_response str_apdu_resp;
			TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

			if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
			{
				switch (str_apdu_resp.u16_sw)
				{
					case APDU_RESP_SUCCESS:
					{	
						/* wallet id is successfully received */
						if(USB_WALLET_ID_LEN >= str_apdu_resp.u32_rsp_data_len)
						{
							switch(pstr_cntxt->str_cur_op.enu_cur_op)
							{
								case USB_WALLET_APP_GET_EXTENDED_PUBKEY_OP:
								case USB_WALLET_APP_SIGN_TX_OP:
								case USB_WALLET_APP_SIGN_MSG_OP:
								{
									static twi_u8 au8_wallet_id[USB_WALLET_ID_LEN];
									TWI_MEMSET(au8_wallet_id, 0x0, (USB_WALLET_ID_LEN - str_apdu_resp.u32_rsp_data_len));
									TWI_MEMCPY(&au8_wallet_id[USB_WALLET_ID_LEN - str_apdu_resp.u32_rsp_data_len], str_apdu_resp.pu8_rsp_data, str_apdu_resp.u32_rsp_data_len);

									if(0 == TWI_MEMCMP(au8_wallet_id, pstr_cntxt->str_cur_op.au8_verify_id, USB_WALLET_ID_LEN))
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_OPEN_APP;

										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_OPEN_APP_CMD))
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}
									}
									else
									{
										current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_UNMATCHED_WALLET_ID, TWI_FALSE);	
									}

									break;
								}	
								
								case USB_WALLET_APP_GET_ID_OP:
								{
									tstr_usb_get_wallet_id_info* pstr_info = (tstr_usb_get_wallet_id_info*)pstr_cntxt->str_cur_op.pv;
									TWI_ASSERT(NULL != pstr_info);

									TWI_MEMSET(pstr_info->au8_wallet_id, 0x0, (USB_WALLET_ID_LEN - str_apdu_resp.u32_rsp_data_len));
									TWI_MEMCPY(&pstr_info->au8_wallet_id[USB_WALLET_ID_LEN - str_apdu_resp.u32_rsp_data_len], str_apdu_resp.pu8_rsp_data, str_apdu_resp.u32_rsp_data_len);
									current_operation_finalize(pstr_cntxt, pstr_info->au8_wallet_id, (twi_u8)sizeof(pstr_info->au8_wallet_id), (twi_s32)USB_IF_NO_ERR, TWI_FALSE);									
									
									break;
								}

								default:
								{
									break;
								}
							}		
						}
						else
						{
							current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_GET_WALLET_ID_FAILED, TWI_FALSE);
						}
						
						break;
					}
					
					default:
					{
						/* wallet id is not successfully received */
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_GET_WALLET_ID_FAILED, TWI_FALSE);
						break;
					}
				}
			}
			else
			{
				current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
			}

			break;
		}

		default:
		{
			break;
		}
	}	
}

static void request_open_app_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch(enu_event)
	{
		case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
			break;
		}

		case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
			break;
		}

		case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
		{
			tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
			TWI_ASSERT(pstr_rx != NULL);
			tstr_twi_apdu_response str_apdu_resp;
			TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));

			if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
			{
				switch (str_apdu_resp.u16_sw)
				{
					case APDU_RESP_SUCCESS:
					{	
						/* User confirmation is required */							
						pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONFIRM_OPEN_APP;
						
						if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONFIRM_OPEN_APP_CMD))
						{
							current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
						}
						else
						{																			
							TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationRequested);							
							pstr_cntxt->str_in_param.__onUserConfirmationRequested(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_APP_OPEN_CONFIRMATION);
						}

						break;
					}

					case APDU_RESP_ALREADY_OPENED:
					{	
						/* App is already running */

						switch(pstr_cntxt->str_cur_op.enu_cur_op)
						{
							case USB_WALLET_APP_GET_EXTENDED_PUBKEY_OP:
							{
								tstr_usb_get_extended_pubkey_info* pstr_info = (tstr_usb_get_extended_pubkey_info*)pstr_cntxt->str_cur_op.pv;
								TWI_ASSERT(NULL != pstr_info);
								
								pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_EXTENDED_PUBKEY;
								
								if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_EXTENDED_PUBKEY_CMD))
								{
									current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
								}

								break;
							}

							case USB_WALLET_APP_SIGN_TX_OP:
							{
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_START_SIGN_TX;
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_START_SIGN_TX_CMD))		
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}								
																			
										break;
									}

									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_SIGN_TX;
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_SIGN_TX_CMD))		
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}	

										break;
									}

									default:
										break;
								}									

								break;
							}	

							case USB_WALLET_APP_SIGN_MSG_OP:
							{
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:									
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_START_SIGN_MSG;
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_START_SIGN_MSG_CMD))		
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}								
																			
										break;
									}

									default:
										break;
								}									

								break;
							}	

							default:
							{
								break;
							}
						}								

						break;
					}

					default:
					{
						current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_OPEN_COIN_APP_FAILED, TWI_FALSE);
						break;
					}
				}
			}
			else
			{
				current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
			}

			break;
		}

		default:
		{
			break;
		}
	}	
}


static void confirm_open_app_state_handle(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch(enu_event)
	{
		case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
			break;
		}

		case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
		{
			current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
			break;
		}

		case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
		{
			tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
			TWI_ASSERT(pstr_rx != NULL);
			tstr_twi_apdu_response str_apdu_resp;
			TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

			if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
			{
				switch (str_apdu_resp.u16_sw)
				{
					case APDU_RESP_SUCCESS:
					{	
						/* User confirmation is optained */
						TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationObtained);
						pstr_cntxt->str_in_param.__onUserConfirmationObtained(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_APP_OPEN_CONFIRMATION);
						
						switch(pstr_cntxt->str_cur_op.enu_cur_op)
						{
							case USB_WALLET_APP_GET_EXTENDED_PUBKEY_OP:
							{
								tstr_usb_get_extended_pubkey_info* pstr_info = (tstr_usb_get_extended_pubkey_info*)pstr_cntxt->str_cur_op.pv;
								TWI_ASSERT(NULL != pstr_info);
								
								pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_EXTENDED_PUBKEY;
								
								if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_EXTENDED_PUBKEY_CMD))
								{
									current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
								}

								break;
							}

							case USB_WALLET_APP_SIGN_TX_OP:
							{
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_START_SIGN_TX;
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_START_SIGN_TX_CMD))		
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}								
																			
										break;
									}

									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_SIGN_TX;
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_SIGN_TX_CMD))		
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}	
																																	
										break;
									}

									default:
										break;
								}

								break;
							}	

							case USB_WALLET_APP_SIGN_MSG_OP:
							{
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:									
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_START_SIGN_MSG;
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_START_SIGN_MSG_CMD))		
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}								
																			
										break;
									}

									default:
										break;
								}

								break;
							}

							default:
							{
								break;
							}
						}						

						break;
					}
					
					default:
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_OPEN_COIN_APP_FAILED, TWI_FALSE);
						break;
					}
				}
			}
			else
			{
				current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
			}

			break;
		}

		default:
		{
			break;
		}						
	}					
}

static void get_extended_pubkey_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch (pstr_cntxt->str_cur_op.enu_cur_state)
	{
		case USB_WALLET_STATE_WAITING_TO_CONNECT:
		{
			wait_to_connect_state_handle(pstr_cntxt, enu_event);
			break;
		}

		case USB_WALLET_STATE_GET_ID:
		{
			get_wallet_id_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}	

		case USB_WALLET_STATE_REQUEST_OPEN_APP:
		{
			request_open_app_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}

		case USB_WALLET_STATE_CONFIRM_OPEN_APP:
		{
			confirm_open_app_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}
		
		case USB_WALLET_STATE_GET_EXTENDED_PUBKEY:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								if(USB_WALLET_PUBKEY_MAX_LEN >= str_apdu_resp.u32_rsp_data_len)
								{
									/* Extended Pubkey is successfully received */
									tstr_usb_get_extended_pubkey_info* pstr_info = (tstr_usb_get_extended_pubkey_info*)pstr_cntxt->str_cur_op.pv;
									TWI_ASSERT(NULL != pstr_info);

									TWI_MEMCPY(pstr_info->str_extended_pubkey.au8_pubkey, str_apdu_resp.pu8_rsp_data, str_apdu_resp.u32_rsp_data_len);		
									pstr_info->str_extended_pubkey.u8_pubkey_len = (twi_u8)str_apdu_resp.u32_rsp_data_len;								
									current_operation_finalize(pstr_cntxt, pstr_info->str_extended_pubkey.au8_pubkey, pstr_info->str_extended_pubkey.u8_pubkey_len, (twi_s32)USB_IF_NO_ERR, TWI_FALSE);
								}
								else
								{
									/* Extended Pubkey is not successfully received */
									current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_GET_EXT_PUBKEY_FAILED, TWI_FALSE);
								}	

								break;
							}
							
							default:
							{
								/* Extended Pubkey is not successfully received */
								current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)USB_IF_ERR_GET_EXT_PUBKEY_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}				

		case USB_WALLET_STATE_WAITING_TO_DISCONNECT:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					if(USB_IF_NO_ERR == pstr_cntxt->str_cur_op.enu_err_code)
					{
						tstr_usb_get_extended_pubkey_info* pstr_info = (tstr_usb_get_extended_pubkey_info*)pstr_cntxt->str_cur_op.pv;
						TWI_ASSERT(NULL != pstr_info);

						current_operation_finalize(pstr_cntxt, pstr_info->str_extended_pubkey.au8_pubkey, (twi_u32)pstr_info->str_extended_pubkey.u8_pubkey_len, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
					}
						
					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}

		default:
			break;
	}
}

static void sign_tx_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch (pstr_cntxt->str_cur_op.enu_cur_state)
	{
		case USB_WALLET_STATE_WAITING_TO_CONNECT:
		{
			wait_to_connect_state_handle(pstr_cntxt, enu_event);
			break;
		}

		case USB_WALLET_STATE_GET_ID:
		{
			get_wallet_id_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}	

		case USB_WALLET_STATE_REQUEST_OPEN_APP:
		{
			request_open_app_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}

		case USB_WALLET_STATE_CONFIRM_OPEN_APP:
		{
			confirm_open_app_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}

		case USB_WALLET_STATE_START_SIGN_TX:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	
					
					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								TWI_ASSERT(NULL != pstr_cntxt->str_cur_op.pv);
								tstr_usb_bitcoin_tx* pstr_tx = (tstr_usb_bitcoin_tx*)&((tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv)->uni_sign_tx_info.str_bitcoin_sign_tx.str_tx_info;

								if(pstr_tx->u16_total_inputs_num > 0)
								{
									pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONTINUE_SIGN_TX;

									if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONTINUE_SIGN_TX_CMD))
									{
										current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
									}										
								}
								else
								{
									pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_SIGN_TX;
								
									if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_SIGN_TX_CMD))
									{
										current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
									}										
								}
								
								break;
							}
							
							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}						
			}					

			break;
		}

		case USB_WALLET_STATE_CONTINUE_SIGN_TX:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								TWI_ASSERT(NULL != pstr_cntxt->str_cur_op.pv);

								tstr_usb_bitcoin_tx* pstr_tx = (tstr_usb_bitcoin_tx*)&((tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv)->uni_sign_tx_info.str_bitcoin_sign_tx.str_tx_info;	
								
								pstr_tx->u16_delivered_inputs_count += 1;

								if(pstr_tx->u16_total_inputs_num > pstr_tx->u16_delivered_inputs_count)
								{
									pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONTINUE_SIGN_TX;
								
									if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONTINUE_SIGN_TX_CMD))
									{
										current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
									}									
								}
								else
								{
									pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_SIGN_TX;
								
									if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_SIGN_TX_CMD))
									{
										current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
									}									
								}

								break;
							}
							
							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}						
			}					

			break;
		}

		case USB_WALLET_STATE_REQUEST_SIGN_TX:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								/* User confirmation is required */							
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_FINISH_SIGN_TX;
										
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_FINISH_SIGN_TX_CMD))
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}
										else
										{
											TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationRequested);							
											pstr_cntxt->str_in_param.__onUserConfirmationRequested(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_SIGN_TX_CONFIRMATION);
										}
									
										break;
									}

									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONFIRM_SIGN_TX;	
										
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONFIRM_SIGN_TX_CMD))
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}	

										break;
									}

									default:
										break;
								}

								break;
							}

							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}
			}					

			break;
		}

		case USB_WALLET_STATE_CONFIRM_SIGN_TX:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_FINISH_SIGN_TX;
								
								if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_FINISH_SIGN_TX_CMD))
								{
									current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
								}
								else
								{
									TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationRequested);							
									pstr_cntxt->str_in_param.__onUserConfirmationRequested(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_SIGN_TX_CONFIRMATION);
								}					

								break;
							}

							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
								break;
							}
						}
					}	

					break;
				}

				default:
					break;
			}		
			
			break;
		}
		
		case USB_WALLET_STATE_FINISH_SIGN_TX:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								/* User confirmation is optained */
								TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationObtained);
								pstr_cntxt->str_in_param.__onUserConfirmationObtained(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_SIGN_TX_CONFIRMATION);

								tstr_usb_sign_tx_info* pstr_info = (tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv;
								TWI_ASSERT(NULL != pstr_info);

								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									{
										if(TWI_SUCCESS == signed_tx_parse(pstr_cntxt, str_apdu_resp.pu8_rsp_data, str_apdu_resp.u32_rsp_data_len, &pstr_info->uni_sign_tx_info.str_bitcoin_sign_tx.str_signed_tx))
										{
											current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_tx_info.str_bitcoin_sign_tx.str_signed_tx, 0, (twi_s32)USB_IF_NO_ERR, TWI_FALSE);
										}
										else
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
										}

										break;
									}

									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:
									{
										if(TWI_SUCCESS == signed_tx_parse(pstr_cntxt, str_apdu_resp.pu8_rsp_data, str_apdu_resp.u32_rsp_data_len, &pstr_info->uni_sign_tx_info.str_ethereum_sign_tx.str_signed_tx))
										{
											current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_tx_info.str_ethereum_sign_tx.str_signed_tx, 0, (twi_s32)USB_IF_NO_ERR, TWI_FALSE);
										}
										else
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
										}

										break;
									}

									default:
										break;
								}	

								break;
							}
							
							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_TX_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}						
			}					

			break;
		}

		case USB_WALLET_STATE_WAITING_TO_DISCONNECT:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					if(USB_IF_NO_ERR == pstr_cntxt->str_cur_op.enu_err_code)
					{
						tstr_usb_sign_tx_info* pstr_info = (tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv;
						TWI_ASSERT(NULL != pstr_info);

						switch (pstr_cntxt->str_cur_op.enu_coin_type)
						{
							case USB_WALLET_COIN_BITCOIN:
							case USB_WALLET_COIN_TEST_BITCOIN:
							{
								current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_tx_info.str_bitcoin_sign_tx.str_signed_tx, 0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
								break;
							}

							case USB_WALLET_COIN_ETHEREUM:
							case USB_WALLET_COIN_TEST_ETHEREUM:
							{
								current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_tx_info.str_ethereum_sign_tx.str_signed_tx, 0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
								break;
							}

							default:
								break;
						}						
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
					}
						
					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}

		default:
			break;
	}
}

static void sign_msg_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch (pstr_cntxt->str_cur_op.enu_cur_state)
	{
		case USB_WALLET_STATE_WAITING_TO_CONNECT:
		{
			wait_to_connect_state_handle(pstr_cntxt, enu_event);
			break;
		}

		case USB_WALLET_STATE_GET_ID:
		{
			get_wallet_id_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}	

		case USB_WALLET_STATE_REQUEST_OPEN_APP:
		{
			request_open_app_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}

		case USB_WALLET_STATE_CONFIRM_OPEN_APP:
		{
			confirm_open_app_state_handle(pstr_cntxt, enu_event, pv);
			break;
		}

		case USB_WALLET_STATE_START_SIGN_MSG:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	
					
					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:									
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONTINUE_SIGN_MSG;

										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONTINUE_SIGN_MSG_CMD))
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}	
									
										break;
									}

									default:
										break;
								}									

								break;
							}
							
							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_MSG_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}						
			}					

			break;
		}

		case USB_WALLET_STATE_CONTINUE_SIGN_MSG:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									{
										tstr_usb_sign_msg_info* pstr_sign_msg_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
										tstr_usb_bitcoin_msg* pstr_msg_info = (tstr_usb_bitcoin_msg*)&(pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_msg_info);	
										
										TWI_ASSERT(NULL != pstr_sign_msg_info);

										pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_total_signed_sz += pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_signing_sz;										

										if(pstr_msg_info->u32_msg_len > pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_total_signed_sz)
										{
											pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONTINUE_SIGN_MSG;
										
											if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONTINUE_SIGN_MSG_CMD))
											{
												current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
											}									
										}
										else
										{
											pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_SIGN_MSG;
										
											if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_SIGN_MSG_CMD))
											{
												current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
											}									
										}

										break;
									}

									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:									
									{
										tstr_usb_sign_msg_info* pstr_sign_msg_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
										tstr_usb_ethereum_msg* pstr_msg_info = (tstr_usb_ethereum_msg*)&(pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.str_msg_info);	
										
										pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_total_signed_sz += pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_signing_sz;										

										if(pstr_msg_info->u32_msg_len > pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_total_signed_sz)
										{
											pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_CONTINUE_SIGN_MSG;
										
											if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_CONTINUE_SIGN_MSG_CMD))
											{
												current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
											}									
										}
										else
										{
											pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_SIGN_MSG;
										
											if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_SIGN_MSG_CMD))
											{
												current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
											}									
										}

										break;
									}

									default:
										break;
								}	

								break;
							}
							
							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_MSG_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}						
			}					

			break;
		}

		case USB_WALLET_STATE_REQUEST_SIGN_MSG:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								/* User confirmation is required */							
								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:									
									{
										pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_FINISH_SIGN_MSG;
										
										if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_FINISH_SIGN_MSG_CMD))
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
										}
										else
										{
											TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationRequested);							
											pstr_cntxt->str_in_param.__onUserConfirmationRequested(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_SIGN_MSG_CONFIRMATION);
										}
									
										break;
									}

									default:
										break;
								}

								break;
							}

							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_MSG_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}
			}					

			break;
		}
		
		case USB_WALLET_STATE_FINISH_SIGN_MSG:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_REMOTE_DISCON, TWI_TRUE);	
					break;
				}

				case USB_WALLET_OP_STATE_SEND_FAILED_EVENT:
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);	
					break;
				}

				case USB_WALLET_OP_STATE_DATA_RCVD_EVENT:
				{
					tstr_usb_rx_info* pstr_rx = (tstr_usb_rx_info*)pv;
					TWI_ASSERT(pstr_rx != NULL);
					tstr_twi_apdu_response str_apdu_resp;
					TWI_MEMSET(&str_apdu_resp, 0x0, sizeof(tstr_twi_apdu_response));	

					if(TWI_SUCCESS == twi_apdu_parse_rsp(pstr_rx->pu8_rx_buf, pstr_rx->u16_rx_buf_len, &str_apdu_resp))
					{
						switch (str_apdu_resp.u16_sw)
						{
							case APDU_RESP_SUCCESS:
							{	
								/* User confirmation is optained */
								TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__onUserConfirmationObtained);
								pstr_cntxt->str_in_param.__onUserConfirmationObtained(pstr_cntxt->pv_device_info, (twi_u32)USB_WALLET_SIGN_MSG_CONFIRMATION);

								tstr_usb_sign_msg_info* pstr_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
								TWI_ASSERT(NULL != pstr_info);

								switch (pstr_cntxt->str_cur_op.enu_coin_type)
								{
									case USB_WALLET_COIN_BITCOIN:
									case USB_WALLET_COIN_TEST_BITCOIN:
									{
										if(str_apdu_resp.u32_rsp_data_len == USB_WALLET_BITCOIN_SIGNED_MSG_LEN)
										{			
											TWI_MEMCPY(pstr_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_signed_msg.au8_signed_msg_buf, str_apdu_resp.pu8_rsp_data, str_apdu_resp.u32_rsp_data_len);				
											current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_signed_msg, 0, (twi_s32)USB_IF_NO_ERR, TWI_FALSE);
										}
										else
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_MSG_FAILED, TWI_FALSE);
										}

										break;
									}

									case USB_WALLET_COIN_ETHEREUM:
									case USB_WALLET_COIN_TEST_ETHEREUM:
									{
										if(str_apdu_resp.u32_rsp_data_len == USB_WALLET_ETHEREUM_SIGNED_MSG_LEN)
										{			
											pstr_info->uni_sign_msg_info.str_ethereum_sign_msg.str_signed_msg.u8_sig_v = str_apdu_resp.pu8_rsp_data[0];
											TWI_MEMCPY(pstr_info->uni_sign_msg_info.str_ethereum_sign_msg.str_signed_msg.au8_sig_r, &str_apdu_resp.pu8_rsp_data[TWI_USB_ETHEREUM_SIGNATURE_V_LEN], TWI_USB_ETHEREUM_SIGNATURE_R_LEN);		
											TWI_MEMCPY(pstr_info->uni_sign_msg_info.str_ethereum_sign_msg.str_signed_msg.au8_sig_s, &str_apdu_resp.pu8_rsp_data[TWI_USB_ETHEREUM_SIGNATURE_V_LEN + TWI_USB_ETHEREUM_SIGNATURE_R_LEN], TWI_USB_ETHEREUM_SIGNATURE_S_LEN);														
											current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_msg_info.str_ethereum_sign_msg.str_signed_msg, 0, (twi_s32)USB_IF_NO_ERR, TWI_FALSE);
										}
										else
										{
											current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_MSG_FAILED, TWI_FALSE);
										}

										break;
									}

									default:
										break;
								}	

								break;
							}
							
							default:
							{
								current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SIGN_MSG_FAILED, TWI_FALSE);
								break;
							}
						}
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_RESP_PARSING_FAIL, TWI_FALSE);	
					}

					break;
				}

				default:
				{
					break;
				}						
			}					

			break;
		}

		case USB_WALLET_STATE_WAITING_TO_DISCONNECT:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					if(USB_IF_NO_ERR == pstr_cntxt->str_cur_op.enu_err_code)
					{
						tstr_usb_sign_msg_info* pstr_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
						TWI_ASSERT(NULL != pstr_info);

						switch (pstr_cntxt->str_cur_op.enu_coin_type)
						{
							case USB_WALLET_COIN_BITCOIN:
							case USB_WALLET_COIN_TEST_BITCOIN:
							{
								current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_signed_msg, 0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
								break;
							}

							case USB_WALLET_COIN_ETHEREUM:
							case USB_WALLET_COIN_TEST_ETHEREUM:
							{
								current_operation_finalize(pstr_cntxt, (twi_u8*)&pstr_info->uni_sign_msg_info.str_ethereum_sign_msg.str_signed_msg, 0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
								break;
							}

							default:
								break;
						}						
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
					}
						
					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}

		default:
			break;
	}
}

static void get_wallet_id_op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch (pstr_cntxt->str_cur_op.enu_cur_state)
	{
		case USB_WALLET_STATE_WAITING_TO_CONNECT:
		{	
			wait_to_connect_state_handle(pstr_cntxt, enu_event);
			break;
		}
		
		case USB_WALLET_STATE_GET_ID:
		{
			get_wallet_id_state_handle(pstr_cntxt, enu_event, pv);	
			break;
		}				

		case USB_WALLET_STATE_WAITING_TO_DISCONNECT:
		{
			switch(enu_event)
			{
				case USB_WALLET_OP_STATE_DISCONNECTION_EVENT:
				{
					if(USB_IF_NO_ERR == pstr_cntxt->str_cur_op.enu_err_code)
					{
						tstr_usb_get_wallet_id_info* pstr_info = (tstr_usb_get_wallet_id_info*)pstr_cntxt->str_cur_op.pv;
						TWI_ASSERT(NULL != pstr_info);

						current_operation_finalize(pstr_cntxt, pstr_info->au8_wallet_id, (twi_u8)sizeof(pstr_info->au8_wallet_id), (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
					}
					else
					{
						current_operation_finalize(pstr_cntxt, NULL, (twi_u32)0, (twi_s32)pstr_cntxt->str_cur_op.enu_err_code, TWI_TRUE);
					}
						
					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}

		default:
			break;
	}
}

static void op_state_update(tstr_usb_if_context* pstr_cntxt, tenu_usb_op_state_event enu_event, void* pv)
{
	switch (pstr_cntxt->str_cur_op.enu_cur_op)
	{
		case USB_WALLET_APP_GET_EXTENDED_PUBKEY_OP:
		{
			get_extended_pubkey_op_state_update(pstr_cntxt, enu_event, pv);
			break;
		}

		case USB_WALLET_APP_SIGN_TX_OP:
		{
			sign_tx_op_state_update(pstr_cntxt, enu_event, pv);
			break;		
		}

		case USB_WALLET_APP_SIGN_MSG_OP:
		{
			sign_msg_op_state_update(pstr_cntxt, enu_event, pv);
			break;		
		}

		case USB_WALLET_APP_GET_ID_OP:
		{
			get_wallet_id_op_state_update(pstr_cntxt, enu_event, pv);	
			break;
		}

		default:
		{
			break;
		}
	}
}

static twi_s32 apdu_cmd_send(tstr_usb_if_context* pstr_cntxt, tenu_twi_usb_apdu_cmds enu_apdu_cmd)
{
	twi_u8 au8_cmd_input[USB_WALLET_CMD_INPUT_MAX_SZ];
	static twi_u8 au8_apdu_buf[USB_WALLET_APDU_BUFFER_MAX_SZ];
	twi_u32 u32_apdu_sz = USB_WALLET_APDU_BUFFER_MAX_SZ;
	twi_s32 s32_retval = TWI_SUCCESS;
	tstr_twi_apdu_command str_apdu_cmd;

	TWI_MEMSET(au8_apdu_buf, 0x0, sizeof(au8_apdu_buf));
	TWI_MEMSET(au8_cmd_input, 0x0, sizeof(au8_cmd_input));
	TWI_MEMSET(&str_apdu_cmd, 0x0, sizeof(str_apdu_cmd));

	if((NULL != pstr_cntxt) && (enu_apdu_cmd < USB_WALLET_APDU_INVALID_CMD))
	{
		TWI_MEMSET(&str_apdu_cmd, 0x0, sizeof(tstr_twi_apdu_command));

		switch (enu_apdu_cmd)
		{
			case USB_WALLET_APDU_GET_INSTALLED_APPS_INFO_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_GET_STATUS_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_REQUEST_OPEN_APP_CMD:
			{
				twi_u8 u8_app_name_len = 0;
				twi_u8 au8_app_name[USB_WALLET_APP_NAME_MAX_LEN];			
				twi_u16 u16_bytes_count = 0;
				
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					{
						twi_u8 au8_bitcoin_app_name[] = BITCOIN_APP_NAME;	
						u8_app_name_len = sizeof(au8_bitcoin_app_name) - 1;
						TWI_MEMCPY(au8_app_name, au8_bitcoin_app_name, u8_app_name_len);
						break;
					}

					case USB_WALLET_COIN_TEST_BITCOIN:
					{		
						twi_u8 au8_test_bitcoin_app_name[] = TEST_BITCOIN_APP_NAME;	
						u8_app_name_len = sizeof(au8_test_bitcoin_app_name) - 1;
						TWI_MEMCPY(au8_app_name, au8_test_bitcoin_app_name, u8_app_name_len);
						break;	
					}

					case USB_WALLET_COIN_ETHEREUM:
					{
						twi_u8 au8_ethereum_app_name[] = ETHEREUM_APP_NAME;	
						u8_app_name_len = sizeof(au8_ethereum_app_name) - 1;
						TWI_MEMCPY(au8_app_name, au8_ethereum_app_name, u8_app_name_len);
						break;
					}

					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						twi_u8 au8_test_ethereum_app_name[] = TEST_ETHEREUM_APP_NAME;	
						u8_app_name_len = sizeof(au8_test_ethereum_app_name) - 1;
						TWI_MEMCPY(au8_app_name, au8_test_ethereum_app_name, u8_app_name_len);
						break;
					}

					default:
					{
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
					}		
				}	

				if(u8_app_name_len > 0)
				{
					au8_cmd_input[u16_bytes_count] = u8_app_name_len;
					u16_bytes_count += 1;

					TWI_MEMCPY(&au8_cmd_input[1], au8_app_name, u8_app_name_len);
					u16_bytes_count += u8_app_name_len;
					
					str_apdu_cmd.u8_cla = INTERNAL_COMMANDS_CLASS;
					str_apdu_cmd.u8_ins = REQUEST_OPEN_APP_INS;						
					str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
					str_apdu_cmd.pu8_cmd_data = au8_cmd_input;				
				}

				break;
			}

			case USB_WALLET_APDU_CONFIRM_OPEN_APP_CMD:
			{
				str_apdu_cmd.u8_cla = INTERNAL_COMMANDS_CLASS;
				str_apdu_cmd.u8_ins = CONFIRM_OPEN_APP_INS;
				str_apdu_cmd.u16_cmd_data_len = 0;
				str_apdu_cmd.pu8_cmd_data = NULL;				
				break;
			}

			case USB_WALLET_APDU_REQUEST_INSTALL_APP_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_CONFIRM_INSTALL_APP_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_CONTINUE_INSTALL_APP_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_FINISH_INSTALL_APP_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_REQUEST_UNINSTALL_APP_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_CONFIRM_UNINSTALL_APP_CMD:
			{
				/* code */
				break;
			}

			case USB_WALLET_APDU_GET_WALLET_ID_CMD:
			{
				str_apdu_cmd.u8_cla = INTERNAL_COMMANDS_CLASS;
				str_apdu_cmd.u8_ins = GET_WALLET_ID_INS;
				str_apdu_cmd.u16_cmd_data_len = 0;
				str_apdu_cmd.pu8_cmd_data = NULL;
				
				break;
			}

			case USB_WALLET_APDU_GET_EXTENDED_PUBKEY_CMD:
			{
				if(NULL != pstr_cntxt->str_cur_op.pv)
				{
					tstr_usb_crypto_path* pstr_path = (tstr_usb_crypto_path*)&((tstr_usb_get_extended_pubkey_info*)pstr_cntxt->str_cur_op.pv)->str_path;
					twi_u8 u8_path_step;
					twi_u16 u16_bytes_count = 0;

					if(pstr_path->u8_steps_num <= USB_WALLET_PATH_MAX_STEPS)
					{
						au8_cmd_input[u16_bytes_count] = pstr_path->u8_steps_num;
						u16_bytes_count += 1;

						for(u8_path_step = 0; u8_path_step < pstr_path->u8_steps_num; u8_path_step++)
						{
							SETU32B(au8_cmd_input, u16_bytes_count, pstr_path->au32_path_steps[u8_path_step]);
							u16_bytes_count += USB_WALLET_PATH_STEP_SZ;
						}

						switch (pstr_cntxt->str_cur_op.enu_coin_type)
						{
							case USB_WALLET_COIN_BITCOIN:
							case USB_WALLET_COIN_TEST_BITCOIN:
							{
								str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = BITCOIN_GET_EXTENDED_PUBKEY_INS;
								break;
							}

							case USB_WALLET_COIN_ETHEREUM:
							case USB_WALLET_COIN_TEST_ETHEREUM:
							{
								str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = ETHEREUM_GET_EXTENDED_PUBKEY_INS;	
								break;
							}

							default:
								break;
						}
						
						str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
						str_apdu_cmd.pu8_cmd_data = au8_cmd_input;			
					}
					else
					{
						s32_retval = TWI_ERROR_INVALID_LEN;	
					}
				}
				else
				{
					s32_retval = TWI_ERROR_NULL_PV;	
				}

				break;
			}

			case USB_WALLET_APDU_START_SIGN_TX_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{					
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							twi_u8 u8_internal_input;
							twi_u8 u8_internal_output;
							twi_u8 u8_path_step;
							twi_u16 u16_bytes_count = 0;					
							tstr_usb_bitcoin_tx* pstr_tx = (tstr_usb_bitcoin_tx*)&((tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv)->uni_sign_tx_info.str_bitcoin_sign_tx.str_tx_info;

							do
							{						
								if(pstr_tx->u16_signing_tx_len > USB_WALLET_SIGNING_TX_MAX_LEN)
								{
									s32_retval = TWI_ERROR_INVALID_LEN;
									break;
								}

								SETU16B(au8_cmd_input, u16_bytes_count, pstr_tx->u16_signing_tx_len);
								u16_bytes_count += sizeof(twi_u16);

								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], pstr_tx->au8_signing_tx, pstr_tx->u16_signing_tx_len);
								u16_bytes_count += pstr_tx->u16_signing_tx_len;
								
								if(pstr_tx->u8_internal_inputs_num > USB_WALLET_INTERNAL_INPUTS_MAX_NUM)
								{
									s32_retval = TWI_ERROR_INVALID_LEN;
									break;
								}

								au8_cmd_input[u16_bytes_count] = pstr_tx->u8_internal_inputs_num;
								u16_bytes_count += 1;

								for(u8_internal_input = 0; u8_internal_input < pstr_tx->u8_internal_inputs_num; u8_internal_input++)
								{
									au8_cmd_input[u16_bytes_count] = pstr_tx->astr_internal_inputs[u8_internal_input].u8_idx;
									u16_bytes_count += 1;
									
									if(pstr_tx->astr_internal_inputs[u8_internal_input].u16_lock_script_len > USB_WALLET_LOCK_SCRIPT_MAX_LEN)
									{
										s32_retval = TWI_ERROR_INVALID_LEN;
										break;
									}

									SETU16B(au8_cmd_input, u16_bytes_count, pstr_tx->astr_internal_inputs[u8_internal_input].u16_lock_script_len);
									u16_bytes_count += sizeof(twi_u16);
									
									TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], pstr_tx->astr_internal_inputs[u8_internal_input].au8_lock_script, pstr_tx->astr_internal_inputs[u8_internal_input].u16_lock_script_len);	
									u16_bytes_count += pstr_tx->astr_internal_inputs[u8_internal_input].u16_lock_script_len;

									if(pstr_tx->astr_internal_inputs[u8_internal_input].str_path.u8_steps_num > USB_WALLET_PATH_MAX_STEPS)
									{
										s32_retval = TWI_ERROR_INVALID_LEN;
										break;
									}

									au8_cmd_input[u16_bytes_count] = pstr_tx->astr_internal_inputs[u8_internal_input].str_path.u8_steps_num;
									u16_bytes_count += 1;

									for(u8_path_step = 0; u8_path_step < pstr_tx->astr_internal_inputs[u8_internal_input].str_path.u8_steps_num; u8_path_step++)
									{
										SETU32B(au8_cmd_input, u16_bytes_count, pstr_tx->astr_internal_inputs[u8_internal_input].str_path.au32_path_steps[u8_path_step]);
										u16_bytes_count += USB_WALLET_PATH_STEP_SZ;
									}
								}

								TWI_ERROR_BREAK(s32_retval);

								if(pstr_tx->u8_internal_outputs_num > USB_WALLET_INTERNAL_OUTPUTS_MAX_NUM)
								{
									s32_retval = TWI_ERROR_INVALID_LEN;
									break;
								}

								au8_cmd_input[u16_bytes_count] = pstr_tx->u8_internal_outputs_num;
								u16_bytes_count += 1;
								
								for(u8_internal_output = 0; u8_internal_output < pstr_tx->u8_internal_outputs_num; u8_internal_output++)
								{
									au8_cmd_input[u16_bytes_count] = pstr_tx->astr_internal_outputs[u8_internal_output].u8_idx;
									u16_bytes_count += 1;

									if(pstr_tx->astr_internal_outputs[u8_internal_output].str_path.u8_steps_num > USB_WALLET_PATH_MAX_STEPS)
									{
										s32_retval = TWI_ERROR_INVALID_LEN;
										break;
									}

									au8_cmd_input[u16_bytes_count] = pstr_tx->astr_internal_outputs[u8_internal_output].str_path.u8_steps_num;
									u16_bytes_count += 1;

									for(u8_path_step = 0; u8_path_step < pstr_tx->astr_internal_outputs[u8_internal_output].str_path.u8_steps_num; u8_path_step++)
									{
										SETU32B(au8_cmd_input, u16_bytes_count, pstr_tx->astr_internal_outputs[u8_internal_output].str_path.au32_path_steps[u8_path_step]);
										u16_bytes_count += USB_WALLET_PATH_STEP_SZ;
									}
								}

								TWI_ERROR_BREAK(s32_retval);

								SETU32B(au8_cmd_input, u16_bytes_count, pstr_tx->u32_sighash_value);
								u16_bytes_count += USB_WALLET_SIGHASH_VALUE_LEN;

								str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = BITCOIN_START_SIGN_TX_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;	
							}
							while (0);
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}

						break;
					}
				
					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}	

				break;
			}

			case USB_WALLET_APDU_CONTINUE_SIGN_TX_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{				
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							tstr_usb_bitcoin_tx* pstr_tx = (tstr_usb_bitcoin_tx*)&((tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv)->uni_sign_tx_info.str_bitcoin_sign_tx.str_tx_info;	
							twi_u16 u16_bytes_count = 0;
							
							if(pstr_tx->astr_inputs_info[pstr_tx->u16_delivered_inputs_count].u16_tx_len <= USB_WALLET_SIGNING_TX_MAX_LEN)
							{
								SETU32B(au8_cmd_input, u16_bytes_count, pstr_tx->astr_inputs_info[pstr_tx->u16_delivered_inputs_count].u32_idx);
								u16_bytes_count += sizeof(twi_u32);

								SETU16B(au8_cmd_input, u16_bytes_count, pstr_tx->astr_inputs_info[pstr_tx->u16_delivered_inputs_count].u16_tx_len);
								u16_bytes_count += sizeof(twi_u16);

								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], pstr_tx->astr_inputs_info[pstr_tx->u16_delivered_inputs_count].au8_tx, pstr_tx->astr_inputs_info[pstr_tx->u16_delivered_inputs_count].u16_tx_len);
								u16_bytes_count += pstr_tx->astr_inputs_info[pstr_tx->u16_delivered_inputs_count].u16_tx_len;

								str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = BITCOIN_CONTINUE_SIGN_TX_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;
							}		
							else
							{
								s32_retval = TWI_ERROR_INVALID_LEN;
							}
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}	

						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}						

				break;
			}

			case USB_WALLET_APDU_REQUEST_SIGN_TX_CMD:
			{

				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{
						str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = BITCOIN_REQUEST_SIGN_TX_INS;
						str_apdu_cmd.u16_cmd_data_len = 0;
						str_apdu_cmd.pu8_cmd_data = NULL;						
						break;
					}

					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							twi_u8 u8_path_step;
							twi_u16 u16_bytes_count = 0;					
							tstr_usb_ethereum_tx* pstr_tx = (tstr_usb_ethereum_tx*)&((tstr_usb_sign_tx_info*)pstr_cntxt->str_cur_op.pv)->uni_sign_tx_info.str_ethereum_sign_tx.str_tx_info;
							
							do
							{						
								if(pstr_tx->u16_signing_tx_len > USB_WALLET_SIGNING_TX_MAX_LEN)
								{
									s32_retval = TWI_ERROR_INVALID_LEN;
									break;
								}

								SETU16B(au8_cmd_input, u16_bytes_count, pstr_tx->u16_signing_tx_len);
								u16_bytes_count += sizeof(twi_u16);

								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], pstr_tx->au8_signing_tx, pstr_tx->u16_signing_tx_len);
								u16_bytes_count += pstr_tx->u16_signing_tx_len;
								
								if(pstr_tx->str_signing_key_path.u8_steps_num > USB_WALLET_PATH_MAX_STEPS)
								{
									s32_retval = TWI_ERROR_INVALID_LEN;
									break;
								}

								au8_cmd_input[u16_bytes_count] = pstr_tx->str_signing_key_path.u8_steps_num;
								u16_bytes_count += 1;

								for(u8_path_step = 0; u8_path_step < pstr_tx->str_signing_key_path.u8_steps_num; u8_path_step++)
								{
									SETU32B(au8_cmd_input, u16_bytes_count, pstr_tx->str_signing_key_path.au32_path_steps[u8_path_step]);
									u16_bytes_count += USB_WALLET_PATH_STEP_SZ;
								}

								str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = ETHEREUM_REQUEST_SIGN_TX_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;	
							}
							while (0);
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}
						
						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				break;
			}

			case USB_WALLET_APDU_CONFIRM_SIGN_TX_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = ETHEREUM_CONFIRM_SIGN_TX_INS;	
						str_apdu_cmd.u16_cmd_data_len = 0;
						str_apdu_cmd.pu8_cmd_data = NULL;											
						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				break;
			}

			case USB_WALLET_APDU_FINISH_SIGN_TX_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{
						str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = BITCOIN_FINISH_SIGN_TX_INS;
						break;
					}

					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = ETHEREUM_FINISH_SIGN_TX_INS;						
						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				str_apdu_cmd.u16_cmd_data_len = 0;
				str_apdu_cmd.pu8_cmd_data = NULL;
				break;
			}

			case USB_WALLET_APDU_START_SIGN_MSG_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							twi_u16 u16_bytes_count = 0;
							twi_u8 u8_path_step;
							tstr_usb_sign_msg_info* pstr_sign_msg_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
							tstr_usb_bitcoin_msg* pstr_msg_info = (tstr_usb_bitcoin_msg*)&(pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_msg_info);	
							
							if((pstr_msg_info->u32_msg_len <= USB_WALLET_MSG_MAX_LEN) && (pstr_msg_info->str_sign_key_path.u8_steps_num <= USB_WALLET_PATH_MAX_STEPS))
							{
								SETU32B(au8_cmd_input, u16_bytes_count, pstr_msg_info->u32_msg_len);
								u16_bytes_count += sizeof(twi_u32);

								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], pstr_msg_info->au8_msg_sha_256_hash, USB_WALLET_MSG_SHA_256_HASH_LEN);
								u16_bytes_count += USB_WALLET_MSG_SHA_256_HASH_LEN;

								au8_cmd_input[u16_bytes_count] = pstr_msg_info->str_sign_key_path.u8_steps_num;
								u16_bytes_count += 1;

								for(u8_path_step = 0; u8_path_step < pstr_msg_info->str_sign_key_path.u8_steps_num; u8_path_step++)
								{
									SETU32B(au8_cmd_input, u16_bytes_count, pstr_msg_info->str_sign_key_path.au32_path_steps[u8_path_step]);
									u16_bytes_count += USB_WALLET_PATH_STEP_SZ;
								}		

								str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = BITCOIN_START_SIGN_MSG_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;															
							}
							else
							{
								s32_retval = TWI_ERROR_INVALID_LEN;
							}
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}
						
						break;
					}

					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							twi_u16 u16_bytes_count = 0;
							twi_u8 u8_path_step;
							tstr_usb_sign_msg_info* pstr_sign_msg_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
							tstr_usb_ethereum_msg* pstr_msg_info = (tstr_usb_ethereum_msg*)&(pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.str_msg_info);	
							
							if((pstr_msg_info->u32_msg_len <= USB_WALLET_MSG_MAX_LEN) && (pstr_msg_info->str_sign_key_path.u8_steps_num <= USB_WALLET_PATH_MAX_STEPS))
							{
								SETU32B(au8_cmd_input, u16_bytes_count, pstr_msg_info->u32_msg_len);
								u16_bytes_count += sizeof(twi_u32);

								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], pstr_msg_info->au8_msg_sha_256_hash, USB_WALLET_MSG_SHA_256_HASH_LEN);
								u16_bytes_count += USB_WALLET_MSG_SHA_256_HASH_LEN;

								au8_cmd_input[u16_bytes_count] = pstr_msg_info->str_sign_key_path.u8_steps_num;
								u16_bytes_count += 1;

								for(u8_path_step = 0; u8_path_step < pstr_msg_info->str_sign_key_path.u8_steps_num; u8_path_step++)
								{
									SETU32B(au8_cmd_input, u16_bytes_count, pstr_msg_info->str_sign_key_path.au32_path_steps[u8_path_step]);
									u16_bytes_count += USB_WALLET_PATH_STEP_SZ;
								}		

								str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = ETHEREUM_START_SIGN_MSG_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;															
							}
							else
							{
								s32_retval = TWI_ERROR_INVALID_LEN;
							}
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}
						
						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				break;
			}

			case USB_WALLET_APDU_CONTINUE_SIGN_MSG_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{	
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							twi_u16 u16_bytes_count = 0;
							tstr_usb_sign_msg_info* pstr_sign_msg_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
							tstr_usb_bitcoin_msg* pstr_msg_info = (tstr_usb_bitcoin_msg*)&(pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_msg_info);	
							
							if(pstr_msg_info->u32_msg_len <= USB_WALLET_MSG_MAX_LEN)
							{
								if((pstr_msg_info->u32_msg_len - pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_total_signed_sz) >= USB_WALLET_MSG_CHUNK_SZ)
								{
									pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_signing_sz = USB_WALLET_MSG_CHUNK_SZ;
								}
								else
								{
									pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_signing_sz = pstr_msg_info->u32_msg_len - pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_total_signed_sz;
								}	

								SETU16B(au8_cmd_input, u16_bytes_count, pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_signing_sz);
								u16_bytes_count += sizeof(twi_u16);			
								
								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], &pstr_msg_info->au8_msg_buf[pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_total_signed_sz], pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_signing_sz);						
								u16_bytes_count += pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.u16_signing_sz;

								str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = BITCOIN_CONTINUE_SIGN_MSG_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;
							}		
							else
							{
								s32_retval = TWI_ERROR_INVALID_LEN;
							}
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}		

						break;
					}

					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{	
						if(NULL != pstr_cntxt->str_cur_op.pv)
						{
							twi_u16 u16_bytes_count = 0;
							tstr_usb_sign_msg_info* pstr_sign_msg_info = (tstr_usb_sign_msg_info*)pstr_cntxt->str_cur_op.pv;
							tstr_usb_ethereum_msg* pstr_msg_info = (tstr_usb_ethereum_msg*)&(pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.str_msg_info);	
							
							if(pstr_msg_info->u32_msg_len <= USB_WALLET_MSG_MAX_LEN)
							{
								if((pstr_msg_info->u32_msg_len - pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_total_signed_sz) >= USB_WALLET_MSG_CHUNK_SZ)
								{
									pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_signing_sz = USB_WALLET_MSG_CHUNK_SZ;
								}
								else
								{
									pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_signing_sz = pstr_msg_info->u32_msg_len - pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_total_signed_sz;
								}	

								SETU16B(au8_cmd_input, u16_bytes_count, pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_signing_sz);
								u16_bytes_count += sizeof(twi_u16);			
								
								TWI_MEMCPY(&au8_cmd_input[u16_bytes_count], &pstr_msg_info->au8_msg_buf[pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_total_signed_sz], pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_signing_sz);						
								u16_bytes_count += pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.u16_signing_sz;

								str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
								str_apdu_cmd.u8_ins = ETHEREUM_CONTINUE_SIGN_MSG_INS;
								str_apdu_cmd.u16_cmd_data_len = u16_bytes_count;
								str_apdu_cmd.pu8_cmd_data = au8_cmd_input;
							}		
							else
							{
								s32_retval = TWI_ERROR_INVALID_LEN;
							}
						}
						else
						{
							s32_retval = TWI_ERROR_NULL_PV;	
						}	

						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				break;
			}			

			case USB_WALLET_APDU_REQUEST_SIGN_MSG_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{
						str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = BITCOIN_REQUEST_SIGN_MSG_INS;
						str_apdu_cmd.u16_cmd_data_len = 0;
						str_apdu_cmd.pu8_cmd_data = NULL;						
						break;
					}

					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = ETHEREUM_REQUEST_SIGN_MSG_INS;	
						str_apdu_cmd.u16_cmd_data_len = 0;
						str_apdu_cmd.pu8_cmd_data = NULL;											
						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				break;
			}

			case USB_WALLET_APDU_FINISH_SIGN_MSG_CMD:
			{
				switch (pstr_cntxt->str_cur_op.enu_coin_type)
				{
					case USB_WALLET_COIN_BITCOIN:
					case USB_WALLET_COIN_TEST_BITCOIN:
					{
						str_apdu_cmd.u8_cla = BITCOIN_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = BITCOIN_FINISH_SIGN_MSG_INS;
						str_apdu_cmd.u16_cmd_data_len = 0;
						str_apdu_cmd.pu8_cmd_data = NULL;						
						break;
					}

					case USB_WALLET_COIN_ETHEREUM:
					case USB_WALLET_COIN_TEST_ETHEREUM:
					{
						str_apdu_cmd.u8_cla = ETHEREUM_APP_COMMANDS_CLASS;
						str_apdu_cmd.u8_ins = ETHEREUM_FINISH_SIGN_MSG_INS;
						str_apdu_cmd.u16_cmd_data_len = 0;
						str_apdu_cmd.pu8_cmd_data = NULL;										
						break;
					}

					default:
						s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
						break;
				}

				break;
			}

			default:
			{
				s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
				break;
			}
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}

	if(TWI_SUCCESS == s32_retval)
	{
		/* composing REQUEST_OPEN_APP APDU to open the app of the operation */
		s32_retval = twi_apdu_compose_cmd(&str_apdu_cmd, &u32_apdu_sz, au8_apdu_buf);
		if(TWI_SUCCESS == s32_retval)
		{
			/* sending the composed APDU buffer to HW Wallet through USB */
			s32_retval = twi_stack_send_data(&pstr_cntxt->str_stack_context, TWI_STACK_CLR_MSG, au8_apdu_buf, u32_apdu_sz, (void*)pstr_cntxt);
		}
		else
		{
			/* do nothing */
		}
	}

	return s32_retval;
}

/**
 * 	@fn: 						usb_stack_twi_usbd_send
 * 	@brief      				This function is used to send data to CDC ACM serial port.
 * 	@param[in,out] 	pv_cntxt: 	Void Pointer to the Stack Context. @note: This parameter is not used currently, and reserved for future development.
 * 	@param[in] 		p_tx_buf: 	Pointer to the buffer to send.
 * 	@param[in] 		u32_length: The length of the buffer to send.
 * 	@return:		@val: 		TWI_SUCCESS in case of successful execution. Otherwise, kindly refer to @file: twi_retval.h
 */
static twi_s32 usb_stack_twi_usbd_send(void* pv_cntxt, const void *p_tx_buf,twi_u32 u32_length)
{
	twi_s32 s32_retval = TWI_SUCCESS;
	tstr_usb_if_context* pstr_cntxt = (tstr_usb_if_context*)pv_cntxt;
	TWI_ASSERT(NULL != pstr_cntxt);
	TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_send);	
	pstr_cntxt->str_in_param.__usb_send(pstr_cntxt->pv_device_info, (twi_u8*)p_tx_buf, u32_length);
	return s32_retval;
}

/**
 * @fn: 							usb_stack_twi_usbd_receive
 * @brief: 							This function is used to receive data from CDC ACM serial port.
 * @param[in,out] 	pv_cntxt: 		Void Pointer to the Stack Context. @note: This parameter is not used currently, and reserved for future development.
 * @param[out] 		p_rx_buf: 		Void Pointer to the buffer to receive the data inside it.
 * @param[out] 		pu32_length:  	Pointer to the buffer length.
 * @return:			@val: 			TWI_SUCCESS in case of successful execution. Otherwise, kindly refer to @file: twi_retval.h
 */
static twi_s32 usb_stack_twi_usbd_receive(void* pv_cntxt, void *p_rx_buf, twi_u32* pu32_length)
{
	twi_s32 s32_retval = TWI_SUCCESS;
	tstr_usb_if_context* pstr_cntxt = (tstr_usb_if_context*)pv_cntxt;
	TWI_ASSERT(NULL != pstr_cntxt);
	TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_receive);	
	pstr_cntxt->str_in_param.__usb_receive(pstr_cntxt->pv_device_info, p_rx_buf, pu32_length);
	return s32_retval;
}

/**
 * @fn: 							usb_stack_twi_usbd_stop
 * @brief: 							This function is used to stop the CDC ACM serial port.
 * @param[in,out] 	pv_cntxt: 		Void Pointer to the Stack Context. @note: This parameter is not used currently, and reserved for future development.
 * @return:							None.
 */
static void usb_stack_twi_usbd_stop(void* pv_cntxt)
{
	tstr_usb_if_context* pstr_cntxt = (tstr_usb_if_context*)pv_cntxt;
	TWI_ASSERT(NULL != pstr_cntxt);
	TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_stop);	
	pstr_cntxt->str_in_param.__usb_stop(pstr_cntxt->pv_device_info);
}


/**
 * @brief:	 					This function is used to handle the Dispatch of USB Debug Module.
 * @param[in,out] 	pv_cntxt: 	Void Pointer to the Stack Context. @note: This parameter is not used currently, and reserved for future development.
 * @return:						None.
 */
static void usb_stack_twi_usbd_dispatch(void* pv_cntxt)
{
	tstr_usb_if_context* pstr_cntxt = (tstr_usb_if_context*)pv_cntxt;
	TWI_ASSERT(NULL != pstr_cntxt);
	TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_dispatch);
	pstr_cntxt->str_in_param.__usb_dispatch(pstr_cntxt->pv_device_info);
}

static void usb_stack_sleep_mode_forbiden(void* pv, twi_bool b_forbid)
{
	//TODO
}

static twi_s32 usb_stack_stop_timer(void* pv, tstr_timer_mgmt_timer *pstr_timer)
{
	twi_s32 s32_retval = TWI_SUCCESS;

	if(NULL != pv)
	{
		tstr_usb_if_context* pstr_cntxt	= pv;

		if(NULL != pstr_cntxt->str_in_param.__stop_timer)
		{
			pstr_timer->b_is_active = TWI_FALSE;
			//pstr_cntxt->str_in_param.__stop_timer(); //TODO:?	
		}
		else
		{
			s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
		}		
	}
	else
	{
		s32_retval = TWI_ERROR_NULL_PV;
	}

	return s32_retval;
}

static twi_s32 usb_stack_start_timer(void* pv, tstr_timer_mgmt_timer *pstr_timer, twi_s8 *ps8_name ,tenu_mgmt_timer_mode enu_mode, twi_u32 u32_msec, tpf_twi_timer_mgmt_cb pf_timer_cb, void *pv_user_data)
{
	twi_s32 s32_retval = TWI_SUCCESS;

	if(NULL != pv)
	{
		tstr_usb_if_context* pstr_cntxt	= pv;

		if(NULL != pstr_cntxt->str_in_param.__start_timer)
		{
			pstr_timer->b_is_active = TWI_TRUE;
			//pstr_cntxt->str_in_param.__start_timer(); //TODO: ?	
		}
		else
		{
			s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
		}		
	}
	else
	{
		s32_retval = TWI_ERROR_NULL_PV;
	}

	return s32_retval;
}

/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/
/*
 *  @function   	twi_usb_if_new
 *	@brief			API used to create new usb wallet Interface context.
 *  @return    		::pointer to the new allocated context.
 */
tstr_usb_if_context* twi_usb_if_new(void)
{
	tstr_usb_if_context* pstr_cntxt = malloc(sizeof(tstr_usb_if_context));
	TWI_ASSERT(NULL != pstr_cntxt);
	TWI_MEMSET(pstr_cntxt, 0, sizeof(tstr_usb_if_context));
	pstr_cntxt->str_cur_op.enu_cur_op = USB_WALLET_APP_IDLE_OP;							
	pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_INVALID;	
	pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_FALSE;
	pstr_cntxt->u16_pid = 0;
	pstr_cntxt->u16_vid = 0;
	return pstr_cntxt;	
}	

/*
 *  @function   	twi_usb_if_set_callbacks
 *	@brief			API used to helper APIs of the passed context.    
 *	@param[IN/OUT]	pstr_cntxt:
 *	@param[IN]		__usb_scan_and_connect:
 *	@param[IN]		__usb_disconnect:
 *	@param[IN]		__usb_receive:
 *	@param[IN]		__usb_stop:
 *	@param[IN]		__usb_disable:
 *	@param[IN]		__usb_dispach: 
 *	@param[IN]		__usb_send:
 *	@param[IN]		__start_timer:
 *	@param[IN]		__stop_timer:
 *	@param[IN]		__send_to_cloud:
 *	@param[IN]		__onUserConfirmationRequested:
 *	@param[IN]		__onUserConfirmationObtained:
 *	@param[IN]		__onGetExtendedPubKeyResult:
 *	@param[IN]		__onSignTransactionResult:
 *	@param[IN]		__onSignMessageResult:  
 *	@param[IN]		__onGetWalletIDResult: 
 *	@param[IN]		__save:
 *	@param[IN]		__load:		
 *	@param[IN]		__onConnectionDone: 					 
 */
void twi_usb_if_set_callbacks(	tstr_usb_if_context*            pstr_cntxt,
								usb_scan_and_connect            __usb_scan_and_connect,
								usb_disconnect                  __usb_disconnect,

								usb_receive                     __usb_receive,
								usb_stop                        __usb_stop,
								usb_disable                     __usb_disable,
								usb_dispatch                    __usb_dispatch,                                     
								usb_send                        __usb_send,

								//General helpers
								usb_Start_Timer                 __start_timer,
								usb_Stop_Timer                  __stop_timer,
								usb_send_to_cloud               __send_to_cloud,

								//Upper layer's callbacks
								usb_onUserConfirmationRequested __onUserConfirmationRequested,
								usb_onUserConfirmationObtained  __onUserConfirmationObtained,
								usb_onGetExtendedPubKeyResult   __onGetExtendedPubKeyResult,
								usb_onSignTransactionResult     __onSignTransactionResult,
								usb_onSignMessageResult         __onSignMessageResult,										
								usb_onGetWalletIDResult         __onGetWalletIDResult,                                        
								usb_save                        __save,   
								usb_load                        __load,
								usb_onConnectionDone            __onConnectionDone)										
{
	TWI_ASSERT(NULL != pstr_cntxt);
	pstr_cntxt->str_in_param.__usb_scan_and_connect = __usb_scan_and_connect;
	pstr_cntxt->str_in_param.__usb_disconnect = __usb_disconnect;

	pstr_cntxt->str_in_param.__usb_receive = __usb_receive;
	pstr_cntxt->str_in_param.__usb_stop = __usb_stop;
	pstr_cntxt->str_in_param.__usb_disable = __usb_disable;
	pstr_cntxt->str_in_param.__usb_dispatch = __usb_dispatch;			
	pstr_cntxt->str_in_param.__usb_send = __usb_send;
	pstr_cntxt->str_in_param.__start_timer = __start_timer;
	pstr_cntxt->str_in_param.__stop_timer = __stop_timer;
	pstr_cntxt->str_in_param.__send_to_cloud = __send_to_cloud;
	pstr_cntxt->str_in_param.__onUserConfirmationRequested = __onUserConfirmationRequested;
	pstr_cntxt->str_in_param.__onUserConfirmationObtained = __onUserConfirmationObtained;
	pstr_cntxt->str_in_param.__onGetExtendedPubKeyResult = __onGetExtendedPubKeyResult;
	pstr_cntxt->str_in_param.__onSignTransactionResult = __onSignTransactionResult;
	pstr_cntxt->str_in_param.__onSignMessageResult = __onSignMessageResult;	
	pstr_cntxt->str_in_param.__onGetWalletIDResult = __onGetWalletIDResult;	
	pstr_cntxt->str_in_param.__save = __save;   
	pstr_cntxt->str_in_param.__load = __load;
	pstr_cntxt->str_in_param.__onConnectionDone = __onConnectionDone;

	tstr_stack_helpers* pstr_helpers = calloc(1, sizeof(tstr_stack_helpers));


	pstr_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_send 					= usb_stack_twi_usbd_send,
	pstr_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_receive 				= usb_stack_twi_usbd_receive,
	pstr_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_stop 					= usb_stack_twi_usbd_stop,
	pstr_helpers->uni_ll_helpers.str_usb.pf_twi_usbd_dispatch 				= usb_stack_twi_usbd_dispatch,

	pstr_helpers->pf_twi_system_sleep_mode_forbiden = usb_stack_sleep_mode_forbiden;
	pstr_helpers->pf_stop_timer = usb_stack_stop_timer;
	pstr_helpers->pf_start_timer = usb_stack_start_timer;
	pstr_helpers->pf_stack_sign_cb = NULL;
	pstr_helpers->pf_stack_verify_sig_cb = NULL;
	pstr_helpers->pf_stack_encrypt_cb = NULL;
	pstr_helpers->pf_stack_decrypt_cb = NULL;

	TWI_ASSERT(TWI_SUCCESS == twi_stack_init(&pstr_cntxt->str_stack_context, usb_stack_cb, (void*)pstr_cntxt, TWI_USB_LL, pstr_helpers));
}

/*
 *  @function   	twi_usb_if_set_device_info
 *	@brief			API used to set the connected device info in the passed interface context.
 *	@param[IN/OUT]	pstr_cntxt: pointer to an interface context. 
 *	@param[IN]		pv_dvc_info: pointer to the device info.
 */
void twi_usb_if_set_device_info(tstr_usb_if_context* pstr_cntxt, void* pv_dvc_info)
{	
	TWI_ASSERT((NULL != pstr_cntxt) && (NULL != pv_dvc_info));
	pstr_cntxt->pv_device_info = pv_dvc_info;
}

/*
 *  @function   	twi_usb_if_set_device_id
 *	@brief			API used to set the connected device id in the passed interface context.
 *	@param[IN/OUT]	pstr_cntxt: pointer to an interface context. 
 *	@param[IN]		u16_vid: vid.
 *	@param[IN]		u16_pid: pid. 
 */
void twi_usb_if_set_device_id(tstr_usb_if_context* pstr_cntxt, twi_u16 u16_vid, twi_u16 u16_pid)
{
	TWI_ASSERT(NULL != pstr_cntxt);
	pstr_cntxt->u16_pid = u16_pid;
	pstr_cntxt->u16_vid = u16_vid;
}

/*
 *  @function   	twi_usb_if_free
 *	@brief			API used to free pre-allocated interface context.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 */
void twi_usb_if_free(tstr_usb_if_context* pstr_cntxt)
{
	TWI_ASSERT(NULL != pstr_cntxt);
	free(pstr_cntxt);
}

/*
 *  @function   	twi_usb_if_get_ext_pub_key
 *	@brief			API to get the extended public key of a given path.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		enu_coin_type: coin type.  
 *	@param[IN]		pstr_path: pointer to the path.
 *	@param[IN]		pu8_wallet_id: pointer to wallet id to verify with.
 *	@param[IN]		u8_wallet_id_len: lenght of the wallet id to verify with.
 * 	@param[IN]		b_disconnect: boolen to decide if we gonna disconnect after finishing the operation or not.  
 */
void twi_usb_if_get_ext_pub_key(tstr_usb_if_context* pstr_cntxt, tenu_twi_usb_coin_type enu_coin_type, tstr_usb_crypto_path* const pstr_path, twi_u8* pu8_wallet_id, twi_u8 u8_wallet_id_len, twi_bool b_disconnect)
{
	/* arguments check */
	if((NULL != pstr_cntxt) && (NULL != pstr_path) && (NULL != pstr_cntxt->str_in_param.__usb_send)
		&& (USB_WALLET_PATH_MAX_STEPS >= pstr_path->u8_steps_num) && (enu_coin_type < USB_WALLET_COIN_INVALID))
	{
		/* cntxt idle operation check */
		if((USB_WALLET_STATE_INVALID == pstr_cntxt->str_cur_op.enu_cur_state) && (USB_WALLET_APP_IDLE_OP == pstr_cntxt->str_cur_op.enu_cur_op))
		{
			if(TWI_FALSE == b_disconnect)
			{
				pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_TRUE;
			}

			if((enu_coin_type == USB_WALLET_COIN_TEST_BITCOIN) || (enu_coin_type == USB_WALLET_COIN_BITCOIN))
			{
				if(pstr_path->u8_steps_num == 0)
				{
					twi_u32 au32_path[] = {0x8000002C, 0x80000000};
					
					if(enu_coin_type == USB_WALLET_COIN_TEST_BITCOIN)
					{
						au32_path[1] = 0x80000001;
					}

					pstr_path->u8_steps_num = sizeof(au32_path)/sizeof(twi_u32);	
					TWI_MEMCPY(pstr_path->au32_path_steps, au32_path, sizeof(au32_path));
				}
				else if(pstr_path->u8_steps_num == 1)
				{
					twi_u32 au32_path[] = {0x8000002C, 0x80000000, 0x00000000};

					if(enu_coin_type == USB_WALLET_COIN_TEST_BITCOIN)
					{
						au32_path[1] = 0x80000001;
					}	

					au32_path[2] = pstr_path->au32_path_steps[0];
					pstr_path->u8_steps_num = sizeof(au32_path)/sizeof(twi_u32);	
					TWI_MEMCPY(pstr_path->au32_path_steps, au32_path, sizeof(au32_path));
				}
				else
				{
					/* do nothing */
				}	
			}
			else
			{
				/* do nothing */
			}


			/* updating cntxt current app operation */
			tstr_usb_get_extended_pubkey_info* pstr_get_extended_pubkey_info = calloc(1, sizeof(tstr_usb_get_extended_pubkey_info));
			TWI_MEMCPY(&pstr_get_extended_pubkey_info->str_path, pstr_path, sizeof(tstr_usb_crypto_path));

			pstr_cntxt->str_cur_op.enu_cur_op = USB_WALLET_APP_GET_EXTENDED_PUBKEY_OP;
			pstr_cntxt->str_cur_op.enu_coin_type = enu_coin_type;
			pstr_cntxt->str_cur_op.pv = (void*)pstr_get_extended_pubkey_info;

			TWI_MEMCPY(pstr_cntxt->str_cur_op.au8_verify_id, pu8_wallet_id, u8_wallet_id_len);
			pstr_cntxt->str_cur_op.u8_verify_id_len = u8_wallet_id_len;

			twi_bool b_is_ready = TWI_FALSE;
			twi_stack_is_ready_to_send(&pstr_cntxt->str_stack_context, &b_is_ready);

			if(TWI_TRUE == b_is_ready)
			{
				if(NULL != pu8_wallet_id)
				{
					pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_ID;
					if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_WALLET_ID_CMD))
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
					}
				}
				else
				{
					//skip wallet ID verification
					pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_OPEN_APP;
					if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_OPEN_APP_CMD))
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
					}
				}
			}
			else
			{
				pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_WAITING_TO_CONNECT;
				/* Start Scanning and connect */	
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_scan_and_connect);
				pstr_cntxt->str_in_param.__usb_scan_and_connect(pstr_cntxt->pv_device_info, &pstr_cntxt->str_cur_op.au8_verify_id[DVC_ID_IDX], (twi_u8)DVC_ID_LEN, pstr_cntxt->u16_vid, pstr_cntxt->u16_pid, (twi_u32)USB_SCAN_DURATION_MS, 0);

			}
		}
		else
		{
			pstr_cntxt->str_in_param.__onGetExtendedPubKeyResult(pstr_cntxt->pv_device_info, NULL, 0, (twi_s32)USB_IF_ERR_INVALID_STATE);	
		}
	}
	else
	{
		pstr_cntxt->str_in_param.__onGetExtendedPubKeyResult(pstr_cntxt->pv_device_info, NULL, 0, (twi_s32)USB_IF_ERR_INVALID_ARGS);
	}
}

/*
 *  @function   	twi_usb_if_sign_tx
 *	@brief			API to sign transaction.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		enu_coin_type: coin type.   
 *	@param[IN]		pstr_tx: pointer to the transaction.
 *	@param[IN]		pu8_wallet_id: pointer to wallet id to verify with.
 *	@param[IN]		u8_wallet_id_len: lenght of the wallet id to verify with. 
 *  @param[IN]		b_disconnect: boolen to decide if we gonna disconnect after finishing the operation or not.   
 */
void twi_usb_if_sign_tx(tstr_usb_if_context* pstr_cntxt, tenu_twi_usb_coin_type enu_coin_type, void* const pstr_tx, twi_u8* pu8_wallet_id, twi_u8 u8_wallet_id_len, twi_bool b_disconnect)
{
	/* arguments check */
	if((NULL != pstr_cntxt) && (NULL != pstr_tx) && (NULL != pstr_cntxt->str_in_param.__usb_send)&& (enu_coin_type < USB_WALLET_COIN_INVALID))
	{
		/* cntxt idle operation check */
		if((USB_WALLET_STATE_INVALID == pstr_cntxt->str_cur_op.enu_cur_state) && (USB_WALLET_APP_IDLE_OP == pstr_cntxt->str_cur_op.enu_cur_op))
		{
			if(TWI_FALSE == b_disconnect)
			{
				pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_TRUE;
			}
			/* updating cntxt current app operation */
			pstr_cntxt->str_cur_op.enu_cur_op = USB_WALLET_APP_SIGN_TX_OP;
			pstr_cntxt->str_cur_op.enu_coin_type = enu_coin_type;

			tstr_usb_sign_tx_info* pstr_sign_tx_info = calloc(1, sizeof(tstr_usb_sign_tx_info));

			switch (pstr_cntxt->str_cur_op.enu_coin_type)
			{
				case USB_WALLET_COIN_BITCOIN:
				case USB_WALLET_COIN_TEST_BITCOIN:
				{
					TWI_MEMCPY(&pstr_sign_tx_info->uni_sign_tx_info.str_bitcoin_sign_tx.str_tx_info, pstr_tx, sizeof(tstr_usb_bitcoin_tx));
					break;
				}

				case USB_WALLET_COIN_ETHEREUM:
				case USB_WALLET_COIN_TEST_ETHEREUM:
				{
					TWI_MEMCPY(&pstr_sign_tx_info->uni_sign_tx_info.str_ethereum_sign_tx.str_tx_info, pstr_tx, sizeof(tstr_usb_ethereum_tx));
					break;
				}

				default:
					break;
			}	

			pstr_cntxt->str_cur_op.pv = (void*)pstr_sign_tx_info;

			TWI_MEMCPY(pstr_cntxt->str_cur_op.au8_verify_id, pu8_wallet_id, u8_wallet_id_len);
			pstr_cntxt->str_cur_op.u8_verify_id_len = u8_wallet_id_len;

			twi_bool b_is_ready = TWI_FALSE;
			twi_stack_is_ready_to_send(&pstr_cntxt->str_stack_context, &b_is_ready);

			if(TWI_TRUE == b_is_ready)
			{
				if(NULL != pu8_wallet_id)
				{
					pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_ID;
					if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_WALLET_ID_CMD))
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
					}
				}
				else
				{
					//skip wallet ID verification
					pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_OPEN_APP;
					if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_OPEN_APP_CMD))
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
					}
				}
			}
			else
			{
				pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_WAITING_TO_CONNECT;
				/* Start Scanning and connect */	
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_scan_and_connect);
				pstr_cntxt->str_in_param.__usb_scan_and_connect(pstr_cntxt->pv_device_info, &pstr_cntxt->str_cur_op.au8_verify_id[DVC_ID_IDX], (twi_u8)DVC_ID_LEN, pstr_cntxt->u16_vid, pstr_cntxt->u16_pid, (twi_u32)USB_SCAN_DURATION_MS, 0);
			}
		}
		else
		{
			pstr_cntxt->str_in_param.__onSignTransactionResult(pstr_cntxt->pv_device_info, NULL, (twi_s32)USB_IF_ERR_INVALID_STATE);	
		}
	}
	else
	{
		pstr_cntxt->str_in_param.__onSignTransactionResult(pstr_cntxt->pv_device_info, NULL, (twi_s32)USB_IF_ERR_INVALID_ARGS);
	}
}

/*
 *  @function   	twi_usb_if_sign_msg
 *	@brief			API to sign Message.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		enu_coin_type: coin type.  
 *	@param[IN]		pstr_msg: pointer to the transaction.
 *	@param[IN]		pu8_wallet_id: pointer to wallet id to verify with.
 *	@param[IN]		u8_wallet_id_len: lenght of the wallet id to verify with. 
 *  @param[IN]		b_disconnect: boolen to decide if we gonna disconnect after finishing the operation or not.   
 */
void twi_usb_if_sign_msg(tstr_usb_if_context* pstr_cntxt, tenu_twi_usb_coin_type enu_coin_type, void* const pstr_msg, twi_u8* pu8_wallet_id, twi_u8 u8_wallet_id_len, twi_bool b_disconnect)
{
	/* arguments check */
	if((NULL != pstr_cntxt) && (NULL != pstr_msg) && (NULL != pstr_cntxt->str_in_param.__usb_send)
		 && (enu_coin_type < USB_WALLET_COIN_INVALID))
	{
		/* cntxt idle operation check */
		if((USB_WALLET_STATE_INVALID == pstr_cntxt->str_cur_op.enu_cur_state) && (USB_WALLET_APP_IDLE_OP == pstr_cntxt->str_cur_op.enu_cur_op))
		{
			if(TWI_FALSE == b_disconnect)
			{
				pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_TRUE;
			}
			/* updating cntxt current app operation */
			pstr_cntxt->str_cur_op.enu_cur_op = USB_WALLET_APP_SIGN_MSG_OP;
			pstr_cntxt->str_cur_op.enu_coin_type = enu_coin_type;

			tstr_usb_sign_msg_info* pstr_sign_msg_info = calloc(1, sizeof(tstr_usb_sign_msg_info));

			TWI_MEMSET(pstr_sign_msg_info, 0x0, sizeof(tstr_usb_sign_msg_info));

			switch (pstr_cntxt->str_cur_op.enu_coin_type)
			{
				case USB_WALLET_COIN_BITCOIN:
				case USB_WALLET_COIN_TEST_BITCOIN:
				{
					TWI_MEMCPY(&pstr_sign_msg_info->uni_sign_msg_info.str_bitcoin_sign_msg.str_msg_info, pstr_msg, sizeof(tstr_usb_bitcoin_msg));
					break;
				}

				case USB_WALLET_COIN_ETHEREUM:
				case USB_WALLET_COIN_TEST_ETHEREUM:
				{
					TWI_MEMCPY(&pstr_sign_msg_info->uni_sign_msg_info.str_ethereum_sign_msg.str_msg_info, pstr_msg, sizeof(tstr_usb_ethereum_msg));
					break;
				}

				default:
					break;
			}	

			pstr_cntxt->str_cur_op.pv = (void*)pstr_sign_msg_info;

			TWI_MEMCPY(pstr_cntxt->str_cur_op.au8_verify_id, pu8_wallet_id, u8_wallet_id_len);
			pstr_cntxt->str_cur_op.u8_verify_id_len = u8_wallet_id_len;

			twi_bool b_is_ready = TWI_FALSE;
			twi_stack_is_ready_to_send(&pstr_cntxt->str_stack_context, &b_is_ready);

			if(TWI_TRUE == b_is_ready)
			{
				if(NULL != pu8_wallet_id)
				{
					pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_ID;
					if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_WALLET_ID_CMD))
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
					}
				}
				else
				{
					//skip wallet ID verification
					pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_REQUEST_OPEN_APP;
					if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_REQUEST_OPEN_APP_CMD))
					{
						current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
					}
				}
			}
			else
			{
				pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_WAITING_TO_CONNECT;
				/* Start Scanning and connect */	
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_scan_and_connect);
				pstr_cntxt->str_in_param.__usb_scan_and_connect(pstr_cntxt->pv_device_info, &pstr_cntxt->str_cur_op.au8_verify_id[DVC_ID_IDX], (twi_u8)DVC_ID_LEN, pstr_cntxt->u16_vid, pstr_cntxt->u16_pid, (twi_u32)USB_SCAN_DURATION_MS, 0);
			}
		}
		else
		{
			pstr_cntxt->str_in_param.__onSignMessageResult(pstr_cntxt->pv_device_info, NULL, (twi_s32)USB_IF_ERR_INVALID_STATE);	
		}
	}
	else
	{
		pstr_cntxt->str_in_param.__onSignMessageResult(pstr_cntxt->pv_device_info, NULL, (twi_s32)USB_IF_ERR_INVALID_ARGS);
	}
}

/*
 *  @function   	twi_usb_if_get_wallet_id
 *	@brief			API to get wallet id.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *  @param[IN]		b_disconnect: boolen to decide if we gonna disconnect after finishing the operation or not.   
 */
void twi_usb_if_get_wallet_id(tstr_usb_if_context* pstr_cntxt, twi_bool b_disconnect)
{
	/* arguments check */
	if((NULL != pstr_cntxt) && (NULL != pstr_cntxt->str_in_param.__usb_send))
	{
		/* cntxt idle operation check */
		if((USB_WALLET_STATE_INVALID == pstr_cntxt->str_cur_op.enu_cur_state) && (USB_WALLET_APP_IDLE_OP == pstr_cntxt->str_cur_op.enu_cur_op))
		{
			if(TWI_FALSE == b_disconnect)
			{
				pstr_cntxt->str_cur_op.b_skip_disconnection = TWI_TRUE;
			}
			/* updating cntxt current app operation */
			pstr_cntxt->str_cur_op.enu_cur_op = USB_WALLET_APP_GET_ID_OP;

			tstr_usb_get_wallet_id_info* pstr_get_wallet_id_info = calloc(1, sizeof(tstr_usb_get_wallet_id_info));
			pstr_cntxt->str_cur_op.pv = pstr_get_wallet_id_info;

			twi_bool b_is_ready = TWI_FALSE;
			twi_stack_is_ready_to_send(&pstr_cntxt->str_stack_context, &b_is_ready);

			if(TWI_TRUE == b_is_ready)
			{
				pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_GET_ID;
				if(TWI_SUCCESS != apdu_cmd_send(pstr_cntxt, USB_WALLET_APDU_GET_WALLET_ID_CMD))
				{
					current_operation_finalize(pstr_cntxt, NULL, 0, (twi_s32)USB_IF_ERR_SEND_FAIL, TWI_FALSE);
				}
			}
			else
			{
				pstr_cntxt->str_cur_op.enu_cur_state = USB_WALLET_STATE_WAITING_TO_CONNECT;
				/* Start Scanning and connect */	
				TWI_ASSERT(NULL != pstr_cntxt->str_in_param.__usb_scan_and_connect);
				pstr_cntxt->str_in_param.__usb_scan_and_connect(pstr_cntxt->pv_device_info, &pstr_cntxt->str_cur_op.au8_verify_id[DVC_ID_IDX], (twi_u8)DVC_ID_LEN, pstr_cntxt->u16_vid, pstr_cntxt->u16_pid, (twi_u32)USB_SCAN_DURATION_MS, 0);
			}
		}
		else
		{
			pstr_cntxt->str_in_param.__onGetWalletIDResult(pstr_cntxt->pv_device_info, NULL, 0, (twi_s32)USB_IF_ERR_INVALID_STATE);	
		}
	}
	else
	{
		pstr_cntxt->str_in_param.__onGetWalletIDResult(pstr_cntxt->pv_device_info, NULL, 0, (twi_s32)USB_IF_ERR_INVALID_ARGS);
	}
}

/*
 *  @function   	twi_usb_if_notify_connected
 *	@brief			API to notify this layer with USB connection.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		s32_err_code: error code to represent the connection status.
 */
void twi_usb_if_notify_connected(tstr_usb_if_context* pstr_cntxt, twi_s32 s32_err_code)
{
	TWI_ASSERT(NULL != pstr_cntxt);			
	if(TWI_SUCCESS == s32_err_code)
	{
		//TODO: this is a workaround to open the port before sending the stack specs
		static twi_u8 au8_open_port_buff[64] = {0};
		au8_open_port_buff[0] = 0x40;
		pstr_cntxt->str_in_param.__usb_send(pstr_cntxt->pv_device_info, au8_open_port_buff, sizeof(au8_open_port_buff));
		
		tstr_twi_usb_evt str_usb_evt;
		TWI_MEMSET(&str_usb_evt, 0x0, sizeof(tstr_twi_usb_evt));
		str_usb_evt.enu_usbd_evt = TWI_USBD_PORT_OPEN;
		twi_stack_handle_usb_evt(&pstr_cntxt->str_stack_context, &str_usb_evt);
#if !defined (FIRMWARE_TARGET) && !defined(WIN32)
		//TWI_ASSERT(0 == pthread_create(&pstr_cntxt->thread, NULL, twi_usb_if_dispatch, (void*)pstr_cntxt));
#endif
	}
	else
	{
		op_state_update(pstr_cntxt, USB_WALLET_OP_STATE_CONNECTION_FAILED_EVENT, NULL);
	}
}

/*
 *  @function   	twi_usb_if_notify_disconnected
 *	@brief			API to notify this layer with USB disconnection.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		u8_reason: disconnection reason.
 *	@param[IN]		s32_err_code: error code to represent the disconnection status.
 */
void twi_usb_if_notify_disconnected(tstr_usb_if_context* pstr_cntxt, twi_u8 u8_reason, twi_s32 s32_err_code)
{
	TWI_ASSERT(NULL != pstr_cntxt);	
	if(TWI_SUCCESS == s32_err_code)
	{
		tstr_twi_usb_evt str_usb_evt;
		TWI_MEMSET(&str_usb_evt, 0x0, sizeof(tstr_twi_usb_evt));
		op_state_update(pstr_cntxt, USB_WALLET_OP_STATE_DISCONNECTION_EVENT , NULL);
		str_usb_evt.enu_usbd_evt = TWI_USBD_PORT_CLOSE;
		twi_stack_handle_usb_evt(&pstr_cntxt->str_stack_context, &str_usb_evt);
#if !defined (FIRMWARE_TARGET) && !defined(WIN32)
		//TWI_ASSERT(0 == pthread_join(pstr_cntxt->thread, NULL));
#endif
	}
}	
/*
 *  @function   	twi_usb_if_notify_send_status
 *	@brief			API to notify this layer with USB sending status.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		s32_err_code: error code to represent the sending status.
 */
void twi_usb_if_notify_send_status(tstr_usb_if_context* pstr_cntxt, twi_s32 s32_err_code)
{
	if((pstr_cntxt->str_cur_op.enu_cur_state > USB_WALLET_STATE_WAITING_TO_DISCONNECT) && (pstr_cntxt->str_cur_op.enu_cur_state < USB_WALLET_STATE_INVALID))
	{
		tstr_twi_usb_evt str_usb_evt;
		TWI_MEMSET(&str_usb_evt, 0x0, sizeof(tstr_twi_usb_evt));	

		TWI_ASSERT(NULL != pstr_cntxt);	

		if(TWI_SUCCESS == s32_err_code)
		{
			str_usb_evt.enu_usbd_evt = TWI_USBD_TX_DONE;
		}
		else
		{
			str_usb_evt.enu_usbd_evt = TWI_USBD_TX_FAIL;
		}	

		twi_stack_handle_usb_evt(&pstr_cntxt->str_stack_context, &str_usb_evt);
	}
}

#if 0
void twi_usb_if_notify_cloud_response(twi_u8* const pu8_data, twi_u32 u32_data_sz, twi_s32 s32_err_code)
{
	//TODO		
}

void twi_usb_if_notify_loaded_data(twi_u16 id, twi_u8* const pu8_data, twi_u32 u32_data_sz, twi_s32 s32_error_code)
{
	//TODO
}
#endif 

/*
 *  @function   	twi_usb_if_notify_timer_fired
 *	@brief			API to notify this layer upon timer firing.
 *	@param[IN]		idx: Timer idx.
 *	@param[IN]		s32_err_code: error code to represent the timer status. 
 */
void twi_usb_if_notify_timer_fired(twi_u32 u32_idx, twi_s32 s32_err_code)
{
	//TODO
}

/*
 *  @function   	twi_usb_if_notify_data_received
 *	@brief			API to Notify this layer with the data received.
 *	@param[IN]		pstr_cntxt: pointer to an interface context.
 *	@param[IN]		u16_buf_len: the length of the received data.
 *	@param[IN]		pu8_buf: pointer to the data recevied. 
 *	@param[IN]		s32_err_code: error code to represent the receiving status.  
 */
void twi_usb_if_notify_data_received(tstr_usb_if_context* pstr_cntxt, twi_u8* pu8_buf, twi_u16 u16_buf_len, twi_s32 s32_err_code)
{
	if(TWI_SUCCESS == s32_err_code)
	{	
		TWI_ASSERT(NULL != pstr_cntxt);	
		tstr_twi_usb_evt str_usb_evt;
		TWI_MEMSET(&str_usb_evt, 0x0, sizeof(tstr_twi_usb_evt));

		str_usb_evt.enu_usbd_evt = TWI_USBD_RX_DONE;
		str_usb_evt.pu8_data = pu8_buf;
		str_usb_evt.u16_len = u16_buf_len; 
		twi_stack_handle_usb_evt(&pstr_cntxt->str_stack_context, &str_usb_evt);	
	}
}

/*
 *  @function   	twi_usb_if_dispatch
 *	@brief			Dispatcher of USB Wallet Interface Module.
 *	@param[IN]		arg: void pointer to an interface context.
 */
void* twi_usb_if_dispatch(void* arg)
{
	tstr_usb_if_context* pstr_cntxt = (tstr_usb_if_context*)arg;
#if !defined (FIRMWARE_TARGET) && !defined(WIN32)
	while(TWI_FALSE == twi_stack_is_idle(&pstr_cntxt->str_stack_context))
#endif	
	{	
		if(pstr_cntxt->str_cur_op.enu_cur_state == USB_WALLET_STATE_WAITING_TO_CONNECT)
		{
			twi_bool b_is_ready = TWI_FALSE;
			twi_stack_is_ready_to_send(&pstr_cntxt->str_stack_context, &b_is_ready);
			if(TWI_TRUE == b_is_ready)
			{
				op_state_update(pstr_cntxt, USB_WALLET_OP_STATE_CONNECTION_EVENT , NULL);
			}	
		}

		if(NULL != pstr_cntxt)
		{	
			twi_stack_dispatcher(&pstr_cntxt->str_stack_context);
		}
	}

	return 0;
}

void twi_usb_if_is_ready_to_send(tstr_usb_if_context* pstr_cntxt, twi_bool* pb_is_ready)
{
	twi_stack_is_ready_to_send(&pstr_cntxt->str_stack_context, pb_is_ready);
}

void twi_usb_if_get_size(tenu_twi_usb_buff_types enu_buff_type, twi_u32* pu32_size)
{
	TWI_ASSERT((NULL != pu32_size) && (enu_buff_type < USB_WALLET_INVALID_BUF));

	switch(enu_buff_type)
	{
		case USB_WALLET_CMD_INPUT_BUF:
		{
			*pu32_size = USB_WALLET_CMD_INPUT_MAX_SZ;
			break;
		}    

		case USB_WALLET_APDU_BUF:
		{
			*pu32_size = USB_WALLET_APDU_BUFFER_MAX_SZ;
			break;
		}    		
		
		case USB_WALLET_PATH_STEPS:
		{
			*pu32_size = USB_WALLET_PATH_MAX_STEPS;
			break;
		}    

		case USB_WALLET_PATH_STEP_BUF:
		{
			*pu32_size = USB_WALLET_PATH_STEP_SZ;
			break;
		}    

		case USB_WALLET_SIGNING_TX_BUF:
		{
			*pu32_size = USB_WALLET_SIGNING_TX_MAX_LEN;
			break;
		}    

		case USB_WALLET_INTERNAL_INPUTS_NUM:
		{
			*pu32_size = USB_WALLET_INTERNAL_INPUTS_MAX_NUM;
			break;
		}    

		case USB_WALLET_INTERNAL_OUTPUTS_NUM:
		{
			*pu32_size = USB_WALLET_INTERNAL_OUTPUTS_MAX_NUM;
			break;
		}    

		case USB_WALLET_TX_INPUTS_NUM:
		{
			*pu32_size = USB_WALLET_TX_INPUTS_MAX_NUM;
			break;
		}    

		case USB_WALLET_LOCK_SCRIPT_BUF:
		{
			*pu32_size = USB_WALLET_LOCK_SCRIPT_MAX_LEN;
			break;
		}    

		case USB_WALLET_SIGHASH_VALUE_BUF:
		{
			*pu32_size = USB_WALLET_SIGHASH_VALUE_LEN;
			break;
		}    

		case USB_WALLET_SIGNED_INPUT_BUF:
		{
			*pu32_size = USB_WALLET_SIGNED_INPUT_MAX_LEN;
			break;
		}

		default:
		{
			//TODO: assert or return an error
			break;
		}	
	}
}
