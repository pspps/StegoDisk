/**
* @file file_unix.cc
* @author Martin Kosdy
* @author Matus Kysel
* @date 2016
* @brief Class for files manipulation under UNIX like OSes
*
*/

#if defined(__unix__) || defined(__APPLE__)
#include "file.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <string>
#include <vector>

namespace stego_disk {

/*
 * beautiful C core of this awesome method is written by mY
 * ugly C++ part of this method is writen by drunk martin
*/

static void AddFilesInDir(std::string base_path, std::string current_path, std::string mask, std::vector<File>& file_list) {

  DIR *dir;
  struct dirent *de;
  struct stat sb;

  // base_path_safe contains / at the end of the string
  std::string base_path_safe = base_path;

  if (base_path_safe.empty())
    base_path_safe = ".";

  if (base_path_safe[base_path_safe.length()-1] != '/')
    base_path_safe = base_path_safe.append("/");

  // current_path_safe doesnt contain / at the end of the string nor at the beginning
  std::string current_path_safe = current_path;
  if (current_path_safe[current_path_safe.length() - 1] == '/')
    current_path_safe = current_path_safe.substr(0, current_path_safe.length() - 1);

  if (current_path_safe.length() > 0)
    if (current_path_safe[0] == '/')
      current_path_safe = current_path_safe.substr(1);

  std::string path = base_path_safe + current_path_safe;

  // otvorime adresar
  dir = opendir(path.c_str());
  if ( !dir ) {
    fprintf(stderr,"Error: opendir '%s': chyba pri otvarani adresara\n", path.c_str() );
	throw std::runtime_error("Error occurred while opening directory " + path);
  }

  while ( (de = readdir(dir)) != NULL ) {
    std::string new_current_path = current_path + "/" + std::string(de->d_name);
    std::string new_path = base_path + new_current_path;
    if ( lstat(new_path.c_str(), &sb) == -1 ) {
      fprintf( stderr, "Error: printDir: chyba zistovania info o objekte fs\n" );
      closedir(dir);
	  throw std::runtime_error("Error occurred while getting info of " + new_path);
    }

    // ak je polozka adresarom, tak sa rekurzivne vnorime
    if ( S_ISDIR(sb.st_mode) ) {
      // nesmieme sa vnorit do toho isteho adresara ani o uroven vyssie
      if ( strcmp(de->d_name, ".") && strcmp(de->d_name, "..") ) {
        // treba otestovat opravnenie na prava citania a pristupu do adresara
        if ( access(new_path.c_str(), X_OK | R_OK) != -1 ) {
          // otestovat navratovu hodnotu na chybove stavy
          AddFilesInDir(base_path, current_path, mask, file_list);
        }
      }
    } // if S_ISDIR
    else {
      // TODO: over <mask> ci sedi s priponou
      file_list.push_back(File(base_path, new_current_path));
    }

  } // while readdir

  // zavrieme adresar
  closedir( dir );
}


// TODO: add wildcard parameter to specify supported file extensions (*.bmp, *.jpg ...)
std::vector<File> File::GetFilesInDir(std::string directory, std::string mask)
{
  std::vector<File> file_list;

  AddFilesInDir(directory, "", mask, file_list);

  return file_list;
}

} // stego_disk

#endif // defined(__unix__) || defined(__APPLE__)
