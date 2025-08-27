// [me_Heap] Dynamic memory management

/*
  Author: Martin Eden
  Last mod.: 2025-08-27
*/

/*
  Experimental module
*/

#pragma once

#include <me_BaseTypes.h>
#include <me_ManagedMemory.h>

namespace me_Heap
{
  class THeap
  {
    private:
      TBool IsReadyFlag = false;

    public:
      ~THeap();

      TBool Init(
        TUint_2 Size
      );

      TBool Reserve(
        TAddressSegment * MemSeg,
        TUint_2 Size
      );

      TBool Release(
        TAddressSegment * MemSeg
      );

      TBool IsReady();

    protected:
      me_ManagedMemory::TManagedMemory HeapMem;
      me_ManagedMemory::TManagedMemory Bitmap;
      TUint_2 LastSegSize = 0;

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
        TAddressSegment MemSeg,
        TUint_1 BitsValue
      );

      // Set bitmap bits for segment to given value
      void SetRange(
        TAddressSegment MemSeg,
        TUint_1 BitsValue
      );

      // Set bitmap range bits
      void MarkRange(
        TAddressSegment MemSeg
      );

      // Clear bitmap range bits
      void ClearRange(
        TAddressSegment MemSeg
      );

      // [Sanity] segment in our managed range?
      TBool IsOurs(
        TAddressSegment MemSeg
      );

      // Return bit value in segment's data
      TUint_1 GetBit(
        TUint_2 BitIndex
      );

      // Set bit to given value in segment's data
      void SetBit(
        TUint_2 BitIndex,
        TUint_1 BitValue
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
  2025-07-29
*/
