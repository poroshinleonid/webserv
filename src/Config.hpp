#pragma once

#include <string>

using std::string;

/*
grammar.

OBJ: {LINKLIST}
VALUE: STR | OBJ
LINK: STR:VALUE
LINKLIST: empty | LINK(,LINK)*
STR: "some text"
*/

/* For future: better to create a way to iterate over data structure and use it to implement methods
instead of doing this bullshit. */


/*
Usage: construct Config from a file, and then call something like
config["key1"]["key2"].unwrap() to get values
*/
class Config
{
    public:
        std::string unwrap(); // returns content string or throws std::invalid_argument
        std::string get_content();
        Config operator[](const std::string& key); // gets config by key, throws std::out_of_range if not found

        Config(const std::string& filename); /* parses config from a file and 
        throws if failed to open/failed to parse */

        // idk if I need copy/assignment etc
    private:
        Config(const std::string& content, bool /* dummy */);
        static string remove_spaces(const string& s);
        void throw_if_invalid();
        void get_value(const std::string& s);
        string eat_obj(const string& s);
        void search_linklist(string s);
        string eat_link(string s);
        string eat_value(const string& s);

        int depth_;
        string content_;
        string key_to_find_;
        string key_found_;
        bool is_key_found_;
        string value_found_;
};
class InvalidConfig: public std::runtime_error {
public:
    InvalidConfig(const std::string& msg)
        : std::runtime_error(msg) {
    }
};
