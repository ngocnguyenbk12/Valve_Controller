/**
 * @file
 * Parses incoming ASCII characters to commands for the application layer.
 *
 * The syntax for commands is: "<channel> <command><Carriage return>"
 *
 * 'channel' is letters [a-z] including 'w'.
 * 'command' is specific command for the channel
 *
 * NOTICE: Using this module together with the key_driver.c in
 * ApplicationUtilities on the ZDP03A development kit results in the S5
 * not working since its pin is shared with UART0 TX.
 * @copyright Copyright (c) 2001-2017, Sigma Designs Inc., All Rights Reserved
 */

#ifndef Z_WAVE_INCLUDE_ZW_TEST_INTERFACE_H_
#define Z_WAVE_INCLUDE_ZW_TEST_INTERFACE_H_

#include <ZW_typedefs.h>
#include <ZW_stdint.h>

/**
 * @brief Hooks a function to a given channel.
 * @param[in] channel The desired channel to listen on.
 * @param[in] pCallback Pointer to function to be called when a command is ready.
 * @return TRUE if channel got allocated, FALSE otherwise.
 */
//@ [ZW_test_interface_allocate]
BOOL ZW_test_interface_allocate(
    char channel,
    VOID_CALLBACKFUNC(pCallback)(char channel, char * pString));
//@ [ZW_test_interface_allocate]

#endif /* Z_WAVE_INCLUDE_ZW_TEST_INTERFACE_H_ */
