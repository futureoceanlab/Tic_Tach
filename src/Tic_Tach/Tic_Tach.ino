/* -------------------------------------------------------------
 *   Tic-Tach: a Teensy 3.5 based flowmeter for the Mesobot
 *   Sept 10, 2019
 *   Authors: Junsu Jang, Allan Adams, FOL/MIT & WHOI
 *
 *      Description: 
 *   
 *   Tic-Tach is a Teensy 3.5 based flow-meter widget for the Mesobot's 
 *   eDNA sampler system.   This firmware has three jobs: 
 *   
 *   1. Detect asynchronous pulses from two hall-switch-based flow 
 *      sensors on Mesobot's eDNA sampler (open-collectors pulled up 
 *      with a 15k resistor and filtered with a 300pf cap) and
 *      update the appropriate counter.
 * 
 *   2. Output the two counters over CAN-bus once per sampling_period
 * 
 *   3. Wait on commands via CAN-bus:
 *     A. RESET     Resets both counters
 *     B. SET nnn   Sets hearbeat period to nnnn in us     
 * 
 *   Jobs 1 and 2 are implemented via interrupts:
 *     1. rising-edge on appropriate pin
 *     2. timer interrupt
 * 
 *   Job 3 is processed in the main loop.    
 *   
*/


#include <FlexCAN.h>

// Define global Parameters
#define DEFAULT_SR 1000000      // Default sampling rate at 1Hz
#define DEVICE_ID 0x77         // Arbitrarily chosen device ID for now
#define MSG_LEN 8               // (bytes) 32-bit long counter value
#define RESET 0x10              // Arbitrarily chosen value assignment for reset
#define SET   0x11              // Arbitrarily chosen value assignment for setting sampling rate (us)
#define CAN_BAUDRATE 500000

// Uncomment to see serial  data
#define SERIAL_DUMP 1

// Comment Out to get Little Endian; we want BE  for the network side
#define BIG_ENDIAN_OUT 1

// Define global variables
IntervalTimer fm_timer;         // flowmeter timer instance
static CAN_message_t out_msg;   // FlexCANmessage instance for tx
static uint32_t D1 = 16;        // FlowSensor 1 data line wired to Pin D1
static uint32_t D2 = 39;        // FlowSensor 2 data line wired to Pin D2

// Initialize shared variables
uint32_t sampling_period = DEFAULT_SR;
volatile uint32_t flow_counter1 = 0;
volatile uint32_t flow_counter2 = 0;
volatile uint32_t sample1 = 0;
volatile uint32_t sample2 = 0;

// flag 
volatile uint8_t fSendData = 0;  // flag for new measurement




/***********************************************************
 *       
 *       isr_flowmeter1(), isr_flowmeter2()
 *       
 *   0. Triggered on GPIO Rising Edge Interrupt
 *   1. Iterate the counter
 *   2. Return  as  quickly as possible!
 *       
 */

static void isr_flowmeter1()
{
  flow_counter1++;
#ifdef SERIAL_DUMP
//  Serial.println("Data 1 ISR");
#endif
}

static void isr_flowmeter2() {
  flow_counter2++;
#ifdef SERIAL_DUMP
//  Serial.println("Data 2 ISR");
#endif
}





/***********************************************************      
 *       heartbeat_interrupt()
 *       
 *   0. Triggered on Timer
 *   1. Sample and store the volatile stored counter values 
 *   2. Set the new measurement flag to trigget ouput in the main loop
 * 
 *   NB: Pause interrupts while sampling counters to avoid collision with data interrupts
 *       Dont forget to turn them back on at the end.
 *       
 */
static void heartbeat_interrupt()
{  
  cli();
  sample1 = flow_counter1;
  sample2 = flow_counter2;
  sei();
  fSendData = 1;
#ifdef SERIAL_DUMP
Serial.print("Counter 1: ");
Serial.println(sample1);
Serial.print("Counter 2: ");
Serial.println(sample2);
#endif
}






/***********************************************************
 * 
 *     setup()
 * 
 *   Setup GPIO Pins
 *   Initialize GPIO interrupts
 *   Initialize CAN-bus interface
 *   Initialize timer interrupt
 *   
 * 
 */
void setup(void)
{
  delay(1000);
#ifdef SERIAL_DUMP
  Serial.println(F("Hello Mesobot!!"));  
#endif

  // Setup data GPIO Pins --  pull-up resistors (and filtering caps) present in hardware
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);

  // Attach interrupts to rising edges on the GPIO data lines
  attachInterrupt(D1, isr_flowmeter1, RISING);
  attachInterrupt(D2, isr_flowmeter2, RISING);

  // Initialize the CAN-bus instance for communication
  Can0.begin(CAN_BAUDRATE);

  // Extension 0 for 11bits or 1 for 29bits long ID
  out_msg.ext = 0;
  out_msg.id = DEVICE_ID; 
  // Send out 8-bytes long message everytime
  out_msg.len = MSG_LEN;
  // Initialize the msg buffer to all nulls
  for (int i = 0; i < MSG_LEN; i++) {
    out_msg.buf[i] = 0;
  }

  // Initialize the interval timer instance for sampling
  fm_timer.begin(heartbeat_interrupt, sampling_period);

  delay(1000);
  flow_counter1 = 0;
  flow_counter2 = 0;
}



/***********************************************************
 * 
 *        handle_cmd(uint8_t*)
 * 
  // First byte of the message contains the command 
  // next 4bytes of the message contains the relevant parameter values
  // (i.e. sampling rate in microseconds)
 *    
 */
static void handle_cmd(uint8_t *cmdPtr)
{
  uint8_t cmd = cmdPtr[0];
#ifdef SERIAL_DUMP
  Serial.println(cmd);
#endif
  // Reset counter
  if (cmd == RESET)
  {
    flow_counter1 = 0;
    flow_counter2 = 0;
    fSendData = 0;
    fm_timer.begin(heartbeat_interrupt, sampling_period);
#ifdef SERIAL_DUMP
    Serial.println("reset the flowmeter");
#endif
  }
  else if (cmd == SET)
  {
    uint32_t new_sr = 0;
    for(int i = 1; i < 5; i++) {
      // Big endian
      new_sr = (new_sr << 8) | cmdPtr[5-i];
      // Little endian
//      new_sr = (new_sr << 8) | cmdPtr[i];
    }
    sampling_period = new_sr;
    fm_timer.update(sampling_period);
#ifdef SERIAL_DUMP
    Serial.print("Updated the sampling rate to ");
    Serial.println(sampling_period);
#endif
  }
}



// -------------------------------------------------------------
void loop(void)
{
  // Check if something is in the message
  //  Yes: Handle command
  //  No: If there has been a new sample value, transmit 32bit data
  
  while (Can0.available()) 
  {
    CAN_message_t inMsg;
    Can0.read(inMsg);
    handle_cmd(inMsg.buf);
  }
  
  if (fSendData)
  {
#ifdef SERIAL_DUMP
    Serial.println("Heartbeating");
#endif
    // New counter is available, transmit as heartbeat    
    for (int i = 0; i < 4; i++)
    {
#ifdef BIG_ENDIAN_OUT
    //  Big-Endian:  Byte 0 is MSB
      out_msg.buf[3-i] = (sample1 >> (8*i)) & 0xFF;
      out_msg.buf[7-i] = (sample2 >> (8*i)) & 0xFF;
#else
    // Little-Endian: Byte 0 is LSB
      out_msg.buf[i] = (sample1 >> (8*i)) & 0xFF;
      out_msg.buf[i+4] = (sample2 >> (8*i)) & 0xFF;      
#endif
    }
    Can0.write(out_msg);
    // Reset the new measurement flag
    fSendData = 0;
  }
}
