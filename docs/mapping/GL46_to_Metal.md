# OpenGL 4.6 Core -> Metal (iOS) — Bảng ánh xạ đầy đủ 657 hàm

Nguồn: `docs/khronos/gl.xml` + `docs/khronos/extensions/*.txt` + `docs/apple/Metal-Shading-Language-Specification.pdf`.

| # | GL function | Introduced | Metal / TGLMT strategy |
|---|-------------|------------|--------------------------|
| 1 | `glActiveShaderProgram` | GL_VERSION_4_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 2 | `glActiveTexture` | GL_VERSION_1_3 | Chọn texture unit hiện tại (tracker CPU) -> setFragmentTexture:atIndex: |
| 3 | `glAttachShader` | GL_VERSION_2_0 | Gắn shader object vào program object (tracker CPU) |
| 4 | `glBeginConditionalRender` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 5 | `glBeginQuery` | GL_VERSION_1_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 6 | `glBeginQueryIndexed` | GL_VERSION_4_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 7 | `glBeginTransformFeedback` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 8 | `glBindAttribLocation` | GL_VERSION_2_0 | Ghi attribute binding trước link; ánh xạ vào MTLVertexDescriptor.attribute[n] |
| 9 | `glBindBuffer` | GL_VERSION_1_5 | MTLDevice.newBuffer / setVertexBuffer:setIndexBuffer: ; tracker binding theo target |
| 10 | `glBindBufferBase` | GL_VERSION_3_0 | MTLDevice.newBuffer / setVertexBuffer:setIndexBuffer: ; tracker binding theo target |
| 11 | `glBindBufferRange` | GL_VERSION_3_0 | MTLDevice.newBuffer / setVertexBuffer:setIndexBuffer: ; tracker binding theo target |
| 12 | `glBindBuffersBase` | GL_VERSION_4_4 | MTLDevice.newBuffer / setVertexBuffer:setIndexBuffer: ; tracker binding theo target |
| 13 | `glBindBuffersRange` | GL_VERSION_4_4 | MTLDevice.newBuffer / setVertexBuffer:setIndexBuffer: ; tracker binding theo target |
| 14 | `glBindFragDataLocation` | GL_VERSION_3_0 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 15 | `glBindFragDataLocationIndexed` | GL_VERSION_3_3 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 16 | `glBindFramebuffer` | GL_VERSION_3_0 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 17 | `glBindImageTexture` | GL_VERSION_4_2 | MTLTexture argument buffer read/write (setTexture:atIndex:) + textureType kiểm tra |
| 18 | `glBindImageTextures` | GL_VERSION_4_4 | MTLTexture argument buffer read/write (setTexture:atIndex:) + textureType kiểm tra |
| 19 | `glBindProgramPipeline` | GL_VERSION_4_1 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 20 | `glBindRenderbuffer` | GL_VERSION_3_0 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 21 | `glBindSampler` | GL_VERSION_3_3 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 22 | `glBindSamplers` | GL_VERSION_4_4 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 23 | `glBindTexture` | GL_VERSION_1_1 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 24 | `glBindTextureUnit` | GL_VERSION_4_5 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 25 | `glBindTextures` | GL_VERSION_4_4 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 26 | `glBindTransformFeedback` | GL_VERSION_4_0 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 27 | `glBindVertexArray` | GL_VERSION_3_0 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 28 | `glBindVertexBuffer` | GL_VERSION_4_3 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 29 | `glBindVertexBuffers` | GL_VERSION_4_4 | Bind vào context slot hiện tại (texture unit / buffer base / pipeline) |
| 30 | `glBlendColor` | GL_VERSION_1_4 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 31 | `glBlendEquation` | GL_VERSION_1_4 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 32 | `glBlendEquationSeparate` | GL_VERSION_2_0 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 33 | `glBlendEquationSeparatei` | GL_VERSION_4_0 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 34 | `glBlendEquationi` | GL_VERSION_4_0 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 35 | `glBlendFunc` | GL_VERSION_1_0 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 36 | `glBlendFuncSeparate` | GL_VERSION_1_4 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 37 | `glBlendFuncSeparatei` | GL_VERSION_4_0 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 38 | `glBlendFunci` | GL_VERSION_4_0 | MTLRenderPipelineDescriptor.colorAttachments[i].(blendingEnabled,rgbBlendOperation,source/dest factors) |
| 39 | `glBlitFramebuffer` | GL_VERSION_3_0 | MTLBlitCommandEncoder.copyFromTexture / generateMipmaps; resolve MSAA qua storeActionMultisampleResolve |
| 40 | `glBlitNamedFramebuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 41 | `glBufferData` | GL_VERSION_1_5 | MTLBuffer (newBufferWithBytes:newBufferWithLength:options:) + memcpy via contents() |
| 42 | `glBufferStorage` | GL_VERSION_4_4 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 43 | `glBufferSubData` | GL_VERSION_1_5 | MTLBuffer.contents()+memcpy; didModifyRange trên iOS/macOS managed |
| 44 | `glCheckFramebufferStatus` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 45 | `glCheckNamedFramebufferStatus` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 46 | `glClampColor` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 47 | `glClear` | GL_VERSION_1_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 48 | `glClearBufferData` | GL_VERSION_4_3 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 49 | `glClearBufferSubData` | GL_VERSION_4_3 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 50 | `glClearBufferfi` | GL_VERSION_3_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 51 | `glClearBufferfv` | GL_VERSION_3_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 52 | `glClearBufferiv` | GL_VERSION_3_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 53 | `glClearBufferuiv` | GL_VERSION_3_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 54 | `glClearColor` | GL_VERSION_1_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 55 | `glClearDepth` | GL_VERSION_1_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 56 | `glClearDepthf` | GL_VERSION_4_1 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 57 | `glClearNamedBufferData` | GL_VERSION_4_5 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 58 | `glClearNamedBufferSubData` | GL_VERSION_4_5 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 59 | `glClearNamedFramebufferfi` | GL_VERSION_4_5 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 60 | `glClearNamedFramebufferfv` | GL_VERSION_4_5 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 61 | `glClearNamedFramebufferiv` | GL_VERSION_4_5 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 62 | `glClearNamedFramebufferuiv` | GL_VERSION_4_5 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 63 | `glClearStencil` | GL_VERSION_1_0 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 64 | `glClearTexImage` | GL_VERSION_4_4 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 65 | `glClearTexSubImage` | GL_VERSION_4_4 | MTLRenderPassDescriptor loadAction=Clear + clearColor/clearDepth/clearStencil |
| 66 | `glClientWaitSync` | GL_VERSION_3_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 67 | `glClipControl` | GL_VERSION_4_5 | Metal NDC z 0..1 cố định; GL_CLIP_ORIGIN/CONTROL emulated bằng y-flip + viewport transform |
| 68 | `glColorMask` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 69 | `glColorMaski` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 70 | `glCompileShader` | GL_VERSION_2_0 | Biên dịch MSL offline (metal -c) hoặc runtime newLibraryWithSource; log qua MTLLibrary.error |
| 71 | `glCompressedTexImage1D` | GL_VERSION_1_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 72 | `glCompressedTexImage2D` | GL_VERSION_1_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 73 | `glCompressedTexImage3D` | GL_VERSION_1_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 74 | `glCompressedTexSubImage1D` | GL_VERSION_1_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 75 | `glCompressedTexSubImage2D` | GL_VERSION_1_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 76 | `glCompressedTexSubImage3D` | GL_VERSION_1_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 77 | `glCompressedTextureSubImage1D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 78 | `glCompressedTextureSubImage2D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 79 | `glCompressedTextureSubImage3D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 80 | `glCopyBufferSubData` | GL_VERSION_3_1 | MTLBlitCommandEncoder.copyFromBuffer:toBuffer: |
| 81 | `glCopyImageSubData` | GL_VERSION_4_3 | MTLBlitCommandEncoder copy path |
| 82 | `glCopyNamedBufferSubData` | GL_VERSION_4_5 | MTLBlitCommandEncoder copy path |
| 83 | `glCopyTexImage1D` | GL_VERSION_1_1 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 84 | `glCopyTexImage2D` | GL_VERSION_1_1 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 85 | `glCopyTexSubImage1D` | GL_VERSION_1_1 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 86 | `glCopyTexSubImage2D` | GL_VERSION_1_1 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 87 | `glCopyTexSubImage3D` | GL_VERSION_1_2 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 88 | `glCopyTextureSubImage1D` | GL_VERSION_4_5 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 89 | `glCopyTextureSubImage2D` | GL_VERSION_4_5 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 90 | `glCopyTextureSubImage3D` | GL_VERSION_4_5 | MTLBlitCommandEncoder.copyFromTexture:toTexture: |
| 91 | `glCreateBuffers` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 92 | `glCreateFramebuffers` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 93 | `glCreateProgram` | GL_VERSION_2_0 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 94 | `glCreateProgramPipelines` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 95 | `glCreateQueries` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 96 | `glCreateRenderbuffers` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 97 | `glCreateSamplers` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 98 | `glCreateShader` | GL_VERSION_2_0 | MTLFunction descriptor placeholder; biên dịch ở LinkProgram |
| 99 | `glCreateShaderProgramv` | GL_VERSION_4_1 | MTLFunction descriptor placeholder; biên dịch ở LinkProgram |
| 100 | `glCreateTextures` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 101 | `glCreateTransformFeedbacks` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 102 | `glCreateVertexArrays` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 103 | `glCullFace` | GL_VERSION_1_0 | setCullMode: + setFrontFacingWinding: |
| 104 | `glDebugMessageCallback` | GL_VERSION_4_3 | MTLLog / os_log + validation layer callback |
| 105 | `glDebugMessageControl` | GL_VERSION_4_3 | MTLLog / os_log + validation layer callback |
| 106 | `glDebugMessageInsert` | GL_VERSION_4_3 | MTLLog / os_log + validation layer callback |
| 107 | `glDeleteBuffers` | GL_VERSION_1_5 | Giải phóng MTLResource + xóa registry |
| 108 | `glDeleteFramebuffers` | GL_VERSION_3_0 | Giải phóng MTLResource + xóa registry |
| 109 | `glDeleteProgram` | GL_VERSION_2_0 | Giải phóng MTLResource + xóa registry |
| 110 | `glDeleteProgramPipelines` | GL_VERSION_4_1 | Giải phóng MTLResource + xóa registry |
| 111 | `glDeleteQueries` | GL_VERSION_1_5 | Giải phóng MTLResource + xóa registry |
| 112 | `glDeleteRenderbuffers` | GL_VERSION_3_0 | Giải phóng MTLResource + xóa registry |
| 113 | `glDeleteSamplers` | GL_VERSION_3_3 | Giải phóng MTLResource + xóa registry |
| 114 | `glDeleteShader` | GL_VERSION_2_0 | Giải phóng MTLResource + xóa registry |
| 115 | `glDeleteSync` | GL_VERSION_3_2 | Giải phóng MTLResource + xóa registry |
| 116 | `glDeleteTextures` | GL_VERSION_1_1 | Giải phóng MTLResource + xóa registry |
| 117 | `glDeleteTransformFeedbacks` | GL_VERSION_4_0 | Giải phóng MTLResource + xóa registry |
| 118 | `glDeleteVertexArrays` | GL_VERSION_3_0 | Giải phóng MTLResource + xóa registry |
| 119 | `glDepthFunc` | GL_VERSION_1_0 | MTLDepthStencilDescriptor (depthCompareFunction, isDepthWriteEnabled) -> MTLDepthStencilState |
| 120 | `glDepthMask` | GL_VERSION_1_0 | MTLDepthStencilDescriptor (depthCompareFunction, isDepthWriteEnabled) -> MTLDepthStencilState |
| 121 | `glDepthRange` | GL_VERSION_1_0 | MTLDepthStencilDescriptor (depthCompareFunction, isDepthWriteEnabled) -> MTLDepthStencilState |
| 122 | `glDepthRangeArrayv` | GL_VERSION_4_1 | MTLDepthStencilDescriptor (depthCompareFunction, isDepthWriteEnabled) -> MTLDepthStencilState |
| 123 | `glDepthRangeIndexed` | GL_VERSION_4_1 | MTLDepthStencilDescriptor (depthCompareFunction, isDepthWriteEnabled) -> MTLDepthStencilState |
| 124 | `glDepthRangef` | GL_VERSION_4_1 | MTLDepthStencilDescriptor (depthCompareFunction, isDepthWriteEnabled) -> MTLDepthStencilState |
| 125 | `glDetachShader` | GL_VERSION_2_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 126 | `glDisable` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 127 | `glDisableVertexArrayAttrib` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 128 | `glDisableVertexAttribArray` | GL_VERSION_2_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 129 | `glDisablei` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 130 | `glDispatchCompute` | GL_VERSION_4_3 | MTLComputeCommandEncoder.dispatchThreads / dispatchThreadgroups + setComputePipelineState |
| 131 | `glDispatchComputeIndirect` | GL_VERSION_4_3 | MTLComputeCommandEncoder.dispatchThreads / dispatchThreadgroups + setComputePipelineState |
| 132 | `glDrawArrays` | GL_VERSION_1_1 | MTLRenderCommandEncoder.drawPrimitives:vertexStart:vertexCount: |
| 133 | `glDrawArraysIndirect` | GL_VERSION_4_0 | MTLRenderCommandEncoder.drawPrimitives:vertexStart:vertexCount: |
| 134 | `glDrawArraysInstanced` | GL_VERSION_3_1 | MTLRenderCommandEncoder.drawPrimitives:vertexStart:vertexCount: |
| 135 | `glDrawArraysInstancedBaseInstance` | GL_VERSION_4_2 | MTLRenderCommandEncoder.drawPrimitives:vertexStart:vertexCount: |
| 136 | `glDrawBuffer` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 137 | `glDrawBuffers` | GL_VERSION_2_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 138 | `glDrawElements` | GL_VERSION_1_1 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 139 | `glDrawElementsBaseVertex` | GL_VERSION_3_2 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 140 | `glDrawElementsIndirect` | GL_VERSION_4_0 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 141 | `glDrawElementsInstanced` | GL_VERSION_3_1 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 142 | `glDrawElementsInstancedBaseInstance` | GL_VERSION_4_2 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 143 | `glDrawElementsInstancedBaseVertex` | GL_VERSION_3_2 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 144 | `glDrawElementsInstancedBaseVertexBaseInstance` | GL_VERSION_4_2 | MTLRenderCommandEncoder.drawIndexedPrimitives:indexCount:indexType:indexBuffer:indexBufferOffset: |
| 145 | `glDrawRangeElements` | GL_VERSION_1_2 | drawIndexedPrimitives: (range clamp trên CPU trước khi encode) |
| 146 | `glDrawRangeElementsBaseVertex` | GL_VERSION_3_2 | drawIndexedPrimitives: (range clamp trên CPU trước khi encode) |
| 147 | `glDrawTransformFeedback` | GL_VERSION_4_0 | Emulation: buffer capture pass + drawPrimitives từ captured count (Metal không có TF native) |
| 148 | `glDrawTransformFeedbackInstanced` | GL_VERSION_4_2 | Emulation: buffer capture pass + drawPrimitives từ captured count (Metal không có TF native) |
| 149 | `glDrawTransformFeedbackStream` | GL_VERSION_4_0 | Emulation: buffer capture pass + drawPrimitives từ captured count (Metal không có TF native) |
| 150 | `glDrawTransformFeedbackStreamInstanced` | GL_VERSION_4_2 | Emulation: buffer capture pass + drawPrimitives từ captured count (Metal không có TF native) |
| 151 | `glEnable` | GL_VERSION_1_0 | Trạng thái pipeline: depthCompareFunction/isDepthWriteEnabled, cullMode, blendingEnabled... (bake vào pipeline) |
| 152 | `glEnableVertexArrayAttrib` | GL_VERSION_4_5 | Trạng thái pipeline: depthCompareFunction/isDepthWriteEnabled, cullMode, blendingEnabled... (bake vào pipeline) |
| 153 | `glEnableVertexAttribArray` | GL_VERSION_2_0 | Trạng thái pipeline: depthCompareFunction/isDepthWriteEnabled, cullMode, blendingEnabled... (bake vào pipeline) |
| 154 | `glEnablei` | GL_VERSION_3_0 | Trạng thái pipeline: depthCompareFunction/isDepthWriteEnabled, cullMode, blendingEnabled... (bake vào pipeline) |
| 155 | `glEndConditionalRender` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 156 | `glEndQuery` | GL_VERSION_1_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 157 | `glEndQueryIndexed` | GL_VERSION_4_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 158 | `glEndTransformFeedback` | GL_VERSION_3_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 159 | `glFenceSync` | GL_VERSION_3_2 | MTLEvent/MTLSharedEvent + encodeSignalEvent/encodeWaitForEvent (Metal 3+) |
| 160 | `glFinish` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 161 | `glFlush` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 162 | `glFlushMappedBufferRange` | GL_VERSION_3_0 | didModifyRange: (managed) / flush toàn buffer |
| 163 | `glFlushMappedNamedBufferRange` | GL_VERSION_4_5 | didModifyRange: (managed) / flush toàn buffer |
| 164 | `glFramebufferParameteri` | GL_VERSION_4_3 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 165 | `glFramebufferRenderbuffer` | GL_VERSION_3_0 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 166 | `glFramebufferTexture` | GL_VERSION_3_2 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 167 | `glFramebufferTexture1D` | GL_VERSION_3_0 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 168 | `glFramebufferTexture2D` | GL_VERSION_3_0 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 169 | `glFramebufferTexture3D` | GL_VERSION_3_0 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 170 | `glFramebufferTextureLayer` | GL_VERSION_3_0 | MTLRenderPassDescriptor (colorAttachments[i], depthAttachment, stencilAttachment) |
| 171 | `glFrontFace` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 172 | `glGenBuffers` | GL_VERSION_1_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 173 | `glGenFramebuffers` | GL_VERSION_3_0 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 174 | `glGenProgramPipelines` | GL_VERSION_4_1 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 175 | `glGenQueries` | GL_VERSION_1_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 176 | `glGenRenderbuffers` | GL_VERSION_3_0 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 177 | `glGenSamplers` | GL_VERSION_3_3 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 178 | `glGenTextures` | GL_VERSION_1_1 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 179 | `glGenTransformFeedbacks` | GL_VERSION_4_0 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 180 | `glGenVertexArrays` | GL_VERSION_3_0 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 181 | `glGenerateMipmap` | GL_VERSION_3_0 | MTLBlitCommandEncoder.generateMipmapsForTexture: |
| 182 | `glGenerateTextureMipmap` | GL_VERSION_4_5 | Cấp ID + tạo MTLResource placeholder (lazy alloc) |
| 183 | `glGetActiveAtomicCounterBufferiv` | GL_VERSION_4_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 184 | `glGetActiveAttrib` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 185 | `glGetActiveSubroutineName` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 186 | `glGetActiveSubroutineUniformName` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 187 | `glGetActiveSubroutineUniformiv` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 188 | `glGetActiveUniform` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 189 | `glGetActiveUniformBlockName` | GL_VERSION_3_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 190 | `glGetActiveUniformBlockiv` | GL_VERSION_3_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 191 | `glGetActiveUniformName` | GL_VERSION_3_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 192 | `glGetActiveUniformsiv` | GL_VERSION_3_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 193 | `glGetAttachedShaders` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 194 | `glGetAttribLocation` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 195 | `glGetBooleani_v` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 196 | `glGetBooleanv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 197 | `glGetBufferParameteri64v` | GL_VERSION_3_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 198 | `glGetBufferParameteriv` | GL_VERSION_1_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 199 | `glGetBufferPointerv` | GL_VERSION_1_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 200 | `glGetBufferSubData` | GL_VERSION_1_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 201 | `glGetCompressedTexImage` | GL_VERSION_1_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 202 | `glGetCompressedTextureImage` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 203 | `glGetCompressedTextureSubImage` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 204 | `glGetDebugMessageLog` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 205 | `glGetDoublei_v` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 206 | `glGetDoublev` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 207 | `glGetError` | GL_VERSION_1_0 | Metal không có glGetError -> validation layer + tracker lỗi CPU (KHR_no_error: bỏ check để tối ưu) |
| 208 | `glGetFloati_v` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 209 | `glGetFloatv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 210 | `glGetFragDataIndex` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 211 | `glGetFragDataLocation` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 212 | `glGetFramebufferAttachmentParameteriv` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 213 | `glGetFramebufferParameteriv` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 214 | `glGetGraphicsResetStatus` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 215 | `glGetInteger64i_v` | GL_VERSION_3_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 216 | `glGetInteger64v` | GL_VERSION_3_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 217 | `glGetIntegeri_v` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 218 | `glGetIntegerv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 219 | `glGetInternalformati64v` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 220 | `glGetInternalformativ` | GL_VERSION_4_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 221 | `glGetMultisamplefv` | GL_VERSION_3_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 222 | `glGetNamedBufferParameteri64v` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 223 | `glGetNamedBufferParameteriv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 224 | `glGetNamedBufferPointerv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 225 | `glGetNamedBufferSubData` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 226 | `glGetNamedFramebufferAttachmentParameteriv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 227 | `glGetNamedFramebufferParameteriv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 228 | `glGetNamedRenderbufferParameteriv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 229 | `glGetObjectLabel` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 230 | `glGetObjectPtrLabel` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 231 | `glGetPointerv` | GL_VERSION_1_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 232 | `glGetProgramBinary` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 233 | `glGetProgramInfoLog` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 234 | `glGetProgramInterfaceiv` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 235 | `glGetProgramPipelineInfoLog` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 236 | `glGetProgramPipelineiv` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 237 | `glGetProgramResourceIndex` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 238 | `glGetProgramResourceLocation` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 239 | `glGetProgramResourceLocationIndex` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 240 | `glGetProgramResourceName` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 241 | `glGetProgramResourceiv` | GL_VERSION_4_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 242 | `glGetProgramStageiv` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 243 | `glGetProgramiv` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 244 | `glGetQueryBufferObjecti64v` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 245 | `glGetQueryBufferObjectiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 246 | `glGetQueryBufferObjectui64v` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 247 | `glGetQueryBufferObjectuiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 248 | `glGetQueryIndexediv` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 249 | `glGetQueryObjecti64v` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 250 | `glGetQueryObjectiv` | GL_VERSION_1_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 251 | `glGetQueryObjectui64v` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 252 | `glGetQueryObjectuiv` | GL_VERSION_1_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 253 | `glGetQueryiv` | GL_VERSION_1_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 254 | `glGetRenderbufferParameteriv` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 255 | `glGetSamplerParameterIiv` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 256 | `glGetSamplerParameterIuiv` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 257 | `glGetSamplerParameterfv` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 258 | `glGetSamplerParameteriv` | GL_VERSION_3_3 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 259 | `glGetShaderInfoLog` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 260 | `glGetShaderPrecisionFormat` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 261 | `glGetShaderSource` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 262 | `glGetShaderiv` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 263 | `glGetString` | GL_VERSION_1_0 | Trả về chuỗi hằng: renderer 'Apple GPU (Metal)', version '4.6 (TGLMT Metal port)' |
| 264 | `glGetStringi` | GL_VERSION_3_0 | Trả về chuỗi hằng: renderer 'Apple GPU (Metal)', version '4.6 (TGLMT Metal port)' |
| 265 | `glGetSubroutineIndex` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 266 | `glGetSubroutineUniformLocation` | GL_VERSION_4_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 267 | `glGetSynciv` | GL_VERSION_3_2 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 268 | `glGetTexImage` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 269 | `glGetTexLevelParameterfv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 270 | `glGetTexLevelParameteriv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 271 | `glGetTexParameterIiv` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 272 | `glGetTexParameterIuiv` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 273 | `glGetTexParameterfv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 274 | `glGetTexParameteriv` | GL_VERSION_1_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 275 | `glGetTextureImage` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 276 | `glGetTextureLevelParameterfv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 277 | `glGetTextureLevelParameteriv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 278 | `glGetTextureParameterIiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 279 | `glGetTextureParameterIuiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 280 | `glGetTextureParameterfv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 281 | `glGetTextureParameteriv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 282 | `glGetTextureSubImage` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 283 | `glGetTransformFeedbackVarying` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 284 | `glGetTransformFeedbacki64_v` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 285 | `glGetTransformFeedbacki_v` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 286 | `glGetTransformFeedbackiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 287 | `glGetUniformBlockIndex` | GL_VERSION_3_1 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 288 | `glGetUniformIndices` | GL_VERSION_3_1 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 289 | `glGetUniformLocation` | GL_VERSION_2_0 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 290 | `glGetUniformSubroutineuiv` | GL_VERSION_4_0 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 291 | `glGetUniformdv` | GL_VERSION_4_0 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 292 | `glGetUniformfv` | GL_VERSION_2_0 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 293 | `glGetUniformiv` | GL_VERSION_2_0 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 294 | `glGetUniformuiv` | GL_VERSION_3_0 | Đọc shadow copy trên CPU (Metal không cho đọc GPU buffer đồng bộ) |
| 295 | `glGetVertexArrayIndexed64iv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 296 | `glGetVertexArrayIndexediv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 297 | `glGetVertexArrayiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 298 | `glGetVertexAttribIiv` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 299 | `glGetVertexAttribIuiv` | GL_VERSION_3_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 300 | `glGetVertexAttribLdv` | GL_VERSION_4_1 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 301 | `glGetVertexAttribPointerv` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 302 | `glGetVertexAttribdv` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 303 | `glGetVertexAttribfv` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 304 | `glGetVertexAttribiv` | GL_VERSION_2_0 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 305 | `glGetnCompressedTexImage` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 306 | `glGetnTexImage` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 307 | `glGetnUniformdv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 308 | `glGetnUniformfv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 309 | `glGetnUniformiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 310 | `glGetnUniformuiv` | GL_VERSION_4_5 | Shadow state trên CPU (Metal không cho query GPU đồng bộ); trả về tracker |
| 311 | `glHint` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 312 | `glInvalidateBufferData` | GL_VERSION_4_3 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 313 | `glInvalidateBufferSubData` | GL_VERSION_4_3 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 314 | `glInvalidateFramebuffer` | GL_VERSION_4_3 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 315 | `glInvalidateNamedFramebufferData` | GL_VERSION_4_5 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 316 | `glInvalidateNamedFramebufferSubData` | GL_VERSION_4_5 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 317 | `glInvalidateSubFramebuffer` | GL_VERSION_4_3 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 318 | `glInvalidateTexImage` | GL_VERSION_4_3 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 319 | `glInvalidateTexSubImage` | GL_VERSION_4_3 | storeAction=DontCare cho attachment tương ứng (tile memory tối ưu) |
| 320 | `glIsBuffer` | GL_VERSION_1_5 | Kiểm tra tồn tại trong object registry CPU |
| 321 | `glIsEnabled` | GL_VERSION_1_0 | Kiểm tra tồn tại trong object registry CPU |
| 322 | `glIsEnabledi` | GL_VERSION_3_0 | Kiểm tra tồn tại trong object registry CPU |
| 323 | `glIsFramebuffer` | GL_VERSION_3_0 | Kiểm tra tồn tại trong object registry CPU |
| 324 | `glIsProgram` | GL_VERSION_2_0 | Kiểm tra tồn tại trong object registry CPU |
| 325 | `glIsProgramPipeline` | GL_VERSION_4_1 | Kiểm tra tồn tại trong object registry CPU |
| 326 | `glIsQuery` | GL_VERSION_1_5 | Kiểm tra tồn tại trong object registry CPU |
| 327 | `glIsRenderbuffer` | GL_VERSION_3_0 | Kiểm tra tồn tại trong object registry CPU |
| 328 | `glIsSampler` | GL_VERSION_3_3 | Kiểm tra tồn tại trong object registry CPU |
| 329 | `glIsShader` | GL_VERSION_2_0 | Kiểm tra tồn tại trong object registry CPU |
| 330 | `glIsSync` | GL_VERSION_3_2 | Kiểm tra tồn tại trong object registry CPU |
| 331 | `glIsTexture` | GL_VERSION_1_1 | Kiểm tra tồn tại trong object registry CPU |
| 332 | `glIsTransformFeedback` | GL_VERSION_4_0 | Kiểm tra tồn tại trong object registry CPU |
| 333 | `glIsVertexArray` | GL_VERSION_3_0 | Kiểm tra tồn tại trong object registry CPU |
| 334 | `glLineWidth` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 335 | `glLinkProgram` | GL_VERSION_2_0 | Tạo MTLRenderPipelineState (gfx) / MTLComputePipelineState (compute) từ MSL functions |
| 336 | `glLogicOp` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 337 | `glMapBuffer` | GL_VERSION_1_5 | MTLBuffer.contents() + storageModeShared; flush/didModifyRange khi Unmap |
| 338 | `glMapBufferRange` | GL_VERSION_3_0 | MTLBuffer.contents() + storageModeShared; flush/didModifyRange khi Unmap |
| 339 | `glMapNamedBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 340 | `glMapNamedBufferRange` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 341 | `glMemoryBarrier` | GL_VERSION_4_2 | MTLBarrier / memoryBarrierWithScope (Metal 2+) + encoder barrierAfterQueue |
| 342 | `glMemoryBarrierByRegion` | GL_VERSION_4_5 | MTLBarrier / memoryBarrierWithScope (Metal 2+) + encoder barrierAfterQueue |
| 343 | `glMinSampleShading` | GL_VERSION_4_0 | Ghi nhận; Metal MSAA resolve tương đương (không có min-sample-shading 1:1 -> tài liệu rõ) |
| 344 | `glMultiDrawArrays` | GL_VERSION_1_4 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 345 | `glMultiDrawArraysIndirect` | GL_VERSION_4_3 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 346 | `glMultiDrawArraysIndirectCount` | GL_VERSION_4_6 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 347 | `glMultiDrawElements` | GL_VERSION_1_4 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 348 | `glMultiDrawElementsBaseVertex` | GL_VERSION_3_2 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 349 | `glMultiDrawElementsIndirect` | GL_VERSION_4_3 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 350 | `glMultiDrawElementsIndirectCount` | GL_VERSION_4_6 | Vòng lặp nhiều draw* trên cùng encoder; Metal 3: MTLIndirectCommandBuffer nếu cần batch |
| 351 | `glNamedBufferData` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 352 | `glNamedBufferStorage` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 353 | `glNamedBufferSubData` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 354 | `glNamedFramebufferDrawBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 355 | `glNamedFramebufferDrawBuffers` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 356 | `glNamedFramebufferParameteri` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 357 | `glNamedFramebufferReadBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 358 | `glNamedFramebufferRenderbuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 359 | `glNamedFramebufferTexture` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 360 | `glNamedFramebufferTextureLayer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 361 | `glNamedRenderbufferStorage` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 362 | `glNamedRenderbufferStorageMultisample` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 363 | `glObjectLabel` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 364 | `glObjectPtrLabel` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 365 | `glPatchParameterfv` | GL_VERSION_4_0 | Tessellation: MTLRenderPipelineDescriptor.tessellation* + setTessellationFactorBuffer: |
| 366 | `glPatchParameteri` | GL_VERSION_4_0 | Tessellation: MTLRenderPipelineDescriptor.tessellation* + setTessellationFactorBuffer: |
| 367 | `glPauseTransformFeedback` | GL_VERSION_4_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 368 | `glPixelStoref` | GL_VERSION_1_0 | PixelStore (pack/unpack alignment) áp dụng khi upload/download texture |
| 369 | `glPixelStorei` | GL_VERSION_1_0 | PixelStore (pack/unpack alignment) áp dụng khi upload/download texture |
| 370 | `glPointParameterf` | GL_VERSION_1_4 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 371 | `glPointParameterfv` | GL_VERSION_1_4 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 372 | `glPointParameteri` | GL_VERSION_1_4 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 373 | `glPointParameteriv` | GL_VERSION_1_4 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 374 | `glPointSize` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 375 | `glPolygonMode` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 376 | `glPolygonOffset` | GL_VERSION_1_1 | setDepthBias:slopeScale:clamp: (GL_ARB_polygon_offset_clamp -> depthBiasClamp) |
| 377 | `glPolygonOffsetClamp` | GL_VERSION_4_6 | setDepthBias:slopeScale:clamp: (GL_ARB_polygon_offset_clamp -> depthBiasClamp) |
| 378 | `glPopDebugGroup` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 379 | `glPrimitiveRestartIndex` | GL_VERSION_3_1 | primitiveRestartEnabled trong pipeline? Metal: strip cut value qua index buffer restart (emulate CPU nếu cần) |
| 380 | `glProgramBinary` | GL_VERSION_4_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 381 | `glProgramParameteri` | GL_VERSION_4_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 382 | `glProgramUniform1d` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 383 | `glProgramUniform1dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 384 | `glProgramUniform1f` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 385 | `glProgramUniform1fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 386 | `glProgramUniform1i` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 387 | `glProgramUniform1iv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 388 | `glProgramUniform1ui` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 389 | `glProgramUniform1uiv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 390 | `glProgramUniform2d` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 391 | `glProgramUniform2dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 392 | `glProgramUniform2f` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 393 | `glProgramUniform2fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 394 | `glProgramUniform2i` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 395 | `glProgramUniform2iv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 396 | `glProgramUniform2ui` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 397 | `glProgramUniform2uiv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 398 | `glProgramUniform3d` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 399 | `glProgramUniform3dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 400 | `glProgramUniform3f` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 401 | `glProgramUniform3fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 402 | `glProgramUniform3i` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 403 | `glProgramUniform3iv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 404 | `glProgramUniform3ui` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 405 | `glProgramUniform3uiv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 406 | `glProgramUniform4d` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 407 | `glProgramUniform4dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 408 | `glProgramUniform4f` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 409 | `glProgramUniform4fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 410 | `glProgramUniform4i` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 411 | `glProgramUniform4iv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 412 | `glProgramUniform4ui` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 413 | `glProgramUniform4uiv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 414 | `glProgramUniformMatrix2dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 415 | `glProgramUniformMatrix2fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 416 | `glProgramUniformMatrix2x3dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 417 | `glProgramUniformMatrix2x3fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 418 | `glProgramUniformMatrix2x4dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 419 | `glProgramUniformMatrix2x4fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 420 | `glProgramUniformMatrix3dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 421 | `glProgramUniformMatrix3fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 422 | `glProgramUniformMatrix3x2dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 423 | `glProgramUniformMatrix3x2fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 424 | `glProgramUniformMatrix3x4dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 425 | `glProgramUniformMatrix3x4fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 426 | `glProgramUniformMatrix4dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 427 | `glProgramUniformMatrix4fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 428 | `glProgramUniformMatrix4x2dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 429 | `glProgramUniformMatrix4x2fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 430 | `glProgramUniformMatrix4x3dv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 431 | `glProgramUniformMatrix4x3fv` | GL_VERSION_4_1 | DSA variant: cập nhật program object không cần bind (shadow + argument buffer) |
| 432 | `glProvokingVertex` | GL_VERSION_3_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 433 | `glPushDebugGroup` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 434 | `glQueryCounter` | GL_VERSION_3_3 | MTLCommandBuffer GPUStartTime/GPUEndTime, visibilityResultMode (occlusion), MTLSharedEvent cho timer |
| 435 | `glReadBuffer` | GL_VERSION_1_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 436 | `glReadPixels` | GL_VERSION_1_0 | Blit vào shared buffer + getBytes:bytesPerRow:fromRegion:mipmapLevel: (đồng bộ qua completionHandler) |
| 437 | `glReadnPixels` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 438 | `glReleaseShaderCompiler` | GL_VERSION_4_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 439 | `glRenderbufferStorage` | GL_VERSION_3_0 | MTLTexture type2D multisample/private làm renderbuffer storage |
| 440 | `glRenderbufferStorageMultisample` | GL_VERSION_3_0 | MTLTexture type2D multisample/private làm renderbuffer storage |
| 441 | `glResumeTransformFeedback` | GL_VERSION_4_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 442 | `glSampleCoverage` | GL_VERSION_1_3 | sampleCoverage + alphaToCoverageEnabled trong pipeline rasterSampleCount |
| 443 | `glSampleMaski` | GL_VERSION_3_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 444 | `glSamplerParameterIiv` | GL_VERSION_3_3 | MTLSamplerDescriptor -> MTLSamplerState, gán setFragmentSamplerState/setVertexSamplerState |
| 445 | `glSamplerParameterIuiv` | GL_VERSION_3_3 | MTLSamplerDescriptor -> MTLSamplerState, gán setFragmentSamplerState/setVertexSamplerState |
| 446 | `glSamplerParameterf` | GL_VERSION_3_3 | MTLSamplerDescriptor -> MTLSamplerState, gán setFragmentSamplerState/setVertexSamplerState |
| 447 | `glSamplerParameterfv` | GL_VERSION_3_3 | MTLSamplerDescriptor -> MTLSamplerState, gán setFragmentSamplerState/setVertexSamplerState |
| 448 | `glSamplerParameteri` | GL_VERSION_3_3 | MTLSamplerDescriptor -> MTLSamplerState, gán setFragmentSamplerState/setVertexSamplerState |
| 449 | `glSamplerParameteriv` | GL_VERSION_3_3 | MTLSamplerDescriptor -> MTLSamplerState, gán setFragmentSamplerState/setVertexSamplerState |
| 450 | `glScissor` | GL_VERSION_1_0 | setScissorRect: |
| 451 | `glScissorArrayv` | GL_VERSION_4_1 | setScissorRect: |
| 452 | `glScissorIndexed` | GL_VERSION_4_1 | setScissorRect: |
| 453 | `glScissorIndexedv` | GL_VERSION_4_1 | setScissorRect: |
| 454 | `glShaderBinary` | GL_VERSION_4_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 455 | `glShaderSource` | GL_VERSION_2_0 | Lưu GLSL 4.60, dịch sang MSL qua Tools/glsl_to_msl.py rồi newLibraryWithSource |
| 456 | `glShaderStorageBlockBinding` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 457 | `glSpecializeShader` | GL_VERSION_4_6 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 458 | `glStencilFunc` | GL_VERSION_1_0 | MTLDepthStencilDescriptor (stencil compare/mask/ops front/back) |
| 459 | `glStencilFuncSeparate` | GL_VERSION_2_0 | MTLDepthStencilDescriptor (stencil compare/mask/ops front/back) |
| 460 | `glStencilMask` | GL_VERSION_1_0 | MTLDepthStencilDescriptor (stencil compare/mask/ops front/back) |
| 461 | `glStencilMaskSeparate` | GL_VERSION_2_0 | MTLDepthStencilDescriptor (stencil compare/mask/ops front/back) |
| 462 | `glStencilOp` | GL_VERSION_1_0 | MTLDepthStencilDescriptor (stencil compare/mask/ops front/back) |
| 463 | `glStencilOpSeparate` | GL_VERSION_2_0 | MTLDepthStencilDescriptor (stencil compare/mask/ops front/back) |
| 464 | `glTexBuffer` | GL_VERSION_3_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 465 | `glTexBufferRange` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 466 | `glTexImage1D` | GL_VERSION_1_0 | MTLTextureDescriptor + newTexture + replaceRegion:mipmapLevel:withBytes:bytesPerRow: |
| 467 | `glTexImage2D` | GL_VERSION_1_0 | MTLTextureDescriptor + newTexture + replaceRegion:mipmapLevel:withBytes:bytesPerRow: |
| 468 | `glTexImage2DMultisample` | GL_VERSION_3_2 | MTLTextureDescriptor + newTexture + replaceRegion:mipmapLevel:withBytes:bytesPerRow: |
| 469 | `glTexImage3D` | GL_VERSION_1_2 | MTLTextureDescriptor + newTexture + replaceRegion:mipmapLevel:withBytes:bytesPerRow: |
| 470 | `glTexImage3DMultisample` | GL_VERSION_3_2 | MTLTextureDescriptor + newTexture + replaceRegion:mipmapLevel:withBytes:bytesPerRow: |
| 471 | `glTexParameterIiv` | GL_VERSION_3_0 | MTLSamplerDescriptor -> MTLSamplerState (s/tAddressMode, min/magFilter, maxAnisotropy) |
| 472 | `glTexParameterIuiv` | GL_VERSION_3_0 | MTLSamplerDescriptor -> MTLSamplerState (s/tAddressMode, min/magFilter, maxAnisotropy) |
| 473 | `glTexParameterf` | GL_VERSION_1_0 | MTLSamplerDescriptor -> MTLSamplerState (s/tAddressMode, min/magFilter, maxAnisotropy) |
| 474 | `glTexParameterfv` | GL_VERSION_1_0 | MTLSamplerDescriptor -> MTLSamplerState (s/tAddressMode, min/magFilter, maxAnisotropy) |
| 475 | `glTexParameteri` | GL_VERSION_1_0 | MTLSamplerDescriptor -> MTLSamplerState (s/tAddressMode, min/magFilter, maxAnisotropy) |
| 476 | `glTexParameteriv` | GL_VERSION_1_0 | MTLSamplerDescriptor -> MTLSamplerState (s/tAddressMode, min/magFilter, maxAnisotropy) |
| 477 | `glTexStorage1D` | GL_VERSION_4_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 478 | `glTexStorage2D` | GL_VERSION_4_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 479 | `glTexStorage2DMultisample` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 480 | `glTexStorage3D` | GL_VERSION_4_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 481 | `glTexStorage3DMultisample` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 482 | `glTexSubImage1D` | GL_VERSION_1_1 | MTLTexture.replaceRegion:... (blit nếu cần chuyển đổi format) |
| 483 | `glTexSubImage2D` | GL_VERSION_1_1 | MTLTexture.replaceRegion:... (blit nếu cần chuyển đổi format) |
| 484 | `glTexSubImage3D` | GL_VERSION_1_2 | MTLTexture.replaceRegion:... (blit nếu cần chuyển đổi format) |
| 485 | `glTextureBarrier` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 486 | `glTextureBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 487 | `glTextureBufferRange` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 488 | `glTextureParameterIiv` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 489 | `glTextureParameterIuiv` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 490 | `glTextureParameterf` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 491 | `glTextureParameterfv` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 492 | `glTextureParameteri` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 493 | `glTextureParameteriv` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 494 | `glTextureStorage1D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 495 | `glTextureStorage2D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 496 | `glTextureStorage2DMultisample` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 497 | `glTextureStorage3D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 498 | `glTextureStorage3DMultisample` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 499 | `glTextureSubImage1D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 500 | `glTextureSubImage2D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 501 | `glTextureSubImage3D` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 502 | `glTextureView` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 503 | `glTransformFeedbackBufferBase` | GL_VERSION_4_5 | Emulation bằng MTLBuffer capture + compute/vertex pass ghi ra buffer (không có TF native) |
| 504 | `glTransformFeedbackBufferRange` | GL_VERSION_4_5 | Emulation bằng MTLBuffer capture + compute/vertex pass ghi ra buffer (không có TF native) |
| 505 | `glTransformFeedbackVaryings` | GL_VERSION_3_0 | Emulation bằng MTLBuffer capture + compute/vertex pass ghi ra buffer (không có TF native) |
| 506 | `glUniform1d` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 507 | `glUniform1dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 508 | `glUniform1f` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 509 | `glUniform1fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 510 | `glUniform1i` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 511 | `glUniform1iv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 512 | `glUniform1ui` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 513 | `glUniform1uiv` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 514 | `glUniform2d` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 515 | `glUniform2dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 516 | `glUniform2f` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 517 | `glUniform2fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 518 | `glUniform2i` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 519 | `glUniform2iv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 520 | `glUniform2ui` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 521 | `glUniform2uiv` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 522 | `glUniform3d` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 523 | `glUniform3dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 524 | `glUniform3f` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 525 | `glUniform3fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 526 | `glUniform3i` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 527 | `glUniform3iv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 528 | `glUniform3ui` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 529 | `glUniform3uiv` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 530 | `glUniform4d` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 531 | `glUniform4dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 532 | `glUniform4f` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 533 | `glUniform4fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 534 | `glUniform4i` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 535 | `glUniform4iv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 536 | `glUniform4ui` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 537 | `glUniform4uiv` | GL_VERSION_3_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 538 | `glUniformBlockBinding` | GL_VERSION_3_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 539 | `glUniformMatrix2dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 540 | `glUniformMatrix2fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 541 | `glUniformMatrix2x3dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 542 | `glUniformMatrix2x3fv` | GL_VERSION_2_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 543 | `glUniformMatrix2x4dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 544 | `glUniformMatrix2x4fv` | GL_VERSION_2_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 545 | `glUniformMatrix3dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 546 | `glUniformMatrix3fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 547 | `glUniformMatrix3x2dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 548 | `glUniformMatrix3x2fv` | GL_VERSION_2_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 549 | `glUniformMatrix3x4dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 550 | `glUniformMatrix3x4fv` | GL_VERSION_2_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 551 | `glUniformMatrix4dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 552 | `glUniformMatrix4fv` | GL_VERSION_2_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 553 | `glUniformMatrix4x2dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 554 | `glUniformMatrix4x2fv` | GL_VERSION_2_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 555 | `glUniformMatrix4x3dv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 556 | `glUniformMatrix4x3fv` | GL_VERSION_2_1 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 557 | `glUniformSubroutinesuiv` | GL_VERSION_4_0 | MTLBuffer argument / setVertexBytes:setFragmentBytes: / setBytes cho compute |
| 558 | `glUnmapBuffer` | GL_VERSION_1_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 559 | `glUnmapNamedBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 560 | `glUseProgram` | GL_VERSION_2_0 | Bind pipeline state hiện tại cho encoder tiếp theo |
| 561 | `glUseProgramStages` | GL_VERSION_4_1 | Bind pipeline state hiện tại cho encoder tiếp theo |
| 562 | `glValidateProgram` | GL_VERSION_2_0 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 563 | `glValidateProgramPipeline` | GL_VERSION_4_1 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 564 | `glVertexArrayAttribBinding` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 565 | `glVertexArrayAttribFormat` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 566 | `glVertexArrayAttribIFormat` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 567 | `glVertexArrayAttribLFormat` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 568 | `glVertexArrayBindingDivisor` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 569 | `glVertexArrayElementBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 570 | `glVertexArrayVertexBuffer` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 571 | `glVertexArrayVertexBuffers` | GL_VERSION_4_5 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 572 | `glVertexAttrib1d` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 573 | `glVertexAttrib1dv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 574 | `glVertexAttrib1f` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 575 | `glVertexAttrib1fv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 576 | `glVertexAttrib1s` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 577 | `glVertexAttrib1sv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 578 | `glVertexAttrib2d` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 579 | `glVertexAttrib2dv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 580 | `glVertexAttrib2f` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 581 | `glVertexAttrib2fv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 582 | `glVertexAttrib2s` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 583 | `glVertexAttrib2sv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 584 | `glVertexAttrib3d` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 585 | `glVertexAttrib3dv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 586 | `glVertexAttrib3f` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 587 | `glVertexAttrib3fv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 588 | `glVertexAttrib3s` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 589 | `glVertexAttrib3sv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 590 | `glVertexAttrib4Nbv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 591 | `glVertexAttrib4Niv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 592 | `glVertexAttrib4Nsv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 593 | `glVertexAttrib4Nub` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 594 | `glVertexAttrib4Nubv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 595 | `glVertexAttrib4Nuiv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 596 | `glVertexAttrib4Nusv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 597 | `glVertexAttrib4bv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 598 | `glVertexAttrib4d` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 599 | `glVertexAttrib4dv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 600 | `glVertexAttrib4f` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 601 | `glVertexAttrib4fv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 602 | `glVertexAttrib4iv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 603 | `glVertexAttrib4s` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 604 | `glVertexAttrib4sv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 605 | `glVertexAttrib4ubv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 606 | `glVertexAttrib4uiv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 607 | `glVertexAttrib4usv` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 608 | `glVertexAttribBinding` | GL_VERSION_4_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 609 | `glVertexAttribDivisor` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 610 | `glVertexAttribFormat` | GL_VERSION_4_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 611 | `glVertexAttribI1i` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 612 | `glVertexAttribI1iv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 613 | `glVertexAttribI1ui` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 614 | `glVertexAttribI1uiv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 615 | `glVertexAttribI2i` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 616 | `glVertexAttribI2iv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 617 | `glVertexAttribI2ui` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 618 | `glVertexAttribI2uiv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 619 | `glVertexAttribI3i` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 620 | `glVertexAttribI3iv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 621 | `glVertexAttribI3ui` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 622 | `glVertexAttribI3uiv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 623 | `glVertexAttribI4bv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 624 | `glVertexAttribI4i` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 625 | `glVertexAttribI4iv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 626 | `glVertexAttribI4sv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 627 | `glVertexAttribI4ubv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 628 | `glVertexAttribI4ui` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 629 | `glVertexAttribI4uiv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 630 | `glVertexAttribI4usv` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 631 | `glVertexAttribIFormat` | GL_VERSION_4_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 632 | `glVertexAttribIPointer` | GL_VERSION_3_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 633 | `glVertexAttribL1d` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 634 | `glVertexAttribL1dv` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 635 | `glVertexAttribL2d` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 636 | `glVertexAttribL2dv` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 637 | `glVertexAttribL3d` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 638 | `glVertexAttribL3dv` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 639 | `glVertexAttribL4d` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 640 | `glVertexAttribL4dv` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 641 | `glVertexAttribLFormat` | GL_VERSION_4_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 642 | `glVertexAttribLPointer` | GL_VERSION_4_1 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 643 | `glVertexAttribP1ui` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 644 | `glVertexAttribP1uiv` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 645 | `glVertexAttribP2ui` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 646 | `glVertexAttribP2uiv` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 647 | `glVertexAttribP3ui` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 648 | `glVertexAttribP3uiv` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 649 | `glVertexAttribP4ui` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 650 | `glVertexAttribP4uiv` | GL_VERSION_3_3 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 651 | `glVertexAttribPointer` | GL_VERSION_2_0 | MTLVertexDescriptor + setVertexBuffer:offset:atIndex: ; layout từ glVertexAttribPointer/Format |
| 652 | `glVertexBindingDivisor` | GL_VERSION_4_3 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
| 653 | `glViewport` | GL_VERSION_1_0 | MTLViewport qua setViewport: (chuyển hệ tọa độ GL->Metal: y-flip, depth 0..1) |
| 654 | `glViewportArrayv` | GL_VERSION_4_1 | MTLViewport qua setViewport: (chuyển hệ tọa độ GL->Metal: y-flip, depth 0..1) |
| 655 | `glViewportIndexedf` | GL_VERSION_4_1 | MTLViewport qua setViewport: (chuyển hệ tọa độ GL->Metal: y-flip, depth 0..1) |
| 656 | `glViewportIndexedfv` | GL_VERSION_4_1 | MTLViewport qua setViewport: (chuyển hệ tọa độ GL->Metal: y-flip, depth 0..1) |
| 657 | `glWaitSync` | GL_VERSION_3_2 | State tracker CPU + bake vào MTLRender/Compute/DepthStencil/Sampler descriptor tương ứng |
