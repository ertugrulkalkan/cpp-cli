#include "CLI.h"

#include <cstring>
#include <algorithm>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

static CLI *cli = nullptr;
static size_t cli_width = 0;
static size_t cli_height = 0;

void set_cli_size(size_t width, size_t height)
{
    cli_width = width;
    cli_height = height;
}

CLI* get_cli()
{
    if(cli == nullptr)
    {
        cli = new CLI(cli_width, cli_height);
    }
    return cli;
}

CLI::CLI(size_t width, size_t height)
{
    this->width = width;
    this->height = height;

    // last line is reserved
    this->log_bottom = height - 1;
    this->log_top = 3;
}

CLI::~CLI()
{
    for (size_t i = 0; i < this->height; i++)
    {
        delete[] this->map[i];
    }
    delete[] this->map;
}

void CLI::initMap()
{
    map = new char*[height];
    for (size_t i = 0; i < height; i++)
    {
        map[i] = new char[width];
    }

    input_buffer = new char[1024];
    header_buffer = new char[1024];
    variable_buffer_l1 = new char[1024];
    variable_buffer_l2 = new char[1024];
    variable_buffer_l1_fmt = new char[1024];
    variable_buffer_l2_fmt = new char[1024];

    memset(input_buffer, 0, 1024);

    memcpy(header_buffer, "CLI", 3);
    strcpy(variable_buffer_l1_fmt, "constant field 1: var1: 0x%x, var2: %d, var3: %d, var4: 0x%x");
    strcpy(variable_buffer_l2_fmt, "constant field 2: var5: 0x%x, var6: %d, var7: %d, var8: 0x%x");
}

void CLI::log(const char *msg)
{
    push_log(msg, strlen(msg));
    update();
}

void CLI::push_log(const char *msg, size_t len)
{
    size_t displayable_len = std::min(width, len);
    
    if(log_index = log_bottom)
    {
        for(size_t i = log_top; i < log_bottom - 1; i++)
        {
            memcpy(map[i], map[i + 1], width);
            memset(map[i + 1], 0, width);
        }
        log_index--;
    }

    memcpy(map[log_index], msg, displayable_len);
    log_index++;
}

void CLI::update()
{
    strcpy(map[0], header_buffer);

    sprintf(variable_buffer_l1, variable_buffer_l1_fmt, 0, 0, 0, 0);
    sprintf(variable_buffer_l2, variable_buffer_l2_fmt, 0, 0, 0, 0);

    strcpy(map[1], variable_buffer_l1);
    strcpy(map[2], variable_buffer_l2);

    memcpy(map[height - 1], input_buffer, width);
    std::cout << std::endl;
    for(size_t i = 0; i < height; i++)
    {
        std::cout << map[i];
        if(i != height - 1)
        {
            std::cout << std::endl;
        }
        
    }
    std::flush(std::cout);
}

void CLI::key_pressed(const int button)
{
    switch (button)
    {
    case 127:
        if(input_buffer[0] != '\0')
        {
            input_buffer[strlen(input_buffer) - 1] = '\0';
        }
        break;
    
    case '\n':
        log(input_buffer);
        memset(input_buffer, 0, 512);
        break;

    default:
        if(strlen(input_buffer) < width - 1)
        {
            input_buffer[strlen(input_buffer)] = button;
            input_buffer[strlen(input_buffer) + 1] = '\0';
        }
        break;
    }
    update();
}