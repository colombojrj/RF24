#include "main_cpp.h"
#include <stdint.h>
#include <string.h>

// Includes the peripheral initialization files (make sure to mark
// "Generate peripheral initialization as a pair of '.c/.h' files per
// peripheral" in CubeMX editor -> Project Manager -> Code Generator
#include "spi.h"            // from Core/Inc/spi.h
#include "usbd_cdc_if.h"

// RF24 driver
// Copy the RF24 repository files into a new folder: Drivers/RF24
#include "Drivers/RF24/RF24.h"

// The BluePill has a USB Virtual COM, but you may customize this define
// to what fits your hardware
#define print(x) CDC_Transmit_FS((uint8_t*) x, strlen(x))

// Instantiate an object for the nRF24L01 transceiver and the respective spi
RF24 radio(RF24_PB0, RF24_PB1);  // using pin PB0 for the CE pin, and pin PB1 for the CSN pin
RF24_SPI rf24_spi;               // you want to use other spi handler than spi 1

// Let these addresses be used for the pair
uint8_t address[][6] = { "1Node", "2Node" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// Use this variable to control whether this node is sending or receiving
bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing a single number
// that will be incremented on every successful transmission
int payload = 0;

void setup() {
    // Initialize the RF24 SPI driver
    rf24_spi.begin(&hspi1);

    // initialize the transceiver on the SPI bus
    if (!radio.begin()) {
        print("radio hardware is not responding!!\n");
        while (1) {}  // hold in infinite loop
    }

    // print example's introductory prompt
    print("RF24/examples/GettingStarted\n");

    // To set the radioNumber via the Serial monitor on startup
    if (role)
        print("This is a TX node\n");
    else
        print("This is a RX node\n");

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit a float
    radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[role]);  // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!role]);  // using pipe 1

    // additional setup specific to the node's role
    if (role)
        radio.stopListening();  // put radio in TX mode
    else
        radio.startListening();  // put radio in RX mode
}

void loop() {
    char string_buffer[80];

    if (role)
    {
        // This device is a TX node

        int start_timer = rf24_get_time_us();                // start the timer
        bool report = radio.write(&payload, sizeof(float));  // transmit & save the report
        int end_timer = rf24_get_time_us();                  // end the timer

        if (report) {
            sprintf(string_buffer, "Transmission successful! Time to transmit = %d us. Sent: %d\n", end_timer - start_timer, payload);
            print(string_buffer);  // print payload sent
            payload += 1;          // increment float payload
        } else {
            print("Transmission failed or timed out\n");         // payload was not delivered
        }

        // to make this example readable in the serial monitor
        HAL_Delay(1000);  // slow transmissions down by 1 second

    }
    else
    {
        // This device is a RX node

        uint8_t pipe;
        if (radio.available(&pipe)) {                // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
            radio.read(&payload, bytes);             // fetch payload from FIFO
            sprintf(string_buffer, "Received %d bytes on pipe %d: %d\n", bytes, pipe, payload);
            print(string_buffer);
        }
    }  // role
}

