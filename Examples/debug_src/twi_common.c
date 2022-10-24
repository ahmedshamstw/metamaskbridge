
/****************************************************************************/
/* Copyright (c) 2014 Thirdwayv, Inc. All Rights Reserved. */

/****************************************************************************/
/**
 ** @file					twi_common.c
 ** @brief					This file implements the common functions/MACROS.
 **
 */

/*-*********************************************************/
/*- INCLUDES ----------------------------------------------*/
/*-*********************************************************/
#include "twi_common.h"

/*-*********************************************************/
/*- LOCAL FUNCTIONS IMPLEMENTATION ------------------------*/
/*-*********************************************************/

/**
 *	@brief		This function is used to guess a better square root.
 *	@param[in]	u64_num		Number to calculate the square root for it multiplied by 1000.
 *	@param[in]	u32_guess	Guessed number multiplied by 1000.
 *	@return     Return a better guessed number multiplied by 1000.
 */
static twi_u32 get_better_guess(twi_u64 u64_num, twi_u32 u32_guess)
{
	return (twi_u32)((u32_guess + MUL1000(u64_num)/u32_guess) >> 1);
}

/**
 *	@brief		This function is used to calculate the square root using the best-guess.
 *	@param[in]	u64_num		Number to calculate the square root for it multiplied by 1000.
 *	@param[in]	u32_guess	Guessed number multiplied by 1000.
 *	@return     Returns the square root multiplied by 1000.
 */
static twi_u32 calc_sqrt_using_guess(twi_u64 u64_num, twi_u32 u32_guess)
{
    if (((u32_guess < 5) || ((MUL1000(u64_num) / u32_guess) > (u32_guess - 5))) && ((MUL1000(u64_num) / u32_guess) < (u32_guess + 5)))
		return u32_guess;
	else
		return calc_sqrt_using_guess(u64_num, get_better_guess(u64_num, u32_guess));
}


/**
 *	@brief		This function is used to calculate Taylor expansion for arctan(u64_x / u64_y).
 *	@param[in]	u64_x	Numerator.
 *	@param[in]	u64_y	Denominator.
 *	@return     Returns the angle in degree.
 */
static twi_s16 calc_arctan_taylor(twi_u64 u64_x, twi_u64 u64_y)
{
	/* arctan(x/y) 	= 180 * ((x/y) - (x/y)^3/3 + (x/y)^5/5) / pi
	 *				= 180 * (15 * x * y^4 - 5 * x^3 * y^2 + 3 * x^5) / (15 * y^5) / pi
	 *				= (57 * x * y^4 - 19 * x^3 * y^2 + 11 * x^5) / (y^5)
	 */
	twi_s16 s16_degree;
	twi_u64 u64_x_pwr_3;
	twi_u64 u64_x_pwr_5;
	twi_u64 u64_y_pwr_2;
	twi_u64 u64_y_pwr_4;
	twi_u64 u64_y_pwr_5;
	
	/* Reduce the numbers to prevent overflow. */
	while (u64_y > 1000UL)
    {
        u64_y >>= 1;
        u64_x >>= 1;
    }

	u64_x_pwr_3 = u64_x * u64_x * u64_x;
	u64_x_pwr_5 = u64_x_pwr_3 * u64_x * u64_x;
	
	u64_y_pwr_2 = u64_y * u64_y;
	u64_y_pwr_4 = u64_y_pwr_2 * u64_y_pwr_2;
	u64_y_pwr_5 = u64_y_pwr_4 * u64_y;
	
	s16_degree = (twi_s16)(((57 * u64_x * u64_y_pwr_4) - (19 * u64_x_pwr_3 * u64_y_pwr_2) + (11 * u64_x_pwr_5)) / u64_y_pwr_5);

	return s16_degree;
}

/*-*********************************************************/
/*- APIs IMPLEMENTATION -----------------------------------*/
/*-*********************************************************/
/*
 *  @function		void twi_mem_cpy(twi_u8 * pu8_dst, twi_u8 * pu8_src, twi_u32  u32_sz)
 *  @brief			Internal implementation of memory copy function, used to copy a block memory from location to another.
 *  @param[in]  	pu8_dst			:	 Pointer to the destination memory location that will hold the data.
 *  @param[in]  	pu8_src			:	 Pointer to the source memory location that holds the data already.
 *  @param[in]  	u32_sz			:	 Size of the data that will be copied in terms of bytes.
 */
void twi_mem_cpy(twi_u8 * pu8_dst, twi_u8 * pu8_src, twi_u32  u32_sz)
{
    if(u32_sz == 0)
    {
        return;
    }
    TWI_ASSERT((pu8_src != NULL)&&(pu8_dst != NULL));
    
	while(u32_sz-- > 0)
	{
		* pu8_dst = *pu8_src;
		  pu8_dst++;
		  pu8_src++;
	}
}

/*
 *  @function		void twi_invert_add(twi_u8* pu8_des_add,twi_u8* pu8_src_add)
 *  @brief			invert the data at source "pu8_src_add" to destination "pu8_des_add".
 *  @param[in]  	pu8_des_add			:	 Pointer to the destination memory location that will hold the data.
 *  @param[in]  	pu8_src_add			:	 Pointer to the source memory location that holds the data already.
 *  @param[in]  	u32_sz			    :	 Size of the data that will be copied in terms of bytes.
 */
void twi_invert_add(twi_u8* pu8_des_add,twi_u8* pu8_src_add,twi_u32  u32_sz)
{
    if(u32_sz == 0)
    {
        return;
    }
    TWI_ASSERT((pu8_des_add != NULL)&&(pu8_src_add != NULL));
    twi_u32   u32_buff_length = u32_sz - 1;
    twi_u32   u32_idx;
    for(u32_idx = 0 ; u32_idx < u32_sz ; u32_idx++)
    {
        pu8_des_add[u32_idx] = pu8_src_add[u32_buff_length-u32_idx];
    }
}
/*
 *  @function		void twi_mem_set(twi_u8 * pu8_dst, twi_u8   u8_val,  twi_u32  u32_sz)
 *  @brief			Internal implementation of memory set function, used to set a block memory with a certain input value.
 *  @param[in]  	pu8_dst			:	 Pointer to the destination memory location that will hold the data.
 *  @param[in]  	u8_val			:	 The value that the destination block will set to.
 *  @param[in]  	u32_sz			:	 Size of the data that will be set in terms of bytes.
 */
void twi_mem_set(twi_u8 * pu8_dst, twi_u8  u8_val,  twi_u32  u32_sz)
{
    if(u32_sz == 0)
    {
        return;
    }
    
    TWI_ASSERT(pu8_dst != NULL);
    
	while(u32_sz-- > 0)
	{
		* pu8_dst = u8_val;
		  pu8_dst++;
	}
}

/*
 *  @function		twi_s32 twi_mem_cmp( twi_u8 * pu8_b1,  twi_u8 * pu8_b2,  twi_u32  u32_sz)
 *  @brief			Internal implementation of memory compare function, used to compare a block memory location with another one.
 *  @param[in]  	pu8_b1			:	 Pointer to the first memory buffer/location.
 *  @param[in]  	pu8_b2			:	 Pointer to the second memory buffer/location.
 *  @param[in]  	u32_sz			:	 Size of the data that will be compared in terms of bytes.
 *  @return			0 in case of comparison success, -1 in case index 0 is unmatched, other return with unmatched index
 */
twi_s32 twi_mem_cmp( twi_u8 * pu8_b1,  twi_u8 * pu8_b2,  twi_u32  u32_sz)
{
	twi_u32 	u32_counter;
    
    if(u32_sz == 0)
    {
        return 0;
    }

    TWI_ASSERT((pu8_b1 != NULL)&&(pu8_b2 != NULL));
    
	if(pu8_b1[0] != pu8_b2[0])
	{
		return -1;
	}

	for(u32_counter = 1; u32_counter < u32_sz; u32_counter++)
	{
		if(pu8_b1[u32_counter] != pu8_b2[u32_counter])
		{
			break;
		}
		else
		{
			/* Do Nothing. */
		}
	}

	if(u32_counter == u32_sz)
	{
		u32_counter = 0;
	}
	else
	{
		/* Do Nothing. */
	}
	return u32_counter;
}

/*
 *  @function		void twi_reverse(twi_u8 au8_str[], twi_u8 u8_len);
 *  @brief			Internal implementation of byte array reverse function
 *  @param[in]  	au8_str		:   the byte array
 *  @param[in]  	u16_len			:   the length of the byte array
*/
void twi_reverse(twi_u8 *pu8_str, twi_u16 u16_len)
{
     twi_u16 u16_start;
     twi_u16 u16_end;
     twi_u8 u8_char;
    
     TWI_ASSERT((pu8_str != NULL)&&(u16_len != 0));
    
     u16_start = 0;
     u16_end = u16_len - 1;
    
     while ( u16_start < u16_end )
     {
         u8_char = pu8_str[u16_start];
         pu8_str[u16_start] = pu8_str[u16_end];
         pu8_str[u16_end] = u8_char;
         u16_start++;
         u16_end--;
     }
}

/*
 *  @function		twi_u16 twi_atos16(twi_u8* pu8_str, twi_u16 u16_len, twi_s16 *ps16_result)
 *  @brief			Internal implementation of string to integer conversion
 *  @details        It will parse the given string and convert it to a twi_s16. It will stop parsing at the first non-numeric character
 *  @param[in]  	pu8_str		:   A pointer to a string buffer
 *  @param[in]  	u16_len		:   The length of the string
 *  @param[out]     ps16_result :   The converted integer
 *	@return 		twi_u16		:	The actual length of the parsed string
*/
twi_u16 twi_atos64(const twi_u8* pu8_str, twi_u16 u16_len, twi_s64 *ps64_result) 
{
    twi_u16 u16_index = 0;
    twi_bool b_negative = TWI_FALSE;
    
    TWI_ASSERT(NULL != ps64_result);
	TWI_ASSERT(NULL != pu8_str);

    if(u16_len != 0)
    {
        *ps64_result = 0;
        if (((twi_u8) '-') == pu8_str[0])
        {
            b_negative = TWI_TRUE;
            pu8_str = &pu8_str[1];
            u16_len -= 1;
        }

        while (u16_index < u16_len)
        {
            if (IS_NUMBER(pu8_str[u16_index]))
            {
                *ps64_result = MUL10_64(*ps64_result) + pu8_str[u16_index] - ((twi_u8)'0');
                u16_index++;
            }
            else
            {
                break;
            }
        }

        if (TWI_TRUE == b_negative)
        {
            (*ps64_result) = -(*ps64_result);
            u16_index++;
        }
    }
    return u16_index;
}

/*
 *  @function		twi_u8 twi_s16toa(twi_s16 s16_num, twi_u8 * pu8_str)
 *  @brief			Internal implementation of integer to string function
 *  @param[in]  	s16_num			:   integer to convert
 *  @param[in]  	u16_str_len		:   Length of the given string
 *  @param[Out] 	pu8_str			:   pointer to the string
 *	@return			twi_u8: the index at which the '\0' was placed. Which is the length of the string. In case of error it returns -1.
*/
twi_s16 twi_s64toa(twi_s64 s64_num, twi_u16 u16_str_len, twi_u8 * pu8_str)
{
	twi_s16 s16_i;
	twi_bool b_negative;
    
    TWI_ASSERT((pu8_str != NULL)&&(u16_str_len != 0));

	b_negative = TWI_FALSE;
	if (s64_num < 0)
	{
		s64_num = -s64_num;          	/* make n positive */
		b_negative = TWI_TRUE;
	}
	s16_i = 0;
	do
	{
		if(0 < u16_str_len)
		{
			/* generate digits in reverse order */
			pu8_str[s16_i++] = s64_num % 10 + '0';   	/* get next digit */
			u16_str_len--;
		}
		else
		{
			s16_i = -1;
			break;
		}
	}
	while ((s64_num /= 10) > 0);     		/* delete it */

	if(-1 != s16_i)
	{
		if (TWI_TRUE == b_negative)
		{
			if(0 < u16_str_len)
			{
				pu8_str[s16_i++] = '-';
				u16_str_len--;
			}
			else
			{
				s16_i = -1;
			}
		}

		if((-1 != s16_i) && (0 < u16_str_len))
		{
			pu8_str[s16_i] = '\0';
			twi_reverse(pu8_str, s16_i);
		}
		else
		{
			s16_i = -1;
		}
	}
	return s16_i;
}

/*
 *  @function		twi_s16 twi_u64toa_hex(twi_u64 u64_num, twi_u16 u16_str_len, twi_u8 * pu8_str)
 *  @brief			Internal implementation of Integer to Hexadecimal String function
 *  @param[in]  	u64_num			:   Integer to convert
 *  @param[in]  	u16_str_len		:   Length of the given string
 *  @param[Out] 	pu8_str			:   pointer to the string
 *	@return			twi_s16: the index at which the '\0' was placed. Which is the length of the string. In case of error it returns -1.
*/
twi_s16 twi_u64toa_hex(twi_u64 u64_num, twi_u16 u16_str_len, twi_u8 * pu8_str)
{
	/*Input Parameters Validation.*/
	TWI_ASSERT(pu8_str != NULL);
	TWI_ASSERT(u16_str_len != 0);

	twi_s16		s16_local_string_index = 0;
	twi_u64	 	u64_remainder = 0;
	twi_u64	 	u64_temp_value = u64_num;

	while(u64_temp_value != 0)
	{
		u64_remainder = u64_temp_value % 16;		/*Converting the integer value to hexadecimal representation*/
		if(u64_remainder < 10)						/*As the Numeric value in Hexadecimal Representation are from 0 Till 9*/
		{
			/*Convert the numeric value to its ASCII Representation, by Adding The Zero Value in ASCII Representation.*/
			pu8_str[s16_local_string_index] = u64_remainder + 0x30;	
		}
		else
		{
			/*Convert the Other values (A,B,C,D,E,F) to their ASCII Representation, by Adding The Offset between the letters decimal values and their corresponding ASCII code.*/
			pu8_str[s16_local_string_index] = u64_remainder + 0x37;	
		}
		/*Calculating the new value by dividing the old one over the Hexadecimal Base.*/
		u64_temp_value = u64_temp_value / 16;	
		/*Incrementing the current index to store in the next index in the array*/
		s16_local_string_index++;		

		if(s16_local_string_index > u16_str_len)
		{
			/*This means that we will corrupt the memory by accessing out of array size index. Returning Error*/
			s16_local_string_index = -1;
			break;
		}
	}
	
	if(-1 != s16_local_string_index)
	{

		if((-1 != s16_local_string_index) && (s16_local_string_index < u16_str_len))
		{
			pu8_str[s16_local_string_index] = '\0';
			twi_reverse(pu8_str, s16_local_string_index);
		}
		else
		{
			s16_local_string_index = -1;
		}
	}
	return s16_local_string_index;
}

/*
 *  @function		twi_u16 twi_strlen(twi_u8 * pu8_str);
 *  @brief			Internal implementation of string length function
 *  @param[in]  	pu8_str	:   pointer to the string
 *	@return			twi_u16: the length of the string
*/
twi_u16 twi_strlen(twi_u8 * pu8_str)
{
	twi_u16 u16_result = 0;
    
    TWI_ASSERT(pu8_str != NULL);

	while('\0' !=pu8_str[u16_result])
	{
		u16_result++;
	}

	return u16_result;
}

/*
 * @function		twi_str_contains
 * @brief			determines whether a string has a given character
 * param[IN]		pu8_str: pointer to the beginning of the string
 * param[IN]		u16_len: length of the given string
 * param[IN]		u8_search: character to search for inside the string
 * @return 			twi_s32: the index of the character if found. -1 otherwise.
 */
twi_s32 twi_str_contains(const twi_u8 * pu8_str, twi_u16 u16_len, twi_u8 u8_search)
{
	twi_s32 s32_result = -1;
	twi_s16 u16_index = 0;
    
    TWI_ASSERT((pu8_str != NULL)&&(u16_len != 0));

	while(u16_index < u16_len) {
		if(pu8_str[u16_index] == u8_search) {
			s32_result = u16_index;
			break;
		}
		u16_index++;
	}
	return s32_result;
}

/*
* @function		twi_lowercase
* @brief		Convert the upper case characters to a lower case characters.
* param[IN]		pu8_str: pointer to the beginning of the string
* param[IN]		u32_len: length of the given string
* @return 		::TWI_SUCCESS, ::TWI_ERROR_INVALID_ARGUMENTS.
*/
twi_s32 twi_lowercase(twi_u8 *pu8_str, twi_u32 u32_len)
{
    twi_u32 u32_i;
    twi_s32 s32_retval;

    s32_retval = TWI_SUCCESS;

    if (NULL != pu8_str)
    {
        for (u32_i = 0; u32_i < u32_len; u32_i++)
        {
            if ((pu8_str[u32_i] >= 'A') && (pu8_str[u32_i] <= 'Z'))
            {
                pu8_str[u32_i] ^= 0x20;
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
* @function		twi_uppercase
* @brief		Convert the lower case characters to an upper case characters.
* param[IN]		pu8_str: pointer to the beginning of the string
* param[IN]		u32_len: length of the given string
* @return 		::TWI_SUCCESS, ::TWI_ERROR_INVALID_ARGUMENTS.
*/
twi_s32 twi_uppercase(twi_u8 *pu8_str, twi_u32 u32_len)
{
    twi_u32 u32_i;
    twi_s32 s32_retval;
    

    s32_retval = TWI_SUCCESS;

    if (NULL != pu8_str)
    {
        for (u32_i = 0; u32_i < u32_len; u32_i++)
        {
            if ((pu8_str[u32_i] >= 'a') && (pu8_str[u32_i] <= 'z'))
            {
                pu8_str[u32_i] ^= 0x20;
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
 *	@brief		This function used to calculate the square root.
 *	@param[in]	u64_num:	Number to calculate the square root for, it's multiplied by 1000.
 *	@return     Returns the square root multiplied by 1000.
 */
twi_u32 twi_sqrt(twi_u64 u64_num)
{
	/*
	 * Ref: http://stackoverflow.com/questions/3581528/how-is-the-square-root-function-implemented
	 * Problem statement: Given x>0, find y such that y^2=x. => y=x/y (this is the key step) Ref
	 *
	 * 1) Guess some value g for y and test it.
	 * 2) Compute x / g.
	 * 3) If x / g is close enough to g, return g. Otherwise, try a better guess.
	 */
    if (0 == u64_num)
        return 0UL;
    else
        return calc_sqrt_using_guess(u64_num, 1000UL);
}

/*
 *	@brief		This function used to calculate the arctan(u32_numenator/u32_denumenator) in degrees.
 *	@param[in]	s32_numerator	Numerator.
 *	@param[in]	u32_denominator	Denominator.
 *	@return     Returns the degree in range -90 to 90.
 */
twi_s16 twi_arctan(twi_s32 s32_numerator, twi_u32 u32_denominator)
{
/*
 * arctan(a) = 180 * (a - a^3/3 + a^5/5 - ...) / pi			|a| < 1.
 * arctan(a) = 90 - arctan(1 / a)       					|a| > 1.
 * arctan(-a) = -arctan(a)       							|a| < 1.
 *
 * This implementation is using only three terms of the Taylor expansion.
 */

	twi_bool b_is_negative;
	twi_s16 s16_degree;
	
	b_is_negative = TWI_FALSE;
	
	if(s32_numerator < 0)
	{
		b_is_negative = TWI_TRUE;
		s32_numerator = -s32_numerator;
	}
	
	if((0 == u32_denominator) ||  (60 <= ((twi_u32)s32_numerator / u32_denominator)))
	{
		s16_degree = 90;
	}
    else if ((0 == (twi_u32)s32_numerator) || (60 <= (u32_denominator / (twi_u32)s32_numerator)))
	{
		s16_degree = 0;
	}
	else
	{
		twi_bool b_is_inverted;
		
		b_is_inverted = TWI_FALSE;

		if((twi_u32)s32_numerator > u32_denominator)
		{
			/* s32_numerator / u32_denominator is greater than one. */
			twi_u32 u32_temp;
			
			u32_temp 		= s32_numerator;
			s32_numerator 	= u32_denominator;
			u32_denominator = u32_temp;
			
			b_is_inverted = TWI_TRUE;
		}
		/*Calculate Taylor expansion */
		s16_degree = calc_arctan_taylor((twi_u64)((twi_s64)s32_numerator), (twi_u64)u32_denominator);
		
		if(b_is_inverted)
		{
			s16_degree = 90 - s16_degree;
		}
	}
	
	if(b_is_negative)
	{
		s16_degree = -s16_degree;
	}

	return s16_degree;
}

/*
 *	@brief	function to increment the index of a circular buffer
 */
void twi_next_circular_index(twi_u8* pu8_index, twi_u8 u8_queue_len)
{
	TWI_ASSERT(NULL != pu8_index);
	
	if(0 < u8_queue_len)
	{
		(*pu8_index) = (*pu8_index) + 1;
		if ((*pu8_index) == u8_queue_len)
		{
			(*pu8_index) = 0;
		}
		else
		{
			/* Do Nothing */
		}
	}
	else
	{
		
	}
}

void twi_assert(twi_bool b_cond, const char* func_name, unsigned int line_number)
{
	if(b_cond == TWI_FALSE)
	{
		volatile long int count =0;
		TWI_LOGGER("ASSERT in <%s>:<%d>\r\n", func_name, line_number);
		while(1)
		{
			//add breakpoint here
			count++;
		}
	}
}
