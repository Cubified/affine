/*
 * affine.c: An image viewer for the terminal
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <inttypes.h>
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
  arr[OVERFLOW((int)((y*w*n)+(x*n)+0), 0, w*h*n, 0)], \
  arr[OVERFLOW((int)((y*w*n)+(x*n)+1), 0, w*h*n, 0)], \
  arr[OVERFLOW((int)((y*w*n)+(x*n)+2), 0, w*h*n, 0)]

#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)

enum {
  MODE_VIEW = 10,
  MODE_AFFINE = 0
};

/* Global variables -- several of these
 *   are double-use for the two different
 *   modes, but the variable names should
 *   still be relatively self-explanatory
 */
struct winsize ws;
char c;
unsigned char *data, *out;
int ix, iy, iw, ih, in,
    i_dest, inc, mode;
double posx, posy,
       w, h, n,
       x, y, z,
       i, view_angle,
       xtemp, ytemp,
       xprime, yprime,
       theta = M_PI,
       horizon = 0,
       viewheight = 100,
       tilt = 100,
       zoom = 1,
       velocity = 0,
       accel = 0.3,
       decel = 0.2,
       max_vel = 10,
       turn_velocity = 0,
       turn_accel = M_PI / 1024,
       turn_decel = M_PI / 2048,
       turn_max_vel = M_PI / 128;

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

/* Non-blocking keyboard IO */
void keyb(){
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = mode;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  if(FD_ISSET(0, &fds)){
    read(STDIN_FILENO, &c, 1);
  } else {
    c = '\0';
  }
  
  if(c == 'q') { stop(); }

  /* View mode-specific keybinds */
  if(c == '\033'){
    if(getchar() == 91){
      switch(getchar()){
        case 65:
          posy -= 10;
          break;
        case 66:
          posy += 10;
          break;
        case 67:
          posx += 10;
          break;
        case 68:
          posx -= 10;
          break;
      }
    }
  }

  if(c == '+') { zoom += 0.1; }
  if(c == '-') { zoom -= 0.1; }

  /* Affine mode-specific keybinds */
  if(c == 'w') { velocity = MIN(velocity+accel, max_vel); }
  else if(c == 's') { velocity = MAX(velocity-accel, -max_vel); }
  else if(velocity < 0) { velocity = MIN(velocity+decel, 0); }
  else { velocity = MAX(velocity-decel, 0); }

  if(c == 'a') { turn_velocity = MIN(turn_velocity+turn_accel, turn_max_vel); }
  else if(c == 'd') { turn_velocity = MAX(turn_velocity-turn_accel, -turn_max_vel); }
  else if(turn_velocity < 0) { turn_velocity = MIN(turn_velocity+turn_decel, 0); }
  else { turn_velocity = MAX(turn_velocity-turn_decel, 0); }

  if(c == 'c') { tilt += 5; }
  if(c == 'e') { tilt -= 5; }

  if(c == 'r') { viewheight += 1; }
  if(c == 'v') { viewheight -= 1; }
}

void view(){
  printf("\033[0;0H");
  for(y=0;y<ws.ws_row;y++){
    for(x=0;x<ws.ws_col;x++){
      printf(
        "\033[48;2;%i;%i;%im ",
        GET_PIXEL(data, floor((((x/ws.ws_col)*w)+posx)/zoom), floor((((y/ws.ws_row)*h)+posy)/zoom), w, h, n)
      );
    }
  }
}

void affine(){
  posx += velocity * sin(theta);
  posy += velocity * cos(theta);
  
  theta += turn_velocity;

  for(i=w*n*horizon;i<w*h*n;i+=n){
    y = floor(i/(n*w));
    if(y >= horizon){
      x = floor(fmod((i/n),w))-(w/2);
      z = y/viewheight;
      view_angle = y-(h/2)+tilt;

      if(view_angle == 0) { view_angle = 1; }

      xtemp = (x/(z*view_angle))*(w/2);
      ytemp = (viewheight/view_angle)*(h/2);

      xprime = floor((xtemp * cos(theta)) - (ytemp * sin(theta)) - posx);
      yprime = floor((xtemp * sin(theta)) + (ytemp * cos(theta)) + posy);

      if(xprime >= 0 && xprime <= w &&
         yprime >= 0 && yprime <= h &&
         xtemp >= -w/2 && ytemp >= 0){
        i_dest = ((yprime * w) + xprime) * n;

        if(i_dest < w*h*n){
          out[(int)(i+0)] = data[(int)(i_dest+0)];
          out[(int)(i+1)] = data[(int)(i_dest+1)];
          out[(int)(i+2)] = data[(int)(i_dest+2)];
        }
      }
    }
  }

  printf("\033[0;0H");
  for(iy=0;iy<ws.ws_row;iy++){
    for(ix=0;ix<ws.ws_col;ix++){
      printf("\033[48;2;%i;%i;%im ", GET_PIXEL(out, floor(((double)ix/ws.ws_col)*w), floor(((double)iy/ws.ws_row)*h), iw, ih, in));
    }
  }
}

void loop(int mode){
  w = (double)iw;
  h = (double)ih;
  n = (double)in;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

  if(mode == MODE_VIEW){
    posx = 0;
    posy = 0;

    while(1){
      view();
      keyb();
    }
  } else if(mode == MODE_AFFINE){
    posx = -w/2;
    posy = h;

    while(1){
      affine();
      keyb();
    }
  }
}

void init(char *file){
  struct termios raw;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  data = stbi_load(file, &iw, &ih, &in, 0);
  if(data == NULL){
    printf("Invalid image file \"%s\"\n", file);
    stop();
  }

  printf("\033[?25l");

  out = malloc(iw*ih*3);
}

void rsze(){
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

  if(mode == MODE_VIEW) { view(); }
}

int main(int argc, char **argv){
  signal(SIGINT, stop);
  signal(SIGTERM, stop);
  signal(SIGWINCH, rsze);

  if(argc < 2){
    printf("Usage: affine [image] [mode]\n");
    return 0;
  }

  if(argc < 3){
    mode = MODE_VIEW;
  } else switch(argv[2][0]){
    case 'v':
      mode = MODE_VIEW;
      break;
    case 'a':
      mode = MODE_AFFINE;
      break;
    default:
      printf("Unrecognized mode \"%s,\" available modes are \"view\" and \"affine.\"\n", argv[2]);
      return 1;
  }

  init(argv[1]);
  loop(mode);
  stop();
  return 0;
}
