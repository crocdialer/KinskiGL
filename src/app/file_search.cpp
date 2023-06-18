//
// Created by crocdialer on 18.06.23.
//

#include <shared_mutex>
#include <crocore/utils.hpp>
#include <crocore/filesystem.hpp>

#include "file_search.hpp"

namespace kinski::app
{

namespace
{
std::set<std::filesystem::path> g_search_paths;

std::shared_mutex g_mutex;
}

std::string expand_user(std::string path)
{
  path = crocore::trim(path);

  if(!path.empty() && path[0] == '~')
  {
    if(path.size() != 1 && path[1] != '/'){ return path; } // or other error handling ?
    char const *home = getenv("HOME");
    if(home || ((home = getenv("USERPROFILE"))))
    {
      path.replace(0, 1, home);
    }
    else
    {
      char const *hdrive = getenv("HOMEDRIVE"),
                 *hpath = getenv("HOMEPATH");
      if(!(hdrive && hpath)){ return path; } // or other error handling ?
      path.replace(0, 1, std::string(hdrive) + hpath);
    }
  }
  return path;
}

/////////// end implemantation internal /////////////

///////////////////////////////////////////////////////////////////////////////

std::set<std::filesystem::path> search_paths()
{
  std::shared_lock<std::shared_mutex> lock(g_mutex);
  return g_search_paths;
}

///////////////////////////////////////////////////////////////////////////////

void add_search_path(const std::filesystem::path &path, int recursion_depth)
{
  std::filesystem::path path_expanded(expand_user(path));

  if(!std::filesystem::exists(path_expanded))
  {
    spdlog::debug("directory {} not existing", path_expanded.string());
    return;
  }

  std::unique_lock<std::shared_mutex> lock(g_mutex);

  if(recursion_depth)
  {
    g_search_paths.insert(crocore::fs::get_directory_part(path_expanded.string()));
    std::filesystem::recursive_directory_iterator it;
    try
    {
      it = std::filesystem::recursive_directory_iterator(path_expanded);
      std::filesystem::recursive_directory_iterator end;

      while(it != end)
      {
        if(std::filesystem::is_directory(*it)){ g_search_paths.insert(canonical(it->path()).string()); }
        try{ ++it; }
        catch(std::exception &e)
        {
          // e.g. no permission
          spdlog::error(e.what());
          it.disable_recursion_pending();
          try{ ++it; } catch(...)
          {
            spdlog::error("Got trouble in recursive directory iteration: {}", it->path().string());
            return;
          }
        }
      }
    }
    catch(std::exception &e){ spdlog::error(e.what()); }
  }
  else{ g_search_paths.insert(canonical(path_expanded).string()); }
}

std::filesystem::path search_file(const std::filesystem::path &file_name)
{
  auto trim_file_name = crocore::trim(file_name);

  std::string expanded_name = expand_user(trim_file_name);
  std::filesystem::path ret_path(expanded_name);

  try
  {
    if(ret_path.is_absolute() && is_regular_file(ret_path)){ return ret_path.string(); }

    std::shared_lock<std::shared_mutex> lock(g_mutex);

    for(const auto &p : g_search_paths)
    {
      ret_path = p / std::filesystem::path(expanded_name);

      if(std::filesystem::exists(ret_path) && is_regular_file(ret_path))
      {
        spdlog::trace("found '{}' as: {}", trim_file_name, ret_path.string());
        return ret_path.string();
      }
    }
  }
  catch(std::exception &e){ spdlog::debug(e.what()); }

  // not found
  throw std::runtime_error(fmt::format("{} not found", file_name.string()));
}

}