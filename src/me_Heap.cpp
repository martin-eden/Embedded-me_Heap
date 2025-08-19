// [me_Heap] Dynamic memory management

/*
  Author: Martin Eden
  Last mod.: 2025-08-19
*/

/*
  Implementation uses bitmap. So overhead is (N / 8) for managing
  (N) bytes of memory. Runtime is constant O(N).
*/

#include "me_Heap.h"

#include <me_BaseTypes.h>
#include <me_MemorySegment.h>
#include <me_ManagedMemory.h>
#include <Arduino.h> // [Debug] for PSTR()
#include <stdio.h> // [Debug] for printf_P()
#include <me_Bits.h>

using namespace me_Heap;

using
  me_MemorySegment::TMemorySegment;

// Another lame constant I should move somewhere
const TUint_1 BitsInByte = 8;

// ( THeap

/*
  Free our data via stock free()
*/
THeap::~THeap()
{
  /*
    If we're alive via global (then we shouldn't die but anyway)
    destructors for our [me_ManagedMemory] data will check
    "<OurGlobalName>.IsReady()".

    If it returns "false" they will use stock free().
  */
  IsReadyFlag = false;
}

/*
  Allocate memory and bitmap for given memory size

  This implementation can't allocate more than 8 KiB memory.
*/
TBool THeap::Init(
  TUint_2 Size
)
{
  /*
    We'll allocate <Size> bytes via stock malloc().
    And then <Size> / 8 bytes for bitmap.
    If we'll not fail, we'll return true.

    Actually we don't care about memory segment data that we
    will be subdividing. We'll store it in our class just for
    caller's convenience. What is ours is bitmap for that segment.
  */

  /*
    We want to keep things simple. Bit index is eight times more
    than byte index. So for 64Ki max bit index is 512Ki.

    That's outside of UInt_2 range that I'm trying to stay
    (unless absolutely necessary or unless I really want to.).

    In this case my system having 2Ki memory. Maximum this code
    can allocate is 8Ki. I'm not going to make deviations and
    write code that can allocate 64Ki. Or 4Gi. Or 4Gi*4Gi.
  */
  const TUint_2 MaxSize = 0xFFFF / BitsInByte;

  if (Size > MaxSize)
    return false;

  IsReadyFlag = false;

  // Get memory for data
  // No memory for data? Return
  if (!HeapMem.ResizeTo(Size))
    return false;

  // Get memory for bitmap
  {
    TUint_2 BitmapSize = (HeapMem.GetSize() + BitsInByte - 1) / BitsInByte;

    // No memory for bitmap? Release data and return.
    if (!Bitmap.ResizeTo(BitmapSize))
    {
      HeapMem.Release();
      return false;
    }
  }

  IsReadyFlag = true;

  return true;
}

/*
  Return true when we are ready
*/
TBool THeap::IsReady()
{
  return IsReadyFlag;
}

/*
  Reserve block of given size

  Allocated block is always filled with zeroes.
*/
TBool THeap::Reserve(
  TMemorySegment * MemSeg,
  TUint_2 Size
)
{
  // Zero size? Job done!
  if (Size == 0)
    return true;

  TUint_2 InsertIndex;

  // No idea where to place it? Return
  if (!GetInsertIndex(&InsertIndex, Size))
  {
    printf_P(PSTR("[Heap] Failed to reserve.\n"));
    return false;
  }

  // So far <MemSeg.Addr> is just zero-based index
  MemSeg->Addr = InsertIndex;
  MemSeg->Size = Size;

  //* [Sanity] All bits for span in bitmap must be clear (span is free)
  if (!RangeIsSolid(*MemSeg, 0))
  {
    printf_P(PSTR("[Heap] Span is not free.\n"));
    return false;
  }
  //*/

  // Set all bits in bitmap for span
  MarkRange(*MemSeg);

  // Now <MemSeg.Addr> is real address
  MemSeg->Addr = MemSeg->Addr + HeapMem.GetData().Addr;

  // Zero data (design requirement)
  me_MemorySegment::Freetown::ZeroMem(*MemSeg);

  printf_P(PSTR("[Heap] Reserve ( Addr %05u Size %05u )\n"), MemSeg->Addr, MemSeg->Size);

  return true;
}

/*
  Release block described by segment

  Released block may be filled with zeroes.
*/
TBool THeap::Release(
  TMemorySegment * MemSeg
)
{
  // Zero size? Job done!
  if (MemSeg->Size == 0)
  {
    MemSeg->Addr = 0;
    return false;
  }

  printf_P(PSTR("[Heap] Release ( Addr %05u Size %05u )\n"), MemSeg->Addr, MemSeg->Size);

  LastSegSize = MemSeg->Size;

  /*
    We're marking span as free in bitmap.
  */

  // Segment is not in our memory?
  if (!IsOurs(*MemSeg))
  {
    printf_P(PSTR("[Heap] Not ours.\n"));
    return false;
  }

  // Zero data for security (optional)
  me_MemorySegment::Freetown::ZeroMem(*MemSeg);

  // Now <MemSeg.Addr> is zero-based index
  MemSeg->Addr = MemSeg->Addr - HeapMem.GetData().Addr;

  //* [Sanity] All bits for span in bitmap must be set (span is used)
  if (!RangeIsSolid(*MemSeg, 1))
  {
    printf_P(PSTR("[Heap] Span is not solid.\n"));
    return false;
  }
  //*/

  // Clear all bits in bitmap for span
  ClearRange(*MemSeg);

  // Yep, you're free to go
  MemSeg->Addr = 0;
  MemSeg->Size = 0;

  return true;
}

/*
  Get index of empty span in bitmap where we will allocate

  We have freedom where to place span in bitmap.
*/
TBool THeap::GetInsertIndex(
  TUint_2 * Index,
  TUint_2 SegSize
)
{
  /*
    Below is best-fit algorithm.

    But when we found best segment, we'll attach our span to
    it's start or to it's end. We'll attach span to the end if
    current segment size is less than segment size from previous
    call of us. (Current segment size is less than size of
    previously allocated segment.)
  */

  TUint_2 Cursor = 0;
  TUint_2 Limit = HeapMem.GetSize();
  TUint_2 BestIndex = 0xFFFF;
  TUint_2 BestDelta = 0xFFFF;
  TUint_2 BestSpanLen = 0xFFFF;

  TUint_2 NextBusy;
  TUint_2 SpanLen;
  TUint_2 Delta; // Remaining gap
  do
  {
    NextBusy = GetNextBusyIndex(Cursor);

    SpanLen = NextBusy - Cursor;

    if (SpanLen >= SegSize)
    {
      Delta = SpanLen - SegSize;

      if (Delta < BestDelta)
      {
        BestIndex = Cursor;
        BestDelta = Delta;
        BestSpanLen = SpanLen;
      }
    }

    Cursor = NextBusy;

    ++Cursor;
  } while (Cursor < Limit);

  if (BestIndex < Limit)
  {
    TBool AttachToRight = (SegSize < LastSegSize);
    if (AttachToRight)
      BestIndex = BestIndex + (BestSpanLen - SegSize);

    LastSegSize = SegSize;

    *Index = BestIndex;

    return true;
  }

  return false;
}

/*
  Find nearest busy byte to the right
*/
TUint_2 THeap::GetNextBusyIndex(
  TUint_2 StartIdx
)
{
  TUint_2 Limit = HeapMem.GetSize();
  TUint_2 Cursor = StartIdx;

  for (Cursor = StartIdx; Cursor < Limit; ++Cursor)
    if (GetBit(Cursor))
      break;

  return Cursor;
}

/*
  [Internal] Set bits to given value in range
*/
void THeap::SetRange(
  TMemorySegment MemSeg,
  TUint_1 BitsValue
)
{
  TUint_2 StartBitIdx = MemSeg.Addr;
  TUint_2 EndBitIdx = StartBitIdx + MemSeg.Size - 1;

  for (TUint_2 Offset = StartBitIdx; Offset <= EndBitIdx; ++Offset)
    SetBit(Offset, BitsValue);
}

/*
  [Internal] Set bitmap range bits
*/
void THeap::MarkRange(
  TMemorySegment MemSeg
)
{
  SetRange(MemSeg, 1);
}

/*
  [Internal] Clear bitmap range bits
*/
void THeap::ClearRange(
  TMemorySegment MemSeg
)
{
  SetRange(MemSeg, 0);
}

/*
  [Sanity] segment in our managed range?

  Empty segment is never ours.
*/
TBool THeap::IsOurs(
  me_MemorySegment::TMemorySegment MemSeg
)
{
  return
    me_MemorySegment::Freetown::IsInside(MemSeg, HeapMem.GetData());
}

/*
  [Internal] Bits in range have same given value?

  I.e. all bytes in range are marked as used/free in bitmap.
*/
TBool THeap::RangeIsSolid(
  TMemorySegment MemSeg,
  TUint_1 BitsValue
)
{
  TUint_2 StartBitIdx = MemSeg.Addr;
  TUint_2 EndBitIdx = StartBitIdx + MemSeg.Size - 1;

  for (TUint_2 Offset = StartBitIdx; Offset < EndBitIdx; ++Offset)
    if (GetBit(Offset) != BitsValue)
      return false;

  return true;
}

/*
  Return bit value in bitmap's data

  Typically it's called from cycle, so no range checks.
*/
TUint_1 THeap::GetBit(
  TUint_2 BitIndex
)
{
  using
    me_Bits::GetBit;

  TUint_2 ByteIndex = BitIndex / BitsInByte;

  TUint_1 ByteValue = Bitmap.GetData().Bytes[ByteIndex];

  TUint_1 BitOffset = BitIndex % BitsInByte;

  TUint_1 BitValue;

  if (!GetBit(&BitValue, ByteValue, BitOffset))
    return 0;

  return BitValue;
}

/*
  Set bit to given value in bitmap's data

  Same notice as for GetBit() - no checks here.
*/
void THeap::SetBit(
  TUint_2 BitIndex,
  TUint_1 BitValue
)
{
  TUint_2 ByteIndex = BitIndex / BitsInByte;

  TUint_1 ByteValue = Bitmap.GetData().Bytes[ByteIndex];

  TUint_1 BitOffset = BitIndex % BitsInByte;

  me_Bits::SetBitTo(&ByteValue, BitOffset, BitValue);

  Bitmap.GetData().Bytes[ByteIndex] = ByteValue;
}

// ) THeap

/*
  Global instance
*/
me_Heap::THeap Heap;

/*
  2024-10-11
  2024-10-12
  2024-10-13 Two insertion points, optimizing gap for next iteration
  2024-10-25 Using [me_Bits]
  2025-07-29
*/
