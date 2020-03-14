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

void stop_random_config(void)
{
	//This is an example code segment which resets the EP6 FIFO
	//where EP6 has been configured as AUTOIN
	//Note: Settings of other bits of EPxFIFOCFG are ignored here
	
	FIFORESET = 0x80; // activate NAK-ALL to avoid race conditions
	SYNCDELAY;

	EP6FIFOCFG = 0x00; //switching to manual mode
	SYNCDELAY;

	FIFORESET = 0x06; // Reset FIFO 6
	SYNCDELAY;

	EP6FIFOCFG = 0x0C; //switching to auto mode
	SYNCDELAY;

	FIFORESET = 0x00; //Release NAKALL
	SYNCDELAY;
}

void send_random_config(void)
{
	// handle the case where we were already in AUTO mode...
  // ...for example: back to back firmware downloads...                     
  EP2FIFOCFG = 0x00;            // AUTOOUT=0, WORDWIDE=1
	SYNCDELAY;
  
  // core needs to see AUTOOUT=0 to AUTOOUT=1 switch to arm endp's                   
  EP2FIFOCFG = 0x11;            // AUTOOUT=1, WORDWIDE=1  
  SYNCDELAY; 
	
  EP6FIFOCFG = 0x0D;            // AUTOIN=1, ZEROLENIN=1, WORDWIDE=1
  SYNCDELAY;
}
//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------
void TD_Init( void )
{ 
	// Called once at startup
  CPUCS = 0x12;// CLKSPD[1:0]=10, for 48MHz operation, output CLKOUT 
	
  IFCONFIG = 0xE3;   //Internal clock, 48 MHz, Slave FIFO interface
  SYNCDELAY;
	
	REVCTL = 0x03;    // REVCTL.0 and REVCTL.1 set to 1
  SYNCDELAY;
	
	EP2CFG = 0xA0;  //out 512 bytes,bulk_out, 4x,
  SYNCDELAY;
	
  EP6CFG = 0xE0;    // sets EP6 valid for IN's
	SYNCDELAY;				// and defines the endpoint for 512 byte packets, 4x buffered
  
  EP4CFG = 0x02;  //clear valid bit
  SYNCDELAY;   
	
  EP8CFG = 0x02;  //clear valid bit
  SYNCDELAY;  
	
	//reset all FIFOs
  SYNCDELAY;
  FIFORESET = 0x80;       // activate NAK-ALL to avoid race conditions
  SYNCDELAY;              // see TRM section 15.14
  FIFORESET = 0x82;       // reset, FIFO 2
  SYNCDELAY;                     
  FIFORESET = 0x84;       // reset, FIFO 4
  SYNCDELAY;                     
  FIFORESET = 0x86;       // reset, FIFO 6
  SYNCDELAY;                     
  FIFORESET = 0x88;       // reset, FIFO 8
  SYNCDELAY;                     
  FIFORESET = 0x00;       // deactivate NAK-ALL
	SYNCDELAY;              // this defines the external interface to be the following:
	
	
	// handle the case where we were already in AUTO mode...                
  // core needs to see AUTOOUT=0 to AUTOOUT=1 switch to arm endp's                   
  EP2FIFOCFG = 0x11;      // AUTOOUT=1, WORDWIDE=1  
  SYNCDELAY; 
	
  EP6FIFOCFG = 0x0D;      // AUTOIN=1, ZEROLENIN=1, WORDWIDE=1
  SYNCDELAY;
		
	
  PINFLAGSAB = 0x81;			// defines FLAGA as prog-level flag, pointed to by FIFOADR[1:0]
  SYNCDELAY;							// FLAGB as full flag, as pointed to by FIFOADR[1:0]

  PINFLAGSCD = 0x1E;			// FLAGC as empty flag, as pointed to by FIFOADR[1:0]
	SYNCDELAY;						  // won't generally need FLAGD

	PORTACFG = 0x00;        // used PA7/FLAGD as a port pin, not as a FIFO flag
	SYNCDELAY;
	
	FIFOPINPOLAR = 0x00; // set all slave FIFO interface pins as active low
	SYNCDELAY;
	EP6AUTOINLENH = 0x02; // EZ-USB automatically commits data in 512-byte chunks
	SYNCDELAY;
	EP6AUTOINLENL = 0x00;
	SYNCDELAY;
	
	SYNCDELAY;
	EP6FIFOPFH = 0x80; // you can define the programmable flag (FLAGA)
	SYNCDELAY; 				 // to be active at the level you wish
	EP6FIFOPFL = 0x00;
	
	timer0_init();
	
	PORTACFG = 0x00;		// port function
	OEA = 0x03;					// PA is output
	SYNCDELAY;

	LED_FLAG = 1;
}

// Called repeatedly while the device is idle
void TD_Poll( void )
{ 
	if(LED_FLAG)
	{
		// master currently points to EP6, pins FIFOADR[1:0]=11
		if( !( EP68FIFOFLGS & 0x01 ) )
		{ 
			// EP6FF = 0 when buffer available
			INPKTEND = 0x86; // firmware skips EP6 packet
											 // by writing 0x86 to INPKTEND
	//		release_master( EP6 );
		}
		
		INPKTEND = 0x86;
		stop_random_config();
		
	}
	
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
		 {
//				stop_random_config(); 
				LED_FLAG = 1;	 
		 }
        break;
     case VR_NAKALL_OFF:   //0xD1
		 {
				TD_Init();	
//				send_random_config();		 
				LED_FLAG = 0;
		 }
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