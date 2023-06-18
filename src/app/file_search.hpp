//
// Created by crocdialer on 18.06.23.
//

#ifndef KINSKIGL_FILE_SEARCH_H
#define KINSKIGL_FILE_SEARCH_H

#pragma once

#include <set>
#include <filesystem>

namespace kinski::app {

std::filesystem::path search_file(const std::filesystem::path &file_name);

void add_search_path(const std::filesystem::path &path, int recursion_depth);

std::set<std::filesystem::path> search_paths();

}

#endif // KINSKIGL_FILE_SEARCH_H
