#pragma once

#include <string>
#include <vector>
#include <exception>
#include <stdexcept> 
#include <map>

using std::string;
using std::vector;

/*
Usage: construct Config from a file, and then call something like
config["key1"].get_vec["key2"][1]["key2"].unwrap() to get values
*/
class Config
{
    public:
        std::string unwrap(); // returns content string or throws std::invalid_argument
        std::string get_content();
        Config operator[](const std::string& key); // gets config by key, throws std::out_of_range if not found, if multiple elements, returns the last one
        vector<Config> get_vec(const std::string& key); // returns vector of configs (one entry for each key entry). doesn not throw if vec is empty

        Config(const std::string& filename); /* parses config from a file and 
        throws if failed to open/failed to parse */
        bool key_exists(const std::string &key);
        std::map<string, string> get_content_dict();
        static unsigned long string_to_ip(const std::string &ip_string);
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
        vector<string> values_found_;

    public:
     const static int client_default_max_body_size = 1<<15; // 32K
};

class InvalidConfig : public std::runtime_error {
public:
    InvalidConfig(const std::string& msg)
        : std::runtime_error(msg) {
    }
};

/*
grammar.

OBJ: {LINKLIST}
VALUE: STR | OBJ
LINK: STR:VALUE
LINKLIST: empty | LINK(,LINK)*
STR: "some text"
*/
