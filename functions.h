#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <zconf.h>
#include <curses.h>
#include <locale.h>
#include <arpa/inet.h>

/* Window defintion constants*/
#define ULW 0 /* upper left window      */
#define URW 1 /* upper right window     */
#define BLW 2 /* bottom left window     */
#define BRW 3 /* bottom right window    */
#define CMW 4 /* command window  */
#define INW 5 /* inputs window   */
#define ERW 6 /* errors window     */



#define DEFAULT 36795 /* default protocol port number, booknumber */
#define QLEN 6          /* size of request queue */
#define MAXSIZE 8
#define NUMWINS 7
#define RES_BUF_SIZE 80
#define NULLCONNECTION 10;


#define DROPL "REMOTE-LEFT-DROP"
#define PERSL "REMOTE-LEFT-CONN"
#define LEFT  "left"
#define RIGHT "right"


WINDOW *w[NUMWINS];
WINDOW *sw[NUMWINS];
WINDOW wh[NUMWINS];


struct hostent;
struct sockaddr_in;

typedef struct flags{

    int llport;                 /* Left local port, left side lisenting port    */
    int rrport;                 /* Remote right port                            */
    int lrport;                 /* Left accepting conditional port              */
    int rlport;                 /* Right side listening port                    */


    
    unsigned char dsplr;
    unsigned char dsprl;   
    unsigned char dropr;
    unsigned char dropl;
    unsigned char persl;
    unsigned char persr;
    unsigned char loopr;
    unsigned char loopl;
    unsigned char reset;    
    unsigned char reconl;
    unsigned char reconr;
    unsigned char noleft;
    unsigned char output; /* output left (0), output right (1)*/    
    unsigned char setupl;
    unsigned char setupr;    
    unsigned char noright;    
    unsigned char display;
    unsigned char position;
    
    char lraddr    [64];
    char rraddr    [64];
    char lladdr    [64];
    char localaddr [64];
    char source    [64]; /* Source file*/    
    char connectl [64];
    char connectr [64];
    char listenl  [64];
    char listenr  [64];
}icmd;

/***************************************************/
/* Windows cursor postions                          */
/***************************************************/
short yul, xul;                         /* Top left window position variables            */
short yur, xur;                         /* Top right window position variables           */
short ybl, xbl;                         /* Bottom left window position variables         */
short ybr, xbr;                         /* Bottom right window position variables        */
short ycm, xcm;                         /* Command window position variables             */
short yiw, xiw;                         /* I/O window position variables                 */


int number(char*);

int max(int, int);


void nerror(char *);

char *strdup(const char *);

char fileRead(const char *, char *[]);

void flags_init(icmd *);

int sock_init(int, int, int, char *, struct sockaddr_in , struct hostent *);

void sockettype(char *, unsigned char*, unsigned char *, unsigned char *, int *, int *, icmd * , fd_set *, char *);

int flagsfunction(icmd *, char *, int , int, unsigned char *, unsigned char *, int *, int *, struct sockaddr_in, struct sockaddr_in, int , unsigned char *, unsigned char *);


#endif