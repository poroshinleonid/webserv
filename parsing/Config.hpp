#pragma once

#include <string>

class Config
{
    public:
        std::string unwrap(); // returns content string or throws std::invalid_argument
        const Config& operator[](const std::string& key); // gets config by key, throws std::out_of_range if not found

        Config(const std::string& filename); /* parses config from a file and 
        throws std::invalid_argument (both if failed to open or failed to parse), 
        prints approptiate errors */

        // idk if I need copy/assignment etc
    private:
        std::string content_;
};
