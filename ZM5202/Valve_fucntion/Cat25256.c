#include <Cat25256.h>
//#include <ZW_spi_api.h>
#include <gpio_driver.h>


//void Cat25256_init(void){
//	ZW_SPI0_init(SPI_SPEED_8_MHZ|SPI_MODE_0|SPI_MSB_FIRST|SPI_MASTER);
//	ZW_SPI0_enable(TRUE);
//}

//BYTE Cat25256_read(BYTE address){
//	BYTE data;
//	gpio_Setpin(spi_cs, FALSE);
//	ZW_SPI0_tx_set(READ);
//	ZW_SPI0_tx_set(address);
//	data = ZW_SPI0_rx_get();
//	gpio_Setpin(spi_cs, TRUE);
//	return data;
//}

//void Cat25256_write(BYTE address, BYTE data){
//	gpio_SetPin(spi_cs, FALSE);
//	ZW_SPI0_tx_set(WRITE);
//	ZW_SPI0_tx_set(address);
//	ZW_SPI0_tx_set(data);
//	gpio_SetPin(spi_cs,TRUE);

//}

//	

//void reg_status(void){
//	BYTE data;
//	gpio_SetPin(spi_cs, FALSE):
//	ZW_SPI0_tx_set(RDSR);
//	data = ZW_SPI0_rx_get();
//	return data;
//}
