//C.Moore for CCHS, modified H.Lloyd
//Demo reel and stress test for the overhead RGB lighting system
//Dev board:
//  WEMOS D1 R32 (ESP32)
//This is not perfect, but it's good enough!
//Supply voltage/current can be monitored with TrashPower

#include <FastLED.h>
#include <serial_io.h>
#include <iostream>

FASTLED_USING_NAMESPACE

#define DATA_PIN2    26
#define DATA_PIN3    25
#define DATA_PIN4    17
#define DATA_PIN5    16
#define DATA_PIN6    27
#define DATA_PIN7    14

#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    12
#define NUM_LINES    3
CRGB leds[NUM_LINES][NUM_LEDS];

#define BRIGHTNESS         255  //100% brightness (led colour of white fades along legnth of strip)
//#define BRIGHTNESS         191  //75% brightness (led colour still fades to red a little)
//#define BRIGHTNESS         127  //50% brightness, results in best uniformity of white when all illuminated

#define FRAMES_PER_SECOND  25   //Must be a whole integer factor of 1000 (ms), eg: 100/50/25/20

void setup() {
  delay(2000);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN2,COLOR_ORDER>(leds[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN3,COLOR_ORDER>(leds[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN4,COLOR_ORDER>(leds[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN5,COLOR_ORDER>(leds[3], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN6,COLOR_ORDER>(leds[4], NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN7,COLOR_ORDER>(leds[5], NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  randomSeed(50);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { bivar_trig, max_white_seq, rainbow_seq_lines, };
//SimplePatternList gPatterns = { bivar_trig};
//SimplePatternList gPatterns = { black_white_flash_rand };
//SimplePatternList gPatterns = { synthwave_highway };
//SimplePatternList gPatterns = { colour_pinwheel };
SimplePatternList gPatterns = { gaussian_circles };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
long unsigned millis_prev = 0;
long unsigned millis_curr = 0;

bool first_run=true;


void loop()
{
  bool update_display = false;

  int update_period_ms = 1000/FRAMES_PER_SECOND;
  //Detect millis() rollover
  millis_prev = millis_curr;
  millis_curr = millis();
  if(millis_curr != millis_prev){
    //millis rollover, check if we need to update on this event:
    if(millis_curr % update_period_ms == 0){
      //Set flag
      update_display = true;
    }
  }
  
  if(update_display){
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();

    // send the 'leds' array out to the actual LED strip
    FastLED.show(); 
  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 5 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 60 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
  fill_solid(leds[0], NUM_LEDS*NUM_LINES, 0);
  first_run = true;
}

//Validates:
// correct order of the lines
// full functionality of each LED in RGB
// voltage drop along length of strip resulting in colour drift (orange)
void max_white_seq() 
{
  static int line;
  EVERY_N_SECONDS( 1 ){ ++line; }
  if(line >= NUM_LINES){ line = 0; }
  if(first_run){
    line = 0;
    first_run = false;
  }
  // FastLED's built-in rainbow generator
  if(line < NUM_LINES ){
    fill_solid(leds[line], NUM_LEDS, CRGB::White);
  }
}

//Validates that pattern is being sent
void rainbow_seq_lines() 
{
  static int line;
  EVERY_N_SECONDS( 1 ){ ++line; }
  if(line >= NUM_LINES){ line = 0; }
  if(first_run){
    line = 0;
    first_run = false;
  }
  // FastLED's built-in rainbow generator
  if(line < NUM_LINES ){
    fill_rainbow( leds[line], NUM_LEDS, gHue, 7);
  }
}

//This function is nonsense really, it just demonstrates how to use trig functions and how to include the time variable
void bivar_trig(){
  for(int x = 0; x < NUM_LINES; ++x){
    for(int y = 0; y < NUM_LEDS; ++y){
      //leds[x][y].r = (1+sin(M_2_PI*(millis_curr % 1000)/1000.0)*round(float(x)/float(NUM_LINES-1)))*127; 
      //leds[x][y].g = (1+cos(M_2_PI*(millis_curr % 1000)/1000.0)*round(float(y)/float(NUM_LEDS -1)))*127; 
      //leds[x][y].b = 0;

      int periods = 6;
      leds[x][y].r = round((1+sin(2.0*M_PI*(float(x)/float(NUM_LINES)) + (millis_curr / 2000.0)))*64.0);
      leds[x][y].g = round((1+sin(2.0*M_PI*(float(y)/float(NUM_LEDS)*periods) + float(x)*(millis_curr / 500.0)))*127.0);
      leds[x][y].b = round((1+cos(2.0*M_PI*(float(y)/float(NUM_LEDS)*5) - float(x)*(millis_curr / 500.0)))*127.0);
    }
  }

}

//This makes the centre of the space the centre of a wheel of colour in which the hue shifts around the circle
void colour_pinwheel(){

  float x_mid = float(NUM_LINES)/2.0;
  float y_mid = float(NUM_LEDS)/2.0;

  for(int x = 0; x < NUM_LINES; ++x){
    for(int y = 0; y < NUM_LEDS; ++y){ 

      //Cartesian to polar conversion with normalisation
      float a = (float(x)+0.5-x_mid)/x_mid;
      float b = (float(y)+0.5-y_mid)/y_mid;
      float mag = sqrt(a*a+b*b);
      float arg = atan(b/a);

      //Limit extent of circle to less than or equal unity magnitude
      if (mag > 1.0){
        mag = 0.0;
      }
      float brightness = round(mag*255.0);  //Scale up for fastled
      uint8_t hue = round(((M_PI/2.0 + arg)/M_PI)*255);
      hue += gHue; //For time-based rotation, uint8_t truncates most significant bits of operation

      leds[x][y] = CHSV( hue, 255, brightness );
    }
  }
}

//////////
//create struct to store xys
struct Position {
  float x; // X-coordinate
  float y; // Y-coordinate

  Position(float xVal = 0, float yVal = 0) : x(xVal), y(yVal) {} // Constructor
};

struct Gaussian{
  int height=255;
  float mean=20.;
  float deviation=15.;
};

struct Circle{
  float x_pos=0.;
  float y_pos=0.;
  float x_vel=1.;
  float y_vel=1.;
};

//create array:
Position positions[NUM_LINES][NUM_LEDS]; // 2D array of Position structs

//store positional geometric starting information
float x_offsets[6]={0.,12.5,12.5*2};//.,60.,77.5,90.};
float y_offsets[6]={0.,0.,0.};//,30.,20.,10.};
float LED_pitch = 32.5;

// define stage limits
int xmin=-140;
int xmax=140;
int ymin=-50;
int ymax=350;

// Initialize objects at global scope.
Gaussian g0{255,100,20};
Circle c0{10,150,2.5,1};

void update_circle_pos(){
  c0.x_pos+=c0.x_vel;
  c0.y_pos+=c0.y_vel;
  if (c0.x_pos>=xmax || c0.x_pos<=xmin){
    c0.x_vel*=-1;
  }
  if (c0.y_pos>=ymax || c0.y_pos<=ymin){
    c0.y_vel*=-1;
  }
}

//fill array with position data by calculations using starting information
void initializePositions() {
  for (int i = 0; i < NUM_LINES; ++i) {
    for (int j = 0; j < NUM_LEDS; ++j) {
        positions[i][j] = Position(x_offsets[i], j*LED_pitch+y_offsets[i]); // Example initialization
    }
  }
}

void printPositions() {
    for (int i = 0; i < NUM_LINES; ++i) {
        for (int j = 0; j < NUM_LEDS; ++j) {
            Position pos = positions[i][j];
            std::cout << "Position[" << i << "][" << j << "] = ("
                      << pos.x << ", " << pos.y << ")\n";
        }
    }
}

void gaussian_circles() {
  if (first_run==true){ 
    initializePositions();
    first_run=false;
    Serial.begin(115200);  // Replace 115200 with your desired baud rate
    Serial.println("ESP32 Serial Communication Initialized");
    //printPositions();
    // Serial.print("Test access of circle \n");
    Serial.println(c0.x_pos);
    // Serial.print(" End test \n");
  }
  
  //loop over each LED
  for(int x = 0; x < NUM_LINES; ++x){
    for(int y = 0; y < NUM_LEDS; ++y){ 
      // pull out the xy positional info for that pixel
      Position pix=positions[x][y];
      float x_diff; x_diff=pix.x-c0.x_pos;
      float y_diff; y_diff=pix.y-c0.y_pos;
      // find the distance that pixel is from the circle
      float dist=sqrt(pow(x_diff,2)+pow(y_diff,2));
      // Serial.print(dist);
      // Serial.print("\t");
      // use distance to evalue the output or height of a tunable gasusian curve
      uint8_t output=round(g0.height*exp(-0.5*pow((dist-g0.mean)/g0.deviation,2)));
      
      // Serial.print(output);
      // Serial.print("\t");

      // There are all sorts of colour maps that could be imagined here: using a red to start
      // uint8_t hue = output;
      uint8_t hue = 0;
      hue += gHue; //For time-based rotation, uint8_t truncates most significant bits of operation

      leds[x][y] = CHSV( hue, 255, output);
    }
    // Serial.println();
  }
  // Serial.println();
  update_circle_pos();
}

