//
// Created by vwann on 28-Oct-26.
//

#ifndef CUSTOM_ERROR_H
#define CUSTOM_ERROR_H

#include <stdexcept>

struct custom_error : std::runtime_error {
    //generic exception which may be intercepted and recovered from
    //to be refactored to log directly into a log stream or file
    custom_error() : std::runtime_error("") { }
    explicit custom_error(const std::string& message) : std::runtime_error(message) {
        std::cerr<<message<<std::endl;
    }
};

struct out_of_range : std::out_of_range {
    out_of_range() : std::out_of_range("") { }
    explicit out_of_range(const std::string& message) : std::out_of_range(message) { }
};

struct invalid_type : std::invalid_argument {
    invalid_type() : std::invalid_argument("") { }
    explicit invalid_type(const std::string& message) : std::invalid_argument(message) { }
};

#endif CUSTOM_ERROR_H
