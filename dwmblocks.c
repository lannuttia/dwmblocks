#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<X11/Xlib.h>
#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		50

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Block;
void dummysighandler(int num);
void sighandler(int num);
void buttonhandler(int sig, siginfo_t *st, void *ucontext);
void getcmds(int time);
#ifndef __OpenBSD__
void getsigcmds(int signal);
void setupsignals();
void sighandler(int signum);
#endif
int getstatus(char *str, char *last);
void setroot();
void statusloop();
void termhandler(int signum);


#include "blocks.h"

static Display *dpy;
static int screen;
static Window root;
static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static char statusstr[2][256];
static char exportstring[CMDLENGTH + 16] = "export BUTTON=-;";
static int button = 0;
static int statusContinue = 1;
static void (*writestatus) () = setroot;

//opens process *cmd and stores output in *output
void getcmd(const Block *block, char *output)
{
    if (block->signal)
    {
        output[0] = block->signal;
        output++;
    }
	strcpy(output, block->icon);
	char *cmd = block-> command;
	FILE *cmdf = popen(cmd, "r");
	if (!cmdf)
		return;
	char c;
	int i = strlen(block->icon);
	fgets(output+i, CMDLENGTH-i, cmdf);
	i = strlen(output);
	if (delim != '\0' && i)
		output[i++] = delim;
	output[i++] = '\0';
	pclose(cmdf);
}

void getcmds(int time)
{
	const Block* current;
	for(int i = 0; i < LENGTH(blocks); i++)
	{
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1)
			getcmd(current,statusbar[i]);
	}
}

#ifndef __OpenBSD__
void getsigcmds(int signal)
{
	const Block *current;
	for (int i = 0; i < LENGTH(blocks); i++)
	{
		current = blocks + i;
		if (current->signal == signal)
			getcmd(current,statusbar[i]);
	}
}

void setupsignals()
{
    struct sigaction sa;
    /* initialize all real time signals with dummy handler */
    for(int i = SIGRTMIN; i <= SIGRTMAX; i++)
        signal(i, dummysighandler);

	for(int i = 0; i < LENGTH(blocks); i++)
	{
		if (blocks[i].signal > 0)
        {
			signal(SIGRTMIN+blocks[i].signal, sighandler);
           sigaddset(&sa.sa_mask, SIGRTMIN+blocks[i].signal);
        }
	}
    sa.sa_sigaction = buttonhandler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    signal(SIGCHLD, SIG_IGN);

}
#endif

int getstatus(char *str, char *last)
{
	strcpy(last, str);
	str[0] = '\0';
	for(int i = 0; i < LENGTH(blocks); i++)
		strcat(str, statusbar[i]);
	str[strlen(str)-1] = '\0';
	return strcmp(str, last);//0 if they are the same
}

void setroot()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only set root if text has changed.
		return;
	Display *d = XOpenDisplay(NULL);
	if (d) {
		dpy = d;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XStoreName(dpy, root, statusstr[0]);
	XCloseDisplay(dpy);
}

void pstdout()
{
	if (!getstatus(statusstr[0], statusstr[1]))//Only write out if text has changed.
		return;
	printf("%s\n",statusstr[0]);
	fflush(stdout);
}


void statusloop()
{
#ifndef __OpenBSD__
	setupsignals();
#endif
	int i = 0;
	getcmds(-1);
	while(statusContinue)
	{
		getcmds(i);
		writestatus();
		sleep(1.0);
		i++;
	}
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum)
{
    return;
}
#endif

#ifndef __OpenBSD__
void sighandler(int signum)
{
	getsigcmds(signum-SIGRTMIN);
	writestatus();
}
#endif

void buttonhandler(int sig, siginfo_t *si, void *ucontext)
{
    int button = si-> si_value.sival_int & 0xff;
    sig = si-> si_value.sival_int >> 8;
    getsigcmds(sig);
    writestatus();
    if (fork() == 0)
    {
        static char exportstring[CMDLENGTH + 22] = "export BLOCK_BUTTON=-;";
        const Block *current;
        int i;
        for (i = 0; i < LENGTH(blocks); i++)
        {
            current = blocks + i;
            if (current->signal == sig)
                break;
        }
        char *cmd = strcat(exportstring, blocks[i].command);
        cmd[20] = '0' + button;
        char *command[] = { "/bin/sh", "-c", cmd, NULL };
        setsid();
        execvp(command[0], command);
        exit(EXIT_SUCCESS);
        cmd[22] = '\0';
    }
}

void termhandler(int signum)
{
	statusContinue = 0;
	exit(0);
}

int main(int argc, char** argv)
{
	for(int i = 0; i < argc; i++)
	{
		if (!strcmp("-d",argv[i]))
			delim = argv[++i][0];
		else if(!strcmp("-p",argv[i]))
			writestatus = pstdout;
	}
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
}
