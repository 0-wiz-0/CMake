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
#include "cmTryRunCommand.h"
#include "cmCacheManager.h"
#include "cmTryCompileCommand.h"

// cmExecutableCommand
bool cmTryRunCommand::InitialPass(std::vector<std::string> const& argv)
{
  if(argv.size() < 4)
    {
    return false;
    }

  // build an arg list for TryCompile and extract the runArgs
  std::vector<std::string> tryCompile;
  std::string outputVariable;
  std::string runArgs;
  unsigned int i;
  for (i = 1; i < argv.size(); ++i)
    {
    if (argv[i] == "ARGS")
      {
      ++i;
      while (i < argv.size() && argv[i] != "COMPILE_DEFINITIONS" &&
             argv[i] != "CMAKE_FLAGS")
        {
        runArgs += " ";
        runArgs += argv[i];
        ++i;
        }
      if (i < argv.size())
        {
        tryCompile.push_back(argv[i]);
        }
      } 
    else 
      {
      tryCompile.push_back(argv[i]);
      if (argv[i] == "OUTPUT_VARIABLE")
        {  
        if ( argv.size() <= (i+1) )
          {
          cmSystemTools::Error(
                               "OUTPUT_VARIABLE specified but there is no variable");
          return false;
          }
        outputVariable = argv[i+1];
        }
      }
    }
  // do the try compile
  int res = cmTryCompileCommand::CoreTryCompileCode(m_Makefile, tryCompile, false);
  
  // now try running the command if it compiled
  std::string binaryDirectory = argv[2] + "/CMakeTmp";
  if (!res)
    {
    int retVal = -1;
    std::string output;
    std::string command;
    command = binaryDirectory;
    command += "/cmTryCompileExec";
    command += cmSystemTools::GetExecutableExtension();
    std::string fullPath;
    if(cmSystemTools::FileExists(command.c_str()))
      {
      fullPath = cmSystemTools::CollapseFullPath(command.c_str());
      }
    else
      {
      command = binaryDirectory;
      command += "/Debug/cmTryCompileExec";
      command += cmSystemTools::GetExecutableExtension();
      if(cmSystemTools::FileExists(command.c_str()))
        {
        fullPath = cmSystemTools::CollapseFullPath(command.c_str());
        }
      else
        {
        cmSystemTools::Error("Unable to find executable for TRY_RUN",
                             command.c_str());
        }
      }
    if (fullPath.size() > 1)
      {
      std::string finalCommand = fullPath;
      finalCommand = cmSystemTools::ConvertToRunCommandPath(fullPath.c_str());
      if(runArgs.size())
        {
        finalCommand += runArgs;
        }
      int timeout = 0;
      bool worked = cmSystemTools::RunSingleCommand(finalCommand.c_str(),
                                                    &output, &retVal,
                                                    0, false, timeout);
      if(outputVariable.size())
        {
        // if the TryCompileCore saved output in this outputVariable then
        // prepend that output to this output
        const char* compileOutput
          = m_Makefile->GetDefinition(outputVariable.c_str());
        if(compileOutput)
          {
          output = std::string(compileOutput) + output;
          }
        m_Makefile->AddDefinition(outputVariable.c_str(), output.c_str());
        }
      // set the run var
      char retChar[1000];
      if(worked)
        {
        sprintf(retChar,"%i",retVal);
        }
      else
        {
        strcpy(retChar, "FAILED_TO_RUN");
        }
      m_Makefile->AddCacheDefinition(argv[0].c_str(), retChar,
                                     "Result of TRY_RUN",
                                     cmCacheManager::INTERNAL);
      }
    }
  
  // if we created a directory etc, then cleanup after ourselves  
  std::string cacheFile = binaryDirectory;
  cacheFile += "/CMakeLists.txt";
  cmListFileCache::GetInstance()->FlushCache(cacheFile.c_str());
  if(!m_Makefile->GetCMakeInstance()->GetDebugTryCompile())
    {
    cmTryCompileCommand::CleanupFiles(binaryDirectory.c_str());
    }
  return true;
}


      
