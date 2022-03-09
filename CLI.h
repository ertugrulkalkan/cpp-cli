#ifndef CLI_H
#define CLI_H

#include <cstdlib>


class CLI
{
private:
    size_t width;
    size_t height;

    size_t log_top = 0;
    size_t log_bottom = 0;
    size_t log_index = 0;
    
    char **map;
    char *input_buffer;
    char *header_buffer;
    char *variable_buffer_l1;
    char *variable_buffer_l2;
    char *variable_buffer_l1_fmt;
    char *variable_buffer_l2_fmt;

private:
    CLI() = delete;
    CLI(const CLI&) = delete;
    CLI(const CLI&&) = delete;
    CLI& operator=(const CLI&) = delete;

    void push_log(const char *msg, size_t len);

public:
    CLI(size_t width, size_t height);
    ~CLI();

    void initMap();

    void log(const char *msg);
    void key_pressed(const int button);
    void update();

};

void set_cli_size(size_t width, size_t height);
CLI* get_cli();

#endif