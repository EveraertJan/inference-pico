#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include <stdio.h>
#include "f_util.h"
#include "ff.h"

#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "pico/analog_microphone.h"
#include <pico/stdio_usb.h>
#include "pico/time.h"


#include "tusb.h"
#include "rtc.h"
#include "sd_card.h"
#include <string> 

#include <hardware/gpio.h>
#include <hardware/uart.h>
#include "hardware/rtc.h"
#include "hardware/spi.h"

#include "st7735/ST7735_TFT.hpp"
ST7735_TFT myTFT;
bool bTestFPS = false;

extern "C" void writ();



FRESULT fr;
FATFS fs;
FIL fil;
int ret;
char buf[100];
char filename[] = "INFERENCE.CSV";


void SetupTFT(void)
{

  
  //*************** USER OPTION 0 SPI_SPEED + TYPE ***********
    bool bhardwareSPI = true; // true for hardware spi, 
    
    if (bhardwareSPI == true) { // hw spi
      uint32_t TFT_SCLK_FREQ =  1000 ; // Spi freq in KiloHertz , 1000 = 1Mhz
      myTFT.TFTInitSPIType(TFT_SCLK_FREQ, spi1); 
    } else { // sw spi
      myTFT.TFTInitSPIType(); 
    }
  //**********************************************************

  // ******** USER OPTION 1 GPIO *********
  // NOTE if using Hardware SPI clock and data pins will be tied to 
  // the chosen interface eg Spi0 CLK=18 DIN=19)
    int8_t SDIN_TFT = 11; 
    int8_t SCLK_TFT = 10; 
    int8_t DC_TFT = 3;
    int8_t CS_TFT = 2 ;  
    int8_t RST_TFT = 12;
    myTFT.TFTSetupGPIO(RST_TFT, DC_TFT, CS_TFT, SCLK_TFT, SDIN_TFT);
  //**********************************************************

  // ****** USER OPTION 2 Screen Setup ****** 
    uint8_t OFFSET_COL = 0;  // 2, These offsets can be adjusted for any issues->
    uint8_t OFFSET_ROW = 0; // 3, with screen manufacture tolerance/defects
    uint16_t TFT_WIDTH = 128;// Screen width in pixels
    uint16_t TFT_HEIGHT = 160; // Screen height in pixels
    myTFT.TFTInitScreenSize(OFFSET_COL, OFFSET_ROW , TFT_WIDTH , TFT_HEIGHT);
  // ******************************************

  // ******** USER OPTION 3 PCB_TYPE  **************************
    myTFT.TFTInitPCBType(TFT_ST7735R_Red); // pass enum,4 choices,see README
  //**********************************************************
}

void writeToSD(std::string in, bool separate, bool endLine) {
    if (separate) {
      in += ",";
    }
    if (endLine) {
      in += "\r\n";
    }
    
    std::basic_string<TCHAR> converted(in.begin(), in.end());

    const TCHAR *tchar = converted.c_str();

    // Write something to file

    ret = f_printf(&fil, tchar);

    if (ret < 0) {
        printf("ERROR: Could not write to file (%d)\r\n", ret);
        f_close(&fil);
    } else {
      printf("       w(%d) \n", ret);
    }

}

void initSD() {


    // Initialize SD card
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
    }

    // Open file for writing ()
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", fr);
    }
}

void closeSD() {

    // Close file
    fr = f_close(&fil);
    if (fr != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", fr);
    }

    // Unmount drive
    f_unmount("0:");
}

// void SetupTFT(void) {

    
//   //*************** USER OPTION 0 SPI_SPEED + TYPE ***********
//   bool bhardwareSPI = true; // true for hardware spi, 
  
//   if (bhardwareSPI == true) { // hw spi
//       uint32_t TFT_SCLK_FREQ =  1000 ; // Spi freq in KiloHertz , 1000 = 1Mhz
//       myTFT.TFTInitSPIType(TFT_SCLK_FREQ, spi1); 
//   } else { // sw spi
//       myTFT.TFTInitSPIType(); 
//   }
//   //**********************************************************

//   // ******** USER OPTION 1 GPIO *********
//   // NOTE if using Hardware SPI clock and data pins will be tied to 
//   // the chosen interface eg Spi0 CLK=18 DIN=19)
//       int8_t SDIN_TFT = 11; 
//       int8_t SCLK_TFT = 10; 
//       int8_t DC_TFT = 3;
//       int8_t CS_TFT = 2 ;  
//       int8_t RST_TFT = 12;
//       myTFT.TFTSetupGPIO(RST_TFT, DC_TFT, CS_TFT, SCLK_TFT, SDIN_TFT);
//   //**********************************************************

//   // ****** USER OPTION 2 Screen Setup ****** 
//       uint8_t OFFSET_COL = 0;  // 2, These offsets can be adjusted for any issues->
//       uint8_t OFFSET_ROW = 0; // 3, with screen manufacture tolerance/defects
//       uint16_t TFT_WIDTH = 128;// Screen width in pixels
//       uint16_t TFT_HEIGHT = 160; // Screen height in pixels
//       myTFT.TFTInitScreenSize(OFFSET_COL, OFFSET_ROW , TFT_WIDTH , TFT_HEIGHT);
//   // ******************************************

//   // ******** USER OPTION 3 PCB_TYPE  **************************
//       myTFT.TFTInitPCBType(TFT_ST7735R_Red); // pass enum,4 choices,see README
//   //**********************************************************
// }

// configuration
const struct analog_microphone_config config = {
    // GPIO to use for input, must be ADC compatible (GPIO 26 - 28)
    .gpio = 26,

    // bias voltage of microphone in volts
    .bias_voltage = 1.25,

    // sample rate in Hz
    .sample_rate = 16000,

    // number of samples to buffer
    .sample_buffer_size = 256,
};

// variables
int16_t sample_buffer[256];
volatile int samples_read = 0;



const uint MAX_FEATURE_LENGTH = 16000;
const uint LED = 25;

bool inferencing = false;
uint numSamples = 0;

float features[MAX_FEATURE_LENGTH];
volatile int writing_on = 0;

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}


void on_analog_samples_ready()
{
    // callback from library when all the samples in the library
    // internal sample buffer are ready for reading 
    samples_read = analog_microphone_read(sample_buffer, 256);
}

void resultOnDisplay(char toDisplay[]) {
  myTFT.TFTfillScreen(ST7735_BLACK);
  myTFT.TFTdrawText(20, 10, toDisplay, ST7735_WHITE, ST7735_BLACK, 1);
}
int main( void )
{
    ei_impulse_result_t result = {nullptr};
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();


    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];

    // Start on Friday 5th of June 2020 15:45:00
    datetime_t t = {
            .year  = 2023,
            .month = 01,
            .day   = 9,
            .dotw  = 1, // 0 is Sunday, so 5 is Friday
            .hour  = 15,
            .min   = 45,
            .sec   = 00
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }

    printf("hello analog microphone\n");

    // initialize the analog microphone
    if (analog_microphone_init(&config) < 0) {
        printf("analog microphone initialization failed!\n");
        while (1) { tight_loop_contents(); }
    }
    printf("after analog microphone\n");
    ei_sleep(1000);


    // set callback that is called whanalogen all the samples in the library
    // internal sample buffer are ready for reading
    analog_microphone_set_samples_ready_handler(on_analog_samples_ready);
    printf("after sample\n");
    

    // printf("result of microphone: %d \n", analog_microphone_start());
    
    if (analog_microphone_start() < 0) {
        printf("PDM microphone start failed!\n");
        // while (1) { tight_loop_contents();  }
    }    

    initSD();
    writeToSD("time,timingDSP-classification-anomaly,c1,c2,c3,", false, true);
    closeSD();

    SetupTFT();

    while (true) {

        rtc_get_datetime(&t);
        if(inferencing) {


            printf("Edge Impulse standalone inferencing (Raspberry Pi Pico)\n");
            printf("finding: %d\n", sizeof(features));
            printf("\n");
            initSD();
            if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
                ei_printf("The size of your 'features' array is not correct. Expected %d items, but had %u\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
                // inferencing = false;
                printf("error\n");
            } else {

                printf("after check\n");
        


                // the features are stored into flash, and we don't want to load everything into RAM
                signal_t features_signal;
                features_signal.total_length = sizeof(features) / sizeof(features[0]);
                features_signal.get_data = &raw_feature_get_data;

                // invoke the impulse
                printf("invoking\n");
        
                EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

                printf("invoked\n");
        
                printf("run_classifier returned: %d\n", res);

                

                if (res != 0) {
                    printf("error\n");
                } else {

                  // if (FR_OK != fr && FR_EXIST != fr) {
                  //   panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
                  // }


                  datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
                  printf("\r%s      ", datetime_str);
                  writeToSD( datetime_str, false, false);
                  writeToSD( "," + std::to_string(result.timing.dsp)+"-"+std::to_string(result.timing.classification)+"-"+std::to_string(result.timing.anomaly) + ",", false, false);
                  

                  printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n", result.timing.dsp, result.timing.classification, result.timing.anomaly);

                  // print the predictions
                  printf("[");
                 

                  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
                  {
                      ei_printf("%.5f", result.classification[ix].value);
                      writeToSD( std::to_string(result.classification[ix].value), true, false);

                      if(EI_CLASSIFIER_HAS_ANOMALY == 1) {
                        ei_printf(", ");
                      } else {
                        if (ix != EI_CLASSIFIER_LABEL_COUNT - 1)
                        {
                          ei_printf(", ");
                        }
                    }

                  }
                  if(result.classification[0].value > result.classification[1].value && result.classification[0].value > result.classification[2].value) {
                    resultOnDisplay("SOCIAL");
                  }
                  if(result.classification[1].value > result.classification[0].value && result.classification[1].value > result.classification[2].value) {
                    resultOnDisplay("SILENCE");
                  }
                  if(result.classification[2].value > result.classification[0].value && result.classification[2].value > result.classification[1].value) {
                    resultOnDisplay("TALKING");
                  }
                  if(EI_CLASSIFIER_HAS_ANOMALY == 1) {
                      printf("%.3f", result.anomaly);
                  }

                }

                printf("]\n");
            }   
            writeToSD("", false, true); 
            ei_sleep(10000);
            closeSD();
          
            inferencing = false;
            writing_on = 0;
        } else {

            // wait for new samples
            while (samples_read == 0) { tight_loop_contents(); }

            // store and clear the samples read from the callback
            int sample_count = samples_read;
            samples_read = 0;
            
            // loop through any new collected samples
            for (int i = 0; i < sample_count; i++) {
                //printf("%d\n", sample_buffer[i]);
                writing_on++;
                  if(writing_on >= MAX_FEATURE_LENGTH) {
                    inferencing = true;
                  } else {
                    features[writing_on] = sample_buffer[i];
                  }
            }
        }
    }

    return 0;
}
