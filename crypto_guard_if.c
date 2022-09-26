#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
//#include <assert.h>
#include <stdarg.h>

#ifdef __cplusplus
#error
extern "C" {
#include <string.h>	
#include <emscripten/bind.h>
#endif	
#include "twi_usb_wallet_if.h"
#include "twi_debug.h"
#ifdef __cplusplus
}
#endif

#define CHAIN_CODE_IDX        (13)
#define CHAIN_CODE_SZ         (32)
#define COMPRESSED_PUB_KEY_SZ (33)
#define SHARED_MEM_BUF_LEN    (256)
#define CONNECTING            (0)
#define CONNECTED             (1)
#define DISONNECTED           (2)

#define FUN_OUT     TWI_LOGGER("FUN_OUT <<< %s %d\r\n",__FUNCTION__,__LINE__)
typedef enum 
{
  CRYPTO_GUARD_IF_CONNECTED_EVT,
  CRYPTO_GUARD_IF_DISCONNECTED_EVT,
  CRYPTO_GUARD_IF_SEND_STATUS_EVT,
  CRYPTO_GUARD_IF_RECIEVED_DATA_EVT,
  CRYPTO_GUARD_IF_INVALID_EVT,
}
tenum_crypto_guard_if_event;

static tstr_usb_if_context* gp_curr_ctx = NULL;
static twi_bool gb_is_init = TWI_FALSE;
static twi_u8* gpu8_shared_mem = NULL;
static twi_bool gb_conn_state = DISONNECTED;
static twi_u8 gau8_rx_buff[64] = {0};
/////////////////////////////////////////////////////////////////////////
///////////////////////////JS Helpers///////////////////////////////////
extern char* consoleLog(char* data);
//extern twi_u8* allocateOnMemory(twi_u32 data_len);
extern void usbSend(twi_u32 data_len);
extern void onConnectionDone(void);
extern void onGetXpubResult(twi_u32 comp_pub_key_off, twi_u32 comp_pub_key_len, twi_u32 chain_code_off, twi_u32 chain_code_len, twi_s32 error_code); //in this function the bridge should notify the kyring with the operation result
extern void onSignTxResult(twi_u32 v_off, twi_u32 v_len, twi_u32 r_off, twi_u32 r_len, twi_u32 s_off, twi_u32 s_len, twi_s32 error_code); //in this function the bridge should notify the kyring with the operation result
/////////////////////////////////////////////////////////////////////////
void web_printf(const twi_u8* pu8_prnt_msg, ...)
{
    char buffer[SHARED_MEM_BUF_LEN];
    va_list args;
    va_start (args, pu8_prnt_msg);
    vsnprintf (buffer,SHARED_MEM_BUF_LEN,pu8_prnt_msg, args);
    consoleLog(buffer);
    va_end (args);
}
/////////////////////////////////////////////////////////////////////////
///////////////////////////Static functions//////////////////////////////
static void usb_scan_and_connect_cb(void* const pv_device, twi_u8* pu8_dvc_id, twi_u8 u8_dvc_id_len, twi_u16 u16_vid, twi_u16 u16_pid, twi_u32 u32_scan_time_out_msec, twi_u32 u32_mtu_sz)
{
  FUN_IN;
}

static void usb_disconnect_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_receive_cb(void* const pv_device, void *p_rx_buf, twi_u32* pu32_length)
{
  FUN_IN;
  TWI_DUMP_BUF("gau8_rx_buff", gau8_rx_buff, sizeof(gau8_rx_buff));
  *pu32_length = gau8_rx_buff[0]; 
  TWI_MEMCPY(p_rx_buf, &gau8_rx_buff[1], *pu32_length);
  FUN_OUT;
}

static void usb_stop_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_disable_cb(void* const pv_device)
{
  FUN_IN;
}

static void usb_dispatch_cb(void* const pv_device)
{
  //FUN_IN;
}

static void usb_send_cb(void* const pv_device, twi_u8* const pu8_data, twi_u32 u32_data_sz)
{
  //allocate or copy to the JS bufefr
  FUN_IN;
  TWI_LOGGER("Send Buffer::\r\n");
  // for(int i =0; i<u32_data_sz; i++)
  // {
  //   TWI_LOGGER("%d",pu8_data[i]);
  // }
  // TWI_LOGGER("\r\n");
  TWI_MEMSET(gpu8_shared_mem, 0x0, SHARED_MEM_BUF_LEN);
  //gpu8_shared_mem[0] = 0x0; //supported port
  gpu8_shared_mem[0] = u32_data_sz & 0x3f;
  TWI_MEMCPY(&gpu8_shared_mem[1], pu8_data, u32_data_sz);
  usbSend(64);
  FUN_OUT;
}

static void usb_Start_Timer_cb(void* const pv_device, twi_u32 u32_idx, twi_u32 u32_dur_msec)
{
  FUN_IN;
}

static void usb_Stop_Timer_cb(void* const pv_device, twi_u32 u32_idx)
{
  FUN_IN;
}

//TODO: add the URL and port
static void usb_send_to_cloud_cb(void* const pv_device, twi_u8* const pu8_data, twi_u32 u32_data_sz)
{
  FUN_IN;
} 

static void usb_onUserConfirmationRequested_cb(void* const pv_device, twi_u32 u32_conf_type)
{
  FUN_IN;
}

static void usb_onUserConfirmationObtained_cb(void* const pv_device, twi_u32 u32_conf_type)
{
  FUN_IN;
}

static void usb_onGetExtendedPubKeyResult_cb(void* const pv_device, twi_u8* const pu8_pub_key, twi_u32 u32_pub_key_sz, twi_s32 s32_err)
{
  //map the buffers
  FUN_IN;
  TWI_LOGGER("public_key = %d, error = %d \r\n", u32_pub_key_sz, s32_err);
  // for(int i =0; i<u32_pub_key_sz; i++)
  // {
  //   TWI_LOGGER("%d", pu8_pub_key[i]);
  // }
  if(s32_err == 0)
  {
    TWI_MEMSET(gpu8_shared_mem, 0x0, SHARED_MEM_BUF_LEN);
    TWI_MEMCPY(gpu8_shared_mem,  &pu8_pub_key[CHAIN_CODE_IDX], CHAIN_CODE_SZ);
    TWI_MEMCPY(&((twi_u8*)gpu8_shared_mem)[CHAIN_CODE_SZ],  &pu8_pub_key[CHAIN_CODE_IDX + CHAIN_CODE_SZ], COMPRESSED_PUB_KEY_SZ);
  }
  onGetXpubResult(CHAIN_CODE_SZ, COMPRESSED_PUB_KEY_SZ,0, CHAIN_CODE_SZ, s32_err);
  FUN_OUT;
}

static void usb_onSignTransactionResult_cb(void* const pv_device, void* pstr_signed_tx, twi_s32 s32_err)
{
  FUN_IN;
  tstr_usb_ethereum_signed_tx* pstr_sign_tx = (tstr_usb_ethereum_signed_tx*)pstr_signed_tx;
  if(s32_err == 0)
  {
    TWI_MEMSET(gpu8_shared_mem, 0x0, SHARED_MEM_BUF_LEN);
    gpu8_shared_mem[0] = pstr_sign_tx->u8_sig_v;
    TWI_MEMCPY(&gpu8_shared_mem[1], pstr_sign_tx->au8_sig_r, 32);
    TWI_MEMCPY(&gpu8_shared_mem[33], pstr_sign_tx->au8_sig_s, 32);
  }
  onSignTxResult(0, 1, 1, 32, 33, 32, s32_err);
  FUN_OUT;
}

static void usb_onSignMessageResult_cb(void* const pv_device, void* pstr_signed_msg, twi_s32 s32_err)
{
  FUN_IN;
}

static void usb_onGetWalletIDResult_cb(void* const pv_device, twi_u8* pu8_wallet_id, twi_u8 u8_wallet_id_len, twi_s32 s32_err)
{
  FUN_IN;
}

static void usb_save_cb(void* const pv_device, twi_u16 id,  twi_u8* pu8_data, twi_u32 u32_data_sz)
{
  FUN_IN;
}

static void usb_load_cb(void* const pv_device, twi_u16 id, twi_u8* pu8_data, twi_u32* pu32_data_sz)
{
  FUN_IN;
}

static void usb_onConnectionDone_cb(void* const pv_device)
{
  FUN_IN;
  //notify the upper layer
  onConnectionDone();
  FUN_OUT;
}

static tstr_usb_if_context* cyrpto_guard_if_init(void)
{
  tstr_usb_if_context* presult = NULL;
  presult = twi_usb_if_new();
  //assert(NULL != presult);

  twi_usb_if_set_callbacks( presult, 
                            usb_scan_and_connect_cb            ,
                            usb_disconnect_cb                  ,
                            usb_receive_cb                     ,
                            usb_stop_cb                        ,
                            usb_disable_cb                     ,
                            usb_dispatch_cb                    ,
                            usb_send_cb                        ,
                            usb_Start_Timer_cb                 ,
                            usb_Stop_Timer_cb                  ,
                            usb_send_to_cloud_cb               ,
                            usb_onUserConfirmationRequested_cb ,
                            usb_onUserConfirmationObtained_cb  ,
                            usb_onGetExtendedPubKeyResult_cb   ,
                            usb_onSignTransactionResult_cb     ,
                            usb_onSignMessageResult_cb         ,
                            usb_onGetWalletIDResult_cb         ,
                            usb_save_cb                        ,
                            usb_load_cb                        ,
                            usb_onConnectionDone_cb            );
  gb_conn_state = DISONNECTED;
  return  presult;                         
}
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//////////////////////////////APIS///////////////////////////////////////
EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_mem_init(twi_u8* pu8_shared_mem)
{
   if(TWI_FALSE == gb_is_init)
   {
      gb_is_init = TWI_TRUE;
      gp_curr_ctx = cyrpto_guard_if_init();
	    gpu8_shared_mem = pu8_shared_mem;
      twi_usb_if_set_device_info(gp_curr_ctx, pu8_shared_mem);
   }
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_get_xpub(twi_u8* pu8_xpub_path, int num_of_step)
{
  FUN_IN;
  TWI_ASSERT((NULL != pu8_xpub_path) && (0 != num_of_step));
  //communicate with the keyfon_cb(handshake and openning the nano-app) then getting the xpub
  tstr_usb_crypto_path str_usb_crypto_path = {0};
  str_usb_crypto_path.u8_steps_num = num_of_step;
  TWI_MEMCPY(str_usb_crypto_path.au32_path_steps, pu8_xpub_path, num_of_step*4);
  twi_usb_if_get_ext_pub_key(gp_curr_ctx, USB_WALLET_COIN_ETHEREUM, &str_usb_crypto_path,NULL, 0,TWI_FALSE);
  FUN_OUT;
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_sign_tx(twi_u8* pu8_xpub_path, int num_of_step, twi_u8* pu8_tx, twi_u32 u32_tx_len)
{
  FUN_IN;
  TWI_ASSERT((NULL != pu8_xpub_path) && (0 != num_of_step) && (NULL != pu8_tx) && ((u32_tx_len > 0) && (u32_tx_len <= USB_WALLET_SIGNING_TX_MAX_LEN)));
  
  tstr_usb_ethereum_tx eth_tx;
  eth_tx.u16_signing_tx_len = (twi_u16) u32_tx_len;
  TWI_MEMCPY(eth_tx.au8_signing_tx, pu8_tx, u32_tx_len);
  eth_tx.str_signing_key_path.u8_steps_num = num_of_step;
  TWI_MEMCPY(eth_tx.str_signing_key_path.au32_path_steps, pu8_xpub_path, num_of_step*4);
  twi_usb_if_sign_tx(gp_curr_ctx, USB_WALLET_COIN_ETHEREUM, &eth_tx,NULL, 0, TWI_FALSE);
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_notify(tenum_crypto_guard_if_event enum_event, twi_u8* data, int len, int error)
{
  FUN_IN;
  TWI_LOGGER("enum_event = %d, error = %d\r\n", enum_event, error);
  switch(enum_event)
  {
    case CRYPTO_GUARD_IF_CONNECTED_EVT:
    {
      //TODO: this is a workaround to open the port before sending the stack specs
		  gb_conn_state = CONNECTING;
      TWI_MEMSET(gpu8_shared_mem, 0x0, SHARED_MEM_BUF_LEN);
      gpu8_shared_mem[0] = 0x40; //open port
      usbSend(64);
      break;
    }

    case CRYPTO_GUARD_IF_DISCONNECTED_EVT:
    {
      twi_usb_if_notify_disconnected(gp_curr_ctx, 0, error);
      twi_usb_if_free(gp_curr_ctx);
      gp_curr_ctx = NULL;
      gb_is_init = TWI_FALSE;
      gb_conn_state = DISONNECTED;
      break;
    }

    case CRYPTO_GUARD_IF_SEND_STATUS_EVT:
    {
      if(gb_conn_state == CONNECTING)
      {
        gb_conn_state = CONNECTED;
        twi_usb_if_notify_connected(gp_curr_ctx, error);
      }
      else if (gb_conn_state == CONNECTED)
      {
        twi_usb_if_notify_send_status(gp_curr_ctx, error);
      }
      else
      {
        TWI_LOGGER_ERR("Invlaid state\r\n");
        TWI_ASSERT(TWI_FALSE);
      }
      break;
    }

    case CRYPTO_GUARD_IF_RECIEVED_DATA_EVT:
    {
      TWI_LOGGER("CRYPTO_GUARD_IF_RECIEVED_DATA_EVT addr = 0x%x, len = %d\r\n", data, len);
      TWI_MEMSET(gau8_rx_buff, 0x0, 64);
      TWI_MEMCPY(gau8_rx_buff, data, len);
      twi_usb_if_notify_data_received(gp_curr_ctx, gau8_rx_buff, len, error);
      break;
    }

    default:
    {
      TWI_LOGGER_ERR("Invlaid state\r\n");
      TWI_ASSERT(TWI_FALSE);
    }
  }
  FUN_OUT;
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_dispatch(void)
{
  if(NULL != gp_curr_ctx)
  {
    twi_usb_if_dispatch(gp_curr_ctx);
  }
}

EMSCRIPTEN_KEEPALIVE
void* crypto_guard_if_malloc(int size)
{
    return malloc(size);
}

EMSCRIPTEN_KEEPALIVE
void crypto_guard_if_free(void *ptr)
{
    free(ptr);
}
/////////////////////////////////////////////////////////////////////////
