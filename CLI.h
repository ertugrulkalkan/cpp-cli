#ifndef CLI_H
#define CLI_H

#include <cstdlib>
#include <functional>

#define INPUT_BUFFER_SIZE 1024
#define LOG_FILE_NAME_SIZE 128

typedef struct
{
    std::function<char*()> generate_line;
} perm_line_t;

class CLI
{
private:
    static CLI *cli;

    static bool file_logging;
    static int file_logging_fd;

    enum mode_t
    {
        CLI_MODE_COM = 0x0,
        CLI_MODE_INSERT,
    } mode;

    size_t width;
    size_t height;

    perm_line_t **perm_lines;
    size_t perm_line_cnt = 0;

    size_t log_top = 0;
    size_t log_bottom = 0;
    size_t log_index = 0;

    char **map;
    char *input_buffer;


private:
    CLI();
    CLI(const CLI&) = delete;
    CLI(const CLI&&) = delete;
    CLI& operator=(const CLI&) = delete;

    void push_log(const char *msg, size_t len);
    void key_pressed(const int button);
    void resize_cli();
    void close_cli();

public:
    ~CLI();

    // set callbacks if needed
    void (*on_key_pressed)(const int button);
    void (*on_input_sent)(const char* input);
    void (*on_close)();
    void (*on_open)();

    static CLI* get_cli();

    void initMap();
    void update();
    void block_signals();
    void unblock_signals();

    void add_perm_line(perm_line_t *perm_line);
    void log(const char *fmt, ...);

    void enable_file_logging();

    void kb();
};


#endif // CLI_H