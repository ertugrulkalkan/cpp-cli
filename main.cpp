#include "CLI.h"

#include <sys/ioctl.h>
#include <termio.h>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <thread>

int _kbhit()
{
    static const int STDIN = 0;
    static int initialized = 0;

    if (!initialized)
    {
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    int bytes_waiting = 0;
    ioctl(STDIN, FIONREAD, &bytes_waiting);
    return bytes_waiting;
}

int main()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    set_cli_size(w.ws_col, w.ws_row);
    CLI *cli = get_cli();
    cli->initMap();

    int count = 0;
    int real_cnt = 0;
    char msg[100];
    while(true)
    {
        if(_kbhit())
        {
            char c = getchar();
            cli->key_pressed(c);
        }
        if(count % 1000 == 0)
        {
            sprintf(msg, "Hello %d", real_cnt++);
            cli->log(msg);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        count++;
    }
    return 0;
}