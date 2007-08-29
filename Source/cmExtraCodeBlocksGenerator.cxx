/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  Copyright (c) 2004 Alexander Neundorf neundorf@kde.org, All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "cmExtraCodeBlocksGenerator.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmake.h"
#include "cmSourceFile.h"
#include "cmGeneratedFileStream.h"
#include "cmTarget.h"
#include "cmSystemTools.h"

#include <cmsys/SystemTools.hxx>

/* Some useful URLs:
Homepage: 
http://www.codeblocks.org

File format docs:
http://wiki.codeblocks.org/index.php?title=File_formats_description
http://wiki.codeblocks.org/index.php?title=Workspace_file
http://wiki.codeblocks.org/index.php?title=Project_file

Discussion:
http://forums.codeblocks.org/index.php/topic,6789.0.html
*/

//----------------------------------------------------------------------------
void cmExtraCodeBlocksGenerator
::GetDocumentation(cmDocumentationEntry& entry, const char*) const
{
  entry.name = this->GetName();
  entry.brief = "Generates CodeBlocks project files.";
  entry.full =
    "Project files for CodeBlocks will be created in the top directory "
    "and in every subdirectory which features a CMakeLists.txt file "
    "containing a PROJECT() call. "
    "Additionally a hierarchy of makefiles is generated into the "
    "build tree.  The appropriate make program can build the project through "
    "the default make target.  A \"make install\" target is also provided.";
}

cmExtraCodeBlocksGenerator::cmExtraCodeBlocksGenerator()
:cmExternalMakefileProjectGenerator()
{
#if defined(_WIN32)
  this->SupportedGlobalGenerators.push_back("MinGW Makefiles");
// disable until somebody actually tests it:
//  this->SupportedGlobalGenerators.push_back("NMake Makefiles");
//  this->SupportedGlobalGenerators.push_back("MSYS Makefiles");
#endif
  this->SupportedGlobalGenerators.push_back("Unix Makefiles");
}


void cmExtraCodeBlocksGenerator::SetGlobalGenerator(
                                                  cmGlobalGenerator* generator)
{
  cmExternalMakefileProjectGenerator::SetGlobalGenerator(generator);
  cmGlobalUnixMakefileGenerator3* mf = (cmGlobalUnixMakefileGenerator3*)
                                                                     generator;
  mf->SetToolSupportsColor(false);
  mf->SetForceVerboseMakefiles(true);
}

void cmExtraCodeBlocksGenerator::Generate()
{
  // for each sub project in the project create a codeblocks project
  for (std::map<cmStdString, std::vector<cmLocalGenerator*> >::const_iterator
       it = this->GlobalGenerator->GetProjectMap().begin();
      it!= this->GlobalGenerator->GetProjectMap().end();
      ++it)
    {
    // create a project file
    this->CreateProjectFile(it->second);
    }
}


/* create the project file, if it already exists, merge it with the
existing one, otherwise create a new one */
void cmExtraCodeBlocksGenerator::CreateProjectFile(
                                     const std::vector<cmLocalGenerator*>& lgs)
{
  const cmMakefile* mf=lgs[0]->GetMakefile();
  std::string outputDir=mf->GetStartOutputDirectory();
  std::string projectDir=mf->GetHomeDirectory();
  std::string projectName=mf->GetProjectName();

  std::string filename=outputDir+"/";
  filename+=projectName+".cbp";
  std::string sessionFilename=outputDir+"/";
  sessionFilename+=projectName+".layout";

/*  if (cmSystemTools::FileExists(filename.c_str()))
    {
    this->MergeProjectFiles(outputDir, projectDir, filename,
                            cmakeFilePattern, sessionFilename);
    }
  else */
    {
    this->CreateNewProjectFile(lgs, filename);
    }

}


void cmExtraCodeBlocksGenerator
  ::CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                         const std::string& filename)
{
  const cmMakefile* mf=lgs[0]->GetMakefile();
  cmGeneratedFileStream fout(filename.c_str());
  if(!fout)
    {
    return;
    }

  // figure out the compiler
  std::string compiler = this->GetCBCompilerId(mf);
  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");

  fout<<"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n"
        "<CodeBlocks_project_file>\n"
        "   <FileVersion major=\"1\" minor=\"6\" />\n"
        "   <Project>\n"
        "      <Option title=\"" << mf->GetProjectName()<<"\" />\n"
        "      <Option makefile_is_custom=\"1\" />\n"
        "      <Option compiler=\"" << compiler << "\" />\n"
        "      <Build>\n";

  bool installTargetCreated = false;
  bool testTargetCreated = false;
  bool packageTargetCreated = false;
  
  for (std::vector<cmLocalGenerator*>::const_iterator lg=lgs.begin();
       lg!=lgs.end(); lg++)
    {
    cmMakefile* makefile=(*lg)->GetMakefile();
    cmTargets& targets=makefile->GetTargets();
    for (cmTargets::iterator ti = targets.begin();
         ti != targets.end(); ti++)
      {
        switch(ti->second.GetType())
        {
          case cmTarget::GLOBAL_TARGET:
            if ((ti->first=="install") && (installTargetCreated==false)) 
            {
              installTargetCreated=true;
            }
            else if ((ti->first=="package") && (packageTargetCreated==false)) 
            {
              packageTargetCreated=true;
            }
            else if ((ti->first=="test") && (testTargetCreated==false)) 
            {
              testTargetCreated=true;
            }
            else
            {
              break;
            }
          case cmTarget::EXECUTABLE:
          case cmTarget::STATIC_LIBRARY:
          case cmTarget::SHARED_LIBRARY:
          case cmTarget::MODULE_LIBRARY:
            {
            int cbTargetType = this->GetCBTargetType(&ti->second);
            std::string makefileName = makefile->GetStartOutputDirectory();
            makefileName += "/Makefile";
            makefileName = cmSystemTools::ConvertToOutputPath(
                                                         makefileName.c_str());

  fout<<"      <Target title=\"" << ti->first << "\">\n"
        "         <Option output=\"" << ti->second.GetLocation(0) << "\" prefix_auto=\"0\" extension_auto=\"0\" />\n"
        "         <Option working_dir=\"" << makefile->GetStartOutputDirectory() <<"\" />\n"
        "         <Option type=\"" << cbTargetType << "\" />\n"
        "         <Option compiler=\"" << compiler << "\" />\n"
        "         <Compiler>\n";

            // the include directories for this target
            const std::vector<std::string>& incDirs = 
                             ti->second.GetMakefile()->GetIncludeDirectories();
            for(std::vector<std::string>::const_iterator dirIt=incDirs.begin();
                dirIt != incDirs.end();
                ++dirIt)
              {
  fout <<"            <Add directory=\"" << dirIt->c_str() << "\" />\n";
              }

  fout<<"         </Compiler>\n"
        "         <MakeCommands>\n"
        "            <Build command=\"" << this->BuildMakeCommand(make, makefileName.c_str(), ti->first.c_str()) << "\" />\n"
        "            <CompileFile command=\"" << this->BuildMakeCommand(make, makefileName.c_str(), ti->first.c_str()) << "\" />\n"
        "            <Clean command=\"" << this->BuildMakeCommand(make, makefileName.c_str(), "clean") << "\" />\n"
        "            <DistClean command=\"" << this->BuildMakeCommand(make, makefileName.c_str(), "clean") << "\" />\n"
        "         </MakeCommands>\n"
        "      </Target>\n";
            }
            break;
          default:
            break;
        }
      }
    }

  fout<<"      </Build>\n";


  std::map<std::string, std::string> sourceFiles;
  for (std::vector<cmLocalGenerator*>::const_iterator lg=lgs.begin();
       lg!=lgs.end(); lg++)
    {
    cmMakefile* makefile=(*lg)->GetMakefile();
    cmTargets& targets=makefile->GetTargets();
    for (cmTargets::iterator ti = targets.begin();
         ti != targets.end(); ti++)
      {
      switch(ti->second.GetType())
        {
        case cmTarget::EXECUTABLE:
        case cmTarget::STATIC_LIBRARY:
        case cmTarget::SHARED_LIBRARY:
        case cmTarget::MODULE_LIBRARY:
          {
          const std::vector<cmSourceFile*>&sources=ti->second.GetSourceFiles();
          for (std::vector<cmSourceFile*>::const_iterator si=sources.begin();
               si!=sources.end(); si++)
            {
            sourceFiles[(*si)->GetFullPath()] = ti->first;
            }
          }
        default:  // intended fallthrough
          break;
        }
      }
    }

  for (std::map<std::string, std::string>::const_iterator 
       sit=sourceFiles.begin();
       sit!=sourceFiles.end();
       ++sit)
  {
  fout<<"      <Unit filename=\""<<sit->first <<"\">\n"
        "      </Unit>\n";
  }

  fout<<"   </Project>\n"
        "</CodeBlocks_project_file>\n";
}


std::string cmExtraCodeBlocksGenerator::GetCBCompilerId(const cmMakefile* mf)
{
  // figure out which language to use
  // for now care only for C and C++
  std::string compilerIdVar = "CMAKE_CXX_COMPILER_ID";
  cmGlobalGenerator* gg=const_cast<cmGlobalGenerator*>(this->GlobalGenerator);
  if (gg->GetLanguageEnabled("CXX") == false)
    {
    compilerIdVar = "CMAKE_C_COMPILER_ID";
    }

  std::string hostSystemName = mf->GetSafeDefinition("CMAKE_HOST_SYSTEM_NAME");
  std::string systemName = mf->GetSafeDefinition("CMAKE_SYSTEM_NAME");
  std::string compilerId = mf->GetRequiredDefinition(compilerIdVar.c_str());
  std::string compiler = "gcc";
  if (compilerId == "MSVC")
    {
    compiler = "msvc";
    }
  else if (compilerId == "Borland")
    {
    compiler = "bcc";
    }
  else if (compilerId == "SDCC")
    {
    compiler = "sdcc";
    }
  else if (compilerId == "Intel")
    {
    compiler = "icc";
    }
  else if (compilerId == "Watcom")
    {
    compiler = "ow";
    }
  else if (compilerId == "GNU")
    {
    if (hostSystemName == "Windows")
      {
      compiler = "mingw";
      }
    else
      {
      compiler = "gcc";
      }
    }
  return compiler;
}


int cmExtraCodeBlocksGenerator::GetCBTargetType(cmTarget* target)
{
  if ( target->GetType()==cmTarget::EXECUTABLE)
    {
    if ((target->GetPropertyAsBool("WIN32_EXECUTABLE"))
        || (target->GetPropertyAsBool("MACOSX_BUNDLE")))
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else if ( target->GetType()==cmTarget::STATIC_LIBRARY)
    {
    return 2;
    }
  else if ((target->GetType()==cmTarget::SHARED_LIBRARY) 
     || (target->GetType()==cmTarget::MODULE_LIBRARY))
    {
    return 3;
    }
  return 4;
}

std::string cmExtraCodeBlocksGenerator::BuildMakeCommand(
             const std::string& make, const char* makefile, const char* target)
{
  std::string command = make;
  if (strcmp(this->GlobalGenerator->GetName(), "NMake Makefiles")==0)
    {
    command += " /NOLOGO /f \\\"";
    command += makefile;
    command += "\\\" ";
    command += target;
    }
  else
    {
    command += " -f ";
    command += makefile;
    command += " ";
    command += target;
    }
  return command;
}
