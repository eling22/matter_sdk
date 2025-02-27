/**************************************************************************//**
 * @file     flashds.h
 * @version
 * @brief    FLASHDS driver header file
 *
 * @copyright
 ******************************************************************************/
/** @defgroup Flash_ds Flash
*  @ingroup  peripheral_group
*  @{
*  @brief  FLASH_ds_Driver information
*/
#include "chip_define.h"
#include "project_config.h"

#ifndef ___FLASHDS_H__
#define ___FLASHDS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define DS_INVAILD						1
#define DS_VAILD_TYPE					0xFF			
#define DS_INVAILD_TYPE_00				0x00			
#define DS_INVAILD_TYPE_FF				0xFF

#define DS_START_OFFSET					0
#define DS_HEADER_OFFSET				7
#define DS_TAIL_OFFSET					3


#define DS_VAILD_TYPE_SEARCH			0
#define DS_READ_TYPE_FLAG				1
#define DS_READ_TYPE_SEARCH				1
#define DS_READ_TYPE_DELETE				2

#define DS_DATA_CHECK_SIZE				0x100	

#define DS_SERAIL_NUMBER_START			1
#define DS_SERAIL_NUMBER_MAX			0xFFFF

#define DATA_SET_ERASE_SIZE				0x1000		

#if (RAFAEL_BOARD == RT582)
#define DS_ONE_PAGE_END_ADDRESS_MAX		0xFC000 	//For Thread
#define DS_TWO_PAGE_END_ADDRESS_MAX		0xF4000		//For Zigbee/BLE
#elif (RAFAEL_BOARD == RT583)
#define DS_ONE_PAGE_END_ADDRESS_MAX		0x1F0000 	
#define DS_TWO_PAGE_END_ADDRESS_MAX		0x1E0000		
#endif
#define DS_READ_STATUS_SUCCESS			0xF0
#define DS_READ_FINSIH_SUCCESS 			0xF1
#define DS_READ_DELETE_SUCCESS 			0xF2

#define DS_MAGIC_NUMBER				    0x0000

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
 typedef enum
{
 DATA_ONE_PAGE_SIZE = 0,				//data set one page size, accroding to the usr config address to calcation. 	
 DATA_PAGE_SIZE_4K = 0x1000,		//data set page size 4k
 DATA_PAGE_SIZE_8K = 0x2000,		//data set page size 8k
}ds_page_size_t;
 
typedef enum
{
 DS_PAGE_0=0,										//data set page0
 DS_PAGE_1=1,										//data set page1					
}ds_page_t;
 
typedef enum
{
 DS_PAGE_MODE_NULL=0,						//page mode null			
 DS_ONE_PAGE_MODE=1,						//one page mode			
 DS_TWO_PAGE_MODE=2,						//two page mode
}ds_page_mode_t;

typedef enum
{
 DS_UPDATE_DISABLE=0,						//data set update disable				
 DS_UPDATE_ENABLE=1,						//data set update enable
}ds_update_flag_t;

typedef enum
{
 DS_DELETE_DISABLE=0,						//data set update disable				
 DS_DELETE_ENABLE=1,						//data set update enable
}ds_delete_flag_t;
/**
* @brief data set infromation definitions struct
*
* @details The data set information will be recorded current page/offset/address/start address/migration address/type.
*/
typedef struct __attribute__((packed))
{	
	uint8_t  page_number;											/*!< page_number to indicate data set number 											*/
	uint8_t  page_mode;												/*!< page_mode to indicate data set one or tow page mode					*/
	uint8_t  invaild;												/*!< invaild to indicate data set  																*/
	uint8_t  type_max;                                              /*!< type_max to indicate data set type max number 								*/
	uint8_t  current_page;											/*!< current_page to indicate operation page 											*/
	uint16_t current_sn;											/*!< current_page to indicate operation serial number 						*/
	uint32_t current_offset;										/*!< current_offset to indicate operation address offset poistion */
	uint32_t current_address;										/*!< current_address to indicate operation address poistion 			*/
	uint32_t migration_address;									    /*!< migration_address to indicate update migration address 			*/
	uint32_t start_address;											/*!< start_address to indicate page start address 								*/
	uint32_t middle_address;										/*!< middle_address to indicate data set middle address 					*/
	uint32_t end_address;											/*!< end_address to indicate page end address 										*/
}ds_info_t;
/**
 * @brief data set package struct
 *       
 * @details data set package struct, data type/data in/serial number/data address/crc/magic number
 *
 */
typedef struct __attribute__((packed))
{	
		uint8_t   type;														/*!< type to indicate data set package type 						*/
		uint16_t  len;														/*!< len to indicate  data set package data len 				*/
		uint32_t  sn;														/*!< sn to indicate data set package serial number			*/
		uint32_t  buf_addr;												    /*!< buf_addr to indicate data set data buffer					*/
		uint8_t   crc;														/*!< cyclic redundancy check														*/
		uint16_t  magic;													/*!< magic to spilt data set format 										*/	
		uint32_t  location;
} ds_t;
/**
 * @brief data set read/write struct
 *       
 * @details data set read/write struct,will be using in read/write data.
 *
 */
typedef struct __attribute__((packed))
{
	uint8_t	 		ds_page_mode;											/*!< ds_page_mode to indicate data set page onepage mode or two page mode	*/
	uint32_t 		start_address;										    /*!< start_address to indicate page start address 												*/
	uint32_t 		end_address;											/*!< end_address to indicate page end address 														*/
	uint32_t 		page_size;												/*!< page_size to indicate one page size 																	*/
}ds_config_t;
/**
 * @brief data set search struct
 *       
 * @details data set search struct, will be using in search data set .
 *
 */
typedef struct __attribute__((packed))
{
		uint8_t  	type;														/*!< type to indicate search data set package type								*/					
		uint16_t    flag;														/*!< flag to indicate search data set package flag								*/
		uint32_t   	offset;													/*!< offset to indicate search data set package offset						*/
		uint32_t   	address;												/*!< address to indicate search data set package start address		*/
		uint32_t   	end_address;										/*!< end_address to indicate search data set package end address	*/
} ds_search_t;
/**
 * @brief data set read/write struct
 *       
 * @details data set read/write struct,will be using in read/write data.
 *
 */
typedef struct __attribute__((packed))
{
		uint8_t  	type;												/*!< type to indicate read/write data set package type	*/
		uint16_t   	len;												/*!< len to indicate read/write data set package length	*/
		uint32_t   	address;				     						/*!< address to indicate read/write data set package address	*/
		uint32_t    location;
} ds_rw_t;
/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
* @brief ds_initinal
*
* @param None
*
* @details the function will be calcation page number and call for initial data set.
* 
*/
uint32_t ds_initinal(ds_config_t ds_init);
/**
* @brief ds_pagex_check
*
* @param[in] start_addr : data set start address
* 					 	 end_addr : data set end address
* 					 	 		 size : data set one page size 0x1000(4k)~0x2000(8k)
* @details the function will check data set flash data. that search the page valid address and migration
* @return
* @retval   STATUS_ERROR			--- the page have data non-erase page.
* @retval   STATUS_SUCCESS		--- the page is a erase page.
*/
uint32_t ds_pagex_check(void);
/**
* @brief ds_pagex_erase_check
*
* @param[in] ds_search_t for check page data erase success or fail
*
* @details the function will check data flash erase success or fail.
*
* @return
* @retval   STATUS_ERROR.			--- the page have data non-erase page.
* @retval   STATUS_SUCCESS		--- the page is a erase page.
*/
uint32_t ds_erase_check(ds_search_t* dserase);
/**
* @brief ds_page_vaild_address
*
* @param[in] ds_start_address
* 					 dspacktmp
*
* @details the function will check data set valid address and get latest package data
*
* @return
* @retval  STATUS_SUCCESS					--- the page have vaild data and get data set data.
* @retval  STATUS_INVALID_PARAM  	--- the page have invaild data will be recheck data buf.
* @retval  STATUS_INVALID_REQUEST	--- teh saerch end then can't find data
*/
uint32_t ds_page_vaild_address(uint32_t ds_start_address,ds_t* dspacktmp);
/**
* @brief ds_vaild_address_search
*
* @param None
*
* @details the function search vaild address
*
* @return
* @retval  STATUS_SUCCESS		--- get current page address and offset
* @retval  STATUS_INVALID_PARAM  	--- package crc fail or STATUS_INVALID_REQUEST 
* @retval  STATUS_INVALID_REQUEST 	--- can't find vaild data set
*/
uint32_t ds_vaild_address_search(ds_search_t* dssearch, ds_t* dspacktmp);
/**
* @brief ds_vaild
*
* @param[in] 	dssearch 	provide search address/type/
*							ds_get_vaild	rovide search address/type/
* @details for search vaild data set address/offset and get lasts data set
*                  
* @return
* @retval  STATUS_SUCCESS							--- get current page address and offset
* @retval  STATUS_INVALID_PARAM  			--- package crc fail or STATUS_INVALID_REQUEST 
* @retval  STATUS_INVALID_REQUEST			--- can't find vaild type 
* @retval  DS_READ_STATUS_SUCCESS			--- read data status success
*/
uint32_t ds_vaild(ds_search_t * dssearch, ds_t* ds_get_vaild);
/**
* @brief ds_get_current_page
*
* @param None
*
* @details the function return current page number
*
* @return
* @retval   return current page number
*/
extern uint32_t ds_get_current_page(void);
/**
* @brief ds_set_current_page
*
* @param[in] set_page set data set page number
*
* @details the function can set data set page number for change different page
*
* @return
* @retval  STATUS_SUCCESS		--- set current page success
* @retval  STATUS_INVALID_PARAM  	--- set_page is invaild parameter
*/
extern uint32_t ds_set_current_page(uint8_t set_page);
/**
* @brief ds_get_current_page_offset
*
* @param None
*
* @details the function return current page offset
*
* @return
* @retval  curretn page offset
*/
extern uint32_t ds_get_current_page_offset(void);
/**
* @brief ds_set_current_page
*
* @param[in] set_page_offset set data set page address offset
*
* @details the function can set data set page number for change different page
*
* @return
* @retval  STATUS_SUCCESS						--- set page address offset success
* @retval  STATUS_INVALID_PARAM  		--- set_page_offset is invaild parameter
*/
extern uint32_t ds_set_current_page_offset(uint32_t set_page_offset);
/**
* @brief ds_write
*
* @param[in] dspack user can update the data set by ds_package
*
* @details  The function will update data set data into flash.
*                 
*
* @return
* @retval    STATUS_SUCCESS 				---write data set finish
* @retval    STATUS_INVALID_PARAM		---data set init fail and write type error.
*/
extern uint32_t ds_write(ds_rw_t* ds_write);
/**
* @brief ds_read
*
* @param[in] ds_read user read struct to search data
*	
* @details
*                 
* @return
* @retval		STATUS_SUCCESS 					---read data set package finsih 
* 					STATUS_INVALID_PARAM		---data set is invaild
*/
extern uint32_t ds_read(ds_rw_t* ds_read);
/**
* @brief ds_migration
*
* @param[in] page , migration data set Page0 or Page1 
*
* @details The function migration data set data into vaild flash address
*                
* @return
* @retval   STATUS_SUCCESS 					--migration data set finish
* @retval   STATUS_INVALID_PARAM		--the data set is invaild
*/
uint32_t ds_migration(ds_page_t page);
/**
* @brief ds_migration_erase
*
* @param[in] page  earase migration page number
*
* @details The function erase migration flash address
*                
* @return
* @retval   STATUS_SUCCESS 				----erase data set okay
* @retval   STATUS_INVALID_PARAM	----input page param page is fail
*/
uint32_t ds_migration_erase(ds_page_t page);
/**
* @brief ds_migration_write
*
* @param[in] page migration 	flash page
*	     						 ds_write	  migration data parameter
* @details The function update migtration data
*                
* @return
* @retval  	STATUS_SUCCESS.					----migratio write data set okay
* 					STATUS_INVALID_PARAM.		----migratio write data set is invaild
* 					STATUS_INVALID_REQUEST.	----migratio write data set type is invaild
*/
uint32_t ds_migration_write(ds_page_t page,ds_rw_t* ds_write);
/**
* @brief ds_cal_crc
*
* @param[in] ds_crc : input type,len,address for calcation crc value
*						 		sn  : serial number
* @details The function will be calcation the data set package crc
*                
* @return
* @retval   crc result.
*/
uint8_t ds_cal_crc(ds_rw_t* ds_crc,uint32_t sn);
/**
* @brief ds_reset_to_default
*
* @param[None]
*
* @details The function will be reset data set to default.
*                
* @return
* @retval   STATUS_SUCCESS 				----erase data set ok
* @retval   STATUS_INVALID_PARAM	----ds invaild config
*/
uint32_t ds_reset_to_default(void);

/**
* @brief ds_delete_type
*
* @param[in]	ds_type : data set type
*
* @details The function will be delete data set type
*                
* @return
* @retval   STATUS_SUCCESS 				----delete type success
* @retval   STATUS_INVALID_PARAM	----delete fail or input param type error
*/
uint32_t ds_delete_type(uint8_t ds_type);
/*@}*/ /* end of group flash_ds_driver*/
/**
* @brief ds_update_type
*
* @param[in]	ds_type : data set type
*
* @details The function update dsinfo.typemax
*                
* @return
* @retval   STATUS_SUCCESS 				----update type success
*/
uint32_t ds_update_type( uint8_t type);
/**
* @brief ds_update_crrent_sn
*
* @param[in]	serial_number : data set serial number
*
* @details The function update data set serial number
*                
* @return
* @retval   STATUS_SUCCESS 	----update serial number success
*/
uint32_t ds_update_crrent_sn(uint32_t serial_number);
#ifdef __cplusplus
}
#endif

#endif /* end of ___FLASHDS_H__ */
