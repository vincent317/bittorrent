
#ifndef CLI_H_INCLUDED
#define CLI_H_INCLUDED

#include "shared.h"
int direct_connected ;
uint8_t direct_connect_address[4];
uint16_t direct_connect_port;

/*
    The main function which executes when the program begins
*/
int main();

/*
    A function called periodically where the CLI can perform logic
    - Listen to STDIN and process commands
*/
void cli_periodic();

#endif
