#include <Arduino.h>
#include "line_tracker.h"

void setup()
{
    Serial.begin(115200);

    line_tracker_init();
}

void loop()
{
    line_tracker_state_t state;

    if(line_tracker_read_all(&state))
    {
        Serial.printf("%d %d %d %d %d\n",
                      state.sensor1,
                      state.sensor2,
                      state.sensor3,
                      state.sensor4,
                      state.sensor5);
    }

    delay(100);
}