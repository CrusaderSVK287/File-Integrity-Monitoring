#pragma once

#include <string>
namespace logging {
    enum class LogVerbosity {
        low = 0,        // only log errors, errors show on screen
        normal = 1,     // log errors and msg, only errors show on screen
        high = 2,       // log both errors and msgs, both show on screen
        highest = 3,    // log info as well, all show on screen
    };

    bool init(LogVerbosity v);
    // Call after initialising configuration manager
    bool setup();

    bool msg(std::string);
    bool err(std::string);
    //same as err but serves as information on program function, 
    //not errors (like waring password changed)
    bool warn(std::string); 
    bool info(std::string);
}
