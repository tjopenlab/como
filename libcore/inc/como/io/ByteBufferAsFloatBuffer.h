//=========================================================================
// Copyright (C) 2018 The C++ Component Model(COMO) Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#ifndef __COMO_IO_BYTEBUFFERASFLOATBUFFER_H__
#define __COMO_IO_BYTEBUFFERASFLOATBUFFER_H__

#include "como/io/ByteBuffer.h"
#include "como/io/FloatBuffer.h"
#include "como.io.IByteOrder.h"

namespace como {
namespace io {

class ByteBufferAsFloatBuffer
    : public FloatBuffer
{
public:
    ECode Constructor(
        /* [in] */ ByteBuffer* bb,
        /* [in] */ Integer mark,
        /* [in] */ Integer pos,
        /* [in] */ Integer lim,
        /* [in] */ Integer cap,
        /* [in] */ Integer off,
        /* [in] */ IByteOrder* order);

    ECode Slice(
        /* [out] */ AutoPtr<IFloatBuffer>& buffer) override;

    ECode Duplicate(
        /* [out] */ AutoPtr<IFloatBuffer>& buffer) override;

    ECode AsReadOnlyBuffer(
        /* [out] */ AutoPtr<IFloatBuffer>& buffer) override;

    ECode Get(
        /* [out] */ Float& f) override;

    ECode Get(
        /* [in] */ Integer index,
        /* [out] */ Float& f) override;

    ECode Get(
        /* [out] */ Array<Float>& dst,
        /* [in] */ Integer offset,
        /* [in] */ Integer length) override;

    ECode Put(
        /* [in] */ Float f) override;

    ECode Put(
        /* [in] */ Integer index,
        /* [in] */ Float f) override;

    ECode Put(
        /* [in] */ const Array<Float>& src,
        /* [in] */ Integer offset,
        /* [in] */ Integer length) override;

    ECode Compact() override;

    ECode IsDirect(
        /* [out] */ Boolean& direct) override;

    ECode IsReadOnly(
        /* [out] */ Boolean& readOnly) override;

    ECode GetOrder(
        /* [out] */ AutoPtr<IByteOrder>& bo) override;

protected:
    Integer Ix(
        /* [in] */ Integer i);

protected:
    AutoPtr<ByteBuffer> mBB;
    Integer mOffset;

private:
    AutoPtr<IByteOrder> mOrder;
};

inline Integer ByteBufferAsFloatBuffer::Ix(
    /* [in] */ Integer i)
{
    return (i << 2) + mOffset;
}

} // namespace io
} // namespace como

#endif // __COMO_IO_BYTEBUFFERASFLOATBUFFER_H__
