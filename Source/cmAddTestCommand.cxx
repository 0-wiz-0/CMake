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
#include "cmAddTestCommand.h"

// cmExecutableCommand
bool cmAddTestCommand::InitialPass(std::vector<std::string> const& args)
{
  // First argument is the name of the test
  // Second argument is the name of the executable to run (a target or external
  //    program)
  // Remaining arguments are the arguments to pass to the executable
  if(args.size() < 2 )
    {
    this->SetError("called with incorrect number of arguments");
    return false;
    }
  
  // store the arguments for the final pass
  // also expand any CMake variables

  m_Args = args;
  return true;
}

// we append to the file in the final pass because Enable Testing command
// creates the file in the final pass.
void cmAddTestCommand::FinalPass()
{
  // Create a full path filename for output Testfile
  std::string fname;
  fname = m_Makefile->GetStartOutputDirectory();
  fname += "/";
  if ( m_Makefile->IsOn("DART_ROOT") )
    {
    fname += "DartTestfile.txt";
    }
  else
    {
    fname += "CTestTestfile.cmake";
    }
  

  // If the file doesn't exist, then ENABLE_TESTING hasn't been run
  if (cmSystemTools::FileExists(fname.c_str()))
    {
    // Open the output Testfile
    std::ofstream fout(fname.c_str(), std::ios::app);
    if (!fout)
      {
        cmSystemTools::Error("Error Writing ", fname.c_str());
        return;
      }

    std::vector<std::string>::iterator it;

    // for each arg in the test
    fout << "ADD_TEST(";
    it = m_Args.begin();
    fout << (*it).c_str();
    ++it;
    for (; it != m_Args.end(); ++it)
      {
      // Just double-quote all arguments so they are re-parsed
      // correctly by the test system.
      fout << " \"";
      for(std::string::iterator c = it->begin(); c != it->end(); ++c)
        {
        // Escape quotes within arguments.  We should escape
        // backslashes too but we cannot because it makes the result
        // inconsistent with previous behavior of this command.
        if((*c == '"'))
          {
          fout << '\\';
          }
        fout << *c;
        }
      fout << "\"";
      }
    fout << ")" << std::endl;
    fout.close();
    }
  return;
}

