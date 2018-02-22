#include "mcc_generated_files/mcc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


#define SB_DATA_WINDOW 50

uint16_t sb_data[SB_DATA_WINDOW];
int8_t sb_front = -1;
int8_t sb_rear = -1;

uint8_t start_flag = 0;
uint8_t i;


bool sbuf_isfull() {
    if ((sb_front == sb_rear + 1) || (sb_front == 0 && sb_rear == SB_DATA_WINDOW - 1))
        return true;
    else
        return false;
}

bool sbuf_isempty() {
    if (sb_front == -1)
        return true;
    else
        return false;
}

bool sbuf_insert(uint16_t element) {
    if (sbuf_isfull()) {
        // Can't insert data because buffer is full
        return false;
    } else {
        if (sb_front == -1)
            sb_front = 0;
        sb_rear = (sb_rear + 1) % SB_DATA_WINDOW;
        sb_data[sb_rear] = element;
        return true;
    }
}

bool sbuf_remove() {
    uint16_t element;
    if (sbuf_isempty()) {
        return false;
    } else {
        element = sb_data[sb_front];
        if (sb_front == sb_rear) {
            sb_front = -1;
            sb_rear = -1;
        } else {
            sb_front = (sb_front + 1) % SB_DATA_WINDOW;
        }
        return true;
    }
}

uint16_t sbuf_peek(){
  return sb_data[sb_front];
}

void TMR6_EMG_InterruptHandler(void)
{
    if (start_flag == 1) {
        ADCC_StartConversion(POT_RA0);
        //ADCC_StartConversion(EMG_RA1);
        adc_result_t adval = ADCC_GetConversionResult();
        sbuf_insert(adval/100);
    }
}

void main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    
    TMR6_SetInterruptHandler(TMR6_EMG_InterruptHandler);
    TMR6_Start();
    
    int count=0;
    uint16_t neutral_datapoint, result, datapoint;
    double time_elapsed;

    while (1)
    {
        
        if (SWITCH_RC5_GetValue() == 0 && start_flag == 0) {
            printf("START\r\n");
            start_flag = 1;
            __delay_ms(700);
        } else if (SWITCH_RC5_GetValue() == 0 && start_flag == 1) {
            //printf("STOP\r\n");
            start_flag = 0;
            __delay_ms(700);
            TMR6_Stop();
        }
        
        if(start_flag == 1)
        {
            for (i = sb_front; i != sb_rear; i = (i + 1) % SB_DATA_WINDOW) {
                count++;
            }

            if(count>0)
            {
                datapoint = sbuf_peek()/100;
                //neutral_datapoint = get_neutral_peaktopeak(datapoint);
                //result = get_moving_average(abs(datapoint - neutral_datapoint));
                
                printf("%u,%f\r\n",datapoint, time_elapsed);
                sbuf_remove();
                time_elapsed += 5.0;
            }

            count = 0;
        }
    }
}
/**
 End of File
*/