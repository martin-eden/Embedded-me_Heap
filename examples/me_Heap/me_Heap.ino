// [me_Heap] Simple test

/*
  Author: Martin Eden
  Last mod.: 2025-08-29
*/

#include <me_Heap.h>

#include <me_BaseTypes.h>
#include <me_Console.h>

void RunTest()
{
  /*
    We'll use global "Heap"

    Which is declared in "me_Heap.cpp" as

      me_Heap::THeap Heap;
  */

  const TUint_2 HeapSize = 400;

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

  TAddressSegment Seg_1;
  TAddressSegment Seg_2;
  TAddressSegment Seg_3;

  Heap.Reserve(&Seg_1, 100);
  Heap.Reserve(&Seg_2, 10);
  Heap.Reserve(&Seg_3, 60);

  Heap.Release(&Seg_2);

  Heap.Reserve(&Seg_2, 30);

  Heap.Release(&Seg_3);

  Heap.Release(&Seg_2);

  Heap.Reserve(&Seg_2, 60);

  Heap.Release(&Seg_1);

  Heap.Release(&Seg_2);
}

void setup()
{
  Console.Init();

  Console.Print("( [me_Heap] test");
  Console.Indent();
  RunTest();
  Console.Unindent();
  Console.Print(") Done");
}

void loop()
{
}

/*
  2024 # # # #
*/
