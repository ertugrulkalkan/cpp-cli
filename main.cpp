#include "CLI.h"

#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <thread>


char* my_generate_line_1()
{
    char *buffer = new char[256];
    sprintf(buffer, "VAR1 : %d", rand());
    return buffer;
}

char* my_generate_line_2()
{
    char *buffer = new char[256];
    sprintf(buffer, "VAR2 : %d", rand());
    return buffer;
}

void my_close()
{
    printf("CLI closed\n");
    exit(0);
}

void my_open()
{
    printf("CLI opened\n");
}

int main()
{
    CLI *cli = CLI::get_cli();

    perm_line_t perm_line = {&my_generate_line_1};
    cli->add_perm_line(&perm_line);
    perm_line = {&my_generate_line_2};
    cli->add_perm_line(&perm_line);

    cli->block_signals();

    cli->on_open = &my_open;
    cli->on_close = &my_close;
    cli->initMap();

    int count = 0;
    int real_cnt = 0;
    char msg[100];
    while(true)
    {
        cli->kb();

        if(count % 100 == 0)
        {
            sprintf(msg, "Hello %d", real_cnt++);
            cli->log(msg);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        count++;
    }
    return 0;
}