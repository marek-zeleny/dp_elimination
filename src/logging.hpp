#pragma once

#include <iostream>
#include <fstream>
#include <streambuf>

namespace dp {

class logging : public std::ostream {
public:
    logging(bool log_to_console) : std::ostream(&buffer), buffer(log_to_console) {}
    logging(bool log_to_console, const std::string& filename)
        : std::ostream(&buffer), buffer(log_to_console, filename) {}

private:
    class log_buffer : public std::streambuf {
    public:
        log_buffer(bool log_to_console);
        log_buffer(bool log_to_console, const std::string& filename);

    protected:
        int_type overflow(int_type c) override;
        int sync() override;

    private:
        const bool m_log_to_console;
        const bool m_write_to_file;
        std::streambuf *m_cout_buffer {std::cout.rdbuf()};
        std::ofstream m_file;
        bool m_is_new_line {true};

        void check_new_line();
        std::string get_time_str() const;
    };

    log_buffer buffer;
};

#ifdef DEBUG
inline logging log {true, "debug/dp.log"};
#else
inline logging log {false, "release/dp.log"};
#endif

} // namespace dp
