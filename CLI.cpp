#include "CLI.h"

#include <cstring>
#include <cstdio>
#include <algorithm>
#include <sys/ioctl.h>
#include <termio.h>
#include <unistd.h>
#include <csignal>

#pragma region UTILS
void signal_handler(int signum)
{
    char *msg = new char[100];
    sprintf(msg, "PRESS <ESC> TO EXIT");
    CLI::get_cli()->log(msg);
    delete[] msg;
}

int keyboard_hit()
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
#pragma endregion

CLI *CLI::cli = nullptr;

CLI::CLI()
{
    this->width = 0;
    this->height = 0;

    this->perm_lines = nullptr;
    this->perm_line_cnt = 0;

    this->input_buffer = new char[INPUT_BUFFER_SIZE];
    memset(this->input_buffer, 0, INPUT_BUFFER_SIZE);

    this->on_close = nullptr;
    this->map = nullptr;

    this->mode = mode_t::CLI_MODE_COM;
}

CLI::~CLI()
{
    delete[] this->perm_lines;
    delete[] this->input_buffer;

    for (size_t i = 0; i < this->height; i++)
    {
        delete[] this->map[i];
    }
    delete[] this->map;
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

void CLI::key_pressed(const int button)
{
    if(this->mode == mode_t::CLI_MODE_COM)
    {
        switch (button)
        {
        case 'i':
            this->mode = mode_t::CLI_MODE_INSERT;
            break;

        case '\e':
            close_cli();
            break;

        default:
            break;
        }
    }
    else if (this->mode == mode_t::CLI_MODE_INSERT)
    {
        switch (button)
        {
        case '\x7f':
            if(input_buffer[0] != '\0')
            {
                input_buffer[strlen(input_buffer) - 1] = '\0';
            }
            break;

        case '\n':
            log(input_buffer);
            memset(input_buffer, 0, INPUT_BUFFER_SIZE);
            break;

        case '\e':
            memset(input_buffer, 0, INPUT_BUFFER_SIZE);
            this->mode = mode_t::CLI_MODE_COM;
            break;

        default:
            if((button >= ' ') && (button <= '~') && strlen(input_buffer) < INPUT_BUFFER_SIZE - 1)
            {
                input_buffer[strlen(input_buffer)] = button;
                input_buffer[strlen(input_buffer) + 1] = '\0';
            }
            break;
        }
    }

    update();
}

void CLI::resize_cli()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    if(this->width != w.ws_col || this->height != w.ws_row)
    {
        char **old_map = this->map;
        size_t old_map_width = this->width;
        size_t old_map_height = this->height;

        this->width = w.ws_col;
        this->height = w.ws_row;

        this->log_top = perm_line_cnt;
        this->log_bottom = height - 1;



        this->map = new char*[this->height];
        for (size_t i = 0; i < this->height; i++)
        {
            this->map[i] = new char[this->width];
            memset(this->map[i], 0, this->width);
        }

        if(old_map != nullptr)
        {
            {
                if(old_map_height > this->height)
                {
                    // new map is smaller
                    for(size_t i = this->log_top; i < this->height - 1; ++i)
                    {
                        memcpy(this->map[i], old_map[i + (old_map_height - this->height)], std::min(old_map_width, this->width));
                    }
                }
                else
                {
                    // new map is bigger
                    for(size_t i = this->log_top; i < old_map_height - 1; i++)
                    {
                        memcpy(this->map[(this->height - old_map_height) + i], old_map[i], std::min(old_map_width, this->width));
                    }
                }
            }

            for (size_t i = 0; i < old_map_height; i++)
            {
                delete[] old_map[i];
            }
            delete[] old_map;
        }
    }
}

void CLI::close_cli()
{
    printf("\e[?1049l");
    if(on_close != nullptr) on_close();
    exit(0);
}

CLI* CLI::get_cli()
{
    if(cli == nullptr)
    {
        cli = new CLI();
    }
    return cli;
}

void CLI::initMap()
{
    if(on_open != nullptr) on_open();
    printf("\e[?1049h\e[H");
    resize_cli();
}

void CLI::update()
{
    resize_cli();
    printf("\e[2J\n");

    for(size_t i = 0; i < perm_line_cnt; i++)
    {
        char *line = perm_lines[i].generate_line();
        memcpy(map[i], line, std::min(width, strlen(line)));
    }

    memcpy(map[height - 1], input_buffer, std::min(this->width, (size_t)INPUT_BUFFER_SIZE));

    for(size_t i = 0; i < height; i++)
    {
        printf("%s", map[i]);
        if(i != height - 1)
        {
            printf("\n");
        }
    }
    fflush(stdout);
}

void CLI::block_signals()
{
    signal(SIGINT, &signal_handler);
}

void CLI::add_perm_line(perm_line_t *line)
{
    perm_lines = (perm_line_t*)realloc(perm_lines, sizeof(perm_line_t) * (perm_line_cnt + 1));
    perm_lines[perm_line_cnt] = *line;
    perm_line_cnt++;
}

void CLI::log(const char *msg)
{
    push_log(msg, strlen(msg));
    update();
}

void CLI::kb()
{
    if(keyboard_hit())
    {
        int ch = getchar();
        key_pressed(ch);
    }
}
