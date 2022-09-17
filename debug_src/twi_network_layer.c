/****************************************************************************/
/* Copyright (c) 2022 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/



/**
 * @file:	twi_network_layer.c
 * @brief:	This file implements the network manager layer in the communication stack.
 */

#include "twi_network_layer.h"
#include "twi_link_layer.h"
#if defined (TWI_BLE_STACK_ENABLED)
#include "twi_ble_hal_conf.h"
#endif
#include "twi_common.h"
#include "crc_16.h"

#define NTWRK_LOG_ERR(...)
#define NTWRK_LOG_INFO(...)
#define NTWRK_LOG(...)
#define NTWRK_LOG_HEX(MSG, HEX_BUFFER, LEN)

#define NTWRK_LOG_LEVEL_NONE	(0) 
#define NTWRK_LOG_LEVEL_ERR		(1)
#define NTWRK_LOG_LEVEL_INFO	(2)
#define NTWRK_LOG_LEVEL_DBG		(3)

#define NTWRK_LOG_ENABLE
#define NTWRK_LOG_LEVEL 		NTWRK_LOG_LEVEL_DBG

#ifndef NTWRK_LOG_LEVEL
	#define NTWRK_LOG_LEVEL 		NTWRK_LOG_LEVEL_DBG
#endif

#if defined (NTWRK_LOG_ENABLE) && defined(DEBUGGING_ENABLE)  
	#include "twi_debug.h"
#if (NTWRK_LOG_LEVEL >= NTWRK_LOG_LEVEL_ERR)

    #undef NTWRK_LOG_ERR
	#define	NTWRK_LOG_ERR(...)							 do{TWI_LOGGER_ERR("[NTRWK]: "__VA_ARGS__);}while(0)

    #if (NTWRK_LOG_LEVEL >= NTWRK_LOG_LEVEL_INFO)

        #undef NTWRK_LOG_INFO
        #define NTWRK_LOG_INFO(...)  					 do{TWI_LOGGER_INFO("[NTRWK]: "__VA_ARGS__);}while(0)

        #if (NTWRK_LOG_LEVEL >= NTWRK_LOG_LEVEL_DBG)

            #undef NTWRK_LOG
			#define NTWRK_LOG(...)						 do{TWI_LOGGER("[NTRWK]: "__VA_ARGS__);}while(0)
            #undef NTWRK_LOG_HEX
            #define NTWRK_LOG_HEX(MSG, HEX_BUFFER, LEN)  do{TWI_DUMP_BUF("[NTRWK]: "MSG,HEX_BUFFER,LEN);}while(0)
        #endif
    #endif
#endif
#endif

#define FRAGMENT_HEADER_LEN    		   ((twi_u8) sizeof(tstr_fragment_header) )                     /** @brief:	Macro that describes fragment header length. */

#define RESEND_FRAGMENT_TIMES 		    3
#define SEND_FRAGMENT_TIMES 		    1
#define RESEND_PACKET_TIMES 			3
#define BUFFER_SIZE_FOR_WINDOWS 		600
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
 *	@brief: This is the callback function that is used to notify the network manager layer with the link layer events.
 *	@param[in]  pstr_evt: 		a pointer to the link layer structure that contains all the needed event(s) data.
*/
TWI_STATIC_FN void twi_ll_cb(tstr_twi_ll_evt* pstr_evt);

/**
 *	@brief:	This function initializes all the network manager layer global variables used.
 *	@param[in]  pstr_ctx: 		a pointer to the context structure that contains all the needed context data.
*/
static void twi_init_nl_global_variables(tstr_nl_ctx *pstr_ctx);

/**
 *	@brief: This is the function that is used to fragment the PDU from APP to MTUs and send it to the link layer
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*/
static twi_s32 twi_nl_snd_fgmnts(tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len);

/**
 *	@brief:	This function calculate fragmentation data needed.
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
 *	@param [in]	pu8_data		Pointer to data to be sent. The data will be fragmented if needed to.
 *                              This buffer shall remain untouched by the application till a TWI_NL_SEND_STATUS_EVT is passed from the network layer.
 *	@param [in]	u16_data_len    Data length.
 *  @param [in]	pv_arg    		User argument.
*/
static void twi_nl_fragment_prepare(tstr_nl_ctx *pstr_ctx ,twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg);

/**
 *	@brief:	This function receive fragments and make defragmentation 
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
 *	@param [in]	pu8_data		Pointer to data to be receive. The data will be defragmented if needed to.
 *                              This buffer shall remain untouched by the LL till a TWI_NL_RCV_DATA_EVT is passed from the network layer.
 *	@param [in]	u16_data_len    Data length.
*/
static tenu_stack_err_code  twi_nl_defragment( tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len);

/**
 *	@brief: Receive and Check Fragment Header
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*/
static tenu_stack_err_code  twi_nl_validate_rcv_fgmnt_header( tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data);

/**
 *	@brief: Receive Fragments 
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*/
static tenu_stack_err_code  twi_nl_rcv_fgmnt( tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len );

/**
 *	@brief: Propagate Failure Sending To the upper Layer
 *	@param [in]  pstr_ctx  : 	a pointer to the context structure that contains all the needed context data.
*/
static void twi_nl_propagate_snd_fail(tstr_nl_ctx *pstr_ctx );

/**
 *	@brief: Preparing fragment structure for next packet 
*/
static void twi_nl_prepare_frgmt_next_pkt(tstr_nl_ctx *pstr_ctx);

/**
 *	@brief: Preparing defragment structure for next packet 
*/
static void twi_nl_prepare_defrgmt_next_pkt(tstr_nl_ctx *pstr_ctx);

/**
*	@brief		This is an API to get the size of fragment payload = (twi_nl_get_fragment_threshold_size() - FRAGMENT_HEADER_LEN ).
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@return	    The fragment payload size by the network layer.
*/
static twi_u16 twi_nl_get_fragment_payload_size(tstr_nl_ctx *pstr_ctx);

/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/

/**
 *	@brief: This is the callback function that is used to notify the network manager layer with the link layer events.
 *	@param[in]  pstr_evt: 		a pointer to the link layer structure that contains all the needed event(s) data.
*/
TWI_STATIC_FN void twi_ll_cb(tstr_twi_ll_evt* pstr_evt)
{
	TWI_ASSERT(NULL != pstr_evt); 
	TWI_ASSERT(NULL != pstr_evt->pv_args); 

	tstr_twi_nl_evt str_nl_evt;
	tstr_nl_ctx * pstr_ctx = (tstr_nl_ctx *) pstr_evt->pv_args;

	TWI_MEMSET(&str_nl_evt, 0, sizeof(tstr_twi_nl_evt));
	
	NTWRK_LOG("EVENT = %d\r\n", pstr_evt->enu_event);
	switch(pstr_evt->enu_event)
	{
		case TWI_LL_SEND_STATUS_EVT:
		{
			NTWRK_LOG_INFO("TWI_LL_SEND_STATUS_EVT\r\n");
			/* check if the fragment is sent successfully */
			if( TWI_TRUE == pstr_evt->uni_data.str_send_stts_evt.b_is_success)
			{	
				pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index++;

				/* Send next fragment */
				if ( pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index < pstr_ctx->str_global.str_twi_nl_fgmt_data.u8_frgmts_num )
				{
					pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);
	
					pstr_ctx->str_global.u8_resend_frgmt_cnt 					 = 0;
					pstr_ctx->str_global.str_twi_nl_fgmt_data.pu8_pkt_buf 		+= twi_nl_get_fragment_payload_size(pstr_ctx) ;							// Point to next fragment 	
					pstr_ctx->str_global.b_need_send 							 = TWI_TRUE;
					str_nl_evt.enu_event 								   		 = TWI_NL_INVALID_EVT;
				}
				/* Full Packet is sent successfully */
				else
				{
					twi_nl_prepare_frgmt_next_pkt(pstr_ctx);
					
					pstr_ctx->str_global.b_send_in_progress 					 = TWI_FALSE;

					pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_packet_sequence_number = ~ (pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_packet_sequence_number) ;

					str_nl_evt.enu_event 										 = TWI_NL_SEND_STATUS_EVT;
					str_nl_evt.uni_data.str_send_stts_evt.b_is_success 			 = pstr_evt->uni_data.str_send_stts_evt.b_is_success;
					str_nl_evt.uni_data.str_send_stts_evt.pv_user_arg 			 = pstr_evt->uni_data.str_send_stts_evt.pv_user_arg;
				}
			}
			else
			{
				/* In Dispatcher : Resend Fragment up to three times then propagate to the upper layer*/
				pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);
				pstr_ctx->str_global.u8_resend_frgmt_cnt++;
				pstr_ctx->str_global.b_need_send 								 = TWI_TRUE;
				str_nl_evt.enu_event 											 = TWI_NL_INVALID_EVT;
			}
			break;
		}

		case TWI_LL_RCV_DATA_EVT:
		{	
			NTWRK_LOG_INFO("TWI_LL_RCV_DATA_EVT\r\n");
			tenu_stack_err_code enu_retval = TWI_STACK_INVALID_ERR_CODE;

			if(TWI_FALSE == pstr_ctx->str_global.str_twi_nl_defgmt_data.b_is_locked)
			{
				/* Receive Fragment */
				enu_retval = twi_nl_defragment( pstr_ctx , pstr_evt->uni_data.str_rcv_data_evt.pu8_data , pstr_evt->uni_data.str_rcv_data_evt.u16_data_len) ;
			}

			/* Success */
			if((TWI_STACK_INVALID_ERR_CODE == enu_retval) && (TWI_FALSE == pstr_ctx->str_global.str_twi_nl_defgmt_data.b_is_locked))
			{
				/* receive fragment header from first byte of fragment */
				tstr_fragment_header str_fragment_header;

				TWI_MEMCPY( &str_fragment_header , pstr_evt->uni_data.str_rcv_data_evt.pu8_data, FRAGMENT_HEADER_LEN );

				if ( 1 == str_fragment_header.u8_last_fragment_flag )
				{
					/* Logging Full Reassembled Packet For Testing */
					NTWRK_LOG_HEX("reassembled Packet : \r\n", pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf , pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx );

					str_nl_evt.enu_event 									= TWI_NL_RCV_DATA_EVT;
					str_nl_evt.uni_data.str_rcv_data_evt.pu8_data			= pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf ;
					str_nl_evt.uni_data.str_rcv_data_evt.u16_data_len		= pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx ;

					/*Lock the buffer, to be unlocked from the app layer*/
					pstr_ctx->str_global.str_twi_nl_defgmt_data.b_is_locked 				= TWI_TRUE;
					/* Expect next packet */
					twi_nl_prepare_defrgmt_next_pkt(pstr_ctx);
				}
				else
				{
					str_nl_evt.enu_event 									= TWI_NL_INVALID_EVT;
				}
			}	
			/* Fragmentation Error */
			else if ( ( enu_retval >=TWI_NL_ERR_BASE ) && ( enu_retval < TWI_SL_ERR_BASE ) ) 
			{
				NTWRK_LOG_ERR("Failed To Receive Data From Link Layer With Error = %d\r\n", enu_retval);
				
				/* discard all the available fragments of the current packet sequence number  */ 
				twi_nl_prepare_defrgmt_next_pkt(pstr_ctx);
				TWI_MEMSET( pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf, 0 , sizeof(pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf) );

				/* send the fragmentation error message to the fragmentation layer of the initiator on the control characteristic, with the specific error type */	
				twi_ll_send_error( pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx) , enu_retval ,pstr_evt->uni_data.str_rcv_data_evt.pu8_data ,  sizeof(tenu_stack_err_code) );

				str_nl_evt.enu_event 									= TWI_NL_INVALID_EVT;
			}
			/* Non-Fragmentation Error */
			else
			{	
				/* discard all the available fragments of the current packet sequence number  */ 
				twi_nl_prepare_defrgmt_next_pkt(pstr_ctx);
				TWI_MEMSET( pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf, 0 , sizeof(pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf) );

				NTWRK_LOG_ERR("Failed To Receive Data From Link Layer With Error = %d\r\n", enu_retval);
				str_nl_evt.enu_event 									= TWI_NL_RCV_DATA_EVT;
				str_nl_evt.uni_data.str_rcv_data_evt.pu8_data			= pstr_evt->uni_data.str_rcv_data_evt.pu8_data;
				str_nl_evt.uni_data.str_rcv_data_evt.u16_data_len		= pstr_evt->uni_data.str_rcv_data_evt.u16_data_len ;
			}
			break;
		}	

		case TWI_LL_RCV_ERROR_EVT:
		{
			NTWRK_LOG_INFO("TWI_LL_RCV_ERROR_EVT\r\n");
			if (  ( pstr_evt->uni_data.str_rcv_error_evt.enu_err_code > TWI_NL_ERR_BASE ) && ( pstr_evt->uni_data.str_rcv_error_evt.enu_err_code < TWI_SL_ERR_BASE) )
			{
				/* Fragmentation Error */
				NTWRK_LOG_ERR("Fragmentation Error = %d\r\n", pstr_evt->uni_data.str_rcv_error_evt.enu_err_code);
				/* Resend fragment */
				pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);
				pstr_ctx->str_global.b_need_send 						= TWI_TRUE;
				pstr_ctx->str_global.u8_resend_frgmt_cnt++;
	
				str_nl_evt.enu_event 									= TWI_NL_INVALID_EVT;
			}	
			else
			{
				/* Non-Fragmentation Error */
				NTWRK_LOG_ERR("Non Fragmentation Error!\r\n");
				str_nl_evt.enu_event 										= TWI_NL_RCV_ERROR_EVT;
				str_nl_evt.uni_data.str_rcv_err_evt.enu_err_code			= pstr_evt->uni_data.str_rcv_error_evt.enu_err_code;
				str_nl_evt.uni_data.str_rcv_err_evt.pu8_err_data			= pstr_evt->uni_data.str_rcv_error_evt.pu8_err_data;
				str_nl_evt.uni_data.str_rcv_err_evt.u16_err_data_len		= pstr_evt->uni_data.str_rcv_error_evt.u16_err_data_len;
			}		
			break;
		}		

		default:
		{
			NTWRK_LOG_ERR("Unhandled Link Layer Event =  %d\r\n", pstr_evt->enu_event);
			str_nl_evt.enu_event = TWI_NL_INVALID_EVT;
			break;
		}
	}

	str_nl_evt.pv_args = pstr_ctx->str_global.pv_args;
	if(TWI_NL_INVALID_EVT != str_nl_evt.enu_event)
	{
		TWI_ASSERT(NULL != pstr_ctx->str_global.pf_nl_cb);
		pstr_ctx->str_global.pf_nl_cb(&str_nl_evt);
	}
}


/**
 *	@brief:	This function initializes all the network manager layer global variables used.
 *	@param [in] pstr_ctx  	 : 	a pointer to the context structure that contains all the needed context data.
*/
static void twi_init_nl_global_variables(tstr_nl_ctx *pstr_ctx)
{

	pstr_ctx->str_global.b_is_initialized 						= TWI_FALSE;
	pstr_ctx->str_global.pf_nl_cb	 							= NULL;
	pstr_ctx->str_global.b_need_send							= TWI_FALSE;
	pstr_ctx->str_global.b_send_in_progress 					= TWI_FALSE;
	pstr_ctx->str_global.u8_resend_frgmt_cnt					= 0;
	pstr_ctx->str_global.u8_resend_packet_cnt  					= 0;
	pstr_ctx->str_global.str_twi_nl_fgmt_data.u8_frgmts_num = 0;

	TWI_MEMSET( &pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header , 0 , FRAGMENT_HEADER_LEN );
	TWI_MEMSET( &pstr_ctx->str_global.str_twi_nl_defgmt_data , 0x00 , sizeof(pstr_ctx->str_global.str_twi_nl_defgmt_data) );
}

/**
 *	@brief:	This function initializes all fragmentation variables used and prepare fragmentaion
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
 *	@param [in]	pu8_data		Pointer to data to be sent. The data will be fragmented if needed to.
 *                              This buffer shall remain untouched by the application till a TWI_NL_SEND_STATUS_EVT is passed from the network layer.
 *	@param [in]	u16_data_len    Data length.
 *  @param [in]	pv_arg    		User argument.
*/
static void twi_nl_fragment_prepare(tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg )
{
	/* TO DO : Substract the full packet header size from data length if the header exists */

	pstr_ctx->str_global.str_twi_nl_fgmt_data.u8_frgmts_num 	= CEIL( ( u16_data_len + CRC_SZ ) , twi_nl_get_fragment_payload_size(pstr_ctx) );	// Calculate the number of fragments 

	pstr_ctx->str_global.str_twi_nl_fgmt_data.pu8_pkt_buf 		= pu8_data ;													// Point to first fragment , TO DO : pu8_data + Packer header length, if exists

	pstr_ctx->str_global.str_twi_nl_fgmt_data.u16_data_len 		= u16_data_len;													// Passing data length to global fragmentation structure

	pstr_ctx->str_global.str_twi_nl_fgmt_data.pv_arg 			= pv_arg;														// Passing user argment to global fragmentation structure 
	
	NTWRK_LOG_INFO("Fragments Number: %d \r\n", pstr_ctx->str_global.str_twi_nl_fgmt_data.u8_frgmts_num  );					// Logging Fragment For Info 
}

/**
 *	@brief: This is the function that is used to fragment the PDU from APP to fragments and send it to the link layer
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*/
static twi_s32 twi_nl_snd_fgmnts(tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len)
{
	TWI_ASSERT( (NULL != pu8_data) && (0 != u16_data_len) );

	twi_s32 s32_retval = TWI_SUCCESS ;
#ifndef WIN32
	twi_u8	au8_frgmt_buf[twi_nl_get_fragment_threshold_size(pstr_ctx)];																 // U8 Array buffer to save ( fragment header , fragment data ) and send it 
#else
	twi_u8*	au8_frgmt_buf = calloc(1, twi_nl_get_fragment_threshold_size(pstr_ctx));
#endif
	TWI_MEMSET( au8_frgmt_buf , 0 , sizeof(au8_frgmt_buf)  );													 // Clear Data Buffer 

	twi_u16 u16_fgmnt_size = 0;																					 // U16 to save fragment size <= twi_nl_get_fragment_threshold_size()  = ( FRAGMENT_HEADER_LEN + twi_nl_get_fragment_payload_size()  ) 

	/* Check if we need to resend fragment or not : in case of resending we shouldn't update the fragment structure with the next fragment data and resend the fragment */
	if ( 0 == pstr_ctx->str_global.u8_resend_frgmt_cnt  )
	{
		/* Set last fragment flag if this is the last fragment */
		if ( pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index == ( pstr_ctx->str_global.str_twi_nl_fgmt_data.u8_frgmts_num - 1 ) )
		{
			pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_last_fragment_flag = 1;
		}
	}	
	else
	{
		/************** Resend the fragment *****************/
		NTWRK_LOG_INFO("Resend Fragment Times : %d\r\n", pstr_ctx->str_global.u8_resend_frgmt_cnt );
	}

	/* Put fragment header on first byte of buffer */
	TWI_MEMCPY( au8_frgmt_buf , &pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header , 1 );

	/* Check the last fragment */
	if (  1 != pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_last_fragment_flag  )
	{
		twi_u16 u16_remain_sz = u16_data_len - (pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index * twi_nl_get_fragment_payload_size(pstr_ctx));
		u16_fgmnt_size = (u16_remain_sz >= twi_nl_get_fragment_payload_size(pstr_ctx))? twi_nl_get_fragment_threshold_size(pstr_ctx): (u16_remain_sz + FRAGMENT_HEADER_LEN);		
		/* Filling fragment data in fragment buffer after fragment header */
		TWI_MEMCPY( &au8_frgmt_buf[1] , pu8_data , u16_fgmnt_size - FRAGMENT_HEADER_LEN);
	}
	else
	{
		twi_u16 u16_max_sent_data = pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index  * twi_nl_get_fragment_payload_size(pstr_ctx);
		twi_u16 u16_sent_data = (u16_data_len < u16_max_sent_data)? u16_data_len:u16_max_sent_data;
		u16_fgmnt_size = FRAGMENT_HEADER_LEN + u16_data_len + CRC_SZ - u16_sent_data;
	
		/* Filling remaining data in the last fragment */
		TWI_MEMCPY( &au8_frgmt_buf[1] , pu8_data , ( u16_fgmnt_size - ( CRC_SZ + FRAGMENT_HEADER_LEN ) ) ) ;

		/* Calculate CRC */

		/* Repoint to the start of the full packet to calculate the crc to the full packet */
		if ( 0 != pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index )
		{
			pu8_data -= ( ( pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index ) * twi_nl_get_fragment_payload_size(pstr_ctx) ) ;
		}

		twi_u16 u16_packet_crc = twi_crc16_compute_checksum ( 0, pu8_data , u16_data_len );

		/* Filling The CRC of the Full Packet in the last fragment */
		TWI_MEMCPY( &au8_frgmt_buf[ ( u16_fgmnt_size - CRC_SZ  ) ]  , &u16_packet_crc , CRC_SZ);
	}	

	/************* Send Fragment *************/

	/* Log Fragment For Debuging */
	NTWRK_LOG_INFO("Fragment Header : Fragment Index : %d , Packet Sequence Number : %d , Last Fragment Flag : %d \r\n",pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index , pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_packet_sequence_number , pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_last_fragment_flag);
	NTWRK_LOG_HEX("Fragment : ",au8_frgmt_buf ,u16_fgmnt_size );

	/* Mapping the error Here to NETWORK LAYER BUSY "BLE BUSY" in case of TWI_USE_BLE_STACK or "USB_BUSY" in case of TWI_USE_USB_STACK */
	s32_retval = twi_ll_send_data(pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx) ,au8_frgmt_buf, u16_fgmnt_size , pstr_ctx->str_global.str_twi_nl_fgmt_data.pv_arg ) ;

#if defined (TWI_BLE_STACK_ENABLED)
	if(s32_retval == TWI_ERR_BLE_HAL_NRF_BUSY)
	{
		s32_retval = TWI_STACK_NL_ERR_SEND_MEDIUM_BUSY;
	}
#endif

#if defined (TWI_USB_STACK_ENABLED)
	if(s32_retval == TWI_ERROR_USBD_SEND_BUSY)
	{
		s32_retval = TWI_STACK_NL_ERR_SEND_MEDIUM_BUSY;
	}
#endif
	/*****************************************/

#ifdef WIN32
	free(au8_frgmt_buf);
#endif
	return s32_retval;
}

/**
 *	@brief:	This function receive fragments and make defragmentation 
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
 *	@param [in]	pu8_data		Pointer to data to be receive. The data will be defragmented if needed to.
 *                              This buffer shall remain untouched by the BLE till a TWI_NL_RCV_DATA_EVT is passed from the network layer.
 *	@param [in]	u16_data_len    Data length.
*/
static tenu_stack_err_code  twi_nl_defragment( tstr_nl_ctx *pstr_ctx ,twi_u8* pu8_data, twi_u16 u16_data_len)
{
	tenu_stack_err_code enu_retval = TWI_STACK_INVALID_ERR_CODE;

	TWI_ASSERT(NULL != pu8_data);

	if ( ( u16_data_len > 0 ) && ( u16_data_len <= twi_nl_get_fragment_threshold_size(pstr_ctx) ) ) 
	{
		if (  ( u16_data_len - FRAGMENT_HEADER_LEN )  > 0 )
		{
			enu_retval = twi_nl_validate_rcv_fgmnt_header( pstr_ctx , pu8_data );
			if( TWI_STACK_INVALID_ERR_CODE  == enu_retval)
			{
				enu_retval = twi_nl_rcv_fgmnt( pstr_ctx , pu8_data , u16_data_len );
			}
		}
		else
		{
			enu_retval = TWI_NL_ERR_FRGMNT_TOO_SHORT;
		}
	}
	else
	{
		enu_retval = TWI_NL_ERR_PKT_TOO_LONG;
	}	
	return enu_retval;
}

/**
 *	@brief: Receive and Check Fragment Header
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*/
static tenu_stack_err_code twi_nl_validate_rcv_fgmnt_header( tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data )
{
	tenu_stack_err_code enu_retval = TWI_STACK_INVALID_ERR_CODE;
	
	tstr_fragment_header str_fragment_header;

	/* receive fragment header from first byte of fragment */
	TWI_MEMCPY( &str_fragment_header , pu8_data , FRAGMENT_HEADER_LEN );

	/* Check Duplicated Fragment */
	if ( str_fragment_header.u8_fragment_index != ( pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_fragment_index - 1 ) )
	{
		/* Check Out of order Fragment */
		if ( str_fragment_header.u8_fragment_index == pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_fragment_index )
		{
			/* Check Packet Sequence Number */
			if ( str_fragment_header.u8_packet_sequence_number == pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_packet_sequence_number )
			{
				pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_fragment_index++;	
			}
			else
			{
				enu_retval = TWI_NL_ERR_INCMPLT_PKT;
			}
		}
		else
		{
			enu_retval = TWI_NL_ERR_FRGMNT_OUT_OF_ORDR;
		}
	}
	else
	{
		enu_retval = TWI_NL_ERR_DUPLCT_FRGMNT;
	}
	return enu_retval;
}

/**
 *	@brief: Receive Fragments 
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*/
static tenu_stack_err_code twi_nl_rcv_fgmnt( tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len )
{
	TWI_ASSERT( (NULL != pu8_data) && (0 != u16_data_len) );

	tenu_stack_err_code enu_retval = TWI_STACK_INVALID_ERR_CODE;

	tstr_fragment_header str_fragment_header;

	/* receive fragment header from first byte of fragment */
	TWI_MEMCPY( &str_fragment_header , pu8_data , FRAGMENT_HEADER_LEN );

	/* Check The Packet Size */
	if ( (pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx + u16_data_len ) < MAX_PKT_SZ)
	{
		/* Point to the fragment payload data */
		pu8_data += FRAGMENT_HEADER_LEN ;

		if ( 1 != str_fragment_header.u8_last_fragment_flag)
		{
			/* Substract the fragment header size from data length */
			u16_data_len -= FRAGMENT_HEADER_LEN ;
			
			/* Receive Fragment data in buffer */
			TWI_MEMCPY( &pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf[pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx] , pu8_data , u16_data_len) ;

			/* Increment data buffer index by length of fragment */
			pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx += u16_data_len ;
		}
		else
		{
			/* Substract the fragment header size and 16-bit CRC length from data length */
			u16_data_len -= ( FRAGMENT_HEADER_LEN + CRC_SZ ) ;

			/* Check The Packet Size if the packet is one fragment and hold the fragment header and CRC only and there is no data */ 
			if ( (u16_data_len  > 0)  || (str_fragment_header.u8_fragment_index != 0) )
			{
				/* Receive Fragment data on buffer */
				TWI_MEMCPY( &pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf[pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx] , pu8_data , u16_data_len ) ;

				/* Increment data buffer index by length of fragment */
				pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx +=  u16_data_len ;

				/* Point to the 16-bit CRC of the full packet*/
				pu8_data += u16_data_len ;

				/* Receive 16-bit CRC of the full packet */
				twi_u16 u16_packet_crc = 0;
				TWI_MEMCPY( &u16_packet_crc , pu8_data  , CRC_SZ) ;

				/* Check the Received 16-bit CRC */
				if ( u16_packet_crc != twi_crc16_compute_checksum( 0 , pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf , pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx   ) )
				{
					enu_retval = TWI_NL_ERR_INV_CRC;
				}
			}
			else
			{
				enu_retval = TWI_NL_ERR_PKT_TOO_SHORT;
			}
		}
	}
	else
	{
		enu_retval = TWI_NL_ERR_PKT_TOO_LONG;
	}

	return enu_retval;
}

/**
 *	@brief: Propagate Failure Sending To the upper Layer
 *	@param [in]  pstr_ctx  : 	a pointer to the context structure that contains all the needed context data.
*/
static void twi_nl_propagate_snd_fail(tstr_nl_ctx *pstr_ctx )
{
	tstr_twi_nl_evt str_nl_evt;

	TWI_MEMSET(&str_nl_evt, 0, sizeof(tstr_twi_nl_evt));

	str_nl_evt.enu_event 									= TWI_NL_SEND_STATUS_EVT;
	str_nl_evt.uni_data.str_send_stts_evt.b_is_success 		= TWI_FALSE;
	str_nl_evt.uni_data.str_send_stts_evt.pv_user_arg 		= pstr_ctx->str_global.str_twi_nl_fgmt_data.pv_arg;

	pstr_ctx->str_global.b_need_send 						= TWI_FALSE;
	pstr_ctx->str_global.b_send_in_progress 				= TWI_FALSE;
	twi_nl_prepare_frgmt_next_pkt(pstr_ctx);

	str_nl_evt.pv_args = pstr_ctx->str_global.pv_args;
	
	TWI_ASSERT(NULL != pstr_ctx->str_global.pf_nl_cb);
	pstr_ctx->str_global.pf_nl_cb(&str_nl_evt);
}

/**
 *	@brief: Preparing fragment structure for next packet 
*/
static void twi_nl_prepare_frgmt_next_pkt(tstr_nl_ctx *pstr_ctx)
{
	pstr_ctx->str_global.str_twi_nl_fgmt_data.u8_frgmts_num 						    			 = 0;
	pstr_ctx->str_global.str_twi_nl_fgmt_data.u16_data_len 											 = 0;
	pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index 				 = 0;
	pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_last_fragment_flag 			 = 0;
	pstr_ctx->str_global.str_twi_nl_fgmt_data.pu8_pkt_buf 											 = NULL;
	pstr_ctx->str_global.str_twi_nl_fgmt_data.pv_arg 												 = NULL;
}

/**
 *	@brief: Preparing defragment structure for next packet 
*/
static void twi_nl_prepare_defrgmt_next_pkt(tstr_nl_ctx *pstr_ctx)
{
	pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx 									 = 0;
	pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_fragment_index      	 = 0;
	pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_packet_sequence_number = ~ (pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_packet_sequence_number) ;	
	pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_last_fragment_flag  	 = 0;
}

/**
*	@brief		This is an API to get the size of fragment payload = (twi_nl_get_fragment_threshold_size() - FRAGMENT_HEADER_LEN ).
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@return	    The fragment payload size by the network layer.
*/
static twi_u16 twi_nl_get_fragment_payload_size(tstr_nl_ctx *pstr_ctx)
{
	TWI_ASSERT(pstr_ctx != NULL);
	return ( twi_nl_get_fragment_threshold_size(pstr_ctx) - FRAGMENT_HEADER_LEN ) ;   
}

/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/

/**
*	@brief		This is the init function. It shall be called before any other API.
*	@param [in]  pstr_ctx 		a pointer to the context structure that contains all the needed context data.
*	@param [in]	 pv_arg			User argument.
*	@param [in]	 pf_evt_cb		a Pointer to Function that is called by the Network Layer to pass events to the application.
*	@param [in]	enu_ll_type		    Type of the Link Layer.
*	@param [in]	pstr_helpers		Pointer to structure of the whole layer helper APIs.
*	@param [in]	pv_helpers		    Pointer to void to be passed to helpers Apis.
*/
twi_s32 twi_nl_init(tstr_nl_ctx *pstr_ctx,
						void * pv_args, 
						tpf_nl_cb pf_evt_cb,
                        tenu_twi_ll_type enu_ll_type,
						tstr_stack_helpers * pstr_helpers, void* pv_helpers)
{
	twi_s32 s32_retval;
	if( (pf_evt_cb != NULL) && (pstr_ctx != NULL) && (pstr_helpers != NULL) )
	{
		if(TWI_FALSE == pstr_ctx->str_global.b_is_initialized)
		{
			twi_init_nl_global_variables(pstr_ctx);

			pstr_ctx->enu_ll_type = enu_ll_type;
			pstr_ctx->pf_twi_system_sleep_mode_forbiden = pstr_helpers->pf_twi_system_sleep_mode_forbiden;
			pstr_ctx->pv_stack_helpers = pv_helpers;

			s32_retval = twi_ll_init(	pstr_ctx->enu_ll_type,
										&(pstr_ctx->uni_ll_ctx),
										pstr_ctx, 
										twi_ll_cb,
										pstr_helpers, pv_helpers);
			if(TWI_SUCCESS == s32_retval)
			{
				pstr_ctx->str_global.b_is_initialized 	= TWI_TRUE;
				pstr_ctx->str_global.pf_nl_cb			= pf_evt_cb;
				pstr_ctx->str_global.pv_args			= pv_args;
			}
			else
			{
				NTWRK_LOG_ERR("Failed To Init NL With Error = %d\r\n", s32_retval);
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
*	@param [in] pstr_ctx  	 : 	a pointer to the context structure that contains all the needed context data.
*	@param [in]	pstr_ble_evt :	a Pointer to Structure holds the BLE event data.
*/
void twi_nl_handle_ble_evt(tstr_nl_ctx *pstr_ctx, tstr_twi_ble_evt* pstr_ble_evt)
{
	TWI_ASSERT((pstr_ble_evt != NULL)  && (pstr_ctx != NULL));
	switch(pstr_ble_evt->enu_evt)
	{
		case TWI_BLE_EVT_DISCONNECTED:
		{
			if ( pstr_ctx->str_global.b_send_in_progress)
			{
				/* Propagate Failure Sending To the upper Layer */
				twi_nl_propagate_snd_fail(pstr_ctx);
			}

			pstr_ctx->str_global.b_need_send = TWI_FALSE;
			pstr_ctx->str_global.b_send_in_progress = TWI_FALSE;
			pstr_ctx->str_global.u8_resend_frgmt_cnt = 0;
			pstr_ctx->str_global.u8_resend_packet_cnt = 0;

			pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_packet_sequence_number 			= 0;

			pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx 										= 0;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.b_is_locked												= TWI_FALSE;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_fragment_index    		= 0;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_packet_sequence_number    = 0;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_last_fragment_flag    	= 0;						
			TWI_MEMSET( pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf, 0 , sizeof(pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf) );

			break;
		}
		default:
		{
			break;
		}
	}
	twi_ll_handle_evt(pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx), (void*)pstr_ble_evt);
}
#endif

#if defined (TWI_USB_STACK_ENABLED)
/**
*	@brief		This is a function just pass the event to the Link Layer to handle it.
*	@param [in] pstr_ctx  	 : 	a pointer to the context structure that contains all the needed context data.
*	@param [in]	pstr_usb_evt :	a Pointer to Structure holds the USB event data.
*/
void twi_nl_handle_usb_evt(tstr_nl_ctx *pstr_ctx, tstr_twi_usb_evt* pstr_usb_evt)
{
	TWI_ASSERT((pstr_usb_evt != NULL)  && (pstr_ctx != NULL));
	switch(pstr_usb_evt->enu_usbd_evt)
	{
		case TWI_USBD_PORT_CLOSE:
		{
			if ( pstr_ctx->str_global.b_send_in_progress)
			{
				/* Propagate Failure Sending To the upper Layer */
				twi_nl_propagate_snd_fail(pstr_ctx);
			}

			pstr_ctx->str_global.b_need_send = TWI_FALSE;
			pstr_ctx->str_global.b_send_in_progress = TWI_FALSE;
			pstr_ctx->str_global.u8_resend_frgmt_cnt = 0;
			pstr_ctx->str_global.u8_resend_packet_cnt = 0;

			pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_packet_sequence_number 			= 0;

			pstr_ctx->str_global.str_twi_nl_defgmt_data.u16_pkt_buf_idx 										= 0;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.b_is_locked												= TWI_FALSE;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_fragment_index    		= 0;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_packet_sequence_number    = 0;
			pstr_ctx->str_global.str_twi_nl_defgmt_data.str_expected_frgmnt_header.u8_last_fragment_flag    	= 0;						
			TWI_MEMSET( pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf, 0 , sizeof(pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf) );

			break;
		}
		default:
		{
			break;
		}
	}
	twi_ll_handle_evt(pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx), (void*)pstr_usb_evt);
}
#endif

/**
*	@brief		This is the Network Layer send function
*	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*	@param [in]	pu8_data		Pointer to data to be sent. The data will be fragmented if needed to.
*                               This buffer shall remain untouched by the application till a TWI_NL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_data_len    Data length.
*/
twi_s32 twi_nl_send_data(tstr_nl_ctx *pstr_ctx , twi_u8* pu8_data, twi_u16 u16_data_len, void* pv_arg )
{
	twi_s32 s32_retval = TWI_SUCCESS;
	if(TWI_TRUE == pstr_ctx->str_global.b_is_initialized)
	{
		if ( TWI_FALSE == pstr_ctx->str_global.b_send_in_progress )
		{
			if((pu8_data != NULL) && (u16_data_len > 0) && (pstr_ctx != NULL))
			{
				twi_nl_fragment_prepare( pstr_ctx ,pu8_data, u16_data_len , pv_arg );
				pstr_ctx->str_global.u8_resend_frgmt_cnt	= 0;
				pstr_ctx->str_global.u8_resend_packet_cnt 	= 0;
				pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);
				pstr_ctx->str_global.b_need_send 			= TWI_TRUE;
				pstr_ctx->str_global.b_send_in_progress		= TWI_TRUE;			

			}
			else
			{
				s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
			}
		}
		else
		{
			s32_retval = TWI_STACK_NL_ERR_SENDING_IN_PROG;
		}
	}
	else
	{
		s32_retval = TWI_ERROR_NOT_INITIALIZED;
	}
	return s32_retval;
}

/**
*	@brief		This is the Network Layer send error function.
*               This function just pass the upper layer's error to the Link Layer to send it.
*	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
*	@param [in]	enu_err_code	Error code to be sent.
*	@param [in]	pu8_data		Pointer to error data to be sent.
*                               This buffer shall remain untouched by the application till a TWI_LL_SEND_STATUS_EVT is passed from the network layer.
*	@param [in]	u16_data_len    Data length.
*/
twi_s32 twi_nl_send_error( tstr_nl_ctx *pstr_ctx , tenu_stack_err_code enu_err_code, twi_u8* pu8_error_data, twi_u16 u16_error_len)
{
	twi_s32 s32_retval = TWI_SUCCESS;
	if(TWI_TRUE == pstr_ctx->str_global.b_is_initialized)
	{
		if((enu_err_code < TWI_STACK_INVALID_ERR_CODE) && (pu8_error_data != NULL) && (u16_error_len > 0) && (pstr_ctx != NULL))
		{
			s32_retval = twi_ll_send_error(pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx) , enu_err_code, pu8_error_data, u16_error_len);
			if( TWI_SUCCESS != s32_retval )
			{
				NTWRK_LOG_ERR("Failed To Send Error Data to Link Layer With Error = %d\r\n", s32_retval);
			}
		}
		else
		{
			s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
		}
	}
	else
	{
		s32_retval = TWI_ERROR_NOT_INITIALIZED;	
	}
	return s32_retval;
}

/**
 *	@brief:	This is the Network Manager Layer dispatcher. This is used to handle the periodic events of the network manager.
 *	@param [in] pstr_ctx  	  	pointer to the context structure that contains all the needed context data.
 *  Usage : send Fragment and in case of failing Resend it up to three times then resend the packet up to three times then propagate to the upper layer 
*/
void twi_nl_dispatcher(tstr_nl_ctx *pstr_ctx)
{
	TWI_ASSERT(pstr_ctx != NULL);
	if ( pstr_ctx->str_global.b_need_send )
	{
		pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_FALSE);
		pstr_ctx->str_global.b_need_send 		= TWI_FALSE;

		pstr_ctx->str_global.b_send_in_progress = TWI_TRUE;

		twi_s32 s32_retval = TWI_ERROR;

		if (  pstr_ctx->str_global.u8_resend_frgmt_cnt < ( SEND_FRAGMENT_TIMES + RESEND_FRAGMENT_TIMES ) )
		{	
			s32_retval = twi_nl_snd_fgmnts(pstr_ctx , pstr_ctx->str_global.str_twi_nl_fgmt_data.pu8_pkt_buf , pstr_ctx->str_global.str_twi_nl_fgmt_data.u16_data_len );
			
			if( TWI_SUCCESS != s32_retval)
			{					
				if ( TWI_STACK_NL_ERR_SEND_MEDIUM_BUSY == s32_retval )
				{	
					/* Resend fragment */
					pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);				
					pstr_ctx->str_global.b_need_send 		= TWI_TRUE;
					pstr_ctx->str_global.u8_resend_frgmt_cnt++;

				}
				else 
				{
					/* Propagate Failure Sending To the upper Layer */
					twi_nl_propagate_snd_fail(pstr_ctx);
				}	
			}
			else
			{
				// Do Nothing
			}
		}
#if 0		
		else if ( pstr_ctx->str_global.u8_resend_packet_cnt < RESEND_PACKET_TIMES)
		{
			/******************** Resend The Packet ************************/
			pstr_ctx->str_global.u8_resend_frgmt_cnt = 0;
			pstr_ctx->pf_twi_system_sleep_mode_forbiden(pstr_ctx->pv_stack_helpers, TWI_TRUE);	
			pstr_ctx->str_global.b_need_send 		 = TWI_TRUE;
			pstr_ctx->str_global.u8_resend_packet_cnt++;
			NTWRK_LOG_INFO("Resend Packet Times : %d\r\n", pstr_ctx->str_global.u8_resend_packet_cnt );
				
			/* Repoint to the start of the full packet to start fragment and resend it  , if condition : if conditions are false the pointer already point to the start of the packet */
			if ( ( 1 != pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_last_fragment_flag ) && ( 0 != pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index ) )
			{
				pstr_ctx->str_global.str_twi_nl_fgmt_data.pu8_pkt_buf -= ( ( pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index ) * twi_nl_get_fragment_payload_size() ) ;
			}

			/* Back to First Fragment Header */
			pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_fragment_index 	= 0 ;
			pstr_ctx->str_global.str_twi_nl_fgmt_data.str_fragment_header.u8_last_fragment_flag = 0 ;

			twi_nl_fragment_prepare( pstr_ctx ,pstr_ctx->str_global.str_twi_nl_fgmt_data.pu8_pkt_buf , pstr_ctx->str_global.str_twi_nl_fgmt_data.u16_data_len , pstr_ctx->str_global.str_twi_nl_fgmt_data.pv_arg ); 
		}
#endif		
		else
		{
			/* Propagate Failure Sending To the upper Layer */
			twi_nl_propagate_snd_fail(pstr_ctx);
		}
	}
	else
	{
		// Do Nothing
	}

	twi_ll_dispatcher(pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx));
}

/**
*	@brief		This is an API to unlock the receive buffer in the Network Layer.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@param [in]	pu8_buffer_to_unlock		Pointer to buffer to unlock.
*
*	@return	    None.
*/
void twi_nl_unlock_rcv_buf( tstr_nl_ctx *pstr_ctx , twi_u8* pu8_buffer_to_unlock)
{
	if(pstr_ctx->str_global.str_twi_nl_defgmt_data.au8_pkt_buf == pu8_buffer_to_unlock)
	{
		//PLATFORM_CRITICAL_SECTION_ENTER();
		pstr_ctx->str_global.str_twi_nl_defgmt_data.b_is_locked = TWI_FALSE;
		//PLATFORM_CRITICAL_SECTION_EXIT();
	}
}

/**
*	@brief		This is an API to get the size of fragment threshold.
*	@param [in]	pstr_ctx			Pointer to structure of the whole layer context.
*	@return	    The fragment threshold size by the network layer.
*/
twi_u16 twi_nl_get_fragment_threshold_size(tstr_nl_ctx *pstr_ctx)
{
	TWI_ASSERT(pstr_ctx != NULL);
	return ( twi_ll_get_mtu_size(pstr_ctx->enu_ll_type, &(pstr_ctx->uni_ll_ctx)));	
}

void twi_nl_is_ready_to_send(tstr_nl_ctx* pstr_cntxt, twi_bool* pb_is_ready)
{
	TWI_ASSERT(pstr_cntxt != NULL);		
	twi_ll_is_ready_to_send(pstr_cntxt->enu_ll_type, &(pstr_cntxt->uni_ll_ctx), pb_is_ready);
}

twi_bool twi_nl_is_idle(tstr_nl_ctx* pstr_cntxt)
{
	TWI_ASSERT(pstr_cntxt != NULL);	
	return twi_ll_is_idle(pstr_cntxt->enu_ll_type, &(pstr_cntxt->uni_ll_ctx));
}

