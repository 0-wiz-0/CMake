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
#include "cmGeneratedFileStream.h"

#include "cmSystemTools.h"

// Includes needed for implementation of RenameFile.  This is not in
// system tools because it is not implemented robustly enough to move
// files across directories.
#ifdef _WIN32
# include <windows.h>
# include <sys/stat.h>
#endif

#if defined(CMAKE_BUILD_WITH_CMAKE)
# include <cmzlib/zlib.h>
#endif

//----------------------------------------------------------------------------
cmGeneratedFileStream::cmGeneratedFileStream():
  cmGeneratedFileStreamBase(), Stream()
{
}

//----------------------------------------------------------------------------
cmGeneratedFileStream::cmGeneratedFileStream(const char* name, bool quiet):
  cmGeneratedFileStreamBase(name),
  Stream(m_TempName.c_str())
{
  // Check if the file opened.
  if(!*this && !quiet)
    {
    cmSystemTools::Error("Cannot open file for write: ", m_TempName.c_str());
    cmSystemTools::ReportLastSystemError("");
    }
}

//----------------------------------------------------------------------------
cmGeneratedFileStream::~cmGeneratedFileStream()
{
  // This is the first destructor called.  Check the status of the
  // stream and give the information to the private base.  Next the
  // stream will be destroyed which will close the temporary file.
  // Finally the base destructor will be called to replace the
  // destination file.
  m_Okay = (*this)?true:false;
}

//----------------------------------------------------------------------------
cmGeneratedFileStream&
cmGeneratedFileStream::Open(const char* name, bool quiet, bool binary)
{
  // Store the file name and construct the temporary file name.
  this->cmGeneratedFileStreamBase::Open(name);

  // Open the temporary output file.
  if ( binary )
    {
    this->Stream::open(m_TempName.c_str(), std::ios::out | std::ios::binary);
    }
  else
    {
    this->Stream::open(m_TempName.c_str(), std::ios::out);
    }

  // Check if the file opened.
  if(!*this && !quiet)
    {
    cmSystemTools::Error("Cannot open file for write: ", m_TempName.c_str());
    cmSystemTools::ReportLastSystemError("");
    }
  return *this;
}

//----------------------------------------------------------------------------
cmGeneratedFileStream&
cmGeneratedFileStream::Close()
{
  // Save whether the temporary output file is valid before closing.
  m_Okay = (*this)?true:false;

  // Close the temporary output file.
  this->Stream::close();

  // Remove the temporary file (possibly by renaming to the real file).
  this->cmGeneratedFileStreamBase::Close();

  return *this;
}

//----------------------------------------------------------------------------
void cmGeneratedFileStream::SetCopyIfDifferent(bool copy_if_different)
{
  m_CopyIfDifferent = copy_if_different;
}

//----------------------------------------------------------------------------
void cmGeneratedFileStream::SetCompression(bool compression)
{
  m_Compress = compression;
}

//----------------------------------------------------------------------------
void cmGeneratedFileStream::SetCompressionExtraExtension(bool ext)
{
  m_CompressExtraExtension = ext;
}

//----------------------------------------------------------------------------
cmGeneratedFileStreamBase::cmGeneratedFileStreamBase():
  m_Name(),
  m_TempName(),
  m_CopyIfDifferent(false),
  m_Okay(false),
  m_Compress(false),
  m_CompressExtraExtension(true)
{
}

//----------------------------------------------------------------------------
cmGeneratedFileStreamBase::cmGeneratedFileStreamBase(const char* name):
  m_Name(),
  m_TempName(),
  m_CopyIfDifferent(false),
  m_Okay(false),
  m_Compress(false),
  m_CompressExtraExtension(true)
{
  this->Open(name);
}

//----------------------------------------------------------------------------
cmGeneratedFileStreamBase::~cmGeneratedFileStreamBase()
{
  this->Close();
}

//----------------------------------------------------------------------------
void cmGeneratedFileStreamBase::Open(const char* name)
{
  // Save the original name of the file.
  m_Name = name;

  // Create the name of the temporary file.
  m_TempName = name;
  m_TempName += ".tmp";

  // Make sure the temporary file that will be used is not present.
  cmSystemTools::RemoveFile(m_TempName.c_str());

  std::string dir = cmSystemTools::GetFilenamePath(m_TempName);
  cmSystemTools::MakeDirectory(dir.c_str());
}

//----------------------------------------------------------------------------
void cmGeneratedFileStreamBase::Close()
{
  std::string resname = m_Name;
  if ( m_Compress && m_CompressExtraExtension )
    {
    resname += ".gz";
    }

  // Only consider replacing the destination file if no error
  // occurred.
  if(!m_Name.empty() &&
    m_Okay &&
    (!m_CopyIfDifferent ||
     cmSystemTools::FilesDiffer(m_TempName.c_str(), resname.c_str())))
    {
    // The destination is to be replaced.  Rename the temporary to the
    // destination atomically.
    if ( m_Compress )
      {
      std::string gzname = m_TempName + ".temp.gz";
      if ( this->CompressFile(m_TempName.c_str(), gzname.c_str()) )
        {
        this->RenameFile(gzname.c_str(), resname.c_str());
        }
      cmSystemTools::RemoveFile(gzname.c_str());
      }
    else
      {
      this->RenameFile(m_TempName.c_str(), resname.c_str());
      }
    }

  // Else, the destination was not replaced.
  //
  // Always delete the temporary file. We never want it to stay around.
  cmSystemTools::RemoveFile(m_TempName.c_str());
}

//----------------------------------------------------------------------------
#ifdef CMAKE_BUILD_WITH_CMAKE
int cmGeneratedFileStreamBase::CompressFile(const char* oldname,
                                            const char* newname)
{
  gzFile gf = cm_zlib_gzopen(newname, "w");
  if ( !gf )
    {
    return 0;
    }
  FILE* ifs = fopen(oldname, "r");
  if ( !ifs )
    {
    return 0;
    }
  size_t res;
  const size_t BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  while ( (res = fread(buffer, 1, BUFFER_SIZE, ifs)) > 0 )
    {
    if ( !cm_zlib_gzwrite(gf, buffer, res) )
      {
      fclose(ifs);
      cm_zlib_gzclose(gf);
      return 0;
      }
    }
  fclose(ifs);
  cm_zlib_gzclose(gf);
  return 1;
}
#else
int cmGeneratedFileStreamBase::CompressFile(const char*, const char*)
{
  return 0;
}
#endif

//----------------------------------------------------------------------------
int cmGeneratedFileStreamBase::RenameFile(const char* oldname,
                                          const char* newname)
{
#ifdef _WIN32
  /* On Windows the move functions will not replace existing files.
     Check if the destination exists.  */
  struct stat newFile;
  if(stat(newname, &newFile) == 0)
    {
    /* The destination exists.  We have to replace it carefully.  The
       MoveFileEx function does what we need but is not available on
       Win9x.  */
    OSVERSIONINFO osv;
    DWORD attrs;

    /* Make sure the destination is not read only.  */
    attrs = GetFileAttributes(newname);
    if(attrs & FILE_ATTRIBUTE_READONLY)
      {
      SetFileAttributes(newname, attrs & ~FILE_ATTRIBUTE_READONLY);
      }

    /* Check the windows version number.  */
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    if(osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
      {
      /* This is Win9x.  There is no MoveFileEx implementation.  We
         cannot quite rename the file atomically.  Just delete the
         destination and then move the file.  */
      DeleteFile(newname);
      return MoveFile(oldname, newname);
      }
    else
      {
      /* This is not Win9x.  Use the MoveFileEx implementation.  */
      return MoveFileEx(oldname, newname, MOVEFILE_REPLACE_EXISTING);
      }
    }
  else
    {
    /* The destination does not exist.  Just move the file.  */
    return MoveFile(oldname, newname);
    }
#else
  /* On UNIX we have an OS-provided call to do this atomically.  */
  return rename(oldname, newname) == 0;
#endif
}

//----------------------------------------------------------------------------
void cmGeneratedFileStream::SetName(const char* fname)
{
  if ( !fname )
    {
    m_Name = "";
    return;
    }
  m_Name = fname;
}
