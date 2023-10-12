#pragma once

#include <iostream>
#include <fstream>
#include <streambuf>

namespace dp {

class logging : public std::ostream {
public:
    logging() : std::ostream(&buffer) {}
    logging(const std::string& filename) : std::ostream(&buffer), buffer(filename) {}

private:
    class log_buffer : public std::streambuf {
    public:
        log_buffer();
        log_buffer(const std::string& filename);

    protected:
        int_type overflow(int_type c) override;
        int sync() override;

    private:
        const bool m_write_to_file;
        std::streambuf *m_cout_buffer {std::cout.rdbuf()};
        std::ofstream m_file;
        bool m_is_new_line {true};

        void check_new_line();
    };

    log_buffer buffer;
};

inline logging log {};

} // namespace dp
