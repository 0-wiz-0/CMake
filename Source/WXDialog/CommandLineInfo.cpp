/*=========================================================================

  Program:   WXDialog - wxWidgets X-platform GUI Front-End for CMake
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Author:    Jorgen Bodde

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "CommandLineInfo.h" 

#include "cmSystemTools.h"

///////////////////////////////////////////////////////////////
// cmCommandLineInfo 

cmCommandLineInfo::cmCommandLineInfo()
{
  this->m_WhereSource = "";
  this->m_WhereBuild = "";
  this->m_AdvancedValues = false;
  this->m_GeneratorChoiceString = "";
  this->m_LastUnknownParameter = "";
  this->m_ValidArguments = "";
  this->m_ExitAfterLoad = false;
} 

///////////////////////////////////////////////////////////////
cmCommandLineInfo::~cmCommandLineInfo()
{
}

///////////////////////////////////////////////////////////////
void cmCommandLineInfo::ParseCommandLine(int argc, char* argv[])
{
  int cc;
  for ( cc = 1; cc < argc; cc ++ )
    {
    if ( strlen(argv[cc]) < 1 )
      {
      continue;
      }
    bool valid = true;
    std::string argument = argv[cc];
    if ( argument.size() > 1 && 
         this->m_ValidArguments.find(argument[1]) == std::string::npos )
      {
      valid = false;
      }
    this->ParseParam(argument, valid, (cc + 1 == argc));
    }
  this->m_ExecutablePath = cmSystemTools::GetFilenamePath(argv[0]);
}

///////////////////////////////////////////////////////////////
int cmCommandLineInfo::GetBoolValue(const std::string& v) {
  std::string value = cmSystemTools::LowerCase(v);
  if (value == "1" || 
      value == "on" || 
      value == "true" || 
      value == "yes")
    {
    return 1;
    }
  else if (value == "0" || 
           value == "off" || 
           value == "false" || 
           value == "no")
    {
    return -1;
    }
  return 0;
}

///////////////////////////////////////////////////////////////
// Parse param

void cmCommandLineInfo::ParseParam(const std::string& parameter, 
                                   bool know_about, bool /*last*/)
{
  if(!know_about)
    {
    this->m_LastUnknownParameter = parameter;
    }
  else
    {
    std::string sParam(parameter.c_str(), 1, parameter.npos);
    // Single letter valued flag like /B=value or /B:value
    std::string value;
    if (sParam[1] == '=' || sParam[1] == ':')
      {
      value = std::string(parameter.c_str()+3);
      }
    else
      {
      value = std::string(parameter.c_str()+2);
      }
    int res;
    switch (sParam[0])
      {
      case 'A':
        res = cmCommandLineInfo::GetBoolValue(value);
        if (res == 1)
          {
          this->m_AdvancedValues = true;
          }
        else if (res == -1)
          {
          this->m_AdvancedValues = false;
          }
        break;
      case 'B':
        this->m_WhereBuild = value;
        break;
      case 'G':
        this->m_GeneratorChoiceString = value;
        break;
      case 'Q':
        this->m_ExitAfterLoad = true;
        break;
      case 'H':
        this->m_WhereSource = value;
        break;
      }
    }
}
