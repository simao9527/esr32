#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "car_track.h"
#include "tb6612_driver.h"
#include "line_tracker.h"

// OLED显示屏配置
#define SCL_PIN 9
#define SDA_PIN 8

// 创建OLED对象
SSD1306Wire display(0x3c, SDA_PIN, SCL_PIN);

// 循迹任务句柄
TaskHandle_t track_task_handle = NULL;

void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== SETUP START ===");   // 新增
    Serial.println("====================================");
    Serial.println("Line Tracking Program");
    Serial.println("====================================");
    Serial.println("");

    // 初始化I2C
    Wire.begin(SDA_PIN, SCL_PIN);

    // 初始化OLED
    Serial.println("Initializing OLED...");
    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);
    Serial.println("OLED initialized!");
    Serial.println("");

    // 初始化电机驱动
    Serial.println("Initializing motor driver...");
    tb6612_init();
    Serial.println("Motor driver initialized!");
    Serial.println("");

    // 初始化循迹传感器
    Serial.println("Initializing line tracker...");
    if (line_tracker_init()) {
        Serial.println("Line tracker initialized successfully!");
    } else {
        Serial.println("Line tracker initialization failed!");
    }
    Serial.println("");

    // 设置循迹参数
    car_track_config_t track_config = {
        .track_speed = 120,        // 基础速度
        .track_max_speed = 170,    // 最大速度
        .track_min_speed = 60,     // 最小速度
        .max_lost_count = 15,      // 最大丢线次数
        .check_interval_ms = 20,   // 检测周期
        .kp = 22.0f,               // PID参数
        .ki = 0.0f,
        .kd = 10.0f
    };

    // 设置速度档位
    car_track_set_speed_level(CAR_TRACK_SPEED_MID);
    Serial.println("Speed level set to MID");
    Serial.println("");

    // 启动循迹任务
    Serial.println("Starting line tracking task...");
    track_task_handle = car_track_start(&track_config);
    if (track_task_handle != NULL) {
        Serial.println("Line tracking task started successfully!");
    } else {
        Serial.println("Failed to start line tracking task!");
    }
    Serial.println("");

    Serial.println("Setup complete! Line tracking is active...");
    Serial.println("====================================");
    Serial.println("");
}

void loop() {
    static unsigned long last_update = 0;
    unsigned long current_time = millis();

    // 每200ms更新一次显示
    if (current_time - last_update >= 200) {
        last_update = current_time;

        // 读取传感器状态
        line_tracker_state_t tracker_state;
        if (line_tracker_read_all(&tracker_state)) {
            // 获取当前电机PWM值
            int16_t left_pwm, right_pwm;
            car_track_get_motor_pwm(&left_pwm, &right_pwm);

            display.clear();
            display.setFont(ArialMT_Plain_10);

            // 显示标题
            display.drawString(0, 0, "Line Tracking");

            // 显示传感器状态
            display.drawString(0, 16, "S1:" + String(tracker_state.sensor1) + " S2:" + String(tracker_state.sensor2) + " S3:" + String(tracker_state.sensor3));
            display.drawString(0, 32, "S4:" + String(tracker_state.sensor4) + " S5:" + String(tracker_state.sensor5));

            // 显示电机PWM值
            display.drawString(0, 48, "L:" + String(left_pwm) + " R:" + String(right_pwm));

            display.display();
        }
    }
}
