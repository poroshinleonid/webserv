#include "HttpHandle.hpp"
#include <assert.h>
#include <string>

namespace HttpHandle {
std::string
HttpHandle::directory_listing_html(const std::string &root_path,
                                   const std::vector<std::string> &leafs) {
  std::string html = R"(<!DOCTYPE html>
<html>
    <head>
        <style>
            body {
                font-family: Arial, sans-serif;
                padding: 20px;
                background-color: #f9f9f9;
            }

            .root {
                font-size: 1.2em;
                font-weight: bold;
                margin-bottom: 10px;
            }

            .leaf {
                margin-left: 20px;
                margin-bottom: 5px;
            }

            .leaf a {
                text-decoration: none;
                color: #007BFF;
            }

            .leaf a:hover {
                text-decoration: underline;
            }
        </style>
    </head>
    <body>
        <!--root-->
        <!--leaf-->
        <!-- <div class="root">Index of src/execution/directory_listing</div>
        <div class="leaf"><a href="..">..</a></div>
        <div class="leaf"><a href="template.html">template.html</a></div>
        <div class="leaf"><a href="dir">dir</a></div> -->
    </body>
</html>
)";
  const std::string root_placeholded = "<!--root-->";
  const std::string leaf_placeholded = "<!--leaf-->";
  assert(html.find(root_placeholded) != html.size());
  assert(html.find(leaf_placeholded) != html.size());
  const std::string root_str =
      "<div class=\"root\">Index of " + root_path + "</div>";
  html.replace(html.find(root_placeholded), root_placeholded.size(), root_str);
  auto make_leaf = [](const std::string &leaf) {
    return "<div class=\"leaf\"><a href=\"" + leaf + "\">" + leaf +
           "</a></div>\n";
  };
  std::string leaf_str;
  for (const std::string &leaf : leafs) {
    leaf_str += make_leaf(leaf);
  }
  html.replace(html.find(leaf_placeholded), leaf_placeholded.size(), leaf_str);
  return html;
}
}; // namespace HttpHandle
