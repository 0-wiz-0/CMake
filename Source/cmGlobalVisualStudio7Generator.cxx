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
#include "windows.h" // this must be first to define GetCurrentDirectory
#include "cmGlobalVisualStudio7Generator.h"
#include "cmLocalVisualStudio7Generator.h"
#include "cmGeneratedFileStream.h"
#include "cmMakefile.h"
#include "cmake.h"


cmGlobalVisualStudio7Generator::cmGlobalVisualStudio7Generator()
{
  m_FindMakeProgramFile = "CMakeVS7FindMake.cmake";
}


void cmGlobalVisualStudio7Generator::EnableLanguage(std::vector<std::string>const &  lang, 
                                                    cmMakefile *mf)
{
  mf->AddDefinition("CMAKE_CFG_INTDIR","$(IntDir)");
  mf->AddDefinition("CMAKE_GENERATOR_CC", "cl");
  mf->AddDefinition("CMAKE_GENERATOR_CXX", "cl");
  mf->AddDefinition("CMAKE_GENERATOR_RC", "rc");
  mf->AddDefinition("CMAKE_GENERATOR_NO_COMPILER_ENV", "1");
  mf->AddDefinition("CMAKE_GENERATOR_Fortran", "ifort");
  
  // Create list of configurations requested by user's cache, if any.
  this->GenerateConfigurations(mf);
  this->cmGlobalGenerator::EnableLanguage(lang, mf);
}

int cmGlobalVisualStudio7Generator::Build(
  const char *, 
  const char *bindir, 
  const char *projectName,
  const char *targetName,
  std::string *output,
  const char *makeCommandCSTR,
  const char *config,
  bool clean)
{
  // now build the test
  std::string makeCommand = 
    cmSystemTools::ConvertToOutputPath(makeCommandCSTR);
  std::string lowerCaseCommand = makeCommand;
  cmSystemTools::LowerCase(lowerCaseCommand);

  /**
   * Run an executable command and put the stdout in output.
   */
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  cmSystemTools::ChangeDirectory(bindir);

  // if there are spaces in the makeCommand, assume a full path
  // and convert it to a path with no spaces in it as the
  // RunSingleCommand does not like spaces
#if defined(_WIN32) && !defined(__CYGWIN__)      
  if(makeCommand.find(' ') != std::string::npos)
    {
    cmSystemTools::GetShortPath(makeCommand.c_str(), makeCommand);
    }
#endif
  makeCommand += " ";
  makeCommand += projectName;
  makeCommand += ".sln ";
  if(clean)
    {
    makeCommand += "/rebuild ";
    }
  else
    {
    makeCommand += "/build ";
    }

  if(config && strlen(config))
    {
    makeCommand += config;
    }
  else
    {
    makeCommand += "Debug";
    }
  makeCommand += " /project ";

  if (targetName && strlen(targetName))
    {
    makeCommand += targetName;
    }
  else
    {
    makeCommand += "ALL_BUILD";
    }
  
  int retVal;
  int timeout = cmGlobalGenerator::s_TryCompileTimeout;
  if (!cmSystemTools::RunSingleCommand(makeCommand.c_str(), output, &retVal, 
      0, false, timeout))
    {
    cmSystemTools::Error("Generator: execution of devenv failed.");
    // return to the original directory
    cmSystemTools::ChangeDirectory(cwd.c_str());
    return 1;
    }
  *output += makeCommand;
  cmSystemTools::ChangeDirectory(cwd.c_str());
  return retVal;
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator *cmGlobalVisualStudio7Generator::CreateLocalGenerator()
{
  cmLocalGenerator *lg = new cmLocalVisualStudio7Generator;
  lg->SetGlobalGenerator(this);
  return lg;
}


void cmGlobalVisualStudio7Generator::SetupTests()
{
  std::string ctest = 
    m_LocalGenerators[0]->GetMakefile()->GetRequiredDefinition("CMAKE_COMMAND");
  ctest = cmSystemTools::GetFilenamePath(ctest.c_str());
  ctest += "/";
  ctest += "ctest";
  ctest += cmSystemTools::GetExecutableExtension();
  if(!cmSystemTools::FileExists(ctest.c_str()))
    {
    ctest =     
      m_LocalGenerators[0]->GetMakefile()->GetRequiredDefinition("CMAKE_COMMAND");
    ctest = cmSystemTools::GetFilenamePath(ctest.c_str());
    ctest += "/Debug/";
    ctest += "ctest";
    ctest += cmSystemTools::GetExecutableExtension();
    }
  if(!cmSystemTools::FileExists(ctest.c_str()))
    {
    ctest =     
      m_LocalGenerators[0]->GetMakefile()->GetRequiredDefinition("CMAKE_COMMAND");
    ctest = cmSystemTools::GetFilenamePath(ctest.c_str());
    ctest += "/Release/";
    ctest += "ctest";
    ctest += cmSystemTools::GetExecutableExtension();
    }
  // if we found ctest
  if (cmSystemTools::FileExists(ctest.c_str()))
    {
    // Create a full path filename for output Testfile
    std::string fname;
    fname = m_CMakeInstance->GetStartOutputDirectory();
    fname += "/";
    fname += "DartTestfile.txt";
    
    // If the file doesn't exist, then ENABLE_TESTING hasn't been run
    if (cmSystemTools::FileExists(fname.c_str()))
      {
      const char* no_output = 0;
      std::vector<std::string> no_depends;
      std::map<cmStdString, std::vector<cmLocalGenerator*> >::iterator it;
      for(it = m_ProjectMap.begin(); it!= m_ProjectMap.end(); ++it)
        {
        std::vector<cmLocalGenerator*>& gen = it->second;
        // add the ALL_BUILD to the first local generator of each project
        if(gen.size())
          {
          gen[0]->GetMakefile()->
            AddUtilityCommand("RUN_TESTS", false, no_output, no_depends,
                              ctest.c_str(), "-C", "$(IntDir)");
          }
        }
      }
    }
}

void cmGlobalVisualStudio7Generator::GenerateConfigurations(cmMakefile* mf)
{
  // process the configurations
  const char* ct 
    = m_CMakeInstance->GetCacheDefinition("CMAKE_CONFIGURATION_TYPES");
  if ( ct )
    {
    std::string configTypes = ct;
    
    std::string::size_type start = 0;
    std::string::size_type endpos = 0;
    while(endpos != std::string::npos)
      {
      endpos = configTypes.find_first_of(" ;", start);
      std::string config;
      std::string::size_type len;
      if(endpos != std::string::npos)
        {
        len = endpos - start;
        }
      else
        {
        len = configTypes.size() - start;
        }
      config = configTypes.substr(start, len);
      if(config == "Debug" || config == "Release" ||
         config == "MinSizeRel" || config == "RelWithDebInfo")
        {
        // only add unique configurations
        if(std::find(m_Configurations.begin(),
                     m_Configurations.end(), config) == m_Configurations.end())
          {
          m_Configurations.push_back(config);
          }
        }
      else
        {
        cmSystemTools::Error(
          "Invalid configuration type in CMAKE_CONFIGURATION_TYPES: ",
          config.c_str(),
          " (Valid types are Debug,Release,MinSizeRel,RelWithDebInfo)");
        }
      start = endpos+1;
      }
    }
  if(m_Configurations.size() == 0)
    {
    m_Configurations.push_back("Debug");
    m_Configurations.push_back("Release");
    }
  
  // Reset the entry to have a semi-colon separated list.
  std::string configs = m_Configurations[0];
  for(unsigned int i=1; i < m_Configurations.size(); ++i)
    {
    configs += ";";
    configs += m_Configurations[i];
    }
  
  mf->AddCacheDefinition(
    "CMAKE_CONFIGURATION_TYPES",
    configs.c_str(),
    "Semicolon separated list of supported configuration types, "
    "only supports Debug, Release, MinSizeRel, and RelWithDebInfo, "
    "anything else will be ignored.",
    cmCacheManager::STRING);
}

void cmGlobalVisualStudio7Generator::Generate()
{
  // add a special target that depends on ALL projects for easy build
  // of one configuration only.
  const char* no_output = 0;
  std::vector<std::string> no_depends;
  std::map<cmStdString, std::vector<cmLocalGenerator*> >::iterator it;
  for(it = m_ProjectMap.begin(); it!= m_ProjectMap.end(); ++it)
    {
    std::vector<cmLocalGenerator*>& gen = it->second;
    // add the ALL_BUILD to the first local generator of each project
    if(gen.size())
      {
      gen[0]->GetMakefile()->
        AddUtilityCommand("ALL_BUILD", false, no_output, no_depends,
                          "echo", "Build all projects");
      std::string cmake_command = 
        m_LocalGenerators[0]->GetMakefile()->GetRequiredDefinition("CMAKE_COMMAND");
      gen[0]->GetMakefile()->
        AddUtilityCommand("INSTALL", false, no_output, no_depends,
                          cmake_command.c_str(),
                          "-DBUILD_TYPE=$(IntDir)", "-P", "cmake_install.cmake");
      }
    }
  
  // add the Run Tests command
  this->SetupTests();
  
  // first do the superclass method
  this->cmGlobalGenerator::Generate();
  
  // Now write out the DSW
  this->OutputSLNFile();
}

void cmGlobalVisualStudio7Generator::OutputSLNFile(cmLocalGenerator* root,
                                                   std::vector<cmLocalGenerator*>& generators)
{
  if(generators.size() == 0)
    {
    return;
    }
  std::string fname = root->GetMakefile()->GetStartOutputDirectory();
  fname += "/";
  fname += root->GetMakefile()->GetProjectName();
  fname += ".sln";
  cmGeneratedFileStream fout(fname.c_str());
  fout.SetCopyIfDifferent(true);
  if(!fout)
    {
    return;
    }
  this->WriteSLNFile(fout, root, generators);
}

// output the SLN file
void cmGlobalVisualStudio7Generator::OutputSLNFile()
{ 
  std::map<cmStdString, std::vector<cmLocalGenerator*> >::iterator it;
  for(it = m_ProjectMap.begin(); it!= m_ProjectMap.end(); ++it)
    {
    this->OutputSLNFile(it->second[0], it->second);
    }
}


// Write a SLN file to the stream
void cmGlobalVisualStudio7Generator::WriteSLNFile(std::ostream& fout,
                                                  cmLocalGenerator* root,
                                                  std::vector<cmLocalGenerator*>& generators)
{
  // Write out the header for a SLN file
  this->WriteSLNHeader(fout);
  
  // Get the start directory with the trailing slash
  std::string rootdir = root->GetMakefile()->GetStartOutputDirectory();
  rootdir += "/";
  bool doneAllBuild = false;
  bool doneRunTests = false;
  bool doneInstall  = false;
  
  // For each cmMakefile, create a VCProj for it, and
  // add it to this SLN file
  unsigned int i;
  for(i = 0; i < generators.size(); ++i)
    {
    if(this->IsExcluded(root, generators[i]))
      {
      continue;
      }
    cmMakefile* mf = generators[i]->GetMakefile();

    // Get the source directory from the makefile
    std::string dir = mf->GetStartOutputDirectory();
    // remove the home directory and / from the source directory
    // this gives a relative path 
    cmSystemTools::ReplaceString(dir, rootdir.c_str(), "");

    // Get the list of create dsp files names from the cmVCProjWriter, more
    // than one dsp could have been created per input CMakeLists.txt file
    // for each target
    std::vector<std::string> dspnames = 
      static_cast<cmLocalVisualStudio7Generator *>(generators[i])
      ->GetCreatedProjectNames();
    cmTargets &tgts = generators[i]->GetMakefile()->GetTargets();
    cmTargets::iterator l = tgts.begin();
    for(std::vector<std::string>::iterator si = dspnames.begin(); 
        l != tgts.end(); ++l)
      {
      // special handling for the current makefile
      if(mf == generators[0]->GetMakefile())
        {
        dir = "."; // no subdirectory for project generated
        // if this is the special ALL_BUILD utility, then
        // make it depend on every other non UTILITY project.
        // This is done by adding the names to the GetUtilities
        // vector on the makefile
        if(l->first == "ALL_BUILD" && !doneAllBuild)
          {
          unsigned int j;
          for(j = 0; j < generators.size(); ++j)
            {
            const cmTargets &atgts = 
              generators[j]->GetMakefile()->GetTargets();
            for(cmTargets::const_iterator al = atgts.begin();
                al != atgts.end(); ++al)
              {
              if (al->second.IsInAll())
                {
                if (al->second.GetType() == cmTarget::UTILITY)
                  {
                  l->second.AddUtility(al->first.c_str());
                  }
                else
                  {
                  l->second.AddLinkLibrary(al->first,cmTarget::GENERAL);
                  }
                }
              }
            }
          }
        }
      // Write the project into the SLN file
      if (strncmp(l->first.c_str(), "INCLUDE_EXTERNAL_MSPROJECT", 26) == 0)
        {
        cmCustomCommand cc = l->second.GetPostBuildCommands()[0];
        const cmCustomCommandLines& cmds = cc.GetCommandLines();
        std::string project = cmds[0][0];
        std::string location = cmds[0][1];
        this->WriteExternalProject(fout, project.c_str(), location.c_str(), cc.GetDepends());
        }
      else 
        {
        if ((l->second.GetType() != cmTarget::INSTALL_FILES)
            && (l->second.GetType() != cmTarget::INSTALL_PROGRAMS))
          {
          bool skip = false;
          if(l->first == "ALL_BUILD" )
            {
            if(doneAllBuild)
              {
              skip = true;
              }
            else
              {
              doneAllBuild = true;
              }
            }
          if(l->first == "INSTALL")
            {
            if(doneInstall)
              {
              skip = true;
              }
            else
              {
              doneInstall = true;
              }
            }
          if(l->first == "RUN_TESTS")
            {
            if(doneRunTests)
              {
              skip = true;
              }
            else
              {
              doneRunTests = true;
              }
            }
          if(!skip)
            {
            this->WriteProject(fout, si->c_str(), dir.c_str(),l->second);
            }
          ++si;
          }
        }
      }
    }
  fout << "Global\n"
       << "\tGlobalSection(SolutionConfiguration) = preSolution\n";
  
  int c = 0;
  for(std::vector<std::string>::iterator i = m_Configurations.begin();
      i != m_Configurations.end(); ++i)
    {
    fout << "\t\tConfigName." << c << " = " << *i << "\n";
    c++;
    }
  fout << "\tEndGlobalSection\n"
       << "\tGlobalSection(ProjectDependencies) = postSolution\n";

  // loop over again and compute the depends
  for(i = 0; i < generators.size(); ++i)
    {
    cmMakefile* mf = generators[i]->GetMakefile();
    cmLocalVisualStudio7Generator* pg =  
      static_cast<cmLocalVisualStudio7Generator*>(generators[i]);
    // Get the list of create dsp files names from the cmVCProjWriter, more
    // than one dsp could have been created per input CMakeLists.txt file
    // for each target
    std::vector<std::string> dspnames = 
      pg->GetCreatedProjectNames();
    cmTargets &tgts = pg->GetMakefile()->GetTargets();
    cmTargets::iterator l = tgts.begin();
    std::string dir = mf->GetStartDirectory();
    for(std::vector<std::string>::iterator si = dspnames.begin(); 
        l != tgts.end() && si != dspnames.end(); ++l)
      {
       if (strncmp(l->first.c_str(), "INCLUDE_EXTERNAL_MSPROJECT", 26) == 0)
         {
         cmCustomCommand cc = l->second.GetPostBuildCommands()[0];
         const cmCustomCommandLines& cmds = cc.GetCommandLines();
         std::string name = cmds[0][0];
         std::vector<std::string> depends = cc.GetDepends();
         std::vector<std::string>::iterator iter;
         int depcount = 0;
         for(iter = depends.begin(); iter != depends.end(); ++iter)
           {
           fout << "\t\t{" << this->GetGUID(name.c_str()) << "}." << depcount << " = {"
                << this->GetGUID(iter->c_str()) << "}\n";
           depcount++;
           }
         }
       else if ((l->second.GetType() != cmTarget::INSTALL_FILES)
                && (l->second.GetType() != cmTarget::INSTALL_PROGRAMS))
        {
        this->WriteProjectDepends(fout, si->c_str(), dir.c_str(),l->second);
        ++si;
        }
      }
    }
  fout << "\tEndGlobalSection\n";
  fout << "\tGlobalSection(ProjectConfiguration) = postSolution\n";
  // loop over again and compute the depends
  for(i = 0; i < generators.size(); ++i)
    {
    cmMakefile* mf = generators[i]->GetMakefile();
    cmLocalVisualStudio7Generator* pg =  
      static_cast<cmLocalVisualStudio7Generator*>(generators[i]);
    // Get the list of create dsp files names from the cmVCProjWriter, more
    // than one dsp could have been created per input CMakeLists.txt file
    // for each target
    std::vector<std::string> dspnames = 
      pg->GetCreatedProjectNames();
    cmTargets &tgts = pg->GetMakefile()->GetTargets();
    cmTargets::iterator l = tgts.begin();
    std::string dir = mf->GetStartDirectory();
    for(std::vector<std::string>::iterator si = dspnames.begin(); 
        l != tgts.end() && si != dspnames.end(); ++l)
      {
      if(strncmp(l->first.c_str(), "INCLUDE_EXTERNAL_MSPROJECT", 26) == 0)
        {
        cmCustomCommand cc = l->second.GetPostBuildCommands()[0];
        const cmCustomCommandLines& cmds = cc.GetCommandLines();
        std::string name = cmds[0][0];
        this->WriteProjectConfigurations(fout, name.c_str(), l->second.IsInAll());
        }
      else if ((l->second.GetType() != cmTarget::INSTALL_FILES)
          && (l->second.GetType() != cmTarget::INSTALL_PROGRAMS))
        {
        this->WriteProjectConfigurations(fout, si->c_str(), l->second.IsInAll());
        ++si;
        }
      }
    }
  fout << "\tEndGlobalSection\n";

  // Write the footer for the SLN file
  this->WriteSLNFooter(fout);
}


// Write a dsp file into the SLN file,
// Note, that dependencies from executables to 
// the libraries it uses are also done here
void cmGlobalVisualStudio7Generator::WriteProject(std::ostream& fout, 
                               const char* dspname,
                               const char* dir,
                               const cmTarget&)
{
  std::string d = cmSystemTools::ConvertToOutputPath(dir);
  fout << "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"" 
       << dspname << "\", \""
       << d << "\\" << dspname << ".vcproj\", \"{"
       << this->GetGUID(dspname) << "}\"\nEndProject\n";
}



// Write a dsp file into the SLN file,
// Note, that dependencies from executables to 
// the libraries it uses are also done here
void cmGlobalVisualStudio7Generator::WriteProjectDepends(std::ostream& fout, 
                                      const char* dspname,
                                      const char* ,
                                      const cmTarget& target
  )
{
  int depcount = 0;
  // insert Begin Project Dependency  Project_Dep_Name project stuff here 
  if (target.GetType() != cmTarget::STATIC_LIBRARY)
    {
    cmTarget::LinkLibraries::const_iterator j, jend;
    j = target.GetLinkLibraries().begin();
    jend = target.GetLinkLibraries().end();
    for(;j!= jend; ++j)
      {
      if(j->first != dspname)
        {
        // is the library part of this SLN ? If so add dependency
        std::string libPath = j->first + "_CMAKE_PATH";
        const char* cacheValue
          = m_CMakeInstance->GetCacheDefinition(libPath.c_str());
        if(cacheValue && *cacheValue)
          {
          fout << "\t\t{" << this->GetGUID(dspname) << "}." << depcount << " = {"
               << this->GetGUID(j->first.c_str()) << "}\n";
          depcount++;
          }
        }
      }
    }

  std::set<cmStdString>::const_iterator i, end;
  // write utility dependencies.
  i = target.GetUtilities().begin();
  end = target.GetUtilities().end();
  for(;i!= end; ++i)
    {
    if(*i != dspname)
      {
      std::string name = *i;
      if(strncmp(name.c_str(), "INCLUDE_EXTERNAL_MSPROJECT", 26) == 0)
        {
        // kind of weird removing the first 27 letters.
        // my recommendatsions:
        // use cmCustomCommand::GetCommand() to get the project name
        // or get rid of the target name starting with "INCLUDE_EXTERNAL_MSPROJECT_" and use another 
        // indicator/flag somewhere.  These external project names shouldn't conflict with cmake 
        // target names anyways.
        name.erase(name.begin(), name.begin() + 27);
        }
      fout << "\t\t{" << this->GetGUID(dspname) << "}." << depcount << " = {"
           << this->GetGUID(name.c_str()) << "}\n";
      depcount++;
      }
    }
}


// Write a dsp file into the SLN file,
// Note, that dependencies from executables to 
// the libraries it uses are also done here
void 
cmGlobalVisualStudio7Generator::WriteProjectConfigurations(std::ostream& fout, 
                                                           const char* name, 
                                                           bool in_all_build)
{
  std::string guid = this->GetGUID(name);
  for(std::vector<std::string>::iterator i = m_Configurations.begin();
      i != m_Configurations.end(); ++i)
    {
    fout << "\t\t{" << guid << "}." << *i << ".ActiveCfg = " << *i << "|Win32\n";
    if (in_all_build)
      {
      fout << "\t\t{" << guid << "}." << *i << ".Build.0 = " << *i << "|Win32\n";
      }
    }
}



// Write a dsp file into the SLN file,
// Note, that dependencies from executables to 
// the libraries it uses are also done here
void cmGlobalVisualStudio7Generator::WriteExternalProject(std::ostream& fout, 
                               const char* name,
                               const char* location,
                               const std::vector<std::string>&)
{ 
  std::string d = cmSystemTools::ConvertToOutputPath(location);
  fout << "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"" 
       << name << "\", \""
       << d << "\", \"{"
       << this->GetGUID(name)
       << "}\"\n";
  fout << "EndProject\n";
}



// Standard end of dsw file
void cmGlobalVisualStudio7Generator::WriteSLNFooter(std::ostream& fout)
{
  fout << "\tGlobalSection(ExtensibilityGlobals) = postSolution\n"
       << "\tEndGlobalSection\n"
       << "\tGlobalSection(ExtensibilityAddIns) = postSolution\n"
       << "\tEndGlobalSection\n"
       << "EndGlobal\n";
}

  
// ouput standard header for dsw file
void cmGlobalVisualStudio7Generator::WriteSLNHeader(std::ostream& fout)
{
  fout << "Microsoft Visual Studio Solution File, Format Version 7.00\n";
}

std::string cmGlobalVisualStudio7Generator::GetGUID(const char* name)
{
  std::string guidStoreName = name;
  guidStoreName += "_GUID_CMAKE";
  const char* storedGUID = m_CMakeInstance->GetCacheDefinition(guidStoreName.c_str());
  if(storedGUID)
    {
    return std::string(storedGUID);
    }
  cmSystemTools::Error("Internal CMake Error, Could not find GUID for target: ",
                       name);
  return guidStoreName;
}


void cmGlobalVisualStudio7Generator::CreateGUID(const char* name)
{
  std::string guidStoreName = name;
  guidStoreName += "_GUID_CMAKE";
  if(m_CMakeInstance->GetCacheDefinition(guidStoreName.c_str()))
    {
    return;
    }
  std::string ret;
  UUID uid;
  unsigned char *uidstr;
  UuidCreate(&uid);
  UuidToString(&uid,&uidstr);
  ret = reinterpret_cast<char*>(uidstr);
  RpcStringFree(&uidstr);
  ret = cmSystemTools::UpperCase(ret);
  m_CMakeInstance->AddCacheEntry(guidStoreName.c_str(), ret.c_str(), "Stored GUID", 
                                 cmCacheManager::INTERNAL);
}

std::vector<std::string> *cmGlobalVisualStudio7Generator::GetConfigurations()
{
  return &m_Configurations;
};

//----------------------------------------------------------------------------
void cmGlobalVisualStudio7Generator::GetDocumentation(cmDocumentationEntry& entry) const
{
  entry.name = this->GetName();
  entry.brief = "Generates Visual Studio .NET 2002 project files.";
  entry.full = "";
}

// make sure "special" targets have GUID's
void cmGlobalVisualStudio7Generator::Configure()
{
  cmGlobalGenerator::Configure();
  this->CreateGUID("ALL_BUILD");
  this->CreateGUID("INSTALL");
  this->CreateGUID("RUN_TESTS");
}
