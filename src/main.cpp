#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Wire.h"

// OLED显示屏配置
#define SCL_PIN 9
#define SDA_PIN 8

// 创建OLED对象，参数：I2C地址, SDA, SCL
SSD1306Wire display(0x3c, SDA_PIN, SCL_PIN);

// I2C扫描设备
void scanI2CDevices() {
    byte error, address;
    int nDevices;
    
    Serial.println("Scanning I2C bus...");
    Serial.println("SCL: GPIO9, SDA: GPIO8");
    
    nDevices = 0;
    for(address = 1; address < 127; address++) {
        // 使用Wire库的beginTransmission和endTransmission来检测设备
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");
            nDevices++;
        }
        else if (error == 4) {
            Serial.print("Unknown error at address 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0) {
        Serial.println("No I2C devices found\n");
    }
    else {
        Serial.println("done\n");
    }
}

void setup() {
    // 初始化串口
    Serial.begin(115200);
    delay(1000);  // 等待串口稳定
    
    Serial.println("====================================");
    Serial.println("I2C OLED Test Program");
    Serial.println("====================================");
    Serial.println("");
    
    // 初始化Wire（I2C）
    Serial.println("Initializing I2C...");
    Wire.begin(SDA_PIN, SCL_PIN);
    Serial.println("I2C initialized on SDA=GPIO8, SCL=GPIO9");
    Serial.println("");
    
    // 扫描I2C设备
    scanI2CDevices();
    Serial.println("");
    
    // 初始化OLED
    Serial.println("Initializing OLED...");
    display.init();
    Serial.println("OLED initialized successfully!");
    
    // 设置字体方向
    display.flipScreenVertically();
    Serial.println("Screen flipped vertically");
    
    // 设置字体
    display.setFont(ArialMT_Plain_10);
    Serial.println("Font set to ArialMT_Plain_10");
    
    // 显示初始测试信息
    Serial.println("Displaying test information...");
    display.clear();
    display.drawString(0, 0, "OLED Test");
    display.drawString(0, 16, "SCL: GPIO9");
    display.drawString(0, 32, "SDA: GPIO8");
    display.drawString(0, 48, "Display OK!");
    display.display();
    Serial.println("Test information displayed on OLED");
    Serial.println("");
    Serial.println("Setup complete! Starting main loop...");
    Serial.println("====================================");
    Serial.println("");
}

void loop() {
    // 在这里可以添加需要循环显示的内容
    // 例如显示运行时间
    static unsigned long lastTime = 0;
    static unsigned long lastScanTime = 0;
    unsigned long currentTime = millis();
    
    // 每秒更新一次显示
    if(currentTime - lastTime >= 1000) {
        lastTime = currentTime;
        
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, "OLED Test");
        display.drawString(0, 16, "SCL: GPIO9");
        display.drawString(0, 32, "SDA: GPIO8");
        display.drawString(0, 48, "Uptime: " + String(currentTime / 1000) + " s");
        display.display();
        
        Serial.print("Display updated at ");
        Serial.print(currentTime / 1000);
        Serial.println(" seconds");
    }
    
    // 每10秒扫描一次I2C设备
    if(currentTime - lastScanTime >= 10000) {
        lastScanTime = currentTime;
        Serial.println("");
        Serial.println("Periodic I2C scan...");
        scanI2CDevices();
        Serial.println("");
    }
}
