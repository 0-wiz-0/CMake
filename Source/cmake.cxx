/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "cmake.h"
#include "time.h"
#include "cmCacheManager.h"
#include "cmMakefile.h"
#include "cmLocalGenerator.h"
#include "cmCommands.h"
#include "cmCommand.h"

#if defined(CMAKE_BUILD_WITH_CMAKE)
# include "cmVariableWatch.h"
# include "cmVersion.h"
# include "cmLocalUnixMakefileGenerator2.h"
#endif

// only build kdevelop generator on non-windows platforms
// when not bootstrapping cmake
#if !defined(_WIN32)
# if defined(CMAKE_BUILD_WITH_CMAKE)
#   define CMAKE_USE_KDEVELOP
# endif
#endif

// include the generator
#if defined(_WIN32) && !defined(__CYGWIN__)
#  include "cmGlobalVisualStudio6Generator.h"
#  if !defined(__MINGW32__)
#    include "cmGlobalVisualStudio7Generator.h"
#    include "cmGlobalVisualStudio71Generator.h"
#    include "cmGlobalVisualStudio8Generator.h"
#  endif
#  include "cmGlobalBorlandMakefileGenerator.h"
#  include "cmGlobalNMakeMakefileGenerator.h"
#  include "cmWin32ProcessExecution.h"
#else
#endif
#include "cmGlobalUnixMakefileGenerator.h"
#include "cmGlobalXCodeGenerator.h"

#ifdef CMAKE_USE_KDEVELOP
# include "cmGlobalKdevelopGenerator.h"
#endif

#include <stdlib.h> // required for atoi

#ifdef __APPLE__
#  include <sys/types.h>
#  include <sys/time.h>
#  include <sys/resource.h>
#  if defined(CMAKE_BUILD_WITH_CMAKE)
#    include "cmGlobalCodeWarriorGenerator.h"
#  endif
#endif

#include <memory> // auto_ptr

void cmNeedBackwardsCompatibility(const std::string& variable, 
                                  int access_type, void* )
{
#ifdef CMAKE_BUILD_WITH_CMAKE
  if (access_type == cmVariableWatch::UNKNOWN_VARIABLE_READ_ACCESS)
    {
    std::string message = "An attempt was made to access a variable: ";
    message += variable;
    message += " that has not been defined. Some variables were always defined "
      "by CMake in versions prior to 1.6. To fix this you might need to set the "
      "cache value of CMAKE_BACKWARDS_COMPATIBILITY to 1.4 or less. If you are "
      "writing a CMakeList file, (or have already set "
      "CMAKE_BACKWARDS_COMPATABILITY to 1.4 or less) then you probably need to "
      "include a CMake module to test for the feature this variable defines.";
    cmSystemTools::Error(message.c_str());
    }
#else
  (void)variable;
  (void)access_type;
#endif
}

cmake::cmake()
{
  m_DebugTryCompile = false;
#ifdef __APPLE__
  struct rlimit rlp;
  if(!getrlimit(RLIMIT_STACK, &rlp))
    {
    if(rlp.rlim_cur != rlp.rlim_max)
      {
        rlp.rlim_cur = rlp.rlim_max;
         setrlimit(RLIMIT_STACK, &rlp);
      }
    }
#endif

  // If MAKEFLAGS are given in the environment, remove the environment
  // variable.  This will prevent try-compile from succeeding when it
  // should fail (if "-i" is an option).  We cannot simply test
  // whether "-i" is given and remove it because some make programs
  // encode the MAKEFLAGS variable in a strange way.
  if(getenv("MAKEFLAGS"))
    {
    cmSystemTools::PutEnv("MAKEFLAGS=");
    }  
  
  m_Local = false;
  m_Verbose = false;
  m_InTryCompile = false;
  m_CacheManager = new cmCacheManager;
  m_GlobalGenerator = 0;
  m_ProgressCallback = 0;
  m_ProgressCallbackClientData = 0;
  m_ScriptMode = false;

#ifdef CMAKE_BUILD_WITH_CMAKE
  m_VariableWatch = new cmVariableWatch;
  m_VariableWatch->AddWatch("CMAKE_WORDS_BIGENDIAN",
                            cmNeedBackwardsCompatibility);
  m_VariableWatch->AddWatch("CMAKE_SIZEOF_INT",
                            cmNeedBackwardsCompatibility);
  m_VariableWatch->AddWatch("CMAKE_X_LIBS",
                            cmNeedBackwardsCompatibility);
#endif

  this->AddDefaultGenerators();
  this->AddDefaultCommands();

}

cmake::~cmake()
{
  delete m_CacheManager;
  if (m_GlobalGenerator)
    {
    delete m_GlobalGenerator;
    m_GlobalGenerator = 0;
    }
  for(RegisteredCommandsMap::iterator j = m_Commands.begin();
      j != m_Commands.end(); ++j)
    {
    delete (*j).second;
    }
#ifdef CMAKE_BUILD_WITH_CMAKE
  delete m_VariableWatch;
#endif
}

bool cmake::CommandExists(const char* name) const
{
  return (m_Commands.find(name) != m_Commands.end());
}

cmCommand *cmake::GetCommand(const char *name) 
{
  cmCommand* rm = 0;
  RegisteredCommandsMap::iterator pos = m_Commands.find(name);
  if (pos != m_Commands.end())
    {
    rm = (*pos).second;
    }
  return rm;
}

void cmake::AddCommand(cmCommand* wg)
{
  std::string name = wg->GetName();
  // if the command already exists, free the old one
  RegisteredCommandsMap::iterator pos = m_Commands.find(name);
  if (pos != m_Commands.end())
    {
    delete pos->second;
    m_Commands.erase(pos);
    }
  m_Commands.insert( RegisteredCommandsMap::value_type(name, wg));
}

// Parse the args
bool cmake::SetCacheArgs(const std::vector<std::string>& args)
{ 
  for(unsigned int i=1; i < args.size(); ++i)
    {
    std::string arg = args[i];
    if(arg.find("-D",0) == 0)
      {
      std::string entry = arg.substr(2);
      std::string var, value;
      cmCacheManager::CacheEntryType type = cmCacheManager::UNINITIALIZED;
      if(cmCacheManager::ParseEntry(entry.c_str(), var, value, type) ||
        cmCacheManager::ParseEntry(entry.c_str(), var, value))
        {
        this->m_CacheManager->AddCacheEntry(var.c_str(), value.c_str(),
          "No help, variable specified on the command line.",
          type);
        }
      else
        {
        std::cerr << "Parse error in command line argument: " << arg << "\n"
                  << "Should be: VAR:type=value\n";
        cmSystemTools::Error("No cmake scrpt provided.");
        return false;
        }        
      }
    else if(arg.find("-C",0) == 0)
      {
      std::string path = arg.substr(2);
      if ( path.size() == 0 )
        {
        cmSystemTools::Error("No initial cache file provided.");
        return false;
        }
      std::cerr << "loading initial cache file " << path.c_str() << "\n";
      this->ReadListFile(path.c_str());
      }
    else if(arg.find("-P",0) == 0)
      {
      i++;
      std::string path = args[i];
      if ( path.size() == 0 )
        {
        cmSystemTools::Error("No cmake scrpt provided.");
        return false;
        }
      std::cerr << "Running cmake script file " << path.c_str() << "\n";
      this->ReadListFile(path.c_str());
      }
    }
  return true;
}

void cmake::ReadListFile(const char *path)
{
  // if a generator was not yet created, temporarily create one
  cmGlobalGenerator *gg = this->GetGlobalGenerator();
  bool created = false;
  
  // if a generator was not specified use a generic one
  if (!gg)
    {
    gg = new cmGlobalGenerator;
    gg->SetCMakeInstance(this);
    created = true;
    }

  // read in the list file to fill the cache
  if(path)
    {
    std::auto_ptr<cmLocalGenerator> lg(gg->CreateLocalGenerator());
    lg->SetGlobalGenerator(gg);
    if (!lg->GetMakefile()->ReadListFile(0, path))
      {
      std::cerr << "Error in reading cmake initial cache file:"
                << path << "\n";
      }
    }
  
  // free generic one if generated
  if (created)
    {
    delete gg;
    }
}

// Parse the args
void cmake::SetArgs(const std::vector<std::string>& args)
{
  m_Local = false;
  bool directoriesSet = false;
  for(unsigned int i=1; i < args.size(); ++i)
    {
    std::string arg = args[i];
    if(arg.find("-H",0) == 0)
      {
      directoriesSet = true;
      std::string path = arg.substr(2);
      path = cmSystemTools::CollapseFullPath(path.c_str());
      cmSystemTools::ConvertToUnixSlashes(path);
      this->SetHomeDirectory(path.c_str());
      }
    else if(arg.find("-S",0) == 0)
      {
      directoriesSet = true;
      m_Local = true;
      std::string path = arg.substr(2);
      path = cmSystemTools::CollapseFullPath(path.c_str());
      cmSystemTools::ConvertToUnixSlashes(path);
      this->SetStartDirectory(path.c_str());
      }
    else if(arg.find("-O",0) == 0)
      {
      directoriesSet = true;
      std::string path = arg.substr(2);
      path = cmSystemTools::CollapseFullPath(path.c_str());
      cmSystemTools::ConvertToUnixSlashes(path);
      this->SetStartOutputDirectory(path.c_str());
      }
    else if(arg.find("-B",0) == 0)
      {
      directoriesSet = true;
      std::string path = arg.substr(2);
      path = cmSystemTools::CollapseFullPath(path.c_str());
      cmSystemTools::ConvertToUnixSlashes(path);
      this->SetHomeOutputDirectory(path.c_str());
      }
    else if((i < args.size()-1) && (arg.find("--check-build-system",0) == 0))
      {
      m_CheckBuildSystem = args[++i];
      }
    else if(arg.find("-V",0) == 0)
      {
        m_Verbose = true;
      }
    else if(arg.find("-D",0) == 0)
      {
      // skip for now
      }
    else if(arg.find("-C",0) == 0)
      {
      // skip for now
      }
    else if(arg.find("-P",0) == 0)
      {
      // skip for now
      i++;
      }
    else if(arg.find("--debug-trycompile",0) == 0)
      {
      std::cout << "debug trycompile on\n";
      this->DebugTryCompileOn();
      }
    else if(arg.find("-G",0) == 0)
      {
      std::string value = arg.substr(2);
      cmGlobalGenerator* gen = 
        this->CreateGlobalGenerator(value.c_str());
      if(!gen)
        {
        cmSystemTools::Error("Could not create named generator ",
                             value.c_str());
        }
      else
        {
        this->SetGlobalGenerator(gen);
        }
      }
    // no option assume it is the path to the source
    else
      {
      directoriesSet = true;
      this->SetDirectoriesFromFile(arg.c_str());
      }
    }
  if(!directoriesSet)
    {
    this->SetHomeOutputDirectory
      (cmSystemTools::GetCurrentWorkingDirectory().c_str());
    this->SetStartOutputDirectory
      (cmSystemTools::GetCurrentWorkingDirectory().c_str());
    this->SetHomeDirectory
      (cmSystemTools::GetCurrentWorkingDirectory().c_str());
    this->SetStartDirectory
      (cmSystemTools::GetCurrentWorkingDirectory().c_str());
    }
  if (!m_Local)
    {
    this->SetStartDirectory(this->GetHomeDirectory());
    this->SetStartOutputDirectory(this->GetHomeOutputDirectory());
    }
}

//----------------------------------------------------------------------------
void cmake::SetDirectoriesFromFile(const char* arg)
{
  // Check if the argument refers to a CMakeCache.txt or
  // CMakeLists.txt file.
  std::string listPath;
  std::string cachePath;
  bool argIsFile = false;
  if(cmSystemTools::FileIsDirectory(arg))
    {
    std::string path = cmSystemTools::CollapseFullPath(arg);
    cmSystemTools::ConvertToUnixSlashes(path);
    std::string cacheFile = path;
    cacheFile += "/CMakeCache.txt";
    std::string listFile = path;
    listFile += "/CMakeLists.txt";
    if(cmSystemTools::FileExists(cacheFile.c_str()))
      {
      cachePath = path;
      }
    if(cmSystemTools::FileExists(listFile.c_str()))
      {
      listPath = path;
      }
    }
  else if(cmSystemTools::FileExists(arg))
    {
    argIsFile = true;
    std::string fullPath = cmSystemTools::CollapseFullPath(arg);
    std::string name = cmSystemTools::GetFilenameName(fullPath.c_str());
    name = cmSystemTools::LowerCase(name);
    if(name == "cmakecache.txt")
      {
      cachePath = cmSystemTools::GetFilenamePath(fullPath.c_str());
      }
    else if(name == "cmakelists.txt")
      {
      listPath = cmSystemTools::GetFilenamePath(fullPath.c_str());
      }
    }
  else
    {
    // Specified file or directory does not exist.  Try to set things
    // up to produce a meaningful error message.
    std::string fullPath = cmSystemTools::CollapseFullPath(arg);
    std::string name = cmSystemTools::GetFilenameName(fullPath.c_str());
    name = cmSystemTools::LowerCase(name);
    if(name == "cmakecache.txt" || name == "cmakelists.txt")
      {
      argIsFile = true;
      listPath = cmSystemTools::GetFilenamePath(fullPath.c_str());
      }
    else
      {
      listPath = fullPath;
      }
    }

  // If there is a CMakeCache.txt file, use its settings.
  if(cachePath.length() > 0)
    {
    cmCacheManager* cachem = this->GetCacheManager();
    cmCacheManager::CacheIterator it = cachem->NewIterator();
    if(cachem->LoadCache(cachePath.c_str()) && it.Find("CMAKE_HOME_DIRECTORY"))
      {
      this->SetHomeOutputDirectory(cachePath.c_str());      
      this->SetStartOutputDirectory(cachePath.c_str());      
      this->SetHomeDirectory(it.GetValue());
      this->SetStartDirectory(it.GetValue());
      return;
      }
    }
  
  // If there is a CMakeLists.txt file, use it as the source tree.
  if(listPath.length() > 0)
    {
    this->SetHomeDirectory(listPath.c_str());
    this->SetStartDirectory(listPath.c_str());
    
    if(argIsFile)
      {
      // Source CMakeLists.txt file given.  It was probably dropped
      // onto the executable in a GUI.  Default to an in-source build.
      this->SetHomeOutputDirectory(listPath.c_str());      
      this->SetStartOutputDirectory(listPath.c_str());      
      }
    else
      {
      // Source directory given on command line.  Use current working
      // directory as build tree.
      std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
      this->SetHomeOutputDirectory(cwd.c_str());
      this->SetStartOutputDirectory(cwd.c_str());      
      }
    return;
    }
  
  // We didn't find a CMakeLists.txt or CMakeCache.txt file from the
  // argument.  Assume it is the path to the source tree, and use the
  // current working directory as the build tree.
  std::string full = cmSystemTools::CollapseFullPath(arg);
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  this->SetHomeDirectory(full.c_str());
  this->SetStartDirectory(full.c_str());
  this->SetHomeOutputDirectory(cwd.c_str());
  this->SetStartOutputDirectory(cwd.c_str());      
}

// at the end of this CMAKE_ROOT and CMAKE_COMMAND should be added to the cache
int cmake::AddCMakePaths(const char *arg0)
{
  // Find our own executable.
  std::vector<cmStdString> failures;
  std::string cMakeSelf = arg0;
  cmSystemTools::ConvertToUnixSlashes(cMakeSelf);
  failures.push_back(cMakeSelf);
  cMakeSelf = cmSystemTools::FindProgram(cMakeSelf.c_str());
  if(!cmSystemTools::FileExists(cMakeSelf.c_str()))
    {
#ifdef CMAKE_BUILD_DIR
  std::string intdir = ".";
#ifdef  CMAKE_INTDIR
  intdir = CMAKE_INTDIR;
#endif
  cMakeSelf = CMAKE_BUILD_DIR;
  cMakeSelf += "/bin/";
  cMakeSelf += intdir;
  cMakeSelf += "/cmake";
  cMakeSelf += cmSystemTools::GetExecutableExtension();
#endif
    }
#ifdef CMAKE_PREFIX
  if(!cmSystemTools::FileExists(cMakeSelf.c_str()))
    {
    failures.push_back(cMakeSelf);
    cMakeSelf = CMAKE_PREFIX "/bin/cmake";
    }
#endif
  if(!cmSystemTools::FileExists(cMakeSelf.c_str()))
    {
    failures.push_back(cMakeSelf);
    cmOStringStream msg;
    msg << "CMAKE can not find the command line program cmake.\n";
    msg << "  argv[0] = \"" << arg0 << "\"\n";
    msg << "  Attempted paths:\n";
    std::vector<cmStdString>::iterator i;
    for(i=failures.begin(); i != failures.end(); ++i)
      {
      msg << "    \"" << i->c_str() << "\"\n";
      }
    cmSystemTools::Error(msg.str().c_str());
    return 0;
    }
  // Save the value in the cache
  this->m_CacheManager->AddCacheEntry
    ("CMAKE_COMMAND",cMakeSelf.c_str(), "Path to CMake executable.",
     cmCacheManager::INTERNAL);

  // Find and save the command to edit the cache
  std::string editCacheCommand = cmSystemTools::GetFilenamePath(cMakeSelf) +
    "/ccmake" + cmSystemTools::GetFilenameExtension(cMakeSelf);
  if( !cmSystemTools::FileExists(editCacheCommand.c_str()))
    {
    editCacheCommand = cmSystemTools::GetFilenamePath(cMakeSelf) +
      "/CMakeSetup" + cmSystemTools::GetFilenameExtension(cMakeSelf);
    }
  std::string ctestCommand = cmSystemTools::GetFilenamePath(cMakeSelf) +
    "/ctest" + cmSystemTools::GetFilenameExtension(cMakeSelf);
  if(cmSystemTools::FileExists(ctestCommand.c_str()))
    {
    this->m_CacheManager->AddCacheEntry
      ("CMAKE_CTEST_COMMAND", ctestCommand.c_str(),
       "Path to ctest program executable.", cmCacheManager::INTERNAL);
    }
  if(cmSystemTools::FileExists(editCacheCommand.c_str()))
    {
    this->m_CacheManager->AddCacheEntry
      ("CMAKE_EDIT_COMMAND", editCacheCommand.c_str(),
       "Path to cache edit program executable.", cmCacheManager::INTERNAL);
    }
  
  // do CMAKE_ROOT, look for the environment variable first
  std::string cMakeRoot;
  std::string modules;
  if (getenv("CMAKE_ROOT"))
    {
    cMakeRoot = getenv("CMAKE_ROOT");
    modules = cMakeRoot + "/Modules/CMake.cmake";
    }
  if(!cmSystemTools::FileExists(modules.c_str()))
    {
    // next try exe/..
    cMakeRoot  = cmSystemTools::GetProgramPath(cMakeSelf.c_str());
    std::string::size_type slashPos = cMakeRoot.rfind("/");
    if(slashPos != std::string::npos)      
      {
      cMakeRoot = cMakeRoot.substr(0, slashPos);
      }
    // is there no Modules direcory there?
    modules = cMakeRoot + "/Modules/CMake.cmake"; 
    }
  
  if (!cmSystemTools::FileExists(modules.c_str()))
    {
    // try exe/../share/cmake
    cMakeRoot += CMAKE_DATA_DIR;
    modules = cMakeRoot + "/Modules/CMake.cmake";
    }
#ifdef CMAKE_ROOT_DIR
  if (!cmSystemTools::FileExists(modules.c_str()))
    {
    // try compiled in root directory
    cMakeRoot = CMAKE_ROOT_DIR;
    modules = cMakeRoot + "/Modules/CMake.cmake";
    }
#endif
#ifdef CMAKE_PREFIX
  if (!cmSystemTools::FileExists(modules.c_str()))
    {
    // try compiled in install prefix
    cMakeRoot = CMAKE_PREFIX CMAKE_DATA_DIR;
    modules = cMakeRoot + "/Modules/CMake.cmake";
    }
#endif
  if (!cmSystemTools::FileExists(modules.c_str()))
    {
    // try 
    cMakeRoot  = cmSystemTools::GetProgramPath(cMakeSelf.c_str());
    cMakeRoot += CMAKE_DATA_DIR;
    modules = cMakeRoot +  "/Modules/CMake.cmake";
    }
  if(!cmSystemTools::FileExists(modules.c_str()))
    {
    // next try exe
    cMakeRoot  = cmSystemTools::GetProgramPath(cMakeSelf.c_str());
    // is there no Modules direcory there?
    modules = cMakeRoot + "/Modules/CMake.cmake"; 
    }
  if (!cmSystemTools::FileExists(modules.c_str()))
    {
    // couldn't find modules
    cmSystemTools::Error("Could not find CMAKE_ROOT !!!\n"
                         "CMake has most likely not been installed correctly.\n"
                         "Modules directory not found in\n",
                         cMakeRoot.c_str());
    return 0;
    }
  this->m_CacheManager->AddCacheEntry
    ("CMAKE_ROOT", cMakeRoot.c_str(),
     "Path to CMake installation.", cmCacheManager::INTERNAL);

#ifdef _WIN32
  std::string comspec = "cmw9xcom.exe";
  cmSystemTools::SetWindows9xComspecSubstitute(comspec.c_str());
#endif
  return 1;
}



void CMakeCommandUsage(const char* program)
{
  cmOStringStream errorStream;

#ifdef CMAKE_BUILD_WITH_CMAKE
  errorStream 
    << "cmake version " << cmVersion::GetCMakeVersion() << "\n";
#else
  errorStream 
    << "cmake bootstrap\n";
#endif

  errorStream 
    << "Usage: " << program << " -E [command] [arguments ...]\n"
    << "Available commands: \n"
    << "  chdir dir cmd [args]... - run command in a given directory\n"
    << "  copy file destination   - copy file to destination (either file or directory)\n"
    << "  copy_if_different in-file out-file   - copy file if input has changed\n"
    << "  copy_directory source destination    - copy directory 'source' content to directory 'destination'\n"
    << "  echo [string]...        - displays arguments as text\n"
    << "  remove file1 file2 ...  - remove the file(s)\n"
    << "  time command [args] ... - run command and return elapsed time\n";
#if defined(_WIN32) && !defined(__CYGWIN__)
  errorStream
    << "  write_regv key value    - write registry value\n"
    << "  delete_regv key         - delete registry value\n"
    << "  comspec                 - on windows 9x use this for RunCommand\n";
#endif

  cmSystemTools::Error(errorStream.str().c_str());
}

int cmake::CMakeCommand(std::vector<std::string>& args)
{
  if (args.size() > 1)
    {
    // Copy file
    if (args[1] == "copy" && args.size() == 4)
      {
      if(!cmSystemTools::cmCopyFile(args[2].c_str(), args[3].c_str()))
        {
        std::cerr << "Error copying file \"" << args[2].c_str()
                  << "\" to \"" << args[3].c_str() << "\".\n";
        return 1;
        }
      return 0;
      }

    // Copy file if different.
    if (args[1] == "copy_if_different" && args.size() == 4)
      {
      if(!cmSystemTools::CopyFileIfDifferent(args[2].c_str(), args[3].c_str()))
        {
        std::cerr << "Error copying file (if different) from \""
                  << args[2].c_str() << "\" to \"" << args[3].c_str()
                  << "\".\n";
        return 1;
        }
      return 0;
      }

    // Copy directory content
    if (args[1] == "copy_directory" && args.size() == 4)
      {
      if(!cmSystemTools::CopyADirectory(args[2].c_str(), args[3].c_str()))
        {
        std::cerr << "Error copying directory from \""
                  << args[2].c_str() << "\" to \"" << args[3].c_str()
                  << "\".\n";
        return 1;
        }
      return 0;
      }

    // Echo string
    else if (args[1] == "echo" )
      {
      unsigned int cc;
      for ( cc = 2; cc < args.size(); cc ++ )
        {
        std::cout << args[cc] << " ";
        }
      std::cout << std::endl;
      return 0;
      }

    // Remove file
    else if (args[1] == "remove" && args.size() > 2)
      {
      for (std::string::size_type cc = 2; cc < args.size(); cc ++)
        {
        if(args[cc] != "-f")
          {
          if(args[cc] == "\\-f")
            {
            args[cc] = "-f";
            }
          cmSystemTools::RemoveFile(args[cc].c_str());
          }
        }
      return 0;
      }

    // Clock command
    else if (args[1] == "time" && args.size() > 2)
      {
      std::string command = args[2];
      for (std::string::size_type cc = 3; cc < args.size(); cc ++)
        {
        command += " ";
        command += args[cc];
        }

      clock_t clock_start, clock_finish;
      time_t time_start, time_finish;

      time(&time_start);
      clock_start = clock();
      
      cmSystemTools::RunSingleCommand(command.c_str());

      clock_finish = clock();
      time(&time_finish);

      double clocks_per_sec = (double)CLOCKS_PER_SEC;
      std::cout << "Elapsed time: " 
                << (long)(time_finish - time_start) << " s. (time)"
                << ", " 
                << (double)(clock_finish - clock_start) / clocks_per_sec 
                << " s. (clock)"
                << "\n";
      return 0;
    }

    // Clock command
    else if (args[1] == "chdir" && args.size() >= 4)
      {
      std::string directory = args[2];
      unsigned pos = 3;
      if(!cmSystemTools::FileExists(directory.c_str()))
        {
        directory += " ";
        directory += args[3];
        if(!cmSystemTools::FileExists(directory.c_str()))
          {
          cmSystemTools::Error("Directory does not exist for chdir command (try1): ", args[2].c_str());
          cmSystemTools::Error("Directory does not exist for chdir command (try2): ", directory.c_str());
          }
        pos = 4;
        }
      
      std::string command = "\"";
      command += args[pos];
      command += "\"";
      for (std::string::size_type cc = pos+1; cc < args.size(); cc ++)
        {
        command += " \"";
        command += args[cc];
        command += "\"";
        }
      int retval = 0;
      int timeout = 0;
      if ( cmSystemTools::RunSingleCommand(command.c_str(), 0, &retval, 
                                           directory.c_str(), true, timeout) )
        {
        return retval;
        }        

      return 1;
      }

    // Internal CMake shared library support.
    else if (args[1] == "cmake_symlink_library" && args.size() == 5)
      {
      int result = 0;
      std::string realName = args[2];
      std::string soName = args[3];
      std::string name = args[4];
      if(soName != realName)
        {
        std::string fname = cmSystemTools::GetFilenameName(realName);
        if(cmSystemTools::FileExists(soName.c_str()))
          {
          cmSystemTools::RemoveFile(soName.c_str());
          }
        if(!cmSystemTools::CreateSymlink(fname.c_str(), soName.c_str()))
          {
          result = 1;
          }
        }
      if(name != soName)
        {
        std::string fname = cmSystemTools::GetFilenameName(soName);
        if(cmSystemTools::FileExists(soName.c_str()))
          {
          cmSystemTools::RemoveFile(name.c_str());
          }
        if(!cmSystemTools::CreateSymlink(fname.c_str(), name.c_str()))
          {
          result = 1;
          }
        }
      return result;
      }

#ifdef CMAKE_BUILD_WITH_CMAKE
    // Internal CMake dependency scanning support.
    else if (args[1] == "cmake_depends" && args.size() >= 5)
      {
      return cmLocalUnixMakefileGenerator2::ScanDependencies(args)? 0 : 1;
      }
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
    // Write registry value
    else if (args[1] == "write_regv" && args.size() > 3)
      {
      return cmSystemTools::WriteRegistryValue(args[2].c_str(), 
                                               args[3].c_str()) ? 0 : 1;
      }

    // Delete registry value
    else if (args[1] == "delete_regv" && args.size() > 2)
      {
      return cmSystemTools::DeleteRegistryValue(args[2].c_str()) ? 0 : 1;
      }
    // Remove file
    else if (args[1] == "comspec" && args.size() > 2)
      {
      unsigned int cc;
      std::string command = args[2];
      for ( cc = 3; cc < args.size(); cc ++ )
        {
        command += " " + args[cc];
        }
      return cmWin32ProcessExecution::Windows9xHack(command.c_str());
      }
#endif
    }

  ::CMakeCommandUsage(args[0].c_str());
  return 1;
}

void cmake::GetRegisteredGenerators(std::vector<std::string>& names)
{
  for(RegisteredGeneratorsMap::const_iterator i = m_Generators.begin();
      i != m_Generators.end(); ++i)
    {
    names.push_back(i->first);
    }
}

cmGlobalGenerator* cmake::CreateGlobalGenerator(const char* name)
{
  RegisteredGeneratorsMap::const_iterator i = m_Generators.find(name);
  if(i != m_Generators.end())
    {
    cmGlobalGenerator* generator = (i->second)();
    generator->SetCMakeInstance(this);
    return generator;
    }
  else
    {
    return 0;
    }
}

void cmake::SetHomeDirectory(const char* dir) 
{
  m_cmHomeDirectory = dir;
  cmSystemTools::ConvertToUnixSlashes(m_cmHomeDirectory);
}

void cmake::SetHomeOutputDirectory(const char* lib)
{
  m_HomeOutputDirectory = lib;
  cmSystemTools::ConvertToUnixSlashes(m_HomeOutputDirectory);
}

void cmake::SetGlobalGenerator(cmGlobalGenerator *gg)
{
  // delete the old generator
  if (m_GlobalGenerator)
    {
    delete m_GlobalGenerator;
    // restore the original environment variables CXX and CC
    // Restor CC
    std::string env = "CC=";
    if(m_CCEnvironment.size())
      {
      env += m_CCEnvironment;
      }
    cmSystemTools::PutEnv(env.c_str());
    env = "CXX=";
    if(m_CXXEnvironment.size())
      {
      env += m_CXXEnvironment;
      }
    cmSystemTools::PutEnv(env.c_str());
    }

  // set the new
  m_GlobalGenerator = gg;
  // set the global flag for unix style paths on cmSystemTools as 
  // soon as the generator is set.  This allows gmake to be used
  // on windows.
  cmSystemTools::SetForceUnixPaths(m_GlobalGenerator->GetForceUnixPaths());
  // Save the environment variables CXX and CC
  const char* cxx = getenv("CXX");
  const char* cc = getenv("CC");
  if(cxx)
    {
    m_CXXEnvironment = cxx;
    }
  else
    {
    m_CXXEnvironment = "";
    }
  if(cc)
    {
    m_CCEnvironment = cc;
    }
  else
    {
    m_CCEnvironment = "";
    }
  // set the cmake instance just to be sure
  gg->SetCMakeInstance(this);
}

int cmake::DoPreConfigureChecks()
{
  // Make sure the Start directory contains a CMakeLists.txt file.
  std::string srcList = this->GetHomeDirectory();
  srcList += "/CMakeLists.txt";
  if(!cmSystemTools::FileExists(srcList.c_str()))
    {
    cmOStringStream err;
    if(cmSystemTools::FileIsDirectory(this->GetHomeDirectory()))
      {
      err << "The source directory \"" << this->GetHomeDirectory()
          << "\" does not appear to contain CMakeLists.txt.\n";
      }
    else if(cmSystemTools::FileExists(this->GetHomeDirectory()))
      {
      err << "The source directory \"" << this->GetHomeDirectory()
          << "\" is a file, not a directory.\n";
      }
    else
      {
      err << "The source directory \"" << this->GetHomeDirectory()
          << "\" does not exist.\n";
      }
    err << "Specify --help for usage, or press the help button on the CMake GUI.";
    cmSystemTools::Error(err.str().c_str());
    return -2;
    }
  
  // do a sanity check on some values
  if(m_CacheManager->GetCacheValue("CMAKE_HOME_DIRECTORY"))
    {
    std::string cacheStart = 
      m_CacheManager->GetCacheValue("CMAKE_HOME_DIRECTORY");
    cacheStart += "/CMakeLists.txt";
    std::string currentStart = this->GetHomeDirectory();
    currentStart += "/CMakeLists.txt";
    if(!cmSystemTools::SameFile(cacheStart.c_str(), currentStart.c_str()))
      {
      std::string message = "The source \"";
      message += currentStart;
      message += "\" does not match the source \"";
      message += cacheStart;
      message += "\" used to generate cache.  ";
      message += "Re-run cmake with a different source directory.";
      cmSystemTools::Error(message.c_str());
      return -2;
      }
    }
  else
    {
    return 0;
    }
  return 1;
}

int cmake::Configure()
{
  // Construct right now our path conversion table before it's too late:
  this->UpdateConversionPathTable();

  int res = 0;
  if ( !m_ScriptMode )
    {
    res = this->DoPreConfigureChecks();
    }
  if ( res < 0 )
    {
    return -2;
    }
  if ( !res )
    {
    m_CacheManager->AddCacheEntry("CMAKE_HOME_DIRECTORY", 
                                  this->GetHomeDirectory(),
                                  "Start directory with the top level CMakeLists.txt file for this project",
                                  cmCacheManager::INTERNAL);
    }
  
  // set the default BACKWARDS compatibility to the current version
  if(!m_CacheManager->GetCacheValue("CMAKE_BACKWARDS_COMPATIBILITY"))
    {
    char ver[256];
    sprintf(ver,"%i.%i",cmMakefile::GetMajorVersion(),
            cmMakefile::GetMinorVersion());
    this->m_CacheManager->AddCacheEntry
      ("CMAKE_BACKWARDS_COMPATIBILITY",ver, 
       "For backwards compatibility, what version of CMake commands and syntax should this version of CMake allow.",
       cmCacheManager::STRING);
    }  
  
  // no generator specified on the command line
  if(!m_GlobalGenerator)
    {
    const char* genName = m_CacheManager->GetCacheValue("CMAKE_GENERATOR");
    if(genName)
      {
      m_GlobalGenerator = this->CreateGlobalGenerator(genName);
      }
    if(m_GlobalGenerator)
      {
      // set the global flag for unix style paths on cmSystemTools as 
      // soon as the generator is set.  This allows gmake to be used
      // on windows.
      cmSystemTools::SetForceUnixPaths(m_GlobalGenerator->GetForceUnixPaths());
      }
    else
      {
#if defined(__BORLANDC__) && defined(_WIN32)
      this->SetGlobalGenerator(new cmGlobalBorlandMakefileGenerator);
#elif defined(_WIN32) && !defined(__CYGWIN__)  
      std::string installedCompiler;
      std::string mp = "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup;Dbghelp_path]";
      cmSystemTools::ExpandRegistryValues(mp);
      if (!(mp == "/registry"))
        {
        installedCompiler = "Visual Studio 8 2005";
        }
      else
        {
        mp = "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.1;InstallDir]";
        cmSystemTools::ExpandRegistryValues(mp);
        if (!(mp == "/registry"))
          {
          installedCompiler = "Visual Studio 7 .NET 2003";
          }
        else
          {
          mp = "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\7.0;InstallDir]";
          cmSystemTools::ExpandRegistryValues(mp);
          if (!(mp == "/registry"))
            {
            installedCompiler = "Visual Studio 7";
            }
          else
            {
            installedCompiler = "Visual Studio 6";
            }
          }
        }
      cmGlobalGenerator* gen = this->CreateGlobalGenerator(installedCompiler.c_str());
      if(!gen)
        {
        gen = new cmGlobalNMakeMakefileGenerator;
        }
      this->SetGlobalGenerator(gen);
#else
      this->SetGlobalGenerator(new cmGlobalUnixMakefileGenerator);
#endif
      }
    if(!m_GlobalGenerator)
      {
      cmSystemTools::Error("Could not create generator");
      return -1;
      }
    }

  const char* genName = m_CacheManager->GetCacheValue("CMAKE_GENERATOR");
  if(genName)
    {
    if(strcmp(m_GlobalGenerator->GetName(), genName) != 0)
      {
      std::string message = "Error: generator : ";
      message += m_GlobalGenerator->GetName();
      message += "\nDoes not match the generator used previously: ";
      message += genName;
      message +=
        "\nEither remove the CMakeCache.txt file or choose a different"
        " binary directory.";
      cmSystemTools::Error(message.c_str());
      return -2;
      }
    }
  if(!m_CacheManager->GetCacheValue("CMAKE_GENERATOR"))
    {
    m_CacheManager->AddCacheEntry("CMAKE_GENERATOR", m_GlobalGenerator->GetName(),
                                  "Name of generator.",
                                  cmCacheManager::INTERNAL);
    }

  // reset any system configuration information, except for when we are
  // InTryCompile. With TryCompile the system info is taken from the parent's
  // info to save time
  if (!m_InTryCompile)
    {
    m_GlobalGenerator->ClearEnabledLanguages();
    }
  
  this->CleanupWrittenFiles();

  // actually do the configure
  m_GlobalGenerator->Configure();
  
  // Before saving the cache
  // if the project did not define one of the entries below, add them now
  // so users can edit the values in the cache:
  // LIBRARY_OUTPUT_PATH
  // EXECUTABLE_OUTPUT_PATH
  if(!m_CacheManager->GetCacheValue("LIBRARY_OUTPUT_PATH"))
    {
    m_CacheManager->AddCacheEntry("LIBRARY_OUTPUT_PATH", "",
                                  "Single output directory for building all libraries.",
                                  cmCacheManager::PATH);
    } 
  if(!m_CacheManager->GetCacheValue("EXECUTABLE_OUTPUT_PATH"))
    {
    m_CacheManager->AddCacheEntry("EXECUTABLE_OUTPUT_PATH", "",
                                  "Single output directory for building all executables.",
                                  cmCacheManager::PATH);
    }  
  if(!m_CacheManager->GetCacheValue("CMAKE_USE_RELATIVE_PATHS"))
    {
    m_CacheManager->AddCacheEntry("CMAKE_USE_RELATIVE_PATHS", false,
                                  "If true, cmake will use relative paths in makefiles and projects.");
    cmCacheManager::CacheIterator it =
      m_CacheManager->GetCacheIterator("CMAKE_USE_RELATIVE_PATHS");
    if ( !it.PropertyExists("ADVANCED") )
      {
      it.SetProperty("ADVANCED", "1");
      }
    }  
  
  if(cmSystemTools::GetFatalErrorOccured() &&
     (!this->m_CacheManager->GetCacheValue("CMAKE_MAKE_PROGRAM") ||
      cmSystemTools::IsOff(this->m_CacheManager->GetCacheValue("CMAKE_MAKE_PROGRAM"))))
    {
    // We must have a bad generator selection.  Wipe the cache entry so the
    // user can select another.
    m_CacheManager->RemoveCacheEntry("CMAKE_GENERATOR");
    }
  if ( !m_ScriptMode )
    {
    this->m_CacheManager->SaveCache(this->GetHomeOutputDirectory());
    }
  if(cmSystemTools::GetErrorOccuredFlag())
    {
    return -1;
    }
  return 0;
}

bool cmake::CacheVersionMatches()
{
  const char* majv = m_CacheManager->GetCacheValue("CMAKE_CACHE_MAJOR_VERSION");
  const char* minv = m_CacheManager->GetCacheValue("CMAKE_CACHE_MINOR_VERSION");
  const char* relv = m_CacheManager->GetCacheValue("CMAKE_CACHE_RELEASE_VERSION");
  bool cacheSameCMake = false;
  if(majv && 
     atoi(majv) == static_cast<int>(cmMakefile::GetMajorVersion())
     && minv && 
     atoi(minv) == static_cast<int>(cmMakefile::GetMinorVersion())
     && relv && (strcmp(relv, cmMakefile::GetReleaseVersion()) == 0))
    {
    cacheSameCMake = true;
    }
  return cacheSameCMake;
}

void cmake::PreLoadCMakeFiles()
{
  std::string pre_load = this->GetHomeDirectory();
  if ( pre_load.size() > 0 )
    {
    pre_load += "/PreLoad.cmake";
    if ( cmSystemTools::FileExists(pre_load.c_str()) )
      {
      this->ReadListFile(pre_load.c_str());
      }
    }
  pre_load = this->GetHomeOutputDirectory();
  if ( pre_load.size() > 0 )
    {
    pre_load += "/PreLoad.cmake";
    if ( cmSystemTools::FileExists(pre_load.c_str()) )
      {
      this->ReadListFile(pre_load.c_str());
      }
    }
}

// handle a command line invocation
int cmake::Run(const std::vector<std::string>& args, bool noconfigure)
{
  // Process the arguments
  this->SetArgs(args);
  
  // set the cmake command
  m_CMakeCommand = args[0];
  
  if ( !m_ScriptMode )
    {
    // load the cache
    if(this->LoadCache() < 0)
      {
      cmSystemTools::Error("Error executing cmake::LoadCache().  Aborting.\n");
      return -1;
      }
    }

  // Add any cache args
  if ( !this->SetCacheArgs(args) )
    {
    cmSystemTools::Error("Problem processing arguments. Aborting.\n");
    return -1;
    }

  this->PreLoadCMakeFiles();

  std::string systemFile = this->GetHomeOutputDirectory();
  systemFile += "/CMakeSystem.cmake";

  if ( noconfigure )
    {
    return 0;
    }

  int ret = 0;
  // if not local or the cmake version has changed since the last run
  // of cmake, or CMakeSystem.cmake file is not in the root binary
  // directory, run a global generate
  if(m_ScriptMode || !m_Local || !this->CacheVersionMatches() ||
     !cmSystemTools::FileExists(systemFile.c_str()) )
    {
    // Check the state of the build system to see if we need to regenerate.
    if(!this->CheckBuildSystem())
      {
      return 0;
      }

    // If we are doing global generate, we better set start and start
    // output directory to the root of the project.
    std::string oldstartdir = this->GetStartDirectory();
    std::string oldstartoutputdir = this->GetStartOutputDirectory();
    this->SetStartDirectory(this->GetHomeDirectory());
    this->SetStartOutputDirectory(this->GetHomeOutputDirectory());
    bool saveLocalFlag = m_Local;
    m_Local = false;
    ret = this->Configure();
    if (ret || m_ScriptMode)
      {
      return ret;
      }
    ret = this->Generate();
    std::string message = "Build files have been written to: ";
    message += this->GetHomeOutputDirectory();
    this->UpdateProgress(message.c_str(), -1);
    if(ret)
      {
      return ret;
      }
    m_Local = saveLocalFlag;
    this->SetStartDirectory(oldstartdir.c_str());
    this->SetStartOutputDirectory(oldstartoutputdir.c_str());
    }

  // if we are local do the local thing
  if (m_Local)
    {
    ret = this->LocalGenerate();
    }
  return ret;
}

int cmake::Generate()
{
  if(!m_GlobalGenerator)
    {
    return -1;
    }
  m_GlobalGenerator->Generate();
  if(cmSystemTools::GetErrorOccuredFlag())
    {
    return -1;
    }
  return 0;
}

int cmake::LocalGenerate()
{
  // Read in the cache
  m_CacheManager->LoadCache(this->GetHomeOutputDirectory());

  // create the generator based on the cache if it isn't already there
  const char* genName = m_CacheManager->GetCacheValue("CMAKE_GENERATOR");
  if(genName)
    {
    m_GlobalGenerator = this->CreateGlobalGenerator(genName);
    // set the global flag for unix style paths on cmSystemTools as 
    // soon as the generator is set.  This allows gmake to be used
    // on windows.
    cmSystemTools::SetForceUnixPaths(m_GlobalGenerator->GetForceUnixPaths());
    }
  else
    {
    cmSystemTools::Error("Could local Generate called without the GENERATOR being specified in the CMakeCache");
    return -1;
    }
  
  // do the local generate
  m_GlobalGenerator->LocalGenerate();
  if(cmSystemTools::GetErrorOccuredFlag())
    {
    return -1;
    }
  return 0;
}

unsigned int cmake::GetMajorVersion() 
{ 
  return cmMakefile::GetMajorVersion();
}

unsigned int cmake::GetMinorVersion()
{ 
  return cmMakefile::GetMinorVersion();
}

const char *cmake::GetReleaseVersion()
{ 
  return cmMakefile::GetReleaseVersion();
}

void cmake::AddCacheEntry(const char* key, const char* value, 
                          const char* helpString, 
                          int type)
{
  m_CacheManager->AddCacheEntry(key, value, 
                                helpString,
                                cmCacheManager::CacheEntryType(type));
}

const char* cmake::GetCacheDefinition(const char* name) const
{
  return m_CacheManager->GetCacheValue(name);
}

int cmake::DumpDocumentationToFile(std::ostream& f)
{
#ifdef CMAKE_BUILD_WITH_CMAKE
  // Loop over all registered commands and print out documentation
  const char *name;
  const char *terse;
  const char *full;
  char tmp[1024];
  sprintf(tmp,"Version %d.%d (%s)", cmake::GetMajorVersion(),
          cmake::GetMinorVersion(), cmVersion::GetReleaseVersion().c_str());
  f << "<html>\n";
  f << "<h1>Documentation for commands of CMake " << tmp << "</h1>\n";
  f << "<ul>\n";
  for(RegisteredCommandsMap::iterator j = m_Commands.begin();
      j != m_Commands.end(); ++j)
    {
    name = (*j).second->GetName();
    terse = (*j).second->GetTerseDocumentation();
    full = (*j).second->GetFullDocumentation();
    f << "<li><b>" << name << "</b> - " << terse << std::endl
      << "<br><i>Usage:</i> " << full << "</li>" << std::endl << std::endl;
    }
  f << "</ul></html>\n";
#else
  (void)f;
#endif
  return 1;
}

void cmake::AddDefaultCommands()
{
  std::list<cmCommand*> commands;
  GetBootstrapCommands(commands);
  GetPredefinedCommands(commands);
  for(std::list<cmCommand*>::iterator i = commands.begin();
      i != commands.end(); ++i)
    {
    this->AddCommand(*i);
    }
}

void cmake::AddDefaultGenerators()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  m_Generators[cmGlobalVisualStudio6Generator::GetActualName()] =
    &cmGlobalVisualStudio6Generator::New;
#if !defined(__MINGW32__)
  m_Generators[cmGlobalVisualStudio7Generator::GetActualName()] =
    &cmGlobalVisualStudio7Generator::New;
  m_Generators[cmGlobalVisualStudio71Generator::GetActualName()] =
    &cmGlobalVisualStudio71Generator::New;
  m_Generators[cmGlobalVisualStudio8Generator::GetActualName()] =
    &cmGlobalVisualStudio8Generator::New;
#endif
  m_Generators[cmGlobalBorlandMakefileGenerator::GetActualName()] =
    &cmGlobalBorlandMakefileGenerator::New;
  m_Generators[cmGlobalNMakeMakefileGenerator::GetActualName()] =
    &cmGlobalNMakeMakefileGenerator::New;
#else
# if defined(__APPLE__) && defined(CMAKE_BUILD_WITH_CMAKE)
  m_Generators[cmGlobalCodeWarriorGenerator::GetActualName()] =
    &cmGlobalCodeWarriorGenerator::New;
# endif
#endif
  m_Generators[cmGlobalUnixMakefileGenerator::GetActualName()] =
    &cmGlobalUnixMakefileGenerator::New;
  m_Generators[cmGlobalXCodeGenerator::GetActualName()] =
    &cmGlobalXCodeGenerator::New;
#ifdef CMAKE_USE_KDEVELOP
  m_Generators[cmGlobalKdevelopGenerator::GetActualName()] =
     &cmGlobalKdevelopGenerator::New;
#endif
}

int cmake::LoadCache()
{
  m_CacheManager->LoadCache(this->GetHomeOutputDirectory());

  if (m_CMakeCommand.size() < 2)
    {
    cmSystemTools::Error("cmake command was not specified prior to loading the cache in cmake.cxx");
    return -1;
    }
  
  // setup CMAKE_ROOT and CMAKE_COMMAND
  if(!this->AddCMakePaths(m_CMakeCommand.c_str()))
    {
    return -3;
    }  

  // set the default BACKWARDS compatibility to the current version
  if(!m_CacheManager->GetCacheValue("CMAKE_BACKWARDS_COMPATIBILITY"))
    {
    char ver[256];
    sprintf(ver,"%i.%i",cmMakefile::GetMajorVersion(),
            cmMakefile::GetMinorVersion());
    this->m_CacheManager->AddCacheEntry
      ("CMAKE_BACKWARDS_COMPATIBILITY",ver, 
       "For backwards compatibility, what version of CMake commands and syntax should this version of CMake allow.",
       cmCacheManager::STRING);
    }
  
  return 0;
}

void cmake::SetProgressCallback(ProgressCallback f, void *cd)
{
  m_ProgressCallback = f;
  m_ProgressCallbackClientData = cd;
}

void cmake::UpdateProgress(const char *msg, float prog)
{
  if(m_ProgressCallback && !m_InTryCompile)
    {
    (*m_ProgressCallback)(msg, prog, m_ProgressCallbackClientData);
    return;
    }
}

void cmake::GetCommandDocumentation(std::vector<cmDocumentationEntry>& v) const
{
  for(RegisteredCommandsMap::const_iterator j = m_Commands.begin();
      j != m_Commands.end(); ++j)
    {
    cmDocumentationEntry e =
      {
        (*j).second->GetName(),
        (*j).second->GetTerseDocumentation(),
        (*j).second->GetFullDocumentation()
      };
    v.push_back(e);
    }
  cmDocumentationEntry empty = {0,0,0};
  v.push_back(empty);
}

void cmake::GetGeneratorDocumentation(std::vector<cmDocumentationEntry>& v)
{
  for(RegisteredGeneratorsMap::const_iterator i = m_Generators.begin();
      i != m_Generators.end(); ++i)
    {
    cmDocumentationEntry e;
    cmGlobalGenerator* generator = (i->second)();
    generator->GetDocumentation(e);
    delete generator;
    v.push_back(e);
    }
  cmDocumentationEntry empty = {0,0,0};
  v.push_back(empty);
}

void cmake::AddWrittenFile(const char* file)
{
  m_WrittenFiles.insert(file);
}

bool cmake::HasWrittenFile(const char* file)
{
  return m_WrittenFiles.find(file) != m_WrittenFiles.end();
}

void cmake::CleanupWrittenFiles()
{
  m_WrittenFiles.clear();
}

void cmake::UpdateConversionPathTable()
{
  // Update the path conversion table with any specified file:
  const char* tablepath = 
    m_CacheManager->GetCacheValue("CMAKE_PATH_TRANSLATION_FILE");

  if(tablepath)
    {
    std::ifstream table( tablepath );
    if(!table)
      {
      cmSystemTools::Error("CMAKE_PATH_TRANSLATION_FILE set to ", tablepath, ". CMake can not open file.");
      cmSystemTools::ReportLastSystemError("CMake can not open file.");
      }
    else
      {
      std::string a, b;
      while(!table.eof())
        {
        // two entries per line
        table >> a; table >> b;
        cmSystemTools::AddTranslationPath( a.c_str(), b.c_str());
        }
      }
    }
}

//----------------------------------------------------------------------------
int cmake::CheckBuildSystem()
{
  // This method will check the integrity of the build system if the
  // option was given on the command line.  It reads the given file to
  // determine whether CMake should rerun.  If it does rerun then the
  // generation step will check the integrity of dependencies.  If it
  // does not then we need to check the integrity here.

  // If no file is provided for the check, we have to rerun.
  if(m_CheckBuildSystem.size() == 0)
    {
    return 1;
    }

  // If the file provided does not exist, we have to rerun.
  if(!cmSystemTools::FileExists(m_CheckBuildSystem.c_str()))
    {
    return 1;
    }

  // Read the rerun check file and use it to decide whether to do the
  // global generate.
  cmake cm;
  cmGlobalGenerator gg;
  gg.SetCMakeInstance(&cm);
  std::auto_ptr<cmLocalGenerator> lg(gg.CreateLocalGenerator());
  lg->SetGlobalGenerator(&gg);
  cmMakefile* mf = lg->GetMakefile();
  if(!mf->ReadListFile(0, m_CheckBuildSystem.c_str()) ||
     cmSystemTools::GetErrorOccuredFlag())
    {
    // There was an error reading the file.  Just rerun.
    return 1;
    }

  // Get the set of dependencies and outputs.
  const char* dependsStr = mf->GetDefinition("CMAKE_MAKEFILE_DEPENDS");
  const char* outputsStr = mf->GetDefinition("CMAKE_MAKEFILE_OUTPUTS");
  if(!dependsStr || !outputsStr)
    {
    // Not enough information was provided to do the test.  Just rerun.
    return 1;
    }
  std::vector<std::string> depends;
  std::vector<std::string> outputs;
  cmSystemTools::ExpandListArgument(dependsStr, depends);
  cmSystemTools::ExpandListArgument(outputsStr, outputs);

  // If any output is older than any dependency then rerun.
  for(std::vector<std::string>::iterator dep = depends.begin();
      dep != depends.end(); ++dep)
    {
    for(std::vector<std::string>::iterator out = outputs.begin();
        out != outputs.end(); ++out)
      {
      int result = 0;
      if(!cmSystemTools::FileTimeCompare(out->c_str(), dep->c_str(), &result) ||
         result < 0)
        {
        return 1;
        }
      }
    }

#if defined(CMAKE_BUILD_WITH_CMAKE)
  // We do not need to rerun CMake.  Check dependency integrity.
  cmLocalUnixMakefileGenerator2::CheckDependencies(mf);
#endif

  // No need to rerun.
  return 0;
}
