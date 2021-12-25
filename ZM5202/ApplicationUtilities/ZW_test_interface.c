/**
 * @file ZW_test_interface.c
 * @brief Parses incoming ASCII characters to commands for the application layer.
 *
 * The syntax for commands is:
 *
 * <channel> <User string><Carriage return>
 *
 * <channel> is letters [a-z] including 'w'.
 *
 * NOTICE: Using this module together with the key_driver.c in
 * ApplicationUtilities on the ZDP03A development kit results in the S5/KEY05
 * not working since it's pin is shared with UART0 TX.
 *
 * @date 12/02/2015
 * @author Christian Salmony Olsen
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_stdint.h>
#include <ZW_typedefs.h>
#include <ZW_mem_api.h>
#include <ZW_uart_api.h>
#include <ZW_task.h>
#include <ZW_test_interface_driver.h>
#include <ZW_test_interface.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_TEST_INTERFACE
#define ZW_DEBUG_TEST_INTERFACE_SEND_BYTE(data)     ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_TEST_INTERFACE_SEND_STR(STR)       ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_TEST_INTERFACE_SEND_NUM(data)      ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_TEST_INTERFACE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_TEST_INTERFACE_SEND_NL()           ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_TEST_INTERFACE_SEND_BYTE(data)
#define ZW_DEBUG_TEST_INTERFACE_SEND_STR(STR)
#define ZW_DEBUG_TEST_INTERFACE_SEND_NUM(data)
#define ZW_DEBUG_TEST_INTERFACE_SEND_WORD_NUM(data)
#define ZW_DEBUG_TEST_INTERFACE_SEND_NL()
#endif

/* write a single character      */
#define ZDB_SEND_BYTE ZW_UART0_tx_send_byte
#define ZDB_SEND_STR ZW_UART0_tx_send_str



#define TIMER_CALLBACK_PERIOD (10) // In milliseconds.
#define NUMBER_OF_CHANNELS     (2) //  letters (a,b)
#define BUFFER_SIZE           (20)

typedef struct
{
  char channel;
  VOID_CALLBACKFUNC(pCallback) (char channel, char * pString);
}
CHANNEL_T;

typedef enum
{
  INDEX_CHANNEL_FOUND,
  INDEX_CHANNEL_NEW,
  INDEX_CHANNEL_FULL
} INDEX_RESULT_T;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static CHANNEL_T channels[NUMBER_OF_CHANNELS];
static uint8_t taskHandleId = 0;
/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static INDEX_RESULT_T get_index_from_channel(uint8_t * pIndex, char channel);
void init_once(void);
BOOL ZCB_test_interface_poll(void);

/**
 * @brief Initializes the test interface.
 */
void init_once(void)
{
  static BOOL fIsInitialized = FALSE;
  ZW_DEBUG_TEST_INTERFACE_SEND_NL(); ZW_DEBUG_TEST_INTERFACE_SEND_STR("init_once"); ZW_DEBUG_TEST_INTERFACE_SEND_NL();
  if (FALSE == fIsInitialized)
  {
    ZW_test_interface_driver_init();
    // Do this initialization only the first time this function is called.
    fIsInitialized = TRUE;

    memset((uint8_t * )channels, 0x00, (NUMBER_OF_CHANNELS * sizeof(CHANNEL_T)));
    if ((taskHandleId = TaskAdd(ZCB_test_interface_poll,
            "ZCB_test_interface_poll")) == 0)
    {
      ZW_DEBUG_TEST_INTERFACE_SEND_NL(); ZW_DEBUG_TEST_INTERFACE_SEND_STR("Task pool full");
    }

    ZW_DEBUG_TEST_INTERFACE_SEND_NL(); ZW_DEBUG_TEST_INTERFACE_SEND_STR("TI init.");
  }
}

BOOL ZW_test_interface_allocate(char channel, VOID_CALLBACKFUNC(pCallback) (char channel, char * pString))
{
  uint8_t channel_index = 0;

  init_once();

  if (INDEX_CHANNEL_NEW != get_index_from_channel(&channel_index, channel))
  {
    // The channel is not free.
    ZW_DEBUG_TEST_INTERFACE_SEND_NL();
    ZW_DEBUG_TEST_INTERFACE_SEND_STR("Can't allocate :(");
    return FALSE;
  }

  channels[channel_index].channel = channel;
  channels[channel_index].pCallback = pCallback;

  ZW_DEBUG_TEST_INTERFACE_SEND_NL();
  ZW_DEBUG_TEST_INTERFACE_SEND_STR("Allocation done!");
  return TRUE;
}


/**
 * @brief Reads data from the ZW_zdb interface and calls channel functions if
 * they're allocated.
 */
PCB_BOOL(ZCB_test_interface_poll)(void)
{
  uint8_t channel_index;
  static uint8_t buffer[DMA_BUFFER_SIZE_SINGLE];
  uint8_t dataLength;

#ifdef ZW_TEST_INTERFACE_DRIVER
  if (TRUE != ZW_test_interface_driver_getData(buffer, &dataLength))
  {
    return FALSE;
  }
#endif /*ZW_TEST_INTERFACE_DRIVER*/
  ZW_DEBUG_TEST_INTERFACE_SEND_NL();
  ZW_DEBUG_TEST_INTERFACE_SEND_STR("Data length:");
  ZW_DEBUG_TEST_INTERFACE_SEND_NUM(dataLength);

  if ('\r' == buffer[dataLength - 1])
  {
    BOOL channel_status = FALSE;
    ZW_DEBUG_TEST_INTERFACE_SEND_NL();
    ZW_DEBUG_TEST_INTERFACE_SEND_BYTE('2');
    if ((dataLength > 2) && (0x20 == buffer[1])) // 0x20 == <space>
    {
      // We got minimum three characters => valid input.
      INDEX_RESULT_T index_result = get_index_from_channel(&channel_index,
              buffer[0]);
      ZW_DEBUG_TEST_INTERFACE_SEND_BYTE('3');
      if ((INDEX_CHANNEL_FOUND == index_result)
              && (NULL != channels[channel_index].pCallback))
      {
        ZW_DEBUG_TEST_INTERFACE_SEND_BYTE('!');
        buffer[dataLength - 1] = '\0';
        (channels[channel_index].pCallback)(buffer[0], (char *)&buffer[2]);
        channel_status = TRUE;
      }
    }
    if(FALSE == channel_status)
    {
      uint8_t i = 0;
      /* No channel defined*/
      ZDB_SEND_STR((BYTE *)"\r\nChannels are:");
      for(i = 0; i < NUMBER_OF_CHANNELS; i++)
      {
        if(channels[i].channel != 0)
        {
          ZDB_SEND_STR((BYTE *)"\r\n\t >");
          ZDB_SEND_BYTE(channels[i].channel);
        }
      }
    }
  }

  return TRUE;
}

/**
 * @brief Returns the index location for a given channel. If channel is not
 * allocated, the next free index is returned.
 * @param pIndex Pointer to index location.
 * @param channel Channel to search for in the index.
 * @return TRUE if channel is allocated, FALSE otherwise.
 */
static INDEX_RESULT_T get_index_from_channel(uint8_t * pIndex, char channel)
{
  uint8_t count;

  for (count = 0; count < NUMBER_OF_CHANNELS; count++)
  {
    if (channel == channels[count].channel)
    {
      // The channel has already been allocated => return index.
      *pIndex = count;
      ZW_DEBUG_TEST_INTERFACE_SEND_STR("TI INDEX FOUND");
      return INDEX_CHANNEL_FOUND;
    }
    else if (0 == channels[count].channel)
    {
      *pIndex = count;
      ZW_DEBUG_TEST_INTERFACE_SEND_STR("TI INDEX NEW");
      return INDEX_CHANNEL_NEW;
    }
  }

  *pIndex = NUMBER_OF_CHANNELS;
  ZW_DEBUG_TEST_INTERFACE_SEND_NL(); ZW_DEBUG_TEST_INTERFACE_SEND_STR("TI INDEX FULL");
  return INDEX_CHANNEL_FULL;
}
