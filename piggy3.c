/*
Program:
       Piggy3.c
Authors:
       Shahob Mousavi
       Isaias Mondar
Description:
       Program functions as a middle connector between a client and server end for transfering data.
       User specifies piggy as a head or tail to determine if the pig will accept information either as specified on
       command line connection, or from a file input.
       If user does not specify as a heard or tail, the pig will act as an intermidiary to connect to a rightmost
       connection and listen for incoming connections on the left side.
       Piggy3 makes use of multiple gui windows to display data moving from left to right and vice versa.
Arguments:
   Command Line Parameters
       -rraddr value:
           This is used to specify the address of the right side, i.e. the node you should connect to.
           Unlike the lraddr option “*” is never valid for an rraddr.
           Also, note that there is no default IP address for the right side.
           A rraddr must always be specified unless -noright is given.
       -noleft:
           Head. No incoming left side connection. If -noleft is given then any -lraddr option is ignored.
       -noright:
           Indicates that the pig will act as the tail connection.
           If -noright is given then any -rraddr option is ignored.
           Cannot specify both noleft and noright.
       -llport [value]:
           What local port address should be used for the left side connection. This is the port you listen on.
           Port you give to bind before you call listen.
       -rrport [value]:
           The port address you should connect to on the node being connected to on the right side.
       -rlport [value]:
           The port to bind to on the computer piggy is running on for making the right side connection.
           The source port for the right side connection.
       -loopr:
           When looping on the right side we should see the “looped” data leaving from the upper right box
           and also see it entering the lower right box.
           Take the data that would be written out to the right side and
           inject it into the data stream arriving from the right side.
       -loopl:
           When looping on the left side we should see the “looped” data leaving from the lower left box
           and also see it entering the uppper left box.
           Take the data that would be written out to the left side and
           inject it into the data stream arriving from the left side.
           */
/*
    Interactive Commands
        i: Enter insert mode. Program should start in command mode.
        esc: Exit insert mode and return to command mode. Esc entered while in command mode is ignored.
        outputl: Set the output direction to left. Determines where data typed from the keyboard in input mode is sent.
        outputr: Set the output direction to right. Determines where data typed from the keyboard in input mode is sent.
        output: Show what direction the output is set to (left of right).
        left: Show the currently connected “tcp pair” for the left side.
        right: Show the currently connected “tcp pair” for the right side.
            Command ordering
            [left] local IP:local port:remoteIP:remote port.
            [right] remote IP:remote port :local IP:local port.
        loopr: When looping on the right side we should see the “looped” data leaving from the upper right box and also see it entering the lower right box.
        loopl: When looping on the left side we should see the “looped” data leaving from the lower left box and also see it entering the uppper left box.
        dropr: Drop right side connection.
        dropl: Drop left side connection.
*       connectr IP [port]:
            Create a connection to computer with “IP” on their tcp port
            “port” for your “right side” If a port is not specified the current
            value of port for the remote port on the right is used. This may
            have been specified on the command line or may have been
            established via an interactive command. If it has never been set
            than use the default port.
*       connectl: IP [port]:
            create a connection to computer with “IP” on their tcp port
            “port” for your “left side.”
*       listenl: [port]: Use for left side listen for a connection on your local port port. Use default if no port given.
*       listenr [port]: Use for right side listen for a connection on your local port port. Use default if no port given.
        read: filename: Read the contents of file “filename” and write it to the current output direction.
*       llport [port]: Bind to local port “port” for a left side connection.
*       rlport [port]: Bind to local port “port” for a right side connection.
*       lrport [port]: Accept a connection on the left side only if the remote computer attempting to connect has source port “port”.
*       lraddr [IP]:
            When the left is put into passive mode (via a listenl command)
            accept a connection on the left side only if the remote computer
            attempting to connect has IP address “IP” If the left is placed in
            active mode (trying to connect) use this as the address to connect to.
*       rraddr [IP]:
            If the right is set to passive mode to accept a connection on the
            right side, allow it only if the remote computer attempting to
            connect has IP address “IP”. If the right is placed in active mode
            (trying to make a connection) use this as the address to connect to.
*       reset:
            Act as if the program is starting up from the beginning again and
            do whatever was requested by the command line parameters. Any
            active connections or passive opens are dropped and reset. The
            screen should be cleared and redrawn. The program
            should be in exactly the same state it was in at startup after
            processing and acting on the command line parameters.
        q: Terminate the program. When quitting the program be sure to set
           the terminal back into the state it was in before your piggy started
           using the endwin call in ncurses.
        persl: make the left side persistent connection
        persr: make the right side a persistent connection
        Structure:
            declerations
            main
            setup ncurses
            create sockets
*/

#include "functions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <zconf.h>
#include <curses.h>
#include <locale.h>
#include <arpa/inet.h>


#include <net/if.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <ctype.h>

#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
#define closesocket close

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#endif

/* For Windows OS*/
#ifdef WIN32
WSADATA wsaData;
WSAStartup(0x0101, &wsaData);
#endif


extern int errno;
char localhost[] = "localhost";         /* default host name */
char *filename = "scriptin.txt";        /* Set default definition for filename */
struct hostent *host;                   /* Pointer to a host table entry */
struct sockaddr_in left;                /* Structure to hold left address */
struct sockaddr_in right;               /* Structure to hold right address */
struct sockaddr_in lconn;               /* Structure for left passive connection */
struct sockaddr_in rconn;               /* Structure for right passive connection*/




char *output[MAXSIZE];
int inputDesignation = -1;

void update_win(int i) {
    touchwin(w[i]);
    wrefresh(sw[i]);
}

void win_clear(int win){
    wmove(sw[win], 0,0);
    wclrtoeol(sw[win]);
    update_win(win);
}

void nerror(char *str) {
    wmove(sw[ERW], 0, 0);
    wclrtoeol(sw[ERW]);
    waddstr(sw[ERW], str);
    update_win(ERW);
}


void winwrite(int win, char *str) {
    win_clear(win);
    waddstr(sw[win], str);
    update_win(win);
}


/* add string to a window */
void wAddstr(int z, char c[255]);




/* closes windows completely */
void GUIshutdown(char *response) {
    winwrite(CMW, "All finished. Press Enter to terminate the program.");
    wgetstr(sw[CMW], response);
    endwin();
    echo();
}


void winclear(int win, int y, int x){
    wmove(sw[win], y, x);
    wclrtoeol(sw[win]);
    update_win(win);
}

void reset_displays(){

    int i = 4;

    for(; i< 7; i++){
        win_clear(i);
    }

    waddstr(sw[INW], "Data Entry:");
    waddstr(sw[ERW], "Errors:");
    waddstr(sw[CMW], "Commands:");

    update_win(ERW);
    update_win(INW);
    update_win(CMW);
    
}


/* clear the windows and set all variables to default */
void resetWindows( icmd  * flags) {

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);



    /* Clear screen before starting */
    clear();
    w[0] = newwin(0, 0, 0, 0);

    int WPOS[NUMWINS][4] = {
            {16, 66,  0,  0},
            {16, 66,  0,  66},
            {16, 66,  16, 0},
            {16, 66,  16, 66},
            {3,  132, 32, 0},
            {5,  132, 35, 0},
            {3,  132, 40, 0}
    };

    for (int i = 0; i < NUMWINS; i++) {
        w[i] = newwin(WPOS[i][0], WPOS[i][1], WPOS[i][2], WPOS[i][3]);
        sw[i] = subwin(w[i], WPOS[i][0] - 2, WPOS[i][1] - 2, WPOS[i][2] + 1, WPOS[i][3] + 1);
        scrollok(sw[i], TRUE); // allows window to be automatically scrolled
        wborder(w[i], 0, 0, 0, 0, 0, 0, 0, 0);
        touchwin(w[i]);
        wrefresh(w[i]);
        wrefresh(sw[i]);
    }


    flags->noleft = 0;
    flags->noright = 0;
    bzero(flags->rraddr, sizeof(flags->rraddr));    /* Right connecting address                                           */
    bzero(flags->lraddr, sizeof(flags->rraddr));    /* Left connected address                                             */
    bzero(flags->localaddr, sizeof(flags->rraddr)); /* Local address                                                      */
    flags->llport = DEFAULT;                        /* left protocol port number                                          */
    flags->rrport = DEFAULT;                        /* right protocol port number                                         */
    flags->dsplr = 1;                               /* display left to right data, default if no display option provided  */
    flags->dsprl = 0;                               /* display right  to left data                                        */
    flags->loopr = 0;                               /* take data that comes from the left and send it back to the left    */
    flags->loopl = 0;                               /* take data that comes in from the right and send back to the right  */
    flags->output = 1;

    //flags->connectl = NULL;
    //flags->connectr = NULL;
    //flags->listenl = NULL;
    //flags->listenr = NULL;
    flags->reset = 0;

}

static struct option long_options[] ={
        {"s",       optional_argument, NULL, 'a'},
        {"noleft",  optional_argument, NULL, 'l'},
        {"noright", optional_argument, NULL, 'r'},
        {"dsplr",   optional_argument, NULL, 'd'},
        {"dsprl",   optional_argument, NULL, 'e'},
        {"loopr",   optional_argument, NULL, 'f'},
        {"loopl",   optional_argument, NULL, 'g'},
        {"persl",   optional_argument, NULL, 'i'},
        {"persr",   optional_argument, NULL, 'h'},
        {"llport",  optional_argument, NULL, 't'},
        {"rraddr",  required_argument, NULL, 'z'},
        {"lraddr",  required_argument, NULL, 'b'},
        {"rrport",  optional_argument, NULL, 'k'},
        {NULL, 0,                      NULL, 0}
};


char *twoWordCommand(char cbuf[80], icmd *flags){

    win_clear(INW);

    char delimiter[] = " ";
    char *checker = NULL;
    //char *inputCheck = cbuf;
    char *word2, *end;
    char *inputCopy;
    int inputLength;



    inputLength = strlen(cbuf);
    inputCopy = (char *) calloc(inputLength + 1, sizeof(char));

    /* inputs requiring 2nd word in command */
    const char *writtenInputs[] = {"connectl", "connectr", "listenl", "listenr", "llport", "rrport", "lrport", "rlport", "lladdr", "rraddr"};

    /* check if command is of specific set of commands which require 2nd word in command to be read, other than source */
    for(int l = 0; l < 10; l++) {
        //update_win(CMW);
        checker = strstr(cbuf, writtenInputs[l]);
        if (checker == cbuf) {
            strncpy(inputCopy, cbuf, inputLength);
            strtok_r(inputCopy, delimiter, &end);
            word2 = strtok_r(NULL, delimiter, &end);

            int inputDesignation = l;

            /* set flags for values to 2nd word in command */
            // delimiter or inputCheck probably input

            // connectl
            if (inputDesignation == 0) {

                
                strcpy(flags->connectl, word2);
                waddstr(sw[INW], "connectl ");
                wprintw(sw[INW], word2);
            }
                // connectr
            else if (inputDesignation == 1) {
                strcpy(flags->connectr, word2);
                waddstr(sw[INW], "connectr ");
                wprintw(sw[INW], word2);
            }
                //listenl
            else if (inputDesignation == 2) {
                strcpy(flags->listenl, word2);
                waddstr(sw[INW], "listenl ");
                wprintw(sw[INW], word2);
            }
                // listenr
            else if (inputDesignation == 3) {
                strcpy(flags->listenr,word2);
                waddstr(sw[INW], "listenr ");
                wprintw(sw[INW], word2);
            }
                // llport
            else if (inputDesignation == 4) {
                flags->llport = (int) *word2;
                waddstr(sw[INW], "llport ");
                wprintw(sw[INW], word2);

                // rrport
            } else if (inputDesignation == 5) {
                flags->rrport = (int) *word2;
                waddstr(sw[INW], "rrport ");
                wprintw(sw[INW], word2);
            }
                // lrport
            else if (inputDesignation == 6) {
                flags->lrport = (int) *word2;
                waddstr(sw[INW], "lrport ");
                wprintw(sw[INW], word2);
            }
                // rlport
            else if (inputDesignation == 7) {
                flags->rlport = (int) *word2;
                waddstr(sw[INW], "rlport ");
                wprintw(sw[INW], word2);
            }
                // lladdr
            else if (inputDesignation == 8) {
                strcpy(flags->lladdr, word2);
                waddstr(sw[INW], "lladdr ");
                wprintw(sw[INW], word2);
            }
                // rraddr
            else if (inputDesignation == 9) {
                strcpy(flags->rraddr, word2);
                waddstr(sw[INW], "rraddr ");
                wprintw(sw[INW], word2);
            }

            return inputCopy;
        }
    }
    return NULL;
}


/*
*Function:
*       main
*
*Description:
*       Piggybacking socket connections
*
*
*Returns:
*       
*
*/
int main(int argc, char *argv[]) {


    /***************************************************/
    /* Use input arguments from loop to set values     */
    /***************************************************/
    int i, n, x, len, ch;
    int written     = 0;
    int maxfd;                          /* Max descriptor                          */
    int pigopt;                         /* Piggy position indicating variable      */
    int indexptr;                       /* Generic ponter for getopt_long_only API */
    int descl       = -1;               /* Left accepted descriptor                */
    int descr       = -1;               /* Left accepted descriptor                */
    int parentrd    = -1;               /* Main left  descriptors                  */
    int parentld    = -1;               /* Main right descriptors                  */
    int templeft    =  0;               /* Left sending identifier descriptor      */
    int tempright   =  0;               /* Right sending identifier descriptor     */
    char errorstr[32];                  /* Error buf                               */
    char buf[MAXSIZE];                  /* Buffer for string the server sends      */
    char *output[MAXSIZE];
    char cbuf[RES_BUF_SIZE];
    unsigned char lefttype;             /* (0) passive left, else (1) active       */
    unsigned char righttype;            /* (0) passive right, else (1) active      */    
    unsigned char setup = 0;

    /***************************************************/
    /* Control flow variables                          */
    /***************************************************/
    unsigned char openrd = 0;                   /* (1) indicates open right connection, otherwise (0)*/
    unsigned char openld = 0;                   /* (1) indicates open left  connection, otherwise (0)*/
    unsigned char fowardr = 0;
    unsigned char fowardl = 0;

    /***************************************************/
    /* Remote and host information variables           */
    /***************************************************/
    struct addrinfo hints, hints2, *infoptr, *infoptr2; /* used for getting connecting right piggy if give DNS*/
    struct addrinfo *p, *p2;
    struct in_addr ip, ip2;
    struct hostent *lhost;
    char hostinfo[256];
    char hostinfo2[256];
    char hostname[256];


    /***************************************************/
    /* File Descriptor sets                            */
    /***************************************************/
    fd_set readset, masterset;


    /***************************************************/
    /* Init descriptor set                             */
    /***************************************************/
    FD_ZERO(&masterset);
    FD_SET(0, &masterset);

    /***************************************************/
    /* File related variables                          */
    /***************************************************/
    char *inputCheck = buf;
    char *checker = NULL;
    int readLines;
    int fileRequested = 0;
    char *word2, *end;
    char delimiter[] = " ";
    int readCommandLines;
    int inputLength = 0;
    char *inputCopy;


    /***************************************************/
    /* Ncurses windows variables                        */
    /***************************************************/
    int a, c;
    char response[RES_BUF_SIZE];
    int WPOS[NUMWINS][4] = {
            {16,  66,   0,   0},
            {16,  66,   0,  66},
            {16,  66,  16,   0},
            {16,  66,  16,  66},
            { 3,  132, 32,   0},
            { 5,  132, 35,   0},
            { 3,  132, 40,   0}
    };

    /***************************************************/
    /* Setup ncurses for multiple windows              */
    /***************************************************/
    setlocale(LC_ALL, ""); 		/* Use to local character set*/
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    /* Clear screen before starting */
    clear();
    w[0] = newwin(0, 0, 0, 0);

    /****************************************************/
    /* Check for correct terminal size, 132x43 required */
    /***************************************************/
    if (LINES != 43 || COLS != 132) {
        move(0, 0);
        addstr("Piggy3 requires a screen size of 132 columns and 43 rows");
        move(1, 0);
        addstr("Set screen size to 132 by 43 and try again");
        move(2, 0);
        addstr("Press enter to terminate program");
        mvprintw(3, 0, "%dx%d\n", COLS, LINES);
        refresh();
        getstr(response); // Pause so we can see the screen
        endwin();
        exit(EXIT_FAILURE);
    }


    /***************************************************/
    /* Create the 7 windows and the seven subwindows  */
    /***************************************************/
    for (i = 0; i < NUMWINS; i++) {
        w[i] = newwin(WPOS[i][0], WPOS[i][1], WPOS[i][2], WPOS[i][3]);
        sw[i] = subwin(w[i], WPOS[i][0] - 2, WPOS[i][1] - 2, WPOS[i][2] + 1, WPOS[i][3] + 1);
        scrollok(sw[i], TRUE); // allows window to be automatically scrolled
        wborder(w[i], 0, 0, 0, 0, 0, 0, 0, 0);
        touchwin(w[i]);
        wrefresh(w[i]);
        wrefresh(sw[i]);
    }


    /***********************************************/
    /* Windows                                     */
    /***********************************************/
    wmove(sw[INW], 0, 0);
    waddstr(sw[INW], "Data Entry: ");
    wmove(sw[ERW], 0, 0);
    waddstr(sw[ERW], "Errors: ");
    wmove(sw[CMW], 0, 0);
    waddstr(sw[CMW], "Commands: ");

    for (i = 4; i < NUMWINS; i++) {
        update_win(i);
    }


    /***********************************************/
    /*  Flag variables init_init                   */
    /***********************************************/
    icmd *flags;
    flags = malloc(sizeof(icmd));
    flags_init(flags);


    /*********************************/
    /*  Parsing argv[]               */
    /*********************************/
    
    while ((ch = getopt_long_only(argc, argv, "a::l::r::d::e::f::g::h::i::t::k:z:", long_options, &indexptr)) != -1) {
        switch (ch) {
            case 'a':
                /* read file */
                for (int comm = 0; argv[comm] != '\0'; comm++) {
                    if (strstr(argv[comm], ".txt") != NULL) {
                        fileRequested = 1;
                        filename = argv[comm];
                    }
                }

                if (fileRequested) {
                    readLines = fileRead(filename, output);

                    /* read from array and pass into flag function*/
                    for (x = 0; x < readLines; ++x) {
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &descl,
                                        &parentrd, right, lconn, inputDesignation, &lefttype, &righttype);

                        if (n < 0) {
                            nerror("invalid command");
                        }
                        free(output[x]);
                    }

                }
                    /* if none specified read from default filename*/
                else {
                    readLines = fileRead("scriptin.txt", output);
                    /* read from array and pass into flag function  */
                    for (x = 0; x < readLines; ++x) {
                        n = flagsfunction(flags, output[x], sizeof(buf), flags->position, &openld, &openrd, &descl,
                                        &parentrd, right, lconn, inputDesignation, &lefttype, &righttype);

                        if (n < 0) {
                            nerror("invalid command");
                        }
                        free(output[x]);
                    }
                }
                break;
            case 'l':
                openld = 0;
                flags->noleft = 1;
                flags->setupr = flags->setupr + 10;
                winwrite(CMW, "no left");
                                    
                break;

            case 'r':
                openrd = 0;
                flags->noright = 1;
                flags->setupl = flags->setupl + 10;
                winwrite(CMW, "no right ");
                
                break;
            case 'd':
                flags->dsplr = 2;
                /* No change, default is dsplr*/
                break;

            case 'e':
                flags->dsprl = 1;
                winwrite(CMW,  "dsprl ");
                break;

            case 'f':
                flags->loopr = 1;
                flags->output = 1;                
                winwrite(CMW, "loopr ");                
                break;

            case 'g':
                flags->loopl = 1;
                flags->output = 0;
                winwrite(CMW,  "loopl ");                
                break;

            case 'i':
                flags->persl = 1;
                winwrite(CMW, "persl ");                
                break;

            case 'h':
                flags->persr = 1;
                winwrite(CMW,  "persr ");
                break;
            
            /*llport*/
            case 't':
                if (number(argv[optind]) > 0) {
                    flags->llport = atoi(argv[optind]);
                } else {
                    nerror(" left port not a number");
                    GUIshutdown(response);
                    return -1;
                }
                if (flags->llport < 0 || flags->llport > 88889) {
                    nerror(" left port number out of range");
                    GUIshutdown(response);
                    return -1;
                }
                break;

            /*rrport*/
            case 'k':
                if (number(argv[optind - 1]) > 0) {
                    flags->rrport = atoi(argv[optind]);
                } else {
                    nerror(" right port not a number ");
                    GUIshutdown(response);
                    return -1;
                }
                if (flags->rrport < 0 || flags->rrport > 88889) {
                    nerror(" right port number out of range");
                    GUIshutdown(response);
                    return -1;
                }
                /* test for illegal value */
                break;

            case 'z':
                strncpy(flags->rraddr, argv[optind - 1], sizeof(flags->rraddr));
                hints.ai_family = AF_INET;
                n = getaddrinfo(flags->rraddr, NULL, NULL, &infoptr);

                if (n != 0) {
                    nerror(" rraddr error");
                    GUIshutdown(response);
                    return -1;
                }

                for (p = infoptr; p != NULL; p = p->ai_next) {
                    getnameinfo(p->ai_addr, p->ai_addrlen, hostinfo, sizeof(hostinfo), NULL, 0, NI_NUMERICHOST);
                    strcpy(flags->rraddr, hostinfo);
                }
                flags->setupr = flags->setupr +20;
                freeaddrinfo(infoptr);
                break;
                
            case 'b':
                
                strncpy(flags->lraddr, argv[optind - 1], sizeof(flags->lraddr));
                hints2.ai_family = AF_INET;
                n = getaddrinfo(flags->lraddr, NULL, NULL, &infoptr2);

                if (n != 0) {
                    nerror(" rraddr error");
                    GUIshutdown(response);
                    return -1;
                }

                for (p2 = infoptr; p2 != NULL; p2 = p2->ai_next) {
                    getnameinfo(p2->ai_addr, p2->ai_addrlen, hostinfo2, sizeof(hostinfo2), NULL, 0, NI_NUMERICHOST);
                    strcpy(flags->lraddr, hostinfo2);
                }
                
                flags->setupl = flags->setupl+ 20;
                freeaddrinfo(infoptr2);
                break;
            case '?':
                nerror("No valid command");
                GUIshutdown(response);
                return -1;
        }
    }
    /* end switch statement */



    /*********************************/
    /*    Getting local IP address   */
    /*********************************/
    if (gethostname(hostname, sizeof(hostname)) < 0) {
        nerror("gethostname, local machine error");
        GUIshutdown(response);
        return -1;
    }

    lhost = gethostbyname(hostname);
    if (lhost == NULL) {
        nerror("gethostbyname, local machine error");
        GUIshutdown(response);
        return -1;
    }

    ip = *(struct in_addr *) lhost->h_addr_list[0];
    strcpy(flags->localaddr, inet_ntoa(ip));




    /*********************************/
    /*  Left descriptors setup */
    /*********************************/
    
    switch (flags->setupl) {
        case 0:
            openld = 0;
            break;
        /* Passive Left */
        case (10):
            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->llport, NULL, left, NULL);

            if (parentld < 0) {
                nerror("left passive socket_init");
                GUIshutdown(response);
                exit(1);
            }
            
            openld = 1;            
            lefttype = 0;            
            FD_SET(parentld, &masterset);

            break;
            
        /* Active Left */
        case (30):
            
            pigopt = 2;
            parentld = sock_init(pigopt, 0, flags->lrport, flags->lraddr, left, host);
            if (parentrd < 0) {
                nerror("socket_init");
                GUIshutdown(response);
                exit(1);
            }
            openld = 1;
            lefttype  = 1;
            FD_SET(parentld, &masterset);
            
            break;

        
        default:        
            winwrite(ERW, "Invalid options for left descriptor");
            GUIshutdown(response);            
            return -1;
    }
    /* End Left descriptor setup */
    
    
    /*********************************/
    /*  Right descriptors setup      */
    /*********************************/   
    
    
    switch (flags->setupr) {
        
        case 0:
            openrd = 0;
            break;
        /* Passive Right */
        case (10):
            pigopt = 1;
            parentld = sock_init(pigopt, QLEN, flags->rrport, NULL, right, NULL);

            if (parentld < 0) {
                nerror("Right passive socket_init");
                GUIshutdown(response);
                exit(1);
            }
            
            openrd = 1;
            righttype = 0;
            FD_SET(parentrd, &masterset);
            break;
            
        /* Active Right */
        case (30):
            
            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

            if (parentrd < 0) {
                nerror("socket_init");
                GUIshutdown(response);
                exit(1);
            }

            openrd = 1;
            righttype = 1;
            FD_SET(parentrd, &masterset);
            
            break;

        
        default:                   
            wprintw(sw[ERW], "%d", flags->setupr);
            update_win(ERW);
           // winwrite(ERW, "Invalid options for right descriptor");
            GUIshutdown(response);            
            return -1;            
    }    
    /* End Right descriptor setup */    
    
    
    /***************************************************/
    /* Init windows cursor postion                     */
    /***************************************************/
    yul = 0; xul = 0;                           /* Top left window position variables           */
    yur = 0; xur = 0;                           /* Top right window position variables          */
    ybl = 0; xbl = 0;                           /* Bottom left window position variables        */
    ybr = 0; xbr = 0;                           /* Bottom right window position variables       */
    ycm = 0; xcm = 0;                           /* Command window position variables            */
    getyx(sw[ULW], yul, xul);
    getyx(sw[URW], yur, xur);
    getyx(sw[BLW], ybl, xbl);
    getyx(sw[BRW], ybr, xbr);
    getyx(sw[CMW], ycm, xcm);
    

    /************************************************************/
    /************************************************************/
    /*                          SELECT                          */
    /*                                                          */
    /*  Main loop performing network interaction (@tag s3l)     */
    /*                                                          */
    /************************************************************/
    /************************************************************/


    /*  maxfd == parentld or maxfd == parentrd               */
    maxfd = max(parentld, parentrd);

    while (1) {
        memcpy(&readset, &masterset, sizeof(masterset));

        /**/
        n = select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (n < 0) {
            nerror("select error < 0");
        }

        if (n == 0) {
            nerror("select error == 0");
        }

                
        /*****************************************************************/
        /* Standard in descriptor ready (@tag:  0FD)                     */
        /*****************************************************************/

        /* Notes:
        * 
        *
        *
        *
        * 
        */
        if (FD_ISSET(0, &readset)) {
            
            noecho();
            win_clear(CMW);
            c = wgetch(sw[CMW]);
            bzero(buf, sizeof(buf));
            
            switch (c) {                
                /*******************************/
                /* Insert mode                 */
                /*******************************/
                case 105:
                    if (openrd || openld) {
                        win_clear(URW);
                        win_clear(BLW);
                        win_clear(INW);
                        winwrite(INW, "Insert: ");
                        echo();

                        tempright = descr > 0 ? descr : parentrd; 
                        templeft  = descl > 0 ? descl : parentld;
                        
                        
                        while (1) {
                            c = wgetch(sw[INW]);

                            if (c != 27) {

                                wclrtoeol(sw[INW]);                                
                                buf[0] = c;
                                buf[1] = '\0';
                                echo();

                                /* Requires further work to ignore characters not in printable range */
                                if(c == 8){
                                    noecho();
                                    nocbreak();
                                    delch();
                                    delch();
                                    cbreak();
                                    refresh();
                                }



                                /* Preconditions for sending data to the right, output == 1 */
                                /* Data should be displayed in URW window*/
                                /* `w */
                                if (flags->output && openrd){
                                    n = send(tempright, buf, sizeof(buf), 0);


                                    if (n <= 0) {
                                        openrd = 0;
                                        nerror("right send error");
                                        
                                        if( righttype & flags->persr){
                                            flags->reconr = 1;
                                        }                                                                                                                
                                    }

                                    /*  */
                                    if(c == 13){

                                        yur++;
                                        xur = 0;
                                        wmove(sw[URW], yur, xur);
                                        wrefresh(sw[URW]);
                                        /**BOOKMARK, ODD BEHAVIOR WITH REMOTE RECV DATA DISPLAY**/
                                        /**COMMENT FOR NOW**/
                                        //scroll(sw[ULW]);
                                    }
                                    wprintw(sw[URW], buf);
                                    update_win(URW);
                                }
                                    /* Preconditions for sending data to the left, output == 0 */
                                else if (!flags->output && openld) {
                                    n = send(templeft, buf, sizeof(buf), 0);

                                    if (n <= 0) {
                                        openld = 0;
                                        nerror("left send error ");
                                        
                                        if( lefttype & flags->persl){
                                            flags->reconl = 1;
                                        }
                                    }
                                    
                                    /*  */
                                    if(c == 13){
                                        ybl++;
                                        xbl = 0;
                                        wmove(sw[BLW], ybl, xbl);
                                        wrefresh(sw[BLW]);
                                        /**BOOKMARK, ODD BEHAVIOR WITH REMOTE RECV DATA DISPLAY**/
                                        /**COMMENT FOR NOW**/
                                        //scroll(sw[ULW]);
                                    }
                                    wprintw(sw[BLW], buf);
                                    update_win(BLW);
                                }                                
                            }
                            /* ESCAPE key has been hit*/
                            else {
                                /**WE COULD reset the local windows to starting state**/
                                /**Display window modes**/
                                noecho();                            
                                break;
                            }
                        }/* End input loop*/
                    }/* End of at least one socket is open*/
                    else {
                        nerror("no open sockets");                        
                    }
                    break;

                    /*******************************/
                    /*  Quitting                   */
                    /*******************************/
                case 113:
                    bzero(buf, sizeof(buf));
                    winwrite(CMW, "Exiting");
                    GUIshutdown(response);
                    endwin();

                    return 1;

                    /*******************************/
                    /* Interactive commands        */
                    /*******************************/
                default:
                    bzero(cbuf, sizeof(cbuf));

                    /*`1*/
                    i = 0;
                    winwrite(CMW, ":");
                    echo();
                    nocbreak;


                    if( c >31 && c < 127  || c ==8){
                        waddch(sw[CMW], c);
                    }

                    update_win(CMW);

                    while (1) {

                        /* clear input window before new command requested */
                        wclrtoeol(sw[INW]);
                        update_win(INW);

                        /* Needs to be updated, requires addition conditional variable */
                        if(c == 8){
                            delch();
                            delch();
                        }

                        if( c >31 && c < 127){
                            cbuf[i]= (char) c;
                            i++;
                            wclrtoeol(sw[CMW]);
                            update_win(CMW);
                        }

                            /* Enter has been hit*/
                        else if(c == 13){
                            /* PROCESS COMMAND HERE, USER COMMAND STORED IN CBUF*/

                            winwrite(CMW, cbuf);
                            break;
                        }
                        else{
                            noecho();
                            win_clear(CMW);
                            winwrite(CMW, "not a printable character");
                            break;
                        }
                        c = wgetch(sw[CMW]);
                    }
                    /*End accepting commands */

                    noecho();
                    if(i == RES_BUF_SIZE){
                        winwrite(ERW, "buffer full");
                    }
                    
                    win_clear(CMW);
                    noecho();

/**WARNING: REMEMBER TO INSERT I.C. FUNCTIONALITY HERE**/


                    /* two word commands */
                    char *commandWord1 = twoWordCommand(cbuf, flags);

                    inputLength = strlen(cbuf);
                    inputCopy = (char *) calloc(inputLength + 1, sizeof(char));
                    checker = strstr(cbuf, "source");

                    /* check for file input commands */
                    if(checker == cbuf) {
                        strncpy(inputCopy, cbuf, inputLength);
                        strtok_r(inputCopy, delimiter, &end);
                        word2 = strtok_r(NULL, delimiter, &end);

                        /* Get commands from fileread*/
                        readCommandLines = fileRead(word2, output);

                        /* Read from array and pass into flagfunction */
                        for (x = 0; x < readCommandLines; ++x) {
                            waddstr(w[4], output[x]);
                            update_win(4);
                            n = flagsfunction(flags, output[x], sizeof(cbuf), flags->position, &openld, &openrd,
                                              &descl, &parentrd, lconn, right, inputDesignation, &lefttype, &righttype);

                        }

                    /* check for other two word commands */
                    }else if(commandWord1 != NULL){
                        n = flagsfunction(flags, commandWord1, sizeof(cbuf), flags->position, &openld, &openrd, &descl,
                                          &parentrd, right, lconn, inputDesignation, &lefttype, &righttype);
                    }else{
                        n = flagsfunction(flags, cbuf, sizeof(cbuf), flags->position, &openld, &openrd, &descl,
                                          &parentrd, right, lconn, inputDesignation, &lefttype, &righttype);
                    }


                    /* process commmands */

                    if(flags->reset == 1) {

                        resetWindows(flags);
                        break;
                    } else {

                        switch (n) {
                            /*
                            *  valid command
                            */
                            case 1:
                                break;

                            /*
                            *  persl
                            */
                            case 2:
                                if(lefttype){
                                    flags->persl = 1;
                                }else{
                                    nerror("Can't set passive left persl");
                                }
                                bzero(cbuf, sizeof(cbuf));
                                break;

                            /*
                            *  persr
                            */
                            case 3:
                                if(righttype){
                                    flags->persr = 1;                                
                                }
                                else{
                                    nerror("Can't set passive right persr");
                                }
                                break;

                            /*
                            *  dropl
                            */
                            case 4:
                                
                                
                                if (parentld > 0) {
                                    openld = 0;
                                    shutdown(parentld, 2);
                                    FD_CLR(parentld, &masterset);
                                }else if( descl > 0){
                                    openld = 0;
                                    shutdown(descl, 2);
                                    FD_CLR(descl, &masterset);                                    
                                }else{
                                    nerror("Invalid left side command ");
                                }
                                
                                break;

                            /*
                            *  dropr
                            */
                            case 5:
                                
                                if (parentrd > 0) {
                                    openrd = 0;
                                    shutdown(parentrd, 2);
                                    FD_CLR(parentrd, &masterset);
                                }else if( descr > 0){
                                    openrd = 0;
                                    shutdown(descr, 2);
                                    FD_CLR(descr, &masterset);                                    
                                }else{
                                    nerror("Invalid right side command");
                                }
                                break;

                            default:
                                nerror("invalid command");
                                wgetstr(sw[CMW], response);

                        }/* End switch case*/

                    }/* end else */

            }
            /**END MAIN 0-FD SWITCH**/

            reset_displays();
        }/*End stdin */


        /*****************************************************************/
        /* Left Side Accept (Find @tag: LSA)                             */
        /*****************************************************************/
        /*Notes:
        *   Passive left socket, descl will be set
        *   lefttype == 0
        * 
        * 
        * 
        */
        if (!lefttype & FD_ISSET(parentld, &readset)) {
            len = sizeof(lconn);
            descl = accept(parentld, (struct sockaddr *) &lconn, &len);
            if (descl < 0) {
                nerror("left accept error");
                endwin();
                return -1;
            }
            
            openld = 1;
            flags->llport = (int) ntohs(lconn.sin_port);
            strcpy(flags->lladdr, inet_ntoa(lconn.sin_addr));
            maxfd = max(maxfd, descl);
            FD_SET(descl, &masterset);
            reset_displays();
            winwrite(CMW, "Left passive connection established");
        }


        /*****************************************************************/
        /* Right Side Accept (Find @tag: RSA)                             */
        /*****************************************************************/
        /*Notes:
        *   Passive right socket, descr will be set
        *   righttype == 0
        * 
        * 
        * 
        */
        if (!righttype & FD_ISSET(parentrd, &readset)) {
            len = sizeof(rconn);
            descr = accept(parentrd, (struct sockaddr *) &rconn, &len);
            if (descr < 0) {
                nerror("left accept error");
                endwin();
                return -1;
            }

            openrd = 0;
            flags->rrport = (int) ntohs(rconn.sin_port);
            strcpy(flags->rraddr, inet_ntoa(rconn.sin_addr));
            maxfd = max(maxfd, descr);
            FD_SET(descr, &masterset);
            reset_displays();
            winwrite(CMW, "Right passive connection established");
        }




        /*****************************************************************/
        /* Left Listeing Descriptor (find @tag: LLD)                     */
        /*****************************************************************/


        /* Notes:
        *    Descl only set with passive left
        * 
        */
        if (FD_ISSET(descl, &readset)) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, descl, buffer, flags} **/

            if(righttype){                
                sockettype(buf, &righttype, &openld, &openrd, &descl, &parentrd, flags, &masterset, LEFT);
            }
            else{
                sockettype(buf, &righttype, &openld, &openrd, &descl, &descr, flags, &masterset, LEFT);            
            }
            reset_displays();
        }


        /*****************************************************************/
        /* Right Listeing Descriptor (find @tag: RLD)                    */
        /*****************************************************************/


        /* Notes:
        *   Descr only set with passive right
        */
        if (FD_ISSET(descr, &readset)) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, descl, buffer, flags} **/


            if( lefttype){
              sockettype(buf, &lefttype, &openld, &openrd, &descr, &parentld, flags, &masterset, RIGHT);
            }
            else{
                sockettype(buf, &lefttype, &openld, &openrd, &descr, &descl, flags, &masterset, RIGHT);
            }
            reset_displays();
        }

        /*****************************************************************/
        /* RIGHT ACTIVE DESCRIPTROR (find @tag: RAD)                     */
        /*****************************************************************/

        /*
        *
        * Notes:
        *   Active right socket, use parentrd
        * 
        * 
        */
        if (righttype & FD_ISSET(parentrd, &readset) ) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, parentrd, buffer, flags} **/


            /*Case: active left*/
            if(lefttype){
                sockettype(buf, &lefttype, &openld, &openrd, &parentrd, &parentld, flags, &masterset, RIGHT);
            }
            
            /*Case: passive left*/
            else{
                sockettype(buf, &lefttype, &openld, &openrd, &parentrd, &descl, flags, &masterset, RIGHT);
            }
            reset_displays();
        }/* End ready parentrd*/



        /*****************************************************************/
        /* LEFT ACTIVE DESCRIPTROR (find @tag: LAD)                       */
        /*****************************************************************/

        /*
        *
        * Notes:
        *   
        *   
        *   
        */
        if (lefttype & FD_ISSET(parentld, &readset) ) {
            /** REQUIRED VARIABLES = { masterset, opendld, openrd, parentrd, buffer, flags} **/


            /*Case: active left*/
            if(righttype){
                sockettype(buf, &righttype, &openld, &openrd, &parentld, &parentrd, flags, &masterset, LEFT);
            }
                /*Case: passive left*/
            else{
                sockettype(buf, &righttype, &openld, &openrd, &parentld, &descr, flags, &masterset, LEFT);
            }
            reset_displays();
        }/* End ready parentrd*/



        /******************************************/
        /* LEFT & RIGHT RECONNECTION DONE HERE    */
        /******************************************/

        /*
        *Notes:
        *
        *
        *
        *        
        */

        if (flags->reconr) {
            winwrite(CMW, "right side reconnecting ");

            pigopt = 2;
            parentrd = sock_init(pigopt, 0, flags->rrport, flags->rraddr, right, host);

            if (parentrd > 0) {
                flags->reconr = 0;
                openrd = 1;
                
                maxfd = max(maxfd, parentld);                
                maxfd = max(maxfd, descl);
                
                FD_SET(parentrd, &masterset);
            }
            reset_displays();
        }
        
        if (flags->reconl) {
            winwrite(CMW, "left side reconnecting ");

            pigopt = 2;
            parentld = sock_init(pigopt, 0, flags->rlport, flags->lladdr, left, host);

            if (parentrd > 0) {
                flags->reconl = 0;
                openld = 1;
                                                
                maxfd = max(maxfd, parentrd);                          
                maxfd = max(maxfd, descr);
                
                FD_SET(parentld, &masterset);
            }
            reset_displays();
        }        
    }
    /***************************************************/
    /**END SELECT LOOP                                **/
    /***************************************************/

    /* End screen updating */
    endwin();
    echo();

    /**WARNING: THESE TEST CASES MUST BE CHANGED ONCE WE GENERALIZED THE DESCRIPTORS BEHAVIOR**/
    if( parentld> 0){
        shutdown(parentld, 1);
    }
    if( descl > 0){
        shutdown(descl, 2);
    }
    if(descr > 0){
        shutdown(descr, 2);
    }

    if( parentrd> 0){
        shutdown(parentrd, 2);
    }
    
    GUIshutdown(response);
    return 0;    
}/*end main*/