#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

#define GlassViewableWidth 126
#define GlassViewableHeight 126

LilyGo_Class amoled;
WebServer server(80);

const char* ssid = ""; 
const char* password = "";
lv_obj_t *messageLabel;
lv_obj_t *window;

bool messageDisplayed = false;

void button_event_callback(ButtonState state) {
    Serial.print("Button event: ");
    Serial.println(state);
    if (state == BTN_CLICK_EVENT && messageDisplayed) {
        Serial.println("Button clicked, dismissing message");
        messageDisplayed = false;
        lv_label_set_text(messageLabel, "");  // Clear the message
        amoled.setBrightness(0);  // Set brightness back to 0
    }
}

void setup() {
    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");

    // Start the server
    server.on("/", HTTP_GET, handleRoot);
    server.on("/send", HTTP_POST, handleMessage);
    server.begin();
    Serial.println("Web server started");

    // Initialization screen and peripherals
    bool rslt = amoled.begin();
    if (!rslt) {
        while (1) {
            Serial.println("The board model cannot be detected, please raise the Core Debug Level to an error");
            delay(1000);
        }
    }

    amoled.setRotation(0);
    amoled.setBrightness(0);  // Set initial brightness to 0
    amoled.setTouchThreshold(200);
    amoled.setEventCallback(button_event_callback);

    // Initialize lvgl
    beginLvglHelper(amoled);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    window = lv_obj_create(lv_scr_act());
    lv_obj_set_style_border_width(window, 0, 0);
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create message label
    messageLabel = lv_label_create(window);
    lv_label_set_text(messageLabel, "");  // Start with a blank screen
    lv_obj_set_width(messageLabel, GlassViewableWidth - 10);  // Adjust width for padding
    lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP); // Enable text wrapping
    lv_obj_set_style_text_font(messageLabel, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_center(messageLabel);
}

void loop() {
    // Handle web server requests
    server.handleClient();

    // Update the touch button state
    amoled.update();

    // lvgl task processing should be placed in the loop function
    if (messageDisplayed) {
        lv_timer_handler();
        delay(2);
    }
}

// Handle the root URL
void handleRoot() {
    server.send(200, "text/html", "<form action=\"/send\" method=\"post\"><input type=\"text\" name=\"message\"><input type=\"submit\" value=\"Send\"></form>");
}

// Handle the /send URL
void handleMessage() {
    if (server.hasArg("message")) {
        String message = server.arg("message");
        Serial.println("Received message: " + message);

        // Set brightness to 255 when a message is received
        amoled.setBrightness(255);

        // Set up the message label
        lv_label_set_text(messageLabel, message.c_str());
        lv_obj_align(messageLabel, LV_ALIGN_TOP_MID, 0, 0);  // Align to top for multiple lines

        messageDisplayed = true;

        server.send(200, "text/html", "Message received: " + message);
    } else {
        server.send(400, "text/html", "No message received");
    }
}
