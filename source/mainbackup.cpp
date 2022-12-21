#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// microphone addition
#include "tusb.h"
#include <vector>

#include "pico/stdlib.h"

#include "analog_microphone.h"


const struct analog_microphone_config config = {
    .gpio = 26,
    .bias_voltage = 1.25,
    .sample_rate = 16000,
    .sample_buffer_size = 256,
};

int16_t sample_buffer[256];
volatile int samples_read = 0;

// was here
#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/stdio_usb.h>
#include <stdio.h>



const uint LED_PIN = 25;

bool inferencing = false;
uint numSamples = 0


static const float features[] = new float[32000];
volatile int writing_on = 0;


int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  float arr[features.size()];
  memcpy(out_ptr, std::copy(features.begin(), features.end(), arr) + offset, length * sizeof(float));
  return 0;
}

void on_analog_samples_ready() {
    samples_read = analog_microphone_read(sample_buffer, 256);
}

int main() {


  // start was there
  // stdio_usb_init();

  // gpio_init(LED_PIN);
  // gpio_set_dir(LED_PIN, GPIO_OUT);


  ei_impulse_result_t result = {nullptr};
  // end was there

  stdio_init_all();
  while (!tud_cdc_connected()) {
      tight_loop_contents();
  }

  printf("hello analog microphone\n");

  // initialize the analog microphone
  if (analog_microphone_init(&config) < 0) {
      printf("analog microphone initialization failed!\n");
      while (1) { tight_loop_contents(); }
  }

  // set callback that is called when all the samples in the library
  // internal sample buffer are ready for reading
  analog_microphone_set_samples_ready_handler(on_analog_samples_ready);
  
  // start capturing data from the analog microphone
  if (analog_microphone_start() < 0) {
      printf("PDM microphone start failed!\n");
      while (1) { tight_loop_contents();  }
  }


  while (true)
  {

    if(inferencing) {


      ei_printf("Edge Impulse standalone inferencing (Raspberry Pi Pico)\n");

      if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE)
      {
        ei_printf("The size of your 'features' array is not correct. Expected %d items, but had %u\n",
                  EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
        return 1;
      }

      while (1)
      {
        // blink LED
        gpio_put(LED_PIN, !gpio_get(LED_PIN));

        // the features are stored into flash, and we don't want to load everything into RAM
        signal_t features_signal;
        features_signal.total_length = sizeof(features) / sizeof(features[0]);
        features_signal.get_data = &raw_feature_get_data;

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

        ei_printf("run_classifier returned: %d\n", res);

        if (res != 0)
          return 1;

        ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                  result.timing.dsp, result.timing.classification, result.timing.anomaly);

        // print the predictions
        ei_printf("[");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
        {
          ei_printf("%.5f", result.classification[ix].value);
  #if EI_CLASSIFIER_HAS_ANOMALY == 1
          ei_printf(", ");
  #else
          if (ix != EI_CLASSIFIER_LABEL_COUNT - 1)
          {
            ei_printf(", ");
          }
  #endif
        }
  #if EI_CLASSIFIER_HAS_ANOMALY == 1
        printf("%.3f", result.anomaly);
  #endif
        printf("]\n");

        ei_sleep(2000);
      }
      inferencing = false;
      writing_on = 0;
    } else {
      // record
      while (samples_read == 0) { tight_loop_contents(); }

      // store and clear the samples read from the callback
      int sample_count = samples_read;
      samples_read = 0;
      
      // loop through any new collected samples
      for (int i = 0; i < sample_count; i++) {
          // printf("%d\n", sample_buffer[i]);
          writing_on++;
          if(writing_on == 32000) {
            inferencing = true;
          } else {
            features[writing_on] = sample_buffer[i];
          }
      }
    }
  }
}