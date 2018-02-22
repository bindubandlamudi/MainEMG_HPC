#include "mcc_generated_files/mcc.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


#define SB_DATA_WINDOW 50
#define SAMPLE_FREQUENCY 50
#define MIN_FREQUENCY 25
#define MAX_FREQUENCY 150

#define MIN_PK_GAP ((int)(SAMPLE_FREQUENCY / MAX_FREQUENCY))
#define MAX_PK_GAP ((int)(SAMPLE_FREQUENCY / MIN_FREQUENCY))
#define PK_DATA_WINDOW (2*MAX_PK_GAP+1)

#define MA_DATA_WINDOW ((int)(SAMPLE_FREQUENCY * 0.5))


uint16_t sb_data[SB_DATA_WINDOW];
int8_t sb_front = -1;
int8_t sb_rear = -1;

uint16_t pk_data[PK_DATA_WINDOW];
int8_t pk_front = -1;
int8_t pk_rear = -1;

uint16_t ma_data[MA_DATA_WINDOW];
int8_t ma_front = -1;
int8_t ma_rear = -1;
uint16_t ma_window_sum = 0;

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

bool pkdata_isfull() {
    if ((pk_front == pk_rear + 1) || (pk_front == 0 && pk_rear == PK_DATA_WINDOW - 1))
        return true;
    else
        return false;
}

bool pkdata_isempty() {
    if (pk_front == -1)
        return true;
    else
        return false;
}

bool pkdata_insert(uint16_t element) {
    if (pkdata_isfull()) {
        // Can't insert data because buffer is full
        return false;
    } else {
        if (pk_front == -1)
            pk_front = 0;
        pk_rear = (pk_rear + 1) % PK_DATA_WINDOW;
        pk_data[pk_rear] = element;
        return true;
    }
}

bool pkdata_remove() {
    uint16_t element;
    if (pkdata_isempty()) {
        return false;
    } else {
        element = pk_data[pk_front];
        if (pk_front == pk_rear) {
            pk_front = -1;
            pk_rear = -1;
        } else {
            pk_front = (pk_front + 1) % PK_DATA_WINDOW;
        }
        return true;
    }
}

uint16_t get_neutral_peaktopeak(uint16_t datapoint) {
    pkdata_insert(datapoint);

    if (pkdata_isfull()) {
        pkdata_remove();
    }

    uint16_t highest_peak = pk_data[0];
    uint16_t lowest_peak = pk_data[0];
    uint16_t neutral;
    uint8_t i;
    // Running through only valid elements of the array
    for (i = pk_front; i != pk_rear; i = (i + 1) % PK_DATA_WINDOW) {
        if (pk_data[i] > highest_peak) {
            highest_peak = pk_data[i];
        }
        if (pk_data[i] < lowest_peak) {
            lowest_peak = pk_data[i];
        }
    }

    neutral = (highest_peak + lowest_peak) / 2;
    // return -1 if length of array is less than 2*MIN_PK_GAP ?
    return neutral;
}

bool madata_isfull() {
    if ((ma_front == ma_rear + 1) || (ma_front == 0 && ma_rear == MA_DATA_WINDOW - 1))
        return true;
    else
        return false;
}

bool madata_isempty() {
    if (ma_front == -1)
        return true;
    else
        return false;
}

bool madata_insert(uint16_t element) {
    if (madata_isfull()) {
        // Can't insert data because buffer is full
        return false;
    } else {
        if (ma_front == -1)
            ma_front = 0;
        ma_rear = (ma_rear + 1) % MA_DATA_WINDOW;
        ma_data[ma_rear] = element;
        return true;
    }
}

bool madata_remove() {
    uint16_t element;
    if (madata_isempty()) {
        return false;
    } else {
        element = ma_data[ma_front];
        if (ma_front == ma_rear) {
            ma_front = -1;
            ma_rear = -1;
        } else {
            ma_front = (ma_front + 1) % MA_DATA_WINDOW;
        }
        return true;
    }
}

float get_moving_average(uint16_t datapoint) {
    madata_insert(datapoint);
    ma_window_sum += datapoint;

    if (madata_isfull()) {
        ma_window_sum -= ma_data[ma_front];
        madata_remove();
    }

    return ma_window_sum / MA_DATA_WINDOW;
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
