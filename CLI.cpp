#include "CLI.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <sys/ioctl.h>
#include <termio.h>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>

#define KEY_BCKSPC 0x007f
#define KEY_ENTER  0x000a
#define KEY_ESCAPE 0x001b

#pragma region UTILS
static termios old;
static int initialized = 0;

void signal_handler(int signum)
{
    CLI::get_cli()->log("PRESS <ESC> TO EXIT");
}

int keyboard_hit()
{
    if (!initialized)
    {
        tcgetattr(STDIN_FILENO, &old);
        struct termios term;
        tcgetattr(STDIN_FILENO, &term);
        term.c_lflag &= ~ICANON;
        term.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    int bytes_waiting = 0;
    ioctl(STDIN_FILENO, FIONREAD, &bytes_waiting);
    return bytes_waiting;
}

void restore_terminal_settings()
{
    old.c_cflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
}
#pragma endregion

CLI *CLI::cli = nullptr;
bool CLI::file_logging;
int CLI::file_logging_fd;

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

    this->file_logging = false;
}

CLI::~CLI()
{
    if(this->perm_lines)
    {
        for(int i = 0; i < this->perm_line_cnt; ++i)
            delete this->perm_lines[i];
        delete[] this->perm_lines;
    }

    if(this->input_buffer)
    {
        delete[] this->input_buffer;
    }

    for (size_t i = 0; i < this->height; i++)
    {
        delete[] this->map[i];
    }
    delete[] this->map;
}

void CLI::push_log(const char *msg, size_t len)
{

    if(file_logging)
    {
        // timestamp
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);

        char *log_msg = new char[len + strlen(time_str) + 3];
        sprintf(log_msg, "%s: %s\n", time_str, msg);

        write(file_logging_fd, log_msg, strlen(log_msg));
        
    }

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

        case KEY_ESCAPE:
            close_cli();
            restore_terminal_settings();
            if(on_close)
            {
                on_close();
            }

            break;

        default:
            if(on_key_pressed)
            {
                on_key_pressed(button);
            }
            break;
        }
    }
    else if (this->mode == mode_t::CLI_MODE_INSERT)
    {
        switch (button)
        {
        case KEY_BCKSPC:
            if(input_buffer[0] != '\0')
            {
                input_buffer[strlen(input_buffer) - 1] = '\0';
            }
            break;

        case KEY_ENTER:
            log(input_buffer);
            if(on_input_sent)
            {
                on_input_sent(input_buffer);
            }
            memset(input_buffer, 0, INPUT_BUFFER_SIZE);
            break;

        case KEY_ESCAPE:
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
    else
    {
        for(int i = 0; i < this->perm_line_cnt; ++i)
        {
            memset(map[i], 0, this->width);
        }
    }
}

void CLI::close_cli()
{
    printf("\e[?1049l");
    if(this->file_logging)
    {
        close(file_logging_fd);
    }
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
        char *line = perm_lines[i]->generate_line();
        memcpy(map[i], line, std::min(width, strlen(line)));
        delete line;
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

void CLI::unblock_signals()
{
    signal(SIGINT, SIG_DFL);
}

void CLI::add_perm_line(perm_line_t *line)
{
    perm_lines = (perm_line_t**)realloc(perm_lines, sizeof(perm_line_t*) * (perm_line_cnt + 1));
    perm_lines[perm_line_cnt] = line;
    perm_line_cnt++;
}

void CLI::log(const char *fmt, ...)
{
    char msg[INPUT_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, INPUT_BUFFER_SIZE, fmt, args);
    va_end(args);

    push_log(msg, strlen(msg));
    update();
}

void CLI::enable_file_logging()
{
    if(!this->file_logging)
    {
        char *log_file_name = new char[LOG_FILE_NAME_SIZE];
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        strftime(log_file_name, LOG_FILE_NAME_SIZE, "/tmp/CLI-%Y-%m-%d_%H-%M-%S.log", tm);
        this->file_logging_fd = open(log_file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
        this->file_logging = true;
    }
}

void CLI::kb()
{
    if(keyboard_hit())
    {
#define KB_SIZE 4
        char *key = new char[KB_SIZE];
        memset(key, 0, KB_SIZE);

        size_t size = read(STDIN_FILENO, key, (size_t)KB_SIZE);
        
        switch (size)
        {
        case 1:
            key_pressed(key[0]);
            break;
        case 2:
        case 3:
        case 4:
        default:
            break;
        }
    }
}
