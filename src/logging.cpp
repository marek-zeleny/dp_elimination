#include "logging.hpp"

#include <sstream>
#include <chrono>
#include <iomanip>

namespace dp {

logging::log_buffer::log_buffer(bool log_to_console) : m_log_to_console(log_to_console), m_write_to_file(false) {}

logging::log_buffer::log_buffer(bool log_to_console, const std::string &filename)
    : m_log_to_console(log_to_console), m_write_to_file(true), m_file(filename) {
    if (!m_file.is_open()) {
        throw std::runtime_error("Failed to open the log file");
    }
}

auto logging::log_buffer::overflow(int_type c) -> int_type {
    check_new_line();
    if (c == '\n') {
        m_is_new_line = true;
    }
    if (m_log_to_console) {
        m_cout_buffer->sputc(static_cast<char>(c));
    }
    if (m_write_to_file) {
        m_file.put(static_cast<char>(c));
    }
    return c;
}

int logging::log_buffer::sync() {
    //check_new_line();
    if (m_log_to_console) {
        m_cout_buffer->pubsync();
    }
    if (m_write_to_file) {
        m_file.flush();
    }
    return 0;
}

void logging::log_buffer::check_new_line() {
    if (!m_is_new_line) {
        return;
    }
    std::string prefix = "log (" + get_time_str() + "): ";
    if (m_log_to_console) {
        m_cout_buffer->sputn(prefix.c_str(), prefix.size());
    }
    if (m_write_to_file) {
        m_file.write(prefix.c_str(), prefix.size());
    }
    m_is_new_line = false;
}

#define adj(len) std::setfill('0') << std::setw(len)

std::string logging::log_buffer::get_time_str() const {
    std::ostringstream oss;

    auto now = std::chrono::high_resolution_clock::now();
    auto time_since_epoch = now.time_since_epoch();
    auto h = std::chrono::duration_cast<std::chrono::hours>(time_since_epoch).count() % 24;
    auto min = std::chrono::duration_cast<std::chrono::minutes>(time_since_epoch).count() % 60;
    auto s = std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch).count() % 60;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count() % 100;

    oss << adj(2) << h << ":" << adj(2) << min << ":" << adj(2) << s << "." << adj(3) << ms;
    return oss.str();
}

} // namespace dp
