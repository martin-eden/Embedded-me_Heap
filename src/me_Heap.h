// [me_Heap] Dynamic memory management

/*
  Author: Martin Eden
  Last mod.: 2024-10-13
*/

/*
  Why I'm writing this module

    To explore memory fragmentation patterns depending on allocation
    strategy.

  AVR Arduino Uno environment:

    We have direct access to all (256 + 2048) bytes of memory.
    That memory is all we have.

    First 256 bytes are special:

      First 32 bytes are processor registers. Next (256 - 32) bytes are
      control registers.

      Writing values to them makes effects. Like changing next
      instruction pointer, setting pin to LOW or HIGH, or transmitting
      character via UART.

    Last bytes are special too:

      At the end of memory lives stack. Stack stores upvalues
      from upper functions. And stack grows down.

    So "free memory" may start from address 256 and lasts until stack
    end. But stack end changes dynamically. And we have no idea how
    deep it can go down. (No one has.)

    But where global data are allocated? I bet in lower addresses. So
    "free memory" starts after end of last global data and lasts
    until current stack border at high addresses.

  What if we just declare global "TUint_1 Heap[1024]" ?

    I think it's not too bad. Compiler will reserve for us 1KiB
    memory somewhere. We can use it as we please.

    Except "1024". Why not 1023, or 200 or 1500?

      Here I see two approaches.

      One is to make it compile-time constant. Allocate 1311 bytes
      for "heap". Period.

      Another one is ask alloc() to get us 1311 bytes. Problem is that
      it's not always can get us 1311 bytes. Memory may be fragmented.

  We don't care

    We just need memory span that noone will touch.

    So we may use static array or we may ask alloc() to get us it.

    (Spoiler: we're asking alloc().)

  2024-10-12
*/

#pragma once

#include <me_BaseTypes.h>
#include <me_MemorySegment.h>
#include <me_ManagedMemory.h>

namespace me_Heap
{
  class THeap
  {
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
      me_ManagedMemory::TManagedMemory HeapMem;
      me_ManagedMemory::TManagedMemory Bitmap;
      TBool IsReadyFlag = false;
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
