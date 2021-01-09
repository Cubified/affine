/*
 * affine.c: SNES-style mode 7 in the terminal
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <math.h>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define OVERFLOW(a, min, max, val) (a > max ? val : (a < min ? val : a))
#define GET_PIXEL(arr, x, y, w, h, n) \
  arr[OVERFLOW((int)floor((y*w*n)+(x*n)+0), 0, w*h*n, 0)], \
  arr[OVERFLOW((int)floor((y*w*n)+(x*n)+1), 0, w*h*n, 0)], \
  arr[OVERFLOW((int)floor((y*w*n)+(x*n)+2), 0, w*h*n, 0)]

#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)

enum {
  MODE_VIEW,
  MODE_AFFINE
};

unsigned char *data, *out;
int iw, ih, in;

void stop(){
  struct termios raw;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= (ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  printf("\033c\033[0;0H\033[?25h");

  stbi_image_free(data);
  free(out);

  exit(0);
}

/* Fairly unoptimized and ugly (especially mode7) */
void loop(int mode){
  struct winsize ws;
  double x0, y0,
         w, h, n,
         x, y, z,
         i, view_angle,
         xtemp, ytemp,
         xprime, yprime,
         theta = M_PI,
         horizon = h/2,
         height = 15,
         tilt = 0,
         zoom = 1,
         velocity = 0,
         accel = 0.1,
         decel = 0.1,
         max_vel = 10,
         turn_velocity = 0,
         turn_accel = M_PI / 2048,
         turn_decel = M_PI / 2048,
         turn_max_vel = M_PI / 128;
  int i_dest, inc,
      ix, iy;
  char c;

  w = (double)iw;
  h = (double)ih;
  n = (double)in;

  if(mode == MODE_VIEW){
    x0 = 0;
    y0 = 0;
  } else if(mode == MODE_AFFINE){
    x0 = -w/2;
    y0 = h/2;
  }

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

  do {
    if(c == 'q') { stop(); }
    if(c == '\033'){
      if(getchar() == 91){
        switch(getchar()){
          case 65:
            y0 -= 10;
            break;
          case 66:
            y0 += 10;
            break;
          case 67:
            x0 += 10;
            break;
          case 68:
            x0 -= 10;
            break;
        }
      }
    }

    printf("\033[0;0H");

    if(mode == MODE_VIEW){
      if(c == '+') { zoom += 0.1; }
      if(c == '-') { zoom -= 0.1; }

      for(y=0;y<ws.ws_row;y++){
        for(x=0;x<ws.ws_col;x++){
          printf(
            "\033[48;2;%i;%i;%im ",
            GET_PIXEL(data, floor((((x/ws.ws_col)*w)+x0)/zoom), floor((((y/ws.ws_row)*h)+y0)/zoom), w, h, n)
          );
        }
      }
    } else if(mode == MODE_AFFINE){
      if(c == 'w') { velocity = MIN(velocity+accel, max_vel); }
      else if(c == 's') { velocity = MAX(velocity-accel, -max_vel); }
      else if(velocity < 0) { velocity = MIN(velocity+decel, 0); }
      else { velocity = MAX(velocity-decel, 0); }

      x0 += velocity * sin(theta);
      y0 += velocity * cos(theta);

      if(c == 'a') { turn_velocity = MIN(turn_velocity+turn_accel, turn_max_vel); }
      else if(c == 'd') { turn_velocity = MAX(turn_velocity-turn_accel, -turn_max_vel); }
      else if(turn_velocity < 0) { turn_velocity = MIN(turn_velocity+turn_decel, 0); }
      else { turn_velocity = MAX(turn_velocity-turn_decel, 0); }
      
      theta += turn_velocity;

      if(c == 'e') { tilt += 5; }
      if(c == 'c') { tilt -= 5; }

      if(c == 'r') { height += 1; }
      if(c == 'v') { height -= 1; }

      for(i=w*n*horizon;i<w*h*n;i+=n){
        y = floor(i/(n*w));
        if(y >= horizon){
          x = floor(fmod((i/n),w))-(w/2);
          z = y/height;
          view_angle = y-(h/2)+tilt;

          if(view_angle == 0) { view_angle = 1; }

          xtemp = (x/(z*view_angle))*(w/2);
          ytemp = (height/view_angle)*(h/2);

          xprime = floor((xtemp * cos(theta)) - (ytemp * sin(theta)) - x0);
          yprime = floor((xtemp * sin(theta)) + (ytemp * cos(theta)) + y0);

          if(xprime >= 0 && xprime <= w &&
             yprime >= 0 && yprime <= h &&
             xtemp >= -w/2 && ytemp >= 0){
            i_dest = ((yprime * w) + xprime) * n;

            if(i_dest < w*h*n){
              for(inc=0;inc<(int)n;inc++){
                out[(int)(i+inc)] = data[(int)(i_dest+inc)];
              }
            }
          }
        }
      }

      for(iy=0;iy<ws.ws_row;iy++){
        for(ix=0;ix<ws.ws_col;ix++){
          printf("\033[48;2;%i;%i;%im ", GET_PIXEL(out, floor(((double)ix/ws.ws_col)*w), floor(((double)iy/ws.ws_row)*h), iw, ih, in));
        }
      }
    }
  } while((c=getchar()));
}

void init(char *file){
  struct termios raw;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  data = stbi_load(file, &iw, &ih, &in, 0);
  if(data == NULL){
    stop();
  }

  printf("\033[?25l");

  out = malloc(iw*ih*in);
}

int main(int argc, char **argv){
  int mode;

  signal(SIGINT, stop);
  signal(SIGTERM, stop);

  if(argc < 2){
    printf("Usage: affine [image] [mode]\n");
    return 0;
  }

  if(argc < 3){
    mode = MODE_VIEW;
  } else switch(argv[2][0]){ /* This compiles correctly, and I'm astounded */
    case 'v':
      mode = MODE_VIEW;
      break;
    case 'a':
      mode = MODE_AFFINE;
      break;
  }

  init(argv[1]);
  loop(mode);
  stop();
  return 0;
}
