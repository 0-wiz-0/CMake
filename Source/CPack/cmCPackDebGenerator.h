/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc. All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef cmCPackDebGenerator_h
#define cmCPackDebGenerator_h


#include "cmCPackGenericGenerator.h"

/** \class cmCPackDebGenerator
 * \brief A generator for Debian packages
 *
 */
class cmCPackDebGenerator : public cmCPackGenericGenerator
{
public:
  cmCPackTypeMacro(cmCPackDebGenerator, cmCPackGenericGenerator);

  /**
   * Construct generator
   */
  cmCPackDebGenerator();
  virtual ~cmCPackDebGenerator();

protected:
  virtual int CompressFiles(const char* outFileName, const char* toplevel,
    const std::vector<std::string>& files);
  virtual const char* GetOutputExtension() { return ".deb"; }
  virtual const char* GetInstallPrefix() { return "/usr"; }

};

#endif
