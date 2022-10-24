/****************************************************************************/
/* Copyright (c) 2021 Thirdwayv, Inc. All Rights Reserved. 					*/
/****************************************************************************/

/*
 * twi_apdu_parser_composer.c
 */

#include "twi_apdu_parser_composer.h"
#include "twi_common.h"

/*Application Protocol Data Unit (APDU)*/

/*
Command APDU:
_____________________________________________________________________________________________________________________________________________________________________________
|CLA		    |1 Byte		        |Instruction class - indicates the type of command						                                                                |
|INS		    |1 Byte		        |Instruction code - indicates the specific command						                                                                |
|P1			    |1 Byte		        |Instruction parameter 1 for the command								                                                                |
|P2			    |1 Byte		        |Instruction parameter 2 for the command								                                                                |
|Lc			    |0, 1 or 3 Bytes	|0 bytes denotes Nc=0                                                                                                                   |
|               |                   |1 byte with a value from 1 to 255 denotes Nc with the same length                                                                      |
|               |                   |3 bytes, the first of which must be 0, denotes Nc in the range 1 to 65 535                                                             |
|               |                   |                                                                                                                                       |   
|CMD DATA(Nc)	|Lc Bytes 	        |Command data with Lc bytes	                                                                                                            |
|               |                   |                                                                                                                                       |											                                                                |
|Le             |0, 1, 2 or 3 Bytes |0 bytes denotes Ne=0                                                                                                                   |
|               |                   |1 byte in the range 1 to 255 denotes that value of Ne, or 0 denotes Ne=256                                                             |
|               |                   |2 bytes (if extended Lc was present in the command) in the range 1 to 65 535 denotes Ne of that value, or two zero bytes denotes 65 536|
|               |                   |3 bytes (if Lc was not present in the command), the first of which must be 0, denote Ne in the same way as two-byte Le                 |
|_______________|___________________|_______________________________________________________________________________________________________________________________________|
*/

/*
Response APDU:
_____________________________________________________________________________________________________
|RSP DATA(Ne)	|Le Bytes 	|Reponse data (can be empty)											|
|SW			    |2 Bytes	|Status word containing cmd processing status (e.g. 0x9000 for success)	|
|___________________________________________________________________________________________________|
*/

#define TWI_OFFSET_CLA   	0
#define TWI_OFFSET_INS   	1
#define TWI_OFFSET_P1    	2
#define TWI_OFFSET_P2    	3
#define TWI_OFFSET_LC_LE    4

/*---------------------------------------------------------*/
/*- TYPEDEFS ----------------------------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- GLOBAL VARIABLES --------------------------------------*/
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*---------------------------------------------------------*/


/*---------------------------------------------------------*/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*---------------------------------------------------------*/

/*
 *	@brief			This function is used to parse received APDU command.
 *
 *	@param[in]	pu8_apdu_buf     Pointer to the buffer holding the APDU.
 *	@param[in]	u32_apdu_len     The length of the received APDU.
 *	@param[out]	pstr_command     Pointer to command structure, to parse the command in.
 *
 *  @return		::TWI_SUCCESS in case of success, otherwise refer to @ref  twi_retval.h
*/
twi_s32 twi_apdu_parse_cmd(twi_u8* pu8_apdu_buf, twi_u32 u32_apdu_len, tstr_twi_apdu_command* pstr_command)
{
	twi_s32 		s32_retval 				= TWI_SUCCESS;	
	twi_u32			u32_remaining_data_len 	= 0;
	twi_u8 			u8_lc_first_byte;
	twi_u8 			u8_lc_length;
	twi_u8 			u8_le_length;

	if((NULL != pu8_apdu_buf) && (u32_apdu_len >= TWI_APDU_CMD_HEADER_LEN) && (NULL != pstr_command))
	{
		TWI_MEMSET(pstr_command, 0, sizeof(tstr_twi_apdu_command));

		pstr_command->u8_cla 						= pu8_apdu_buf[TWI_OFFSET_CLA];
		pstr_command->u8_ins 						= pu8_apdu_buf[TWI_OFFSET_INS];
		pstr_command->uni_params.str_p1_p2.u8_p1	= pu8_apdu_buf[TWI_OFFSET_P1];
		pstr_command->uni_params.str_p1_p2.u8_p2	= pu8_apdu_buf[TWI_OFFSET_P2];

		u32_remaining_data_len = (u32_apdu_len - (twi_u16)TWI_APDU_CMD_HEADER_LEN);
		if(u32_remaining_data_len > 0)
		{
			if(1 == u32_remaining_data_len)
			{
				/*Only Le is present and it is 1 - 255 bytes*/
				u8_le_length = 1;
				u8_lc_length = 0;

				pstr_command->u32_max_rsp_data_len = pu8_apdu_buf[TWI_OFFSET_LC_LE];

				if(0 == pstr_command->u32_max_rsp_data_len)
				{
					pstr_command->u32_max_rsp_data_len = 256;
				}
			}
			else
			{
				u8_lc_first_byte 	= pu8_apdu_buf[TWI_OFFSET_LC_LE];

				if(0x00 == u8_lc_first_byte)
				{
					u32_remaining_data_len -= 3;
					/*If there is data still remaining this means that there is command data which means that this is Lc*/
					if((twi_s32)u32_remaining_data_len > 0)
					{
						u8_lc_length = 3;

						pstr_command->u16_cmd_data_len = TWO_BYTE_CONCAT(pu8_apdu_buf[TWI_OFFSET_LC_LE + 1], pu8_apdu_buf[TWI_OFFSET_LC_LE + 2]);

						pstr_command->pu8_cmd_data = &(pu8_apdu_buf[TWI_OFFSET_LC_LE + u8_lc_length]);
						/*Le length is still to be determined*/
					}
					else
					{
						/*Only Le is present and it is 1 - 65535 bytes*/
						u8_le_length = 3;
						u8_lc_length = 0;

						pstr_command->u32_max_rsp_data_len = TWO_BYTE_CONCAT(pu8_apdu_buf[TWI_OFFSET_LC_LE + 1], pu8_apdu_buf[TWI_OFFSET_LC_LE + 2]);

						if(0 == pstr_command->u32_max_rsp_data_len)
						{
							pstr_command->u32_max_rsp_data_len = 65536;
						}
					}
				}
				else
				{
					u32_remaining_data_len -= 1;

					u8_lc_length 						= 1;
					pstr_command->u16_cmd_data_len 		= pu8_apdu_buf[TWI_OFFSET_LC_LE];
					pstr_command->pu8_cmd_data 			= &(pu8_apdu_buf[TWI_OFFSET_LC_LE + u8_lc_length]);
				}

				if(u8_lc_length > 0)		
				{
					u8_le_length = (twi_u8)(u32_remaining_data_len - pstr_command->u16_cmd_data_len);
					if(1 == u8_le_length)
					{
						pstr_command->u32_max_rsp_data_len = pu8_apdu_buf[TWI_OFFSET_LC_LE + pstr_command->u16_cmd_data_len];
						if(0 == pstr_command->u32_max_rsp_data_len)
						{
							pstr_command->u32_max_rsp_data_len = 256;
						}
					}
					else if(2 == u8_le_length)
					{
						pstr_command->u32_max_rsp_data_len = TWO_BYTE_CONCAT(	pu8_apdu_buf[TWI_OFFSET_LC_LE + pstr_command->u16_cmd_data_len],
																				pu8_apdu_buf[TWI_OFFSET_LC_LE + pstr_command->u16_cmd_data_len + 1]);

						if(0 == pstr_command->u32_max_rsp_data_len)
						{
							pstr_command->u32_max_rsp_data_len = 65536;
						}
					}
					else
					{
						/*No Le*/
						pstr_command->u32_max_rsp_data_len = 0;
					}
				}				
			}
		}
		else
		{
			/*No Lc or Le*/
			pstr_command->u16_cmd_data_len = 0;
			pstr_command->u32_max_rsp_data_len = 0;
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}

	return s32_retval;
}

/*
 *	@brief			This function is used to parse received APDU response.
 *
 *	@param[in]	pu8_apdu_buf     Pointer to the buffer holding the APDU.
 *	@param[in]	u32_apdu_len     The length of the received APDU.
 *	@param[out]	pstr_response    Pointer to response structure, to parse the response in.
 *
 *  @return		::TWI_SUCCESS in case of success, otherwise refer to @ref  twi_retval.h
*/
twi_s32 twi_apdu_parse_rsp(twi_u8* pu8_apdu_buf, twi_u32 u32_apdu_len, tstr_twi_apdu_response* pstr_response)
{
	twi_s32 s32_retval = TWI_SUCCESS;

	if((NULL != pu8_apdu_buf) && (u32_apdu_len >= TWI_APDU_SW_LEN) && (NULL != pstr_response))
	{
		TWI_MEMSET(pstr_response, 0, sizeof(tstr_twi_apdu_response));

		pstr_response->pu8_rsp_data 	= pu8_apdu_buf;
		pstr_response->u32_rsp_data_len = (u32_apdu_len - TWI_APDU_SW_LEN);

		pstr_response->u16_sw 			= TWO_BYTE_CONCAT(	pu8_apdu_buf[pstr_response->u32_rsp_data_len], 
															pu8_apdu_buf[pstr_response->u32_rsp_data_len + 1]);
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}

	return s32_retval;
}

/*
 *	@brief			This function is used to compose an APDU command.
 *
 *	@param[in]	pstr_command     Pointer to command structure, to compose the command from.
 *	@param[in_out]	pu32_apdu_len    Pointer to the APDU length (takes the expected max len as input and sets the actual len as output).
 *	@param[out]	pu8_apdu_buf     Pointer to the buffer holding the composed APDU command.
 *
 *  @return		::TWI_SUCCESS in case of success, otherwise refer to @ref  twi_retval.h
*/
twi_s32 twi_apdu_compose_cmd(tstr_twi_apdu_command* pstr_command, twi_u32* pu32_apdu_len, twi_u8* pu8_apdu_buf)
{
	twi_s32 s32_retval = TWI_SUCCESS;

	twi_u32 u32_needed_space 	= 0;
	twi_u32 u32_offset 			= 0;

	if((NULL != pu8_apdu_buf) && (NULL != pu32_apdu_len) && (NULL != pstr_command))
	{
		TWI_MEMSET(pu8_apdu_buf, 0, (*pu32_apdu_len));

		u32_needed_space += TWI_APDU_CMD_HEADER_LEN;

		if((pstr_command->u16_cmd_data_len > 0) && (pstr_command->u16_cmd_data_len <= 255))
		{
			u32_needed_space += (pstr_command->u16_cmd_data_len + 1);
		}
		else if((pstr_command->u16_cmd_data_len > 255) && (pstr_command->u16_cmd_data_len <= 65535))
		{
			u32_needed_space += (pstr_command->u16_cmd_data_len + 3);
		}
		else if(pstr_command->u16_cmd_data_len > 65535)
		{
			s32_retval = TWI_ERROR_INVALID_LEN;
		}

		if(TWI_SUCCESS == s32_retval)
		{
			if((pstr_command->u32_max_rsp_data_len > 0) && (pstr_command->u32_max_rsp_data_len <= 256))
			{
				u32_needed_space += 1;
			}
			else if((pstr_command->u32_max_rsp_data_len > 256) && (pstr_command->u32_max_rsp_data_len <= 65536))
			{
				if(pstr_command->u16_cmd_data_len > 0)
				{
					u32_needed_space += 2;
				}
				else
				{
					u32_needed_space += 3;
				}
			}
			else if(pstr_command->u32_max_rsp_data_len > 65536)
			{
				s32_retval = TWI_ERROR_INVALID_LEN;
			}

			if((u32_needed_space <= (*pu32_apdu_len)) && (TWI_SUCCESS == s32_retval))
			{
				pu8_apdu_buf[TWI_OFFSET_CLA] 	= pstr_command->u8_cla;
				pu8_apdu_buf[TWI_OFFSET_INS] 	= pstr_command->u8_ins;
				pu8_apdu_buf[TWI_OFFSET_P1]	 	= (twi_u8)MOST_SIG_BYTE(pstr_command->uni_params.u16_p1_p2);
				pu8_apdu_buf[TWI_OFFSET_P2]	 	= (twi_u8)LEAST_SIG_BYTE(pstr_command->uni_params.u16_p1_p2);

				u32_offset 						= TWI_OFFSET_LC_LE;

				if((pstr_command->u16_cmd_data_len > 0) && (pstr_command->u16_cmd_data_len <= 255))
				{
					pu8_apdu_buf[u32_offset++] = (twi_u8)pstr_command->u16_cmd_data_len;
				}
				else if((pstr_command->u16_cmd_data_len > 255) && (pstr_command->u16_cmd_data_len <= 65535))
				{
					pu8_apdu_buf[u32_offset++] = (twi_u8)0;
					pu8_apdu_buf[u32_offset++] = (twi_u8)MOST_SIG_BYTE(pstr_command->u16_cmd_data_len);
					pu8_apdu_buf[u32_offset++] = (twi_u8)LEAST_SIG_BYTE(pstr_command->u16_cmd_data_len);
				}

				if(pstr_command->u16_cmd_data_len > 0)
				{
					if(NULL != pstr_command->pu8_cmd_data)
					{
						TWI_MEMCPY(&(pu8_apdu_buf[u32_offset]), pstr_command->pu8_cmd_data, pstr_command->u16_cmd_data_len);
						u32_offset += pstr_command->u16_cmd_data_len;
					}
					else
					{
						s32_retval = TWI_ERROR_INTERNAL_ERROR;
					}
				}

				if(TWI_SUCCESS == s32_retval)
				{
					if((pstr_command->u32_max_rsp_data_len > 0) && (pstr_command->u32_max_rsp_data_len <= 255))
					{
						pu8_apdu_buf[u32_offset++] = (twi_u8)pstr_command->u32_max_rsp_data_len;
					}
					else if(256 == pstr_command->u32_max_rsp_data_len)
					{
						pu8_apdu_buf[u32_offset++] = (twi_u8)0;
					}
					else if((pstr_command->u32_max_rsp_data_len > 256) && (pstr_command->u32_max_rsp_data_len <= 65535))
					{
						if(pstr_command->u16_cmd_data_len > 0)
						{
							pu8_apdu_buf[u32_offset++] = (twi_u8)MOST_SIG_BYTE(pstr_command->u32_max_rsp_data_len);
							pu8_apdu_buf[u32_offset++] = (twi_u8)LEAST_SIG_BYTE(pstr_command->u32_max_rsp_data_len);		
						}
						else
						{
							pu8_apdu_buf[u32_offset++] = (twi_u8)0;
							pu8_apdu_buf[u32_offset++] = (twi_u8)MOST_SIG_BYTE(pstr_command->u32_max_rsp_data_len);	
							pu8_apdu_buf[u32_offset++] = (twi_u8)LEAST_SIG_BYTE(pstr_command->u32_max_rsp_data_len);
						}
					}	
					else if(65536 == pstr_command->u32_max_rsp_data_len)	
					{
						if(pstr_command->u16_cmd_data_len > 0)
						{
							pu8_apdu_buf[u32_offset++] = (twi_u8)0;
							pu8_apdu_buf[u32_offset++] = (twi_u8)0;	
						}
						else
						{
							pu8_apdu_buf[u32_offset++] = (twi_u8)0;
							pu8_apdu_buf[u32_offset++] = (twi_u8)0;
							pu8_apdu_buf[u32_offset++] = (twi_u8)0;	
						}		
					}

					(*pu32_apdu_len) = u32_offset;
				}
			}
			else
			{
				s32_retval = TWI_ERROR_INVALID_LEN;
			}
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}

	return s32_retval;
}

/*
 *	@brief			This function is used to compose an APDU response.
 *
 *	@param[in]	pstr_response    Pointer to response structure, to compose the response from.
 *	@param[inout]	pu32_apdu_len    Pointer to the APDU length (takes the expected max len as input and sets the actual len as output).
 *	@param[out]	pu8_apdu_buf     Pointer to the buffer holding the composed APDU response.
 *
 *  @return		::TWI_SUCCESS in case of success, otherwise refer to @ref  twi_retval.h
*/
twi_s32 twi_apdu_compose_rsp(tstr_twi_apdu_response* pstr_response, twi_u32* pu32_apdu_len, twi_u8* pu8_apdu_buf)
{
	twi_s32 s32_retval = TWI_SUCCESS;

	if((NULL != pu8_apdu_buf) && (NULL != pu32_apdu_len) && (NULL != pstr_response) && ((*pu32_apdu_len) >= TWI_APDU_SW_LEN))
	{
		TWI_MEMSET(pu8_apdu_buf, 0, (*pu32_apdu_len));

		if((*pu32_apdu_len) >= ((pstr_response->u32_rsp_data_len) + TWI_APDU_SW_LEN))
		{
			if(NULL != pstr_response->pu8_rsp_data)
			{
				TWI_MEMCPY(pu8_apdu_buf, pstr_response->pu8_rsp_data, pstr_response->u32_rsp_data_len);
			}

			pu8_apdu_buf[pstr_response->u32_rsp_data_len] 		= (twi_u8)MOST_SIG_BYTE(pstr_response->u16_sw);
			pu8_apdu_buf[pstr_response->u32_rsp_data_len + 1] 	= (twi_u8)LEAST_SIG_BYTE(pstr_response->u16_sw);

			(*pu32_apdu_len) = pstr_response->u32_rsp_data_len + TWI_APDU_SW_LEN;
		}
		else
		{
			s32_retval = TWI_ERROR_INVALID_LEN;
		}
	}
	else
	{
		s32_retval = TWI_ERROR_INVALID_ARGUMENTS;
	}

	return s32_retval;
}
