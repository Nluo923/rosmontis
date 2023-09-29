/*
AP CS Principles Create Task 2023
Topography Simulation

Written in C++ 21, compiled with g++.
This project uses the OpenGL graphics library to display graphics.
The outside wrapper library GLFW (by the GLFW development team) is 
used to more easily set up windows and input.

CONTROLS (keyboard)
r    | regenerate without changing seeds (corner values)
s    | reseed and then regenerate
c    | toggle colorscheme (natural, heat)
-, = (+) | change variance values.  

I use a list, "heightmap", which is a 2d array.

Procedure "update_heightmap" and its substeps "diamond_step" and 
"square_step" demonstrate sequencing, iteration, and selection.

The program outputs to a window to display graphics.
*/

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include <string>
#include <iostream>
#include <math.h>

//length needs to be of form 2^n + 1
const int length = 65;

const int left_bound = 0;
const int right_bound = 16;

int heightmap[length][length];
int variance = 8;

const float scale = 10.0f;
const float padding = 10.0f;
const int windowSize = 900;

typedef struct Color {
  int r;
  int g;
  int b;
} Color;

typedef struct Colorset {
  Color color1;
  Color color2;
} Colorset;

const Colorset colorset1 = {Color{255,64,64}, Color{64,255,64}};

const Colorset color_snow = {Color{200,200,200}, Color{255,255,255}};
const Colorset color_mountain = {Color{100,80,51}, Color{141,111,68}};
const Colorset color_grass = {Color{158,194,77}, Color{84,180,77}};
const Colorset color_ocean = {Color{159,197,232}, Color{61,133,198}};

bool colorscheme_heat = false;

bool in_bounds(int x, int y) {
  return x >= 0 && x < length && y >= 0 && y < length;
}

/*Set initial values of heightmap*/
void seed_heightmap(int left_bound, int right_bound) {
  heightmap[0][0] = left_bound + rand() % (right_bound-left_bound+1); 
  heightmap[0][length-1] = left_bound + rand() % (right_bound-left_bound+1); 
  heightmap[length-1][0] = left_bound + rand() % (right_bound-left_bound+1); 
  heightmap[length-1][length-1] = left_bound + rand() % (right_bound-left_bound+1);
}

Color lerp_color(int a, Colorset colorset, int left_bound, int right_bound) {
  float x = (float) (right_bound - a) / (float)(right_bound-left_bound);
  Color out;
  out.r = (colorset.color2.r - colorset.color1.r) * x + colorset.color1.r;
  out.g = (colorset.color2.g - colorset.color1.g) * x + colorset.color1.g;
  out.b = (colorset.color2.b - colorset.color1.b) * x + colorset.color1.b;

  return out;
}

Color lerp_color(int a, Colorset colorset) {
  float x = (float) std::min(std::max(a,left_bound),right_bound) / (float)(right_bound-left_bound);
  Color out;
  out.r = (colorset.color2.r - colorset.color1.r) * x + colorset.color1.r;
  out.g = (colorset.color2.g - colorset.color1.g) * x + colorset.color1.g;
  out.b = (colorset.color2.b - colorset.color1.b) * x + colorset.color1.b;

  return out;
}

//get a color that looks like actual land (snow, mountains, grass, water)
Color natural_color(int a) {
  if(a >= 16) return lerp_color(a,color_snow,16,20);
  else if(a >= 10) return lerp_color(a,color_mountain,10,15);
  else if(a >= 2) return lerp_color(a,color_grass,2,9);
  else return lerp_color(a,color_ocean,-8,1);
}

/*Return float between -1.0 and 1.0, for OpenGL coordinates, considering that y goes positive for up */
float normalize(int x, bool flip) {
  float scalar = (2.0f - (padding / (float) windowSize));
  float out = (flip ? -1 : 1) * (scalar * (((float) x + padding) / (float) windowSize) - 1.0f);
  return out;
}

void display() {
  glPointSize(scale);
  glBegin(GL_POINTS);

  //draw current palette
  for(int i=-8; i<=20; i++) {
    Color c = (colorscheme_heat) ? lerp_color(i, colorset1) : natural_color(i);
    glColor3ub(c.r,c.g,c.b);
    
    glVertex2f(0.9f, normalize(scale*(i+8), true));
  }

  //draw the actual heightmap
  for(int y=0; y<length; y++) {
    for(int x=0; x<length; x++) {
      Color c = (colorscheme_heat) ? lerp_color(heightmap[x][y],colorset1) : natural_color(heightmap[x][y]); // get interpolated color from the height
      glColor3ub(c.r,c.g,c.b);

      glVertex2f(normalize(scale*x, false), normalize(scale*y, true));
    }
  }

  glEnd();
}

const int diag_dir[4][2] = {
  {-1,-1}, //ul
  {1,-1},  //ur
  {-1,1},  //ll
  {1,1}    //lr
};

/* Set this cell at (x,y) to be the average of the 4 cells that are "dist" units diagonally away */
void square_step(int x, int y, int dist, int variance) { 
  if(!in_bounds(x,y)) return;
  
  int sum = 0;
  int valid = 0;

  int offset = -variance + rand() % (2*variance+1); // ranges from [-v, v]

  for(auto &d : diag_dir) { // for each diagonal...
    if(in_bounds(x+dist*d[0], y+dist*d[1])) { // check if it is in bounds
      sum += heightmap[x+dist*d[0]][y+dist*d[1]]; // if so add it to a running sum (later to be an average)
      valid++; 
    }
  }

  int avg = sum / valid + offset;
  heightmap[x][y] = avg;
}

const int ortho_dir[4][2] = {
  {-1,0}, //l
  {0,-1}, //u
  {1,0},  //r
  {0,1}   //d
};

/* Set this cell at (x,y) to be the average of the 4 cells that are "dist" units orthogonally away */
void diamond_step(int x, int y, int dist, int variance) { 
  if(!in_bounds(x,y)) return;

  int sum = 0;
  int valid = 0;

  int offset = -variance + rand() % (2*variance+1); // ranges from [-v, v]

  for(auto &d : ortho_dir) { // for each orthogonal direction...
    if(in_bounds(x+dist*d[0], y+dist*d[1])) { // check if it is in bounds
      sum += heightmap[x+dist*d[0]][y+dist*d[1]]; // if so add it to a running sum (later to be an average)
      valid++; 
    }
  }

  int avg = sum / valid + offset;
  heightmap[x][y] = avg;
}

void update_heightmap() {
  int chunk_size = length - 1;
  int cur_variance = variance; // "randomness" that is sprinkled in the heightmap, halved each iteration

  while (chunk_size > 1) {
    int chunk_half = chunk_size / 2;

    for(int y=0; y<length-1; y+=chunk_size) {
      for(int x=0; x<length-1; x+=chunk_size) {
        // a square of length "chunk_size" with top left corner on x,y 
        square_step(x+chunk_half, y+chunk_half, chunk_half, cur_variance);      
      }
    }

    for(int y=0; y<length; y+=chunk_half) {
      for(int x=(y+chunk_half)%chunk_size; x<length; x+=chunk_size) {
        // a diamond centered on x,y that doesn't repeat over already set values
        diamond_step(x, y, chunk_half, cur_variance);
      }
    }

    chunk_size /= 2; // half it
    if(cur_variance > 1) cur_variance /= 2; // variance needs to taper down to prevent jaggedness
  }
}

int main(void) {
  // seed random
  srand((unsigned)time(NULL));
  
  GLFWwindow *window;

  /* Initialize glfw */
  if (!glfwInit())
    return -1;

  /* Create a window */
  window = glfwCreateWindow(500, 500, "TopSim", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  bool needs_update = true;
  bool needs_reseed = true;

  // to stop key repeat, detect change in press>release before registering another input
  int prev_state_r = GLFW_RELEASE;
  int prev_state_s = GLFW_RELEASE;
  int prev_state_plus = GLFW_RELEASE;
  int prev_state_minus = GLFW_RELEASE;
  int prev_state_c = GLFW_RELEASE;

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window)) {
    //input
    {
    //regenerate map without changing corner "seeds"
    int state_r = glfwGetKey(window, GLFW_KEY_R);
    if(state_r == GLFW_PRESS && prev_state_r == GLFW_RELEASE) { prev_state_r = GLFW_PRESS; needs_update = true; }
    prev_state_r = state_r;

    //reseed and regenerate
    int state_s = glfwGetKey(window, GLFW_KEY_S);
    if(state_s == GLFW_PRESS && prev_state_s == GLFW_RELEASE) { prev_state_s = GLFW_PRESS; needs_reseed = true; needs_update = true; }
    prev_state_s = state_s;

    //alter the variance (plus is shift+equal on the keyboard)
    int state_plus = glfwGetKey(window, GLFW_KEY_EQUAL);
    int state_minus = glfwGetKey(window, GLFW_KEY_MINUS);
    if(state_plus == GLFW_PRESS && prev_state_plus == GLFW_RELEASE) { prev_state_plus = GLFW_PRESS; if(variance < right_bound) variance++;}
    if(state_minus == GLFW_PRESS && prev_state_minus == GLFW_RELEASE) { prev_state_minus = GLFW_PRESS; if(variance > 0) variance--;}
    prev_state_plus = state_plus;
    prev_state_minus = state_minus;

    //toggle colorscheme
    int state_c = glfwGetKey(window, GLFW_KEY_C);
    if(state_c == GLFW_PRESS && prev_state_c == GLFW_RELEASE) { prev_state_c = GLFW_PRESS; colorscheme_heat = !colorscheme_heat; }
    prev_state_c = state_c;
    }

    std::string title = "TopSim - variance = " + std::to_string(variance);
    glfwSetWindowTitle(window, title.c_str());

    /* Seed the heightmap in four corners with random values */
    if(needs_reseed) {
      seed_heightmap(left_bound, right_bound);
      needs_reseed = false;
    }

    /* Recalculate the heightmap, if there are changes to it. */
    if(needs_update) {
      update_heightmap();
    }

    /* Render here */ 

    glClear(GL_COLOR_BUFFER_BIT);
    display();

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();

    if(needs_update) needs_update = false;
  }

  glfwTerminate();
  return 0;
}
