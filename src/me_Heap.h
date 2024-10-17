// [me_Heap] Dynamic memory management

/*
  Author: Martin Eden
  Last mod.: 2024-10-17
*/

/*
  Experimental module.
*/

#pragma once

#include <me_BaseTypes.h>
#include <me_MemorySegment.h>
#include <me_ManagedMemory.h>

namespace me_Heap
{
  class THeap
  {
    private:
      me_ManagedMemory::TManagedMemory HeapMem;
      me_ManagedMemory::TManagedMemory Bitmap;
      TBool IsReadyFlag = false;
      TUint_2 LastSegSize = 0;

    public:
      ~THeap();

      TBool Init(
        TUint_2 Size
      );

      TBool Reserve(
        me_MemorySegment::TMemorySegment * MemSeg,
        TUint_2 Size
      );

      TBool Release(
        me_MemorySegment::TMemorySegment * MemSeg
      );

      TBool IsReady();

    private:

      // Get index of empty span in bitmap where we will allocate
      TBool GetInsertIndex(
        TUint_2 * InsertIndex,
        TUint_2 Size
      );

      // Find nearest busy byte
      TUint_2 GetNextBusyIndex(
        TUint_2 StartIdx
      );

      // Bitmap bits are same for this segment?
      TBool RangeIsSolid(
        me_MemorySegment::TMemorySegment MemSeg,
        TBool BitsValue
      );

      // Set bitmap bits for segment
      void SetRange(
        me_MemorySegment::TMemorySegment MemSeg,
        TBool BitsValue
      );

      // [Sanity] segment in our managed range?
      TBool IsOurs(
        me_MemorySegment::TMemorySegment MemSeg
      );

      // Return bit value in segment's data
      TBool GetBit(
        TUint_2 BitIndex
      );

      // Set bit to given value in segment's data
      void SetBit(
        TUint_2 BitIndex,
        TBool BitValue
      );
  };
}

/*
  Global class instance

  Used to switch [me_ManagedMemory] allocators to our allocator
  when we are ready.

  We are ready when

    Heap.GetIsReady() == true :

      That means that someone completed Heap.Init(<Size>) :

        That means that [me_ManagedMemory] allocators reserved
        memory for our data and bitmap via stock malloc().

  C developers love to hide globals under "__" prefix. Like
  "__flp" and three more with it. We're instead declaring it
  openly. We're assuming you know framework you working with.

  If you named variable "Heap" and then something went wrong,
  you're here to read this comment. Now you know.
*/
extern me_Heap::THeap Heap;

/*
  2024-10-11
  2024-10-12
*/
