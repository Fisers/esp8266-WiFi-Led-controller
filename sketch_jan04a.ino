#include <ArduinoJson.h>
#include "FS.h"

#define FASTLED_ALLOW_INTERRUPTS 0
#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <FastLED.h>
FASTLED_USING_NAMESPACE


#define MILLI_AMPS         3000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
int FRAMES_PER_SECOND = 30;
#define DATA_PIN      D1
#define LED_TYPE      WS2812B
#define COLOR_ORDER   RGB
int NUM_LEDS = 88;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

CRGB* leds;


#include "GradientPalettes.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];

uint8_t gCurrentPaletteNumber = 0;
uint8_t currentPatternIndex = 0;
uint8_t currentPaletteIndex = 0;
uint8_t speed = 30;
uint8_t cooling = 49;
uint8_t sparking = 60;

CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);
CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );
CRGB solidColor = CRGB::Blue;

typedef struct {
  CRGBPalette16 palette;
  String name;
} PaletteAndName;
typedef PaletteAndName PaletteAndNameList[];

const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};

const uint8_t paletteCount = ARRAY_SIZE(palettes);

const String paletteNames[paletteCount] = {
  "Rainbow",
  "Rainbow Stripe",
  "Cloud",
  "Lava",
  "Ocean",
  "Forest",
  "Party",
  "Heat",
};

String ssid; 
String password; 

unsigned int port = 2390;

char packetBuffer[255];
char ReplyBuffer[] = "acknowledged";

WiFiUDP Udp;

const int DATAPIN = 14;

int r = 0;
int b = 0;
int g = 0;

int timeout;

int brightness = 255;
int pulseBrightness = 0;
int minBrightness = 0;
int fadeAmount = 2;
int pulseSpeed = 20;

void setup()
{
 pinMode(14, OUTPUT);

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
 
  if (!loadConfig()) {
    Serial.println("Failed to load config");
  } else {
    Serial.println("Config loaded");
  }
  if (leds != 0) {
    delete [] leds;
  }
leds = new CRGB [NUM_LEDS];


  // begin serial and connect to WiFi
  Serial.begin(115200);
  delay(100);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);         // for WS2812 (Neopixel)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  gTargetPalette = gGradientPalettes[2];

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(getssid());
  Serial.print("Password:");
  Serial.println(getpass());

  timeout = 0;
  WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    IPAddress ip(192,168,8,150); 
    IPAddress gateway(192,168,8,1);   
    IPAddress subnet(255,255,255,0);   
    WiFi.config(ip, gateway, subnet);
    WiFi.begin("Dimanti", "");
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("");
    Serial.println("Config loaded, contents:");
    Serial.println(NUM_LEDS);
    Serial.println(FRAMES_PER_SECOND);
    Serial.println(ssid);
    Serial.println(password);
    Serial.println("----------------------------------------");
  
    Udp.begin(port);
}

typedef void (*SimplePatternList)();
SimplePatternList patterns[] = {
  pride,
  colorWaves,
  rainbow,
  rainbowWithGlitter,
  rainbowSolid,
  confetti,
  sinelon,
  bpm,
  juggle,
  fire,
  water,
  showSolidColor
};
const uint8_t patternCount = ARRAY_SIZE(patterns);

bool pulsate = false;

bool TurnedOn = false;
bool runOnce = false;
bool changed = true;

uint8_t secondsPerPalette = 10;
int value = 0;
String pattern = "";
void loop()
{
  
  // Patterns
  if(TurnedOn == true) {
    patterns[currentPatternIndex]();
    if(pulsate == true) {
      pulse();
    }
    EVERY_N_SECONDS( secondsPerPalette ) {
      gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
      gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
    }
  
    EVERY_N_MILLISECONDS(40) {
      // slowly blend the current palette to the next
      nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 8);
      gHue++;  // slowly cycle the "base color" through the rainbow
    }
  }
  ////////////////
  int packetSize = Udp.parsePacket();
  if(packetSize) {
    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }    
  
    if(String(packetBuffer) == "On") {
      if(pattern == "") {
        fill_solid(leds, NUM_LEDS, CRGB(r/4.011764705882353, g/4.011764705882353, b/4.011764705882353));
      }
      FastLED.show();
      TurnedOn = true;
      replyToPacket();
      Serial.println("ON");
    }
    else if(String(packetBuffer) == "Off") {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      TurnedOn = false;
      replyToPacket();
      Serial.println("OFF");
    }
    else if(String(packetBuffer) == "Rst") {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      replyToPacket();
      ESP.restart();
    }
    else {
        // TEMP parse data from packet
        char * temp;
    
        temp = strtok (packetBuffer,":");
        if(String(temp) == "Led")
        {
          temp = strtok (NULL,":");
          if(String(temp) != "") {
            fill_solid(leds, NUM_LEDS, CRGB(0, 0, CRGB::Black));
            FastLED.show();
            
            NUM_LEDS = atoi(temp);
            if (!saveConfig()) {
              Serial.println("Failed to save config");
            } else {
              Serial.println("Config saved");
            }
            
            delete [] leds;
            leds = new CRGB [NUM_LEDS];
          }
        }
        else if(String(temp) == "fps")
        {
          temp = strtok (NULL,":");
          if(String(temp) != "") {
            FRAMES_PER_SECOND = atoi(temp);
            if (!saveConfig()) {
              Serial.println("Failed to save config");
            } else {
              Serial.println("Config saved");
            }
          }
        }
        else if(String(temp) == "ssid")
        {
          temp = strtok (NULL,":");
          if(String(temp) != "") {
            ssid = temp;
            if (!saveConfig()) {
              Serial.println("Failed to save config");
            } else {
              changed = true;
              Serial.println("Config saved");
            }
          }
        }
        else if(String(temp) == "pass")
        {
          temp = strtok (NULL,":");
          if(String(temp) != "") {
            password = temp;
            if (!saveConfig()) {
              Serial.println("Failed to save config");
            } else {
              changed = true;
              Serial.println("Config saved");
            }
          }
        }
        else if(String(temp) == "pat")
        {
          temp = strtok (NULL,":");
          if(String(temp) != "") {
            currentPatternIndex = atoi(temp);
          }
        }
        else if(String(temp) == "pal")
        {
          temp = strtok (NULL,":");
          if(String(temp) != "") {
            currentPaletteIndex = atoi(temp);
          }
        }
        else if(String(temp) == "puls")
        {
          temp = strtok (NULL,":");
          if(String(temp) == "on") {
            pulsate = true;
          }
          if(String(temp) == "off") {
            pulsate = false;
          }
        }
        else if(String(temp) == "pSpeed")
        {
          temp = strtok (NULL,":");
          pulseSpeed = atoi(temp)/2;
        }
        else if(String(temp) == "bright")
        {
          temp = strtok (NULL,":");
          FastLED.setBrightness(atoi(temp));
          brightness = atoi(temp);
        }
        else if(String(temp) == "minbright")
        {
          temp = strtok (NULL,":");
          minBrightness = atoi(temp);
        }
        else
        {
          if(isInt(temp) == 0) return; 
          r = atoi(temp);
      
          if(temp != NULL){
            temp = strtok (NULL,":");
            if(isInt(temp) == 0) return; 
            g = atoi(temp);
          }
      
          if(temp != NULL){
            temp = strtok (NULL,":");
            if(isInt(temp) == 0) return; 
            b = atoi(temp);
          }
  
        if(TurnedOn == true) {
          setSolidColor(r, g, b);
        }
       }
        // send a reply, to the IP address and port
        // that sent us the packet we received
        replyToPacket();
    }
  }

  if (TurnedOn == false) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    digitalWrite(14, LOW);
    // FastLED.delay(15);
    return;
  }
  
  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  digitalWrite(14, HIGH);
}

void replyToPacket() {
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }


bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  NUM_LEDS = json["NUM_LED"];
  FRAMES_PER_SECOND = json["FRAMES_PER_SECOND"];
  const char* tempSSID = json["SSID"];
  const char* tempPass = json["PASSWORD"];
  json["SSID"].printTo(ssid);
  int lenght = ssid.length();
  ssid.remove(lenght-1);
  ssid.remove(0,1);
  json["PASSWORD"].printTo(password);
  lenght = password.length();
  password.remove(lenght-1);
  password.remove(0,1);

  return true;
}

const char* getssid() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  return json["SSID"];
}
const char* getpass() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  return json["PASSWORD"];
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["NUM_LED"] = NUM_LEDS;
  json["FRAMES_PER_SECOND"] = FRAMES_PER_SECOND;
  json["SSID"] = ssid;
  json["PASSWORD"] = password;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}

void heatMap(CRGBPalette16 palette, bool up)
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Add entropy to random number generator;
  random16_add_entropy(random(256));

  // Array of temperature readings at each simulation cell
  static byte heat[256];

  byte colorindex;

  // Step 1.  Cool down every cell a little
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((cooling * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( uint16_t k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < sparking ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( uint16_t j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    colorindex = scale8(heat[j], 190);

    CRGB color = ColorFromPalette(palette, colorindex);

    if (up) {
      leds[j] = color;
    }
    else {
      leds[(NUM_LEDS - 1) - j] = color;
    }
  }
}

void addGlitter( uint8_t chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void pride()
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend( leds[pixelnumber], newcolor, 64);
  }
}

void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 5);
}

void rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void rainbowSolid()
{
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  // leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds[pos] += ColorFromPalette(palettes[currentPaletteIndex], gHue + random8(64));
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(speed, 0, NUM_LEDS);
  static int prevpos = 0;
  CRGB color = ColorFromPalette(palettes[currentPaletteIndex], gHue, 255);
  if ( pos < prevpos ) {
    fill_solid( leds + pos, (prevpos - pos) + 1, color);
  } else {
    fill_solid( leds + prevpos, (pos - prevpos) + 1, color);
  }
  prevpos = pos;
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t beat = beatsin8( speed, 64, 255);
  CRGBPalette16 palette = palettes[currentPaletteIndex];
  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

void juggle()
{
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for ( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }
}

void fire()
{
  heatMap(HeatColors_p, true);
}

void water()
{
  heatMap(IceColors_p, false);
}

void colorWaves()
{
  colorwaves( leds, NUM_LEDS, gCurrentPalette);
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds - 1) - pixelnumber;

    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

void setSolidColor(uint8_t r, uint8_t g, uint8_t b)
{
  solidColor = CRGB(r, g, b);
  FastLED.setBrightness(255);
  brightness = 255;
  currentPatternIndex = patternCount - 1;
}

void showSolidColor()
{
  fill_solid(leds, NUM_LEDS, solidColor);
}

void pulse()
{
  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    leds[i].fadeLightBy(pulseBrightness);
  }
  FastLED.show();
  pulseBrightness = pulseBrightness + fadeAmount;
  // reverse the direction of the fading at the ends of the fade:
  if((pulseBrightness >= (254-minBrightness) && fadeAmount > 0) || (pulseBrightness <= (255-brightness) && fadeAmount < 0))
  {
    fadeAmount = -fadeAmount ;
  }   
  delay(pulseSpeed);
}

boolean isInt(String tString) { 
  for(int x=0;x<tString.length();x++)
  {   
    if(tString.charAt(x) < '0' || tString.charAt(x) > '9') return false;
  }
  return true;
}
