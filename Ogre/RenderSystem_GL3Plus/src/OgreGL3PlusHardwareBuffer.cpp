/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  -----------------------------------------------------------------------------
*/

#include "OgreGL3PlusHardwareBuffer.h"
#include "OgreRoot.h"
#include "OgreGL3PlusRenderSystem.h"
#include "OgreGL3PlusStateCacheManager.h"

namespace Ogre {

    GL3PlusHardwareBuffer::GL3PlusHardwareBuffer(GLenum target, size_t sizeInBytes, GLenum usage)
    : mTarget(target), mSizeInBytes(sizeInBytes), mUsage(usage)
    {
        mRenderSystem = static_cast<GL3PlusRenderSystem*>(Root::getSingleton().getRenderSystem());

        OGRE_CHECK_GL_ERROR(glGenBuffers(1, &mBufferId));

        if (!mBufferId)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Cannot create GL vertex buffer",
                        "GL3PlusHardwareBuffer::GL3PlusHardwareBuffer");
        }
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(mTarget, mBufferId);
        OGRE_CHECK_GL_ERROR(glBufferData(mTarget, mSizeInBytes, NULL, getGLUsage(mUsage)));

        //        std::cerr << "creating buffer = " << mBufferId << std::endl;
    }

    GL3PlusHardwareBuffer::~GL3PlusHardwareBuffer()
    {
        if(GL3PlusStateCacheManager* stateCacheManager = mRenderSystem->_getStateCacheManager())
            stateCacheManager->deleteGLBuffer(mTarget,mBufferId);
    }

    void* GL3PlusHardwareBuffer::lockImpl(size_t offset, size_t length,
                                          HardwareBuffer::LockOptions options)
    {
        GLenum access = 0;

        // Use glMapBuffer
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(mTarget,mBufferId);

        bool writeOnly =
            options == HardwareBuffer::HBL_WRITE_ONLY ||
            ((mUsage & HardwareBuffer::HBU_WRITE_ONLY) &&
             options != HardwareBuffer::HBL_READ_ONLY && options != HardwareBuffer::HBL_NORMAL);

        if (writeOnly)
        {
            access |= GL_MAP_WRITE_BIT;
            if(options == HardwareBuffer::HBL_DISCARD || options == HardwareBuffer::HBL_NO_OVERWRITE)
            {
                // Discard the buffer
                access |= GL_MAP_INVALIDATE_RANGE_BIT;
            }
            //FIXME Is this correct usage for shader storage buffers?
            access |= GL_MAP_UNSYNCHRONIZED_BIT;
        }
        else if (options == HardwareBuffer::HBL_READ_ONLY)
            access |= GL_MAP_READ_BIT;
        else
            access |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        // FIXME: Big stall here
        void* pBuffer;
        OGRE_CHECK_GL_ERROR(pBuffer = glMapBufferRange(mTarget, offset, length, access));

        if(pBuffer == 0)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Buffer: Out of memory",
                        "GL3PlusHardwareBuffer::lock");
        }


        // pBuffer is already offsetted in glMapBufferRange
        return pBuffer;
    }

    void GL3PlusHardwareBuffer::unlockImpl()
    {
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(mTarget, mBufferId);

        GLboolean mapped;
        OGRE_CHECK_GL_ERROR(mapped = glUnmapBuffer(mTarget));
        if(!mapped)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                        "Buffer data corrupted, please reload",
                        "GL3PlusHardwareBuffer::unlock");
        }
    }

    void GL3PlusHardwareBuffer::readData(size_t offset, size_t length, void* pDest)
    {
        // get data from the real buffer
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(mTarget, mBufferId);

        OGRE_CHECK_GL_ERROR(glGetBufferSubData(mTarget, offset, length, pDest));
    }

    void GL3PlusHardwareBuffer::writeData(size_t offset, size_t length, const void* pSource,
                                          bool discardWholeBuffer)
    {
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(mTarget, mBufferId);

        if (offset == 0 && length == mSizeInBytes)
        {
            OGRE_CHECK_GL_ERROR(glBufferData(mTarget, mSizeInBytes, pSource, getGLUsage(mUsage)));
        }
        else
        {
            if(discardWholeBuffer)
            {
                OGRE_CHECK_GL_ERROR(glBufferData(mTarget, mSizeInBytes, NULL, getGLUsage(mUsage)));
            }

            OGRE_CHECK_GL_ERROR(glBufferSubData(mTarget, offset, length, pSource));
        }
    }

    void GL3PlusHardwareBuffer::copyData(GLuint srcBufferId, size_t srcOffset, size_t dstOffset,
                                         size_t length, bool discardWholeBuffer)
    {
        // Zero out this(destination) buffer
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(mTarget, mBufferId);
        OGRE_CHECK_GL_ERROR(glBufferData(mTarget, length, 0, getGLUsage(mUsage)));

        // Do it the fast way.
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(GL_COPY_READ_BUFFER, srcBufferId);
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(GL_COPY_WRITE_BUFFER, mBufferId);

        OGRE_CHECK_GL_ERROR(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, srcOffset, dstOffset, length));

        mRenderSystem->_getStateCacheManager()->bindGLBuffer(GL_COPY_READ_BUFFER, 0);
        mRenderSystem->_getStateCacheManager()->bindGLBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    GLenum GL3PlusHardwareBuffer::getGLUsage(uint32 usage)
    {
        return  (usage & HardwareBuffer::HBU_DISCARDABLE) ? GL_STREAM_DRAW :
                (usage & HardwareBuffer::HBU_STATIC) ? GL_STATIC_DRAW :
                GL_DYNAMIC_DRAW;
    }
}
