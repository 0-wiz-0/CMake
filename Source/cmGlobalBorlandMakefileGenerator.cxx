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
#include "cmGlobalBorlandMakefileGenerator.h"
#include "cmLocalUnixMakefileGenerator2.h"
#include "cmMakefile.h"
#include "cmake.h"

cmGlobalBorlandMakefileGenerator::cmGlobalBorlandMakefileGenerator()
{
  m_FindMakeProgramFile = "CMakeBorlandFindMake.cmake";
  m_ForceUnixPaths = false;
}


void cmGlobalBorlandMakefileGenerator::EnableLanguage(std::vector<std::string>const& l,
                                                      cmMakefile *mf)
{
  std::string outdir = m_CMakeInstance->GetStartOutputDirectory();
  mf->AddDefinition("BORLAND", "1");
  mf->AddDefinition("CMAKE_GENERATOR_CC", "bcc32");
  mf->AddDefinition("CMAKE_GENERATOR_CXX", "bcc32"); 
  this->cmGlobalUnixMakefileGenerator::EnableLanguage(l, mf);
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator *cmGlobalBorlandMakefileGenerator::CreateLocalGenerator()
{
  cmLocalUnixMakefileGenerator2* lg = new cmLocalUnixMakefileGenerator2;
  lg->SetEmptyCommand("@REM Borland Make needs a command here.");
  lg->SetEchoNeedsQuote(false);
  lg->SetIncludeDirective("!include");
  lg->SetWindowsShell(true);
  lg->SetMakefileVariableSize(32);
  lg->SetPassMakeflags(true);
  lg->SetGlobalGenerator(this);
  return lg;
}


//----------------------------------------------------------------------------
void cmGlobalBorlandMakefileGenerator::GetDocumentation(cmDocumentationEntry& entry) const
{
  entry.name = this->GetName();
  entry.brief = "Generates Borland makefiles.";
  entry.full = "";
}
