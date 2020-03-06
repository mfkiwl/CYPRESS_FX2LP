#pragma NOIV                    // Do not generate interrupt vectors
//-----------------------------------------------------------------------------
//   File:      slave.c
//   Contents:  Hooks required to implement USB peripheral function.
//              Code written for FX2 REVE 56-pin and above.
//              This firmware is used to demonstrate FX2 Slave FIF
//              operation.
//   Copyright (c) 2003 Cypress Semiconductor All rights reserved
//-----------------------------------------------------------------------------
#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro

extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

void timer0_init (void);

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings

BYTE LED_FLAG;
           
#define VR_NAKALL_ON    0xD0
#define VR_NAKALL_OFF   0xD1
#define VR_LED_ON   0xD2

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------
void TD_Init( void )
{ 
	// Called once at startup
	
  // CLKSPD[1:0]=10, for 48MHz operation, output CLKOUT
  CPUCS = 0x12; 
	
	timer0_init();

  //配置FIFO标志输出，FLAG B配置为EP2 OUT FIFO空标志
  PINFLAGSAB = 0x81;			// FLAGB - EP2EF
  SYNCDELAY;

  //配置FIFO标志输出，FLAG C配置为EP6 IN FIFO满标志
  PINFLAGSCD = 0x1E;			// FLAGC - EP6FF
  SYNCDELAY;

  //配置FIFO标志输出，FLAG G配置为EP2 OUT FIFO空标志
  PORTACFG |= 0x80;

  SYNCDELAY;
	
	//Slave使用内部48MHz的时钟
  IFCONFIG = 0xE3; //Internal clock, 48 MHz, Slave FIFO interface
  SYNCDELAY;
 
	//将EP2断端点配置为BULK-OUT端点，使用4倍缓冲，512字节FIFO             
  EP2CFG = 0xA0;                //out 512 bytes, 4x, bulk
  SYNCDELAY;
	//将EP6配置为BULK-OUT端点，                    
  EP6CFG = 0xE0;                // in 512 bytes, 4x, bulk
  SYNCDELAY;              
  EP4CFG = 0x02;                //clear valid bit
  SYNCDELAY;                     
  EP8CFG = 0x02;                //clear valid bit
  SYNCDELAY;   

  //复位FIFO
  SYNCDELAY;
  FIFORESET = 0x80;             // activate NAK-ALL to avoid race conditions
  SYNCDELAY;                    // see TRM section 15.14
  FIFORESET = 0x02;             // reset, FIFO 2
  SYNCDELAY;                     
  FIFORESET = 0x04;             // reset, FIFO 4
  SYNCDELAY;                     
  FIFORESET = 0x06;             // reset, FIFO 6
  SYNCDELAY;                     
  FIFORESET = 0x08;             // reset, FIFO 8
  SYNCDELAY;                     
  FIFORESET = 0x00;             // deactivate NAK-ALL
	SYNCDELAY; 


  // handle the case where we were already in AUTO mode...
  // ...for example: back to back firmware downloads...
                     
  EP2FIFOCFG = 0x00;            // AUTOOUT=0, WORDWIDE=1
	SYNCDELAY;
  
  // core needs to see AUTOOUT=0 to AUTOOUT=1 switch to arm endp's                   
  EP2FIFOCFG = 0x11;            // AUTOOUT=1, WORDWIDE=1  
  SYNCDELAY; 
	
  EP6FIFOCFG = 0x0D;            // AUTOIN=1, ZEROLENIN=1, WORDWIDE=1
  SYNCDELAY;
	
	PORTACFG = 0x00;		// port function
	OEA = 0x03;					// PA4 is output
	SYNCDELAY;
//	IOA = 0x01;
//	SYNCDELAY;

	LED_FLAG = 1;
}

// Called repeatedly while the device is idle
void TD_Poll( void )
{ 

}

BOOL TD_Suspend( void )          
{ // Called before the device goes into suspend mode
//   return( TRUE );
		 return( FALSE ) ;
}

BOOL TD_Resume( void )          
{ // Called after the device resumes
   return( TRUE );
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------
BOOL DR_GetDescriptor( void )
{
   return( TRUE );
}

BOOL DR_SetConfiguration( void )   
{ // Called when a Set Configuration command is received
  
  if( EZUSB_HIGHSPEED( ) )
  { // ...FX2 in high speed mode
    EP6AUTOINLENH = 0x02;
    SYNCDELAY;
    EP8AUTOINLENH = 0x02;   // set core AUTO commit len = 512 bytes
    SYNCDELAY;
    EP6AUTOINLENL = 0x00;
    SYNCDELAY;
    EP8AUTOINLENL = 0x00;
  }
  else
  { // ...FX2 in full speed mode
    EP6AUTOINLENH = 0x00;
    SYNCDELAY;
    EP8AUTOINLENH = 0x00;   // set core AUTO commit len = 64 bytes
    SYNCDELAY;
    EP6AUTOINLENL = 0x40;
    SYNCDELAY;
    EP8AUTOINLENL = 0x40;
  }
      
  Configuration = SETUPDAT[ 2 ];
  return( TRUE );        // Handled by user code
}

BOOL DR_GetConfiguration( void )   
{ // Called when a Get Configuration command is received
   EP0BUF[ 0 ] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);          // Handled by user code
}

BOOL DR_SetInterface( void )       
{ // Called when a Set Interface command is received
   AlternateSetting = SETUPDAT[ 2 ];
   return( TRUE );        // Handled by user code
}

BOOL DR_GetInterface( void )       
{ // Called when a Set Interface command is received
   EP0BUF[ 0 ] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return( TRUE );        // Handled by user code
}

BOOL DR_GetStatus( void )
{
   return( TRUE );
}

BOOL DR_ClearFeature( void )
{
   return( TRUE );
}

BOOL DR_SetFeature( void )
{
   return( TRUE );
}

BOOL DR_VendorCmnd( void )
{  
  switch (SETUPDAT[1])   
  {
     case VR_NAKALL_ON:   //0xD0
			 
			LED_FLAG = 1;
		 
//			IOA = 0x01;				// Green ON/Red flicker
//			SYNCDELAY;
	 
			FIFORESET = 0x80; // activate NAK-ALL to avoid race conditions
			SYNCDELAY;
	 
			EP6FIFOCFG = 0x00; //switching to manual mode
			SYNCDELAY;
	 
			FIFORESET = 0x06; // Reset FIFO 2
			SYNCDELAY;
	 
			OUTPKTEND = 0x86; //OUTPKTEND done twice as EP2 is double buffered by default
			SYNCDELAY;
	 
			OUTPKTEND = 0x86;
			SYNCDELAY;
	 
			EP6FIFOCFG = 0x10; //switching to auto mode
			SYNCDELAY;
	 
			FIFORESET = 0x00; //Release NAKALL
			SYNCDELAY;
		
        break;
     case VR_NAKALL_OFF:   //0xD1
				TD_Init();		 
				LED_FLAG = 0;
        break;
     default:
        return(TRUE);
  }
  return( FALSE );
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav( void ) interrupt 0
{
   GotSUD = TRUE;         // Set flag
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUDAV;      // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok( void ) interrupt 0
{
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUTOK;      // Clear SUTOK IRQ
}

void ISR_Sof( void ) interrupt 0
{
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSOF;        // Clear SOF IRQ
}

void ISR_Ures( void ) interrupt 0
{
   if ( EZUSB_HIGHSPEED( ) )
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }
   
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmURES;       // Clear URES IRQ
}

void ISR_Susp( void ) interrupt 0
{
   Sleep = TRUE;
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUSP;
}

void ISR_Highspeed( void ) interrupt 0
{
   if ( EZUSB_HIGHSPEED( ) )
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }

   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack( void ) interrupt 0
{
}
void ISR_Stub( void ) interrupt 0
{
}
void ISR_Ep0in( void ) interrupt 0
{
}
void ISR_Ep0out( void ) interrupt 0
{
}
void ISR_Ep1in( void ) interrupt 0
{
}
void ISR_Ep1out( void ) interrupt 0
{
}
void ISR_Ep2inout( void ) interrupt 0
{
}
void ISR_Ep4inout( void ) interrupt 0
{
}
void ISR_Ep6inout( void ) interrupt 0
{
}
void ISR_Ep8inout( void ) interrupt 0
{
}
void ISR_Ibn( void ) interrupt 0
{
}
void ISR_Ep0pingnak( void ) interrupt 0
{
}
void ISR_Ep1pingnak( void ) interrupt 0
{
}
void ISR_Ep2pingnak( void ) interrupt 0
{
}
void ISR_Ep4pingnak( void ) interrupt 0
{
}
void ISR_Ep6pingnak( void ) interrupt 0
{
}
void ISR_Ep8pingnak( void ) interrupt 0
{
}
void ISR_Errorlimit( void ) interrupt 0
{
}
void ISR_Ep2piderror( void ) interrupt 0
{
}
void ISR_Ep4piderror( void ) interrupt 0
{
}
void ISR_Ep6piderror( void ) interrupt 0
{
}
void ISR_Ep8piderror( void ) interrupt 0
{
}
void ISR_Ep2pflag( void ) interrupt 0
{
}
void ISR_Ep4pflag( void ) interrupt 0
{
}
void ISR_Ep6pflag( void ) interrupt 0
{
}
void ISR_Ep8pflag( void ) interrupt 0
{
}
void ISR_Ep2eflag( void ) interrupt 0
{
}
void ISR_Ep4eflag( void ) interrupt 0
{
}
void ISR_Ep6eflag( void ) interrupt 0
{
}
void ISR_Ep8eflag( void ) interrupt 0
{
}
void ISR_Ep2fflag( void ) interrupt 0
{
}
void ISR_Ep4fflag( void ) interrupt 0
{
}
void ISR_Ep6fflag( void ) interrupt 0
{
}
void ISR_Ep8fflag( void ) interrupt 0
{
}
void ISR_GpifComplete( void ) interrupt 0
{
}
void ISR_GpifWaveform( void ) interrupt 0
{
}