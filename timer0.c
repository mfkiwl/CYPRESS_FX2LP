//-----------------------------------------------------------------------------
// File: timer.c
// Contents: Programmable timer interrupt that controls step
// interval (ranging from 1-5 s) of LED display from 0-9, using //
//endpoint control from the Control Panel.
// Length Field: Controls how many times the 0-9 count will loop.
// Hex Bytes Field: Controls how fast the count steps
// 1) 01h -> 1s
// 2) 02h -> 2s
// 3) 03h -> 3s
// 4) 04h -> 4s
// 5) 05h -> 5s//
//
//-----------------------------------------------------------------------------
// Copyright 2003, Cypress Semiconductor Corporation
//------------------------------------------------------------------------------------

#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro
// 10ms interrupt
#define TIMER0_COUNT 0x63C0 // 10000h - ((48,000,000 Hz / (12 * 100))
//100us
//#define TIMER0_COUNT 0xFE6F // 10000h - ((48,000,000 Hz / (12 * 100))
// FX2LP runs on either a 4-clock
// bus cycle or the traditional 12-clock bus
// cycle
static unsigned timer0_tick; // timer tick variable
static unsigned delay_count;
extern BYTE LED_FLAG;

// Timer Interrupt
// This function is an interrupt service routine for Timer 0. It should never
// be called by a C or assembly function. It will be executed automatically
// when Timer 0 overflows.
// "interrupt 1" tells the compiler to look for this ISR at address 000Bh
// "using 1" tells the compiler to use register bank 1
void timer0 (void) interrupt 1 using 1
{
	// Stop Timer 0, adjust the Timer 0 counter so that we get another
	// in 10ms, and restart the timer.
//	TR0 = 0; // stop timer
//	TL0 = TL0 + (TIMER0_COUNT & 0x00FF);
//	TH0 = TH0 + (TIMER0_COUNT >> 8);
//	TR0 = 1; // start Timer 0
//	// Increment the timer tick. This interrupt should occur approximately every 10ms. So,
//	//the resolution of the timer will be 100Hz not including interrupt latency.
//	timer0_tick++;
	
	if(LED_FLAG)
	{
		delay_count++;
		if(delay_count >= 2)
		{		
			IOA = 0x02;
			SYNCDELAY;
			delay_count = 0;
		}
		else
		{
			IOA = 0x01;
			SYNCDELAY;
		}
	}
	else
	{
		IOA = 0x01;
		SYNCDELAY;
		delay_count = 0;
	}

}

void timer0_init (void);
// This function enables Timer 0. Timer 0 generates a synchronous interrupt
// once every 100Hz or 10 ms.
void timer0_init (void)
{	

	EA = 0; // disables all interrupts
	timer0_tick = 0;
	TR0 = 0; // stops Timer 0
	CKCON = 0x03; // Timer 0 using CLKOUT/12
	TMOD &= ~0x0F; // clear Timer 0 mode bits
	TMOD |= 0x01; // setup Timer 0 as a 16-bit timer
	TL0 = (TIMER0_COUNT & 0x00FF); // loads the timer counts
	TH0 = (TIMER0_COUNT >> 8);
	PT0 = 0; // sets the Timer 0 interrupt to low priority
	ET0 = 1; // enables Timer 0 interrupt
	TR0 = 1; // starts Timer 0
	EA = 1; // enables all interrupts

}

// This function returns the current Timer 0 tick count.
unsigned timer0_count (void)
{
	unsigned t;
	EA = 0;
	t = timer0_tick;
	EA = 1;
	return (t);
}

// This function waits for 'count' timer ticks to pass.
void timer0_delay (unsigned count)
{
	unsigned start_count;
	start_count = timer0_count(); // get the starting count
	// wait for timer0_tick to reach count
	while ((timer0_count() - start_count) <= count){}
}
 

