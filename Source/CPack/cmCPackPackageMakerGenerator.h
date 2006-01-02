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

#ifndef cmCPackPackageMakerGenerator_h
#define cmCPackPackageMakerGenerator_h


#include "cmCPackGenericGenerator.h"
#include "CPack/cmCPackConfigure.h" // for ssize_t

/** \class cmCPackPackageMakerGenerator
 * \brief A generator for PackageMaker files
 *
 * http://developer.apple.com/documentation/Darwin/Reference/ManPages/man1/packagemaker.1.html
 */
class cmCPackPackageMakerGenerator : public cmCPackGenericGenerator
{
public:
  cmCPackTypeMacro(cmCPackPackageMakerGenerator, cmCPackGenericGenerator);
  /**
   * Do the actual processing. Subclass has to override it.
   * Return < 0 if error.
   */
  virtual int ProcessGenerator();

  /**
   * Initialize generator
   */
  virtual int Initialize(const char* name);

  /**
   * Construct generator
   */
  cmCPackPackageMakerGenerator();
  virtual ~cmCPackPackageMakerGenerator();

protected:
  int CompressFiles(const char* outFileName, const char* toplevel,
    const std::vector<std::string>& files);
  virtual const char* GetOutputExtension() { return "dmg"; }
  virtual const char* GetOutputPostfix() { return "darwin"; }
  virtual const char* GetInstallPrefix() { return "/usr"; }

  bool CopyCreateResourceFile(const char* name);
  bool CopyResourcePlistFile(const char* name);
};

#endif


