cmake_minimum_required(VERSION 3.7)
# project name ==> crypto_guard_if
project(
	crypto_guard_if
  	VERSION 1.0
  	LANGUAGES C CXX)
	
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_COMPILER "emcc")	

#fix "call to undeclared library function 'free'/'calloc'"	
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto=full")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=full")
	
#include paths	
include_directories(
					"../TWIWalletCore/WalletCoreInterface/USBWallet/"
					"../TWIWalletCore/utils/twi_apdu_parser_composer"
					"../TWIWalletCore/helpers/include/"
					"../TWIWalletCore/utils/twi_debug/"
					"../TWIWalletCore/utils/twi_timer_mgmt/"
					"../TWIWalletCore/hal/include/"
					"../TWIWalletCore/hal/source/win/"
					"../TWIWalletCore/helpers/crc_16/"
					"../TWIWalletCore/protocols/twi_generic_stack_proto/inc/"					
					)
#source paths
file(GLOB SOURCES 	"./crypto_guard_if.c"
					"../TWIWalletCore/helpers/source/twi_common.c"
					"../TWIWalletCore/protocols/twi_generic_stack_proto/src/twi_link_layer.c"
					"../TWIWalletCore/protocols/twi_generic_stack_proto/src/twi_network_layer.c"
					"../TWIWalletCore/protocols/twi_generic_stack_proto/src/twi_no_security_layer.c"
					"../TWIWalletCore/protocols/twi_generic_stack_proto/src/twi_stack.c"
					"../TWIWalletCore/protocols/twi_generic_stack_proto/src/twi_usb_link_layer.c"
					"../TWIWalletCore/helpers/source/crc_16.c"
					"../TWIWalletCore/utils/twi_apdu_parser_composer/twi_apdu_parser_composer.c"
					"../TWIWalletCore/helpers/source/twi_debug.c"
					"../TWIWalletCore/WalletCoreInterface/USBWallet/twi_usb_wallet_if.c"				
					)


add_executable(crypto_guard_if ${SOURCES})
set_target_properties(crypto_guard_if PROPERTIES LINK_FLAGS "--bind")

#TARGET_LINK_LIBRARIES(crypto_guard_if
#	Setupapi
#)

#building flags
target_compile_definitions(crypto_guard_if PRIVATE CMAKE_NO_SYSTEM_FROM_IMPORTED=1 NRF_SD_BLE_API=3 NRF_SD_BLE_API_VERSION=3
STDLIB=1 DEBUGGING_ENABLE=1 _DEBUG _CONSOLE _LIB _CRT_SECURE_NO_WARNINGS COMM_LOG_ENABLE TWI_USB_HOST TWI_USE_USB_AS_HID TWI_USB_STACK_ENABLED)