// -------------------------------------------------------------
// Simple flowmeter system with Teensy and CAN-bus
// Aug 29th 2019
// Authors: Junsu Jang, Allan Adams
//
// Description: 
// Teensy uses Can0 (pin3(TX) and pin4(RX)) for communication
// It waits for a command from the master (RESET or SET) and
// at every sampling rate transmits a message as its heartbeat

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




// Define global variables
IntervalTimer fm_timer;         // flowmeter timer instance
static CAN_message_t out_msg;   // message instance for tx
static uint32_t D1 = 16;
static uint32_t D2 = 39;

// Initialize shared variables
uint32_t sampling_period = DEFAULT_SR;
volatile uint32_t flow_counter1 = 0;
volatile uint32_t flow_counter2 = 0;
volatile uint32_t sample1 = 0;
volatile uint32_t sample2 = 0;

// flag 
volatile uint8_t fSendData = 0;  // flag for new measurement



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




// -------------------------------------------------------------
static void heartbeat_interrupt()
{
  // 1. Sample the flow and store the value
  // 2. set the new measurement flag
  
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





// -------------------------------------------------------------
void setup(void)
{
  delay(1000);
#ifdef SERIAL_DUMP
  Serial.println(F("Hello Mesobot!!"));  
#endif

  // Setup data GPIO Pins
  pinMode(D1, INPUT);
  attachInterrupt(D1, isr_flowmeter1, RISING);
  pinMode(D2, INPUT);
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
//    flow_counter1 = 4097;
//    flow_counter2 = 0xFFF00F00;
  flow_counter1 = 0;
  flow_counter2 = 0;
}



// -------------------------------------------------------------
static void handle_cmd(uint8_t *cmdPtr)
{
  // I assume the following:
  // First byte of the message contains the command 
  // next 4bytes of the message contains the relevant parameter values
  // (i.e. sampling rate in microseconds)
  
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
      //  Big-endian
      out_msg.buf[i] = (sample1 >> (8*i)) & 0xFF;
      out_msg.buf[i+4] = (sample2 >> (8*i)) & 0xFF;      
      // Little-Endian
//      out_msg.buf[3-i] = (sample1 >> (8*i)) & 0xFF;
//      out_msg.buf[7-i] = (sample2 >> (8*i)) & 0xFF;
    }
/*
#ifdef SERIAL_DUMP
    uint32_t o0 = out_msg.buf[0] ;
    uint32_t o1 = out_msg.buf[1] ;
    uint32_t o2 = out_msg.buf[2] ;
    uint32_t o3 = out_msg.buf[3] ;
    uint32_t o4 = out_msg.buf[4] ;
    uint32_t o5 = out_msg.buf[5] ;
    uint32_t o6 = out_msg.buf[6] ;
    uint32_t o7 = out_msg.buf[7] ;
    Serial.println(sample1);
    Serial.println(o0);
    Serial.println(o1);
    Serial.println(o2);
    Serial.println(o3);
    Serial.println(sample2);
    Serial.println(o4);
    Serial.println(o5);
    Serial.println(o6);
    Serial.println(o7);
#endif
*/
    Can0.write(out_msg);
    // Reset the new measurement flag
    fSendData = 0;
  }
}
