# py directory_listing_generator.py > ../DirectoryListing.cpp

template = open('template.html', 'r').read()

cpp = r'''#include <string>
#include <assert.h>
#include "HttpHandle.hpp"

namespace HttpHandle {
    std::string HttpHandle::directory_listing_html(const std::string& root_path, const std::vector<std::string>& leafs) {
        std::string html = R"(HTML_HERE)";
        const std::string root_placeholded = "<!--root-->";
        const std::string leaf_placeholded = "<!--leaf-->";
        assert(html.find(root_placeholded) != html.size());
        assert(html.find(leaf_placeholded) != html.size());
        const std::string root_str = "<div class=\"root\">Index of " + root_path + "</div>";
        html.replace(html.find(root_placeholded), root_placeholded.size(), root_str);
        auto make_leaf = [](const std::string& leaf) {return "<div class=\"leaf\"><a href=\"" + leaf + "\">" + leaf + "</a></div>\n";};
        std::string leaf_str;
        for (const std::string& leaf : leafs) {
            leaf_str += make_leaf(leaf);
        }
        html.replace(html.find(leaf_placeholded), leaf_placeholded.size(), leaf_str);
        return html;
    }
};
'''

cpp = cpp.replace("HTML_HERE", template)
print(cpp)
