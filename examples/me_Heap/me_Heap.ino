// [me_Heap] Simple test

/*
  Author: Martin Eden
  Last mod.: 2024-10-12
*/

#include <me_BaseTypes.h>
#include <me_UartSpeeds.h>
#include <me_InstallStandardStreams.h>
#include <me_Console.h>
#include <me_ManagedMemory.h>
#include <me_String.h>
#include <me_Heap.h>

void setup()
{
  InstallStandardStreams();
  Console.Init(me_UartSpeeds::Arduino_Normal_Bps);
  Console.Print("[me_Heap] Start.");
  Console.Indent();
  RunTest();
  Console.Unindent();
  Console.Print("[me_Heap] Done.");
}

void loop()
{
}

// --

void RunTest()
{
  /*
    We'll use global "Heap"

    Which is declared in "me_Heap.cpp" as

      me_Heap::THeap Heap;
  */

  const TUint_2 HeapSize = 800;

  if (!Heap.Init(HeapSize))
  {
    Console.Print("Failed to allocate heap memory.");
    return;
  }

  /*
    This test is not doing much at this time. Just allocates/frees
    memory blocks to make holes. Hope is that debug printf() are
    enabled in [me_MemorySegment].

    ([me_Console] output won't work for memory allocation debug prints
    because console printing of integers allocates memory.)

    Real test is enable debug printing in [me_MemorySegment] and
    run real code. Copy messages, convert them to bitmap. Observe.
  */

  me_MemorySegment::TMemorySegment ms_1;
  me_MemorySegment::TMemorySegment ms_2;
  me_MemorySegment::TMemorySegment ms_3;

  Heap.Reserve(&ms_1, 100);
  Heap.Reserve(&ms_2, 10);
  Heap.Reserve(&ms_3, 60);

  Heap.Release(&ms_2);

  Heap.Reserve(&ms_2, 30);

  Heap.Release(&ms_3);

  Heap.Release(&ms_2);

  Heap.Reserve(&ms_2, 60);

  Heap.Release(&ms_1);

  Heap.Release(&ms_2);
}

/*
  2024-10-11
  2024-10-12
*/
