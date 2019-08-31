/*
                    GPS Module
                By: J.J.K
*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "inc/tm4c123gh6pm.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"


#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "gps.h"
#include "gps_buff.h"
#include "driverlib/interrupt.h"

/*
PD6 -->U2Rx
PD7 -->U2Tx
*/


#define Buff_Data_size      138

/* GPS handle  */
gps_t hgps;

/* GPS buffer */
static gps_buff_t hgps_buff;
static uint8_t hgps_buff_data[Buff_Data_size];

/*
 * 8-bit signed Integer = ASCII Characters
 */



void UART_Init();
void UART2IntHandler(void);



void main(void)
{

        // Run the microcontroller system clock at 80MHz.
         SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

        UART_Init();
        char residual = UARTCharGetNonBlocking(UART2_BASE);
        IntMasterEnable();

        uint8_t rx;

        gps_init(&hgps);                            /* Init GPS */

        /* Create buffer for received data */
        buff_init(&hgps_buff, hgps_buff_data, sizeof(hgps_buff_data));

        while (1) {

            /* Add new character to buffer */
            /* Process all input data */
            /* Read from buffer byte-by-byte and call processing function */
            if (buff_get_full(&hgps_buff)) {    /* Check if anything in buffer now */
                while (buff_read(&hgps_buff, &rx, 1)) {
                    gps_process(&hgps, &rx, 1);     /* Process byte-by-byte */
                }
            }
        }


}
void UART_Init(void)
{
    // Enable clock access to the GPIO Peripheral used by the UART2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    // Enable clock access to UART2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART2));


    // Configure GPIO Pins for UART mode.
    GPIOPinConfigure(GPIO_PD6_U2RX);
    GPIOPinConfigure(GPIO_PD7_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);

     UARTDisable(UART2_BASE);

    // Configure the UART for 9600 8-N-1.
     UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE));
     UARTFIFOLevelSet(UART2_BASE,UART_FIFO_TX4_8,UART_FIFO_RX4_8);
     UARTEnable(UART2_BASE);


     // Enable the NVIC interrupt, clear the UART individual interrupts and then enable.
     IntEnable(INT_UART2);
     UARTIntClear(UART2_BASE, UARTIntStatus(UART2_BASE, false));
     UARTIntEnable(UART2_BASE, (UART_INT_RX | UART_INT_RT));
}

/**
 * \brief           Interrupt handler routing for UART received character
 * \note
 */
void
UART2IntHandler(void) {

    /* Make interrupt handler as fast as possible */
    uint32_t intStatus;

    // Retrieve masked interrupt status (only enabled interrupts).
    intStatus = UARTIntStatus(UART2_BASE, true);


    // Clear interrupt(s) after retrieval.
    UARTIntClear(UART2_BASE, intStatus);


    // Important: Note that they're all IF statements. This is because you can have an NVIC
    // system interrupt that is composed of all three sub-interrupts below. If you use the standard
    // if-elseif-else then you might miss one and drop bytes.

    // The receive timeout interrupt fires when you have received bytes in your FIFO but have not
    // gotten enough to fire your Rx interrupt. This is because the FIFO level select determines when that
    // interrupt goes off.


    if((intStatus & UART_INT_RT) == UART_INT_RT)
        {
            // While there are bytes to read and there is space in the FIFO.
            while(UARTCharsAvail(UART2_BASE) && (buff_get_free(&hgps_buff)>0))
            {
                // Write a byte straight from the hardware FIFO into our Rx FIFO for processing later.
                uint8_t data = (uint8_t)UARTCharGet(UART2_BASE);
                buff_write(&hgps_buff, &data, 1);

            }
        }

    // The Rx interrupt fires when there are more than the fifo level select bytes in the FIFO.
        if((intStatus & UART_INT_RX) == UART_INT_RX)
        {
            // While there are bytes to read and there is space in the FIFO.
            while(UARTCharsAvail(UART2_BASE) && (buff_get_free(&hgps_buff)>0))
            {
                // Write a byte straight from the hardware FIFO into our Rx FIFO for processing later.
                 uint8_t data = (uint8_t)UARTCharGet(UART2_BASE);
                 buff_write(&hgps_buff, &data, 1);
            }
        }
}
