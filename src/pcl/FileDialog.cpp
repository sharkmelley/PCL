// ****************************************************************************
// PixInsight Class Library - PCL 02.00.13.0689
// ****************************************************************************
// pcl/FileDialog.cpp - Released 2014/10/29 07:34:21 UTC
// ****************************************************************************
// This file is part of the PixInsight Class Library (PCL).
// PCL is a multiplatform C++ framework for development of PixInsight modules.
//
// Copyright (c) 2003-2014, Pleiades Astrophoto S.L. All Rights Reserved.
//
// Redistribution and use in both source and binary forms, with or without
// modification, is permitted provided that the following conditions are met:
//
// 1. All redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. All redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the names "PixInsight" and "Pleiades Astrophoto", nor the names
//    of their contributors, may be used to endorse or promote products derived
//    from this software without specific prior written permission. For written
//    permission, please contact info@pixinsight.com.
//
// 4. All products derived from this software, in any form whatsoever, must
//    reproduce the following acknowledgment in the end-user documentation
//    and/or other materials provided with the product:
//
//    "This product is based on software from the PixInsight project, developed
//    by Pleiades Astrophoto and its contributors (http://pixinsight.com/)."
//
//    Alternatively, if that is where third-party acknowledgments normally
//    appear, this acknowledgment must be reproduced in the product itself.
//
// THIS SOFTWARE IS PROVIDED BY PLEIADES ASTROPHOTO AND ITS CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PLEIADES ASTROPHOTO OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, BUSINESS
// INTERRUPTION; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; AND LOSS OF USE,
// DATA OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ****************************************************************************

#include <pcl/FileDialog.h>
#include <pcl/GlobalSettings.h>

#include <pcl/api/APIInterface.h>
#include <pcl/api/APIException.h>

namespace pcl
{

// ----------------------------------------------------------------------------

class FileDialogPrivate
{
public:

   typedef FileDialog::filter_list filter_list;

   FileDialogPrivate() : caption(), initialPath(), filters(), fileExtension()
   {
   }

   String MakeAPIFilters() const
   {
      String apiFilters;
      for ( filter_list::const_iterator i = filters.Begin(); i != filters.End(); ++i )
      {
         if ( i != filters.Begin() )
            apiFilters += ";;";
         apiFilters += (*i).MakeAPIFilter();
      }

      return apiFilters;
   }

   void LoadFiltersFromGlobalString( const String& globalKey )
   {
      filters.Remove();

      String s = PixInsightSettings::GlobalString( globalKey  );
      if ( !s.IsEmpty() )
      {
         for ( size_type p = 0; ; )
         {
            size_type p1 = s.Find( '(', p );
            if ( p1 == String::notFound )
               break;

            size_type p2 = s.Find( ')', p1 );
            if ( p2 == String::notFound )
               break;

            String extStr = s.SubString( p1, p2-p1 );
            if ( !extStr.IsEmpty() )
            {
               StringList extLst;
               extStr.Break( extLst, ' ', true );
               if ( !extLst.IsEmpty() )
               {
                  FileFilter filter;

                  for ( StringList::const_iterator i = extLst.Begin(); i != extLst.End(); ++i )
                  {
                     size_type d = i->Find( '.' );
                     if ( d != String::notFound )
                        filter.AddExtension( i->SubString( d ) );
                  }

                  if ( !filter.Extensions().IsEmpty() )
                  {
                     String desc = s.SubString( p, p1-p );
                     desc.Trim();
                     filter.SetDescription( desc );

                     filters.Add( filter );
                  }
               }
            }

            p = p2 + 1;
         }
      }
   }

   String      caption;
   String      initialPath;
   filter_list filters;
   String      fileExtension;
};

// ----------------------------------------------------------------------------

FileFilter::FileFilter() : description(), extensions()
{
}

FileFilter::FileFilter( const FileFilter& x ) : description( x.description ), extensions( x.extensions )
{
}

FileFilter::~FileFilter()
{
}

void FileFilter::SetDescription( const String& dsc )
{
   description = dsc;
}


void FileFilter::AddExtension( const String& ext )
{
   String x = ext.Trimmed();
   if ( !x.BeginsWith( '.' ) )
      if ( !x.BeginsWith( '*' ) )
         x = '*' + x;
   x.ToLowerCase(); // use case-insensitive file extensions
   if ( !extensions.Has( x ) )
      extensions.Add( x );
}

void FileFilter::Clear()
{
   description.Empty();
   extensions.Remove();
}

String FileFilter::MakeAPIFilter() const
{
   String filter;

   if ( !extensions.IsEmpty() )
   {
      if ( !description.IsEmpty() )
      {
         filter += description;
         filter += " (";
      }

      for ( StringList::const_iterator i = extensions.Begin();
            i != extensions.End();
            ++i )
      {
         if ( i != extensions.Begin() )
            filter += ' '; // also legal are ';' and ','
         if ( i->BeginsWith( '.' ) )
            filter += '*';
         filter += *i;
      }

      if ( !description.IsEmpty() )
         filter += ')';
   }

   return filter;
}

// ----------------------------------------------------------------------------

FileDialog::FileDialog() : p( new FileDialogPrivate() )
{
}

FileDialog::~FileDialog()
{
   if ( p != 0 )
      delete p, p = 0;
}

String FileDialog::Caption() const
{
   return p->caption;
}

void FileDialog::SetCaption( const String& caption )
{
   p->caption = caption;
}

String FileDialog::InitialPath() const
{
   return p->initialPath;
}

void FileDialog::SetInitialPath( const String& path )
{
   p->initialPath = path;
}

const FileDialog::filter_list& FileDialog::Filters() const
{
   return p->filters;
}

FileDialog::filter_list& FileDialog::Filters()
{
   return p->filters;
}

String FileDialog::SelectedFileExtension() const
{
   return p->fileExtension;
}

void FileDialog::SetSelectedFileExtension( const String& ext )
{
   p->fileExtension = File::ExtractExtension( ext );
}

// ----------------------------------------------------------------------------

class OpenFileDialogPrivate
{
public:

   OpenFileDialogPrivate() : multipleSelections( false ), fileNames()
   {
   }

   bool       multipleSelections;
   StringList fileNames;
};

OpenFileDialog::OpenFileDialog() : FileDialog(), q( new OpenFileDialogPrivate() )
{
   p->caption = "Open File";
}

OpenFileDialog::~OpenFileDialog()
{
   if ( q != 0 )
      delete q, q = 0;
}

void OpenFileDialog::LoadImageFilters()
{
   p->LoadFiltersFromGlobalString( "FileFormat/ReadFilters" );
}

bool OpenFileDialog::AllowsMultipleSelections() const
{
   return q->multipleSelections;
}

void OpenFileDialog::EnableMultipleSelections( bool enable )
{
   q->multipleSelections = enable;
}

// This is a file_enumeration_callback function as per APIDefs.h
static uint32 __AddFileNameToList( const char16_type* fileName, void* list )
{
   reinterpret_cast<StringList*>( list )->Add( fileName );
   return api_true;
}

bool OpenFileDialog::Execute()
{
   q->fileNames.Remove();

   String apiFilters = p->MakeAPIFilters();

   String fileName;
   fileName.Reserve( 8192 );
   *fileName.c_str() = CharTraits::Null();

   if ( q->multipleSelections )
   {
      if ( (*API->Dialog->ExecuteOpenMultipleFilesDialog)( fileName.Begin(),
               __AddFileNameToList, &q->fileNames,
               p->caption.c_str(),
               p->initialPath.c_str(), apiFilters.c_str(), p->fileExtension.c_str() ) == api_false )
      {
         return false;
      }
   }
   else
   {
      if ( (*API->Dialog->ExecuteOpenFileDialog)( fileName.Begin(),
               p->caption.c_str(),
               p->initialPath.c_str(), apiFilters.c_str(), p->fileExtension.c_str() ) == api_false )
      {
         return false;
      }

      if ( !fileName.IsEmpty() )
         q->fileNames.Add( fileName );
   }

   if ( !q->fileNames.IsEmpty() )
   {
      p->fileExtension = File::ExtractExtension( *q->fileNames );
      return true;
   }

   return false;
}

const StringList& OpenFileDialog::FileNames() const
{
   return q->fileNames;
}

String OpenFileDialog::FileName() const
{
   return q->fileNames.IsEmpty() ? String() : *q->fileNames;
}

// ----------------------------------------------------------------------------

class SaveFileDialogPrivate
{
public:

   SaveFileDialogPrivate() : overwritePrompt( true ), fileName()
   {
   }

   bool   overwritePrompt;
   String fileName;
};

SaveFileDialog::SaveFileDialog() : FileDialog(), q( new SaveFileDialogPrivate() )
{
   p->caption = "Save File As";
}

SaveFileDialog::~SaveFileDialog()
{
   if ( q != 0 )
      delete q, q = 0;
}

void SaveFileDialog::LoadImageFilters()
{
   p->LoadFiltersFromGlobalString( "FileFormat/WriteFilters" );
}

bool SaveFileDialog::IsOverwritePromptEnabled() const
{
   return q->overwritePrompt;
}

void SaveFileDialog::EnableOverwritePrompt( bool enable )
{
   q->overwritePrompt = enable;
}

bool SaveFileDialog::Execute()
{
   String apiFilters = p->MakeAPIFilters();

   q->fileName.Reserve( 8192 );
   *q->fileName.c_str() = CharTraits::Null();

   if ( (*API->Dialog->ExecuteSaveFileDialog)( q->fileName.Begin(),
               p->caption.c_str(),
               p->initialPath.c_str(), apiFilters.c_str(), p->fileExtension.c_str(),
               q->overwritePrompt ) != api_false )
   {
      if ( !q->fileName.IsEmpty() )
      {
         p->fileExtension = File::ExtractExtension( q->fileName );
         return true;
      }
   }

   return false;
}

String SaveFileDialog::FileName() const
{
   return q->fileName;
}

// ----------------------------------------------------------------------------

class GetDirectoryDialogPrivate
{
public:

   GetDirectoryDialogPrivate() : directory()
   {
   }

   String directory;
};

GetDirectoryDialog::GetDirectoryDialog() : FileDialog(), q( new GetDirectoryDialogPrivate() )
{
   p->caption = "Select Directory";
}

GetDirectoryDialog::~GetDirectoryDialog()
{
   if ( q != 0 )
      delete q, q = 0;
}

bool GetDirectoryDialog::Execute()
{
   q->directory.Reserve( 8192 );
   *q->directory.c_str() = CharTraits::Null();

   if ( (*API->Dialog->ExecuteGetDirectoryDialog)( q->directory.Begin(),
                        p->caption.c_str(), p->initialPath.c_str() ) != api_false )
   {
      return !q->directory.IsEmpty();
   }

   return false;
}

String GetDirectoryDialog::Directory() const
{
   return q->directory;
}

// ----------------------------------------------------------------------------

} // pcl

// ****************************************************************************
// EOF pcl/FileDialog.cpp - Released 2014/10/29 07:34:21 UTC