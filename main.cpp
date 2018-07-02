#include "mbed.h"
#include "lorawan_network.h"
#include "CayenneLPP.h"
#include "mbed_events.h"
#include "mbed_trace.h"
#include "lora_radio_helper.h"
#include "SX1276_LoRaRadio.h"
#include "LoRaWANInterface.h"
#include "hcsr04.h"
#include "standby.h"

#define STANDBY_TIME_S                   1 * 60

extern EventQueue ev_queue;

static uint32_t DEV_ADDR   =      0x26011EE8;
static uint8_t NWK_S_KEY[] =      { 0x7B, 0x2C, 0x68, 0x05, 0xB5, 0x6D, 0x7B, 0x05, 0x5F, 0xA3, 0x1C, 0xF2, 0x1F, 0x03, 0x26, 0xD2 };
static uint8_t APP_S_KEY[] =      { 0x88, 0x33, 0x96, 0x7F, 0xB9, 0x40, 0xB1, 0x8E, 0xDD, 0x46, 0x1A, 0xDE, 0x38, 0x90, 0x5F, 0x2A };

HCSR04 sensor(D4, D3);

  //global variable
long distance; 
bool dist_updated = false;

//void dust_sensor_cb(int lpo, float ratio, float concentration) {
 //   dust_concentration = concentration;
 //   dust_updated = true;
//}

void dist_measure(){
    sensor.start();
    wait_ms(100); 
    distance=sensor.get_dist_cm();
    dist_updated = true;
    }
    
void check_for_updated_dist() {
    if (dist_updated){
        dist_updated = false ;
        printf("Measure Distance = %ld ",distance);
        
        CayenneLPP payload(50);
        uint8_t distance_value = static_cast<uint8_t>(distance);
        printf("Distance=%u\n", distance_value);
        payload.addDigitalInput(3, distance_value);
        
        if (!lorawan_send(&payload)){
           // delete distance;
            standby(STANDBY_TIME_S);
            }
        }
    }

static void lora_event_handler(lorawan_event_t event) {
    switch (event) {
        case CONNECTED:
            printf("[LNWK][INFO] Connection - Successful\n");
            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            printf("[LNWK][INFO] Disconnected Successfully\n");
            break;
        case TX_DONE:
            printf("[LNWK][INFO] Message Sent to Network Server\n");

         //   delete distance;
            standby(STANDBY_TIME_S);
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("[LNWK][INFO] Transmission Error - EventCode = %d\n", event);

            //delete distance;
            standby(STANDBY_TIME_S);
            break;
        case RX_DONE:
            printf("[LNWK][INFO] Received message from Network Server\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("[LNWK][INFO] Error in reception - Code = %d\n", event);
            break;
        case JOIN_FAILURE:
            printf("[LNWK][INFO] OTAA Failed - Check Keys\n");
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}


int main() {
    set_time(0);
    
    printf("=========================================\n");
    printf("      Water Level Monitoring System        \n");
    printf("=========================================\n");

    lorawan_setup(DEV_ADDR, NWK_S_KEY, APP_S_KEY, lora_event_handler);

    printf("Measuring Distance...\n");
    
    //immediately measure the distance
     sensor.start();
     wait_ms(100);
     distance = sensor.get_dist_cm();
    printf("Measuring Dist =%ld...\n",distance);
      dist_measure();
     ev_queue.call_every(3000, &check_for_updated_dist);   

    ev_queue.dispatch_forever();
}



