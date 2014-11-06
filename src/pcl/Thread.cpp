// ****************************************************************************
// PixInsight Class Library - PCL 02.00.13.0689
// ****************************************************************************
// pcl/Thread.cpp - Released 2014/10/29 07:34:20 UTC
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

#include <pcl/AutoLock.h>
#include <pcl/Console.h>
#include <pcl/GlobalSettings.h>
#include <pcl/Math.h>
#include <pcl/Thread.h>

#include <pcl/api/APIException.h>
#include <pcl/api/APIInterface.h>

#ifdef __PCL_UNIX
# include <time.h> // for nanosleep()
#endif

#ifdef __PCL_LINUX
# include <sched.h>
# include <errno.h>
# include <iostream>
#endif

#ifdef __PCL_WINDOWS
# define SLEEP( secs )                                \
   ::Sleep( DWORD( TruncI( secs*1000 ) ) )
#else
# define SLEEP( secs )                                \
   struct timespec ts;                                \
   ts.tv_sec = TruncI( secs );                        \
   ts.tv_nsec = TruncI( Frac( secs )*1000000000 );    \
   ::nanosleep( &ts, 0 )
#endif

namespace pcl
{

// ----------------------------------------------------------------------------

static bool s_enableAffinity = false;
static bool s_featureDataInitialized = false;

// ----------------------------------------------------------------------------

#define T   reinterpret_cast<Thread*>( hThread )

class ThreadDispatcher
{
public:

   static void api_func RunThread( thread_handle hThread )
   {
      try
      {
         if ( s_enableAffinity && T->m_processorIndex >= 0 )
            T->SetAffinity( T->m_processorIndex );
         T->Run();
      }
      catch ( ... )
      {
         /* ### Do _not_ propagate exceptions from a running thread */
      }
   }

}; // ThreadDispatcher

#undef T

// ----------------------------------------------------------------------------

#ifdef _MSC_VER
#  pragma warning( disable: 4355 ) // 'this' : used in base member initializer list
#endif

Thread::Thread() :
UIObject( (*API->Thread->CreateThread)( ModuleHandle(), this, 0 /*flags*/ ) ),
m_processorIndex( -1 ), m_consoleOutputText()
{
   if ( handle == 0 )
      throw APIFunctionError( "CreateThread" );

   (*API->Thread->SetThreadExecRoutine)( handle, ThreadDispatcher::RunThread );

   if ( !s_featureDataInitialized )
   {
      /*
       * ### TODO: Add a new StartThreadEx() API function to allow specifying a
       * logical processor index and other flags. In this way this wouldn't be
       * necessary and thread scheduling and affinity would be controlled by
       * the PI core application.
       */
      s_enableAffinity = PixInsightSettings::GlobalFlag( "Process/EnableThreadCPUAffinity" );
      s_featureDataInitialized = true;
   }
}

Thread::Thread( void* h ) : UIObject( h ), m_processorIndex( -1 ), m_consoleOutputText()
{
}

Thread& Thread::Null()
{
   static Thread* nullThread = 0;
   static Mutex mutex;
   volatile AutoLock lock( mutex );
   if ( nullThread == 0 )
      nullThread = new Thread( reinterpret_cast<void*>( 0 ) );
   return *nullThread;
}

void Thread::Start( Thread::priority p, int processor )
{
   m_processorIndex = Range( processor, -1, PCL_MAX_PROCESSORS );
   (*API->Thread->StartThread)( handle, p );
}

Array<int> Thread::Affinity() const
{
   Array<int> processors;
#ifdef __PCL_LINUX
   cpu_set_t set;
   if ( sched_getaffinity( 0, sizeof( cpu_set_t ), &set ) >= 0 )
      for ( int i = 0; i < CPU_SETSIZE; ++i )
         if ( CPU_ISSET( i, &set ) )
            processors.Add( i );
#endif
#ifdef __PCL_FREEBSD
   // ### TODO
#endif
#ifdef __PCL_MACOSX
   // ### TODO
#endif
#ifdef __PCL_WINDOWS
   // ### TODO - How can we know the affinity mask of a thread on Windows ???
   //            There is no GetThreadAffinityMask() !
#endif
   return processors;
}

bool Thread::SetAffinity( const Array<int>& processors )
{
   if ( (*API->Thread->IsThreadActive)( handle ) == api_false )
      return false;
#ifdef __PCL_LINUX
   cpu_set_t set;
   CPU_ZERO( &set );
   for ( Array<int>::const_iterator i = processors.Begin(); i != processors.End(); ++i )
   {
      if ( *i < 0 || *i >= CPU_SETSIZE )
         return false;
      CPU_SET( *i, &set );
   }
   return sched_setaffinity( 0, sizeof( cpu_set_t ), &set ) >= 0;
#endif
#ifdef __PCL_FREEBSD
   // ### TODO
#endif
#ifdef __PCL_MACOSX
   // ### TODO
#endif
#ifdef __PCL_WINDOWS
   DWORD_PTR mask = 0;
   for ( Array<int>::const_iterator i = processors.Begin(); i != processors.End(); ++i )
   {
      if ( *i < 0 || *i >= sizeof( DWORD_PTR )<<3 )
         return false;
      mask |= DWORD_PTR( 1u ) << *i;
   }
   return SetThreadAffinityMask( GetCurrentThread(), mask ) != 0;
#endif
   return false;
}

bool Thread::SetAffinity( int processor )
{
   if ( (*API->Thread->IsThreadActive)( handle ) == api_false )
      return false;
#ifdef __PCL_LINUX
   if ( processor < 0 || processor >= CPU_SETSIZE )
      return false;
   cpu_set_t set;
   CPU_ZERO( &set );
   CPU_SET( processor, &set );
   return sched_setaffinity( 0, sizeof( cpu_set_t ), &set ) >= 0;
#endif
#ifdef __PCL_FREEBSD
   // ### TODO
#endif
#ifdef __PCL_MACOSX
   // ### TODO
#endif
#ifdef __PCL_WINDOWS
   if ( processor < 0 || processor >= sizeof( DWORD_PTR )<<3 )
      return false;
   DWORD_PTR mask = 1u << processor;
   return SetThreadAffinityMask( GetCurrentThread(), mask ) != 0;
#endif
   return false;
}

void Thread::Kill()
{
   (*API->Thread->KillThread)( handle );
}

bool Thread::IsActive() const
{
   return (*API->Thread->IsThreadActive)( handle ) != api_false;
}

Thread::priority Thread::Priority() const
{
   return priority( (*API->Thread->GetThreadPriority)( handle ) );
}

void Thread::SetPriority( Thread::priority p )
{
   (*API->Thread->SetThreadPriority)( handle, p );
}

bool Thread::Wait( double secs )
{
   return (*API->Thread->WaitThread)( handle, (uint32)(1000*Round( secs, 3 )) ) != api_false;
}

void Thread::Wait()
{
   (void)(*API->Thread->WaitThread)( handle, ~(uint32)(0) );
}

void Thread::Sleep( double secs )
{
   //(*API->Thread->SleepThread)( handle, (uint32)(1000*Round( secs, 3 )) );
   SLEEP( secs );
}

Thread& Thread::CurrentThread()
{
   thread_handle handle = (*API->Thread->GetCurrentThread)();
   if ( handle != 0 )
      if ( (*API->UI->GetUIObjectModule)( handle ) == ModuleHandle() )
      {
         UIObject& object = UIObject::ObjectByHandle( handle );
         if ( !object.IsNull() )
         {
            Thread* thread = dynamic_cast<Thread*>( &object );
            if ( thread != 0 )
               return *thread;
         }
      }
   return Null();
}

bool Thread::IsRootThread()
{
   return (*API->Thread->GetCurrentThread)() == 0;
}

uint32 Thread::Status() const
{
   return (*API->Thread->GetThreadStatus)( handle );
}

bool Thread::TryGetStatus( uint32& status ) const
{
   return (*API->Thread->GetThreadStatusEx)( handle, &status, 0x00000001 ) != api_false;
}

void Thread::SetStatus( uint32 status )
{
   (*API->Thread->SetThreadStatus)( handle, status );
}

String Thread::ConsoleOutputText() const
{
   size_type len = 0;
   (*API->Thread->GetThreadConsoleOutputText)( handle, 0, &len );

   String text;
   if ( len != 0 )
   {
      text.Reserve( len );
      if ( (*API->Thread->GetThreadConsoleOutputText)( handle, text.c_str(), &len ) == api_false )
         throw APIFunctionError( "GetThreadConsoleOutputText" );
   }

   return text;
}

void Thread::ClearConsoleOutputText()
{
   (*API->Thread->ClearThreadConsoleOutputText)( handle );
}

void Thread::FlushConsoleOutputText()
{
   if ( IsRootThread() )
   {
      Console().Write( "<end><cbr>" + ConsoleOutputText() );
      ClearConsoleOutputText();
   }
}

void* Thread::CloneHandle() const
{
   throw Error( "Cannot clone a Thread handle" );
}

// ----------------------------------------------------------------------------

int Thread::NumberOfThreads( size_type N, size_type overheadLimit )
{
   static int numberOfProcessors = 0;
   if ( numberOfProcessors == 0 )
   {
      numberOfProcessors = PixInsightSettings::GlobalInteger( "System/NumberOfProcessors" );
      if ( numberOfProcessors <= 0 )
         return 1;
   }

   if ( N > overheadLimit &&
        PixInsightSettings::GlobalFlag( "Process/EnableParallelProcessing" ) &&
        PixInsightSettings::GlobalFlag( "Process/EnableParallelModuleProcessing" ) )
   {
      size_type np = Min( numberOfProcessors, PixInsightSettings::GlobalInteger( "Process/MaxProcessors" ) );
      return Max( 1, int( Min( np, N/Max( overheadLimit, N/np ) ) ) );
   }

   return 1;
}

// ----------------------------------------------------------------------------

void PCL_FUNC Sleep( double secs )
{
   //(*API->Thread->SleepThread)( 0, (uint32)(1000*Round( secs, 3 )) );
   SLEEP( secs );
}

// ----------------------------------------------------------------------------

} // pcl

// ****************************************************************************
// EOF pcl/Thread.cpp - Released 2014/10/29 07:34:20 UTC