// [me_Heap] Dynamic memory management

/*
  Author: Martin Eden
  Last mod.: 2024-10-12
*/

/*
  Implementation uses bitmap. So to overhead is (N / 8) for managing
  (N) bytes of memory.

  Speed is not our goal. Memory footprint is not our goal.
  Our goal is fragmentation-resilience.
*/

#include "me_Heap.h"

#include <me_BaseTypes.h>
#include <me_MemorySegment.h>
#include <me_ManagedMemory.h>
#include <me_Console.h> // debug output

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
    "<OurGlobalName>.GetIsReady()".

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

  IsReadyFlag = false;

  // No memory for data? Return
  if (!HeapMem.ResizeTo(Size))
    return false;

  // Get memory for bitmap
  {
    TUint_4 NumBits = (TUint_4) HeapMem.GetSize() * BitsInByte;

    const TUint_4 MaxUint_2 = 0x0000FFFF;

    // Number of bits is more than 64 Ki? Means .Size >= 8 KiB. Return.
    if (NumBits > MaxUint_2)
      return false;

    TUint_2 BitmapSize = (HeapMem.GetSize() + BitsInByte - 1) / BitsInByte;

    // No memory for bitmap? Return.
    if (!Bitmap.ResizeTo(BitmapSize))
      return false;
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
*/
TBool THeap::Reserve(
  TMemorySegment * MemSeg,
  TUint_2 Size
)
{
  TUint_2 InsertIndex;

  // No idea where to place it? Return
  if (!GetInsertIndex(Size, &InsertIndex))
    return false;

  MemSeg->Start.Addr = InsertIndex;
  MemSeg->Size = Size;

  // Set all bits in bitmap for span
  SetRange(*MemSeg, true);

  // Add base offset to returned memory segment
  MemSeg->Start.Addr = MemSeg->Start.Addr + HeapMem.GetData().Start.Addr;

  return true;
}

/*
  Release block described by segment
*/
TBool THeap::Release(
  TMemorySegment * MemSeg
)
{
  /*
    We're marking span as free in bitmap.
  */

  // Segment is not in managed memory? WTF?
  if (!IsOurs(*MemSeg))
    return false;

  // Subtract base offset
  MemSeg->Start.Addr = MemSeg->Start.Addr - HeapMem.GetData().Start.Addr;

  // Sanity check: All bits for span in bitmap must be set (span is used)
  if (!RangeIsSolid(*MemSeg, true))
    return false;

  // Clear all bits in bitmap for span
  SetRange(*MemSeg, false);

  // Yep, you're free to go
  MemSeg->Start.Addr = 0;
  MemSeg->Size = 0;

  return true;
}

/*
  Get index of empty span in bitmap where we will allocate

  We have freedom where to place span in bitmap.
*/
TBool THeap::GetInsertIndex(
  TUint_2 SpanSize,
  TUint_2 * InsertIndex
)
{
  /*
    Greedy strategies like "smallest gap nearby" are lame.
    Memory fragmentation punishes them real hard.

    We're using Golden ratio because it sounds cool.

    When we asked to get range for 100 bytes in empty 1000-bytes
    segment, we're returning index range [62, 161].
  */
  TUint_2 IdealSize = (TFloat) 1.62 * SpanSize;

  if (!FindSpan(InsertIndex, SpanSize, IdealSize))
    return false;

  return true;
}

/*
  Find free span between minimum and ideal size
*/
TBool THeap::FindSpan(
  TUint_2 * Index,
  TUint_2 MinSize,
  TUint_2 IdealSize
)
{
  TUint_2 Cursor = 0;
  TUint_2 Limit = HeapMem.GetSize();
  TUint_2 BestIndex = 0xFFFF;
  TUint_2 BestDelta = 0xFFFF;
  TUint_2 BestSpan = 0xFFFF;

  TUint_2 NextBusy;
  TUint_2 SpanLength;
  do
  {
    NextBusy = GetNextBusyIndex(Cursor);

    SpanLength = NextBusy - Cursor;

    if (SpanLength > 0)
    {
      if (SpanLength >= MinSize)
      {
        TUint_2 Delta;

        if (SpanLength <= IdealSize)
          Delta = IdealSize - SpanLength;
        else
          Delta = SpanLength - IdealSize;

        if (Delta < BestDelta)
        {
          BestIndex = Cursor;
          BestDelta = Delta;
          BestSpan = SpanLength;
        }
      }
    }

    Cursor = NextBusy;

    ++Cursor;
  } while (Cursor < Limit);

  if (BestIndex < Limit)
  {
    // Correct best index
    if (BestSpan < IdealSize)
      BestIndex = BestIndex + (BestSpan - MinSize);
    else
      BestIndex = BestIndex + (IdealSize - MinSize);

    *Index = BestIndex;

    return true;
  }

  return false;
}

/*
  Find nearest busy byte
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
  TBool BitsValue
)
{
  TUint_2 StartBitIdx = MemSeg.Start.Addr;
  TUint_2 EndBitIdx = StartBitIdx + MemSeg.Size - 1;

  for (TUint_2 Offset = StartBitIdx; Offset <= EndBitIdx; ++Offset)
    SetBit(Offset, BitsValue);
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
  TBool BitsValue
)
{
  TUint_2 StartBitIdx = MemSeg.Start.Addr;
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
TBool THeap::GetBit(
  TUint_2 BitIndex
)
{
  TUint_2 ByteIndex = BitIndex / BitsInByte;
  TUint_1 ByteValue = Bitmap.GetData().Bytes[ByteIndex];

  TUint_1 BitOffset = BitIndex % BitsInByte;

  TUint_1 ByteMask = (1 << BitOffset);

  TBool BitValue = ((ByteValue & ByteMask) != 0);

  return BitValue;
}

/*
  Set bit to given value in bitmap's data

  Same notice as for GetBit() - no checks here.
*/
void THeap::SetBit(
  TUint_2 BitIndex,
  TBool BitValue
)
{
  TUint_2 ByteIndex = BitIndex / BitsInByte;
  TUint_1 ByteValue = Bitmap.GetData().Bytes[ByteIndex];

  TUint_1 BitOffset = BitIndex % BitsInByte;

  TUint_1 ByteMask;

  if (BitValue == false)
  {
    ByteMask = ~(1 << BitOffset);
    ByteValue = ByteValue & ByteMask;
  }
  else if (BitValue == true)
  {
    ByteMask = (1 << BitOffset);
    ByteValue = ByteValue | ByteMask;
  }

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
*/
