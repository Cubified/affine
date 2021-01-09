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

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

enum {
  MODE_VIEW,
  MODE_AFFINE
};

unsigned char *data;
int iw, ih, in;

void stop(){
  struct termios raw;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= (ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  printf("\033c\033[0;0H\033[?25h");

  stbi_image_free(data);

  exit(0);
}

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
         height, tilt = 0;
  int i_dest, inc,
      ix, iy;
  char c;
  unsigned char *out;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

  w = (double)iw;
  h = (double)ih;
  n = (double)in;

  x0 = -w/2;
  y0 = h/2;

  height = 3;

  do {
    if(c == 'q' || c == 27) { stop(); }
    
    if(c == 'w') { y0 += 1; }
    if(c == 's') { y0 -= 1; }
    if(c == 'a') { theta += 0.01; }
    if(c == 'd') { theta -= 0.01; }
    
    if(c == 'e') { tilt += 5; }
    if(c == 'c') { tilt -= 5; }

    if(c == 'r') { height += 1; }
    if(c == 'v') { height -= 1; }

    printf("\033[0;0H");

    if(mode == MODE_VIEW){
      for(y=0;y<h;y++){
        for(x=0;x<w;x++){
          printf("\033[48;2;%i;%i;%im ", data[(int)round((y*w*n)+(x*n))], data[(int)round((y*w*n)+(x*n)+1)], data[(int)round((y*w*n)+(x*n)+2)]);
        }
      }
    } else if(mode == MODE_AFFINE){
      out = malloc(iw*ih*in);

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
             yprime >= 0 && yprime <= h){
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
          if(iy < ws.ws_row/2){
            printf("\033[48;2;0;0;0m ");
          } else {
            printf("\033[48;2;%i;%i;%im ", out[(iy*iw*in)+(ix*in)], out[(iy*iw*in)+(ix*in)+1], out[(iy*iw*in)+(ix*in)+2]);
          }
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
