#pragma once

#include <array>

class OptionalException : public std::exception
{
    const char * what () const throw () override {
        return "not intilized";
    }
};

class nullopt_t {};

constexpr nullopt_t NULLOPT;

//this is a very simple implmention for optional since we dont have c++17
template <typename T>
class Optional {
public:
    Optional() : m_init(false) { }

    Optional(nullopt_t) : m_init(false) { }

    Optional(T val) : m_val(std::move(val)), m_init(true) { }

    operator T() const {
        if(!m_init) {
            throw OptionalException{};
        }
        return m_val;
    }

    operator bool() const {
        return m_init;
    }

    Optional& operator=(nullopt_t) {
        m_init = false;
        return *this;
    }

    Optional& operator=(T val) {
        m_init = true;
        m_val = std::move(val);
        return *this;
    }

    bool operator==(nullopt_t) const {
        return !m_init;
    }

    bool operator==(const T& val) const {
        return m_init && m_val == val;
    }

private:
    T m_val;
    bool m_init;
};

template <typename T>
std::ostream& operator<< (std::ostream& stream, const Optional<T>& v) {
    if(!v) {
        stream << "None";
    } else {
        stream << static_cast<T>(v);
    }
    return stream;
}