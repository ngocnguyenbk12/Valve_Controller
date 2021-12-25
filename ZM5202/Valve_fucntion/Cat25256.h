//#include <ZW_spi_api.h>
#include <ZW_typedefs.h>

#define WREN 0x06  // enable write ope
#define WRDI 0x04	// disable write ope
#define RDSR 0x05	// read stt reg
#define	WRSR 0x01	// write stt reg
#define READ 0x03	// read 
#define WRITE 0x02	// write 

void Cat25256_init(void);
BYTE Cat25256_read(BYTE address1, BYTE Address2);
void Cat25256_write(BYTE address1, BYTE Address2, BYTE Data);
void Cat25256_reset(void);
BYTE reg_status(void);
void Cat25256_write_reg(BYTE reg_value);
