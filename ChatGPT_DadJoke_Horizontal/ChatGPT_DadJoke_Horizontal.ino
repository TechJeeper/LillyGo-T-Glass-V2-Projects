#include <LilyGo_Wristband.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// The resolution of the non-magnified side of the glasses reflection area is about 126x126,
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

// Wi-Fi credentials
const char* ssid = "";            // Your Wi-Fi SSID
const char* password = "";   // Your Wi-Fi password

// OpenAI API key
const char* openai_api_key = ""; // Your OpenAI API key

// Scrolling text configuration
int scroll_speed = 20;               // Scrolling speed in pixels per second
const lv_font_t* text_font = &lv_font_montserrat_12; // Font size for the text
int num_repeats = 3;                 // Number of times the text will repeat scrolling

LilyGo_Class amoled;
lv_obj_t *scroll_label;

void setup() {
    // Turn on debugging message output
    Serial.begin(115200);

    // Initialize screen and peripherals
    bool rslt = amoled.begin();
    if (!rslt) {
        while (1) {
            Serial.println("The board model cannot be detected, please raise the Core Debug Level to an error");
            delay(1000);
        }
    }

    // Set the bracelet screen orientation to portrait
    amoled.setRotation(0);

    // Brightness range: 0 ~ 255
    amoled.setBrightness(255);

    // Set the default touch threshold
    amoled.setTouchThreshold(200);

    // Initialize lvgl
    beginLvglHelper(amoled);

    // Set display background color to black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // Create a display window object
    lv_obj_t *window = lv_obj_create(lv_scr_act());
    // Set window background color to black
    lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create scroll label
    scroll_label = lv_label_create(window);
    lv_label_set_long_mode(scroll_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(scroll_label, GlassViewableWidth * 2);
    lv_obj_align(scroll_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(scroll_label, text_font, LV_PART_MAIN); // Adjustable text size
    lv_obj_set_style_text_color(scroll_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(scroll_label, "Hello World"); // Initial text to confirm display works

    // Set touch button callback function
    amoled.setEventCallback(button_event_callback);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

void button_event_callback(ButtonState state) {
    if (state == BTN_CLICK_EVENT) {
        Serial.println("Button clicked");
        lv_label_set_text(scroll_label, "Fetching joke...");

        // Fetch and display a random dad joke
        String joke = getDadJoke();
        Serial.println("Dad Joke: " + joke);

        // Display the joke on the screen with scrolling effect
        lv_label_set_text(scroll_label, joke.c_str());
        lv_obj_align(scroll_label, LV_ALIGN_CENTER, 0, 0);
        
        // Set the scrolling animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, scroll_label);
        lv_anim_set_exec_cb(&a, [](void* var, int32_t v) {
            lv_obj_set_x((lv_obj_t*)var, v);
        });
        lv_anim_set_values(&a, lv_obj_get_width(lv_scr_act()), -lv_obj_get_width(scroll_label));
        lv_anim_set_time(&a, (lv_obj_get_width(scroll_label) * 1000) / scroll_speed);
        lv_anim_set_repeat_count(&a, num_repeats);
        lv_anim_start(&a);
    }
}

String getDadJoke() {
    HTTPClient http;
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(openai_api_key));

    StaticJsonDocument<256> doc;
    doc["model"] = "gpt-3.5-turbo";
    doc["messages"][0]["role"] = "system";
    doc["messages"][0]["content"] = "You are a friendly assistant.";
    doc["messages"][1]["role"] = "user";
    doc["messages"][1]["content"] = "Tell me a short dad joke with only the joke and punchline.";

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("Response: " + response);  // Debug line to print the response
        StaticJsonDocument<1024> responseDoc;
        DeserializationError error = deserializeJson(responseDoc, response);

        if (!error) {
            String joke = responseDoc["choices"][0]["message"]["content"];
            joke.trim();
            http.end();
            return joke;
        } else {
            Serial.println("Failed to parse response");
        }
    } else {
        Serial.println("HTTP request failed, response code: " + String(httpResponseCode));
    }

    http.end();
    return "Failed to fetch joke";
}

void loop() {
    // Check button state
    amoled.update();
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(5);
}
