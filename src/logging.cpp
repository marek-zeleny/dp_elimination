#include "logging.hpp"

#include <sstream>

namespace dp {

logging::log_buffer::log_buffer() : m_write_to_file(false) {}

logging::log_buffer::log_buffer(const std::string &filename)
    : m_write_to_file(true), m_file(filename, std::ios::out | std::ios::app) {
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open the log file");
    }
}

auto logging::log_buffer::overflow(int_type c) -> int_type {
    check_new_line();
    if (c == '\n') {
        m_is_new_line = true;
    }
    m_cout_buffer->sputc(static_cast<char>(c));
    if (m_write_to_file) {
        m_file.put(static_cast<char>(c));
    }
    return c;
}

int logging::log_buffer::sync() {
    //check_new_line();
    m_cout_buffer->pubsync();
    if (m_write_to_file) {
        m_file.flush();
    }
    return 0;
}

void logging::log_buffer::check_new_line() {
    if (!m_is_new_line) {
        return;
    }
    std::ostringstream prefix_ss;
    prefix_ss << "log: ";
    std::string prefix = prefix_ss.str();
    m_cout_buffer->sputn(prefix.c_str(), prefix.size());
    if (m_write_to_file) {
        m_file.write(prefix.c_str(), prefix.size());
    }
    m_is_new_line = false;
}

} // namespace dp
