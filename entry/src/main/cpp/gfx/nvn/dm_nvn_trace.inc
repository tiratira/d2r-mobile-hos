/* 自动生成(源:上游 names.txt)。
 * 每个入口只在前 3 次调用时打日志:引擎热路径每秒上千次 Bind/Draw,
 * 全量打日志会把 hilog 刷爆并拖慢启动。 */
static int nvn_trace_n_nvnDeviceBuilderSetDefaults;
static int nvn_trace_n_nvnDeviceBuilderSetFlags;
static int nvn_trace_n_nvnDeviceBuilderGetFlags;
static int nvn_trace_n_nvnDeviceInitialize;
static int nvn_trace_n_nvnDeviceFinalize;
static int nvn_trace_n_nvnDeviceSetDebugLabel;
static int nvn_trace_n_nvnDeviceGetProcAddress;
static int nvn_trace_n_nvnDeviceGetInteger;
static int nvn_trace_n_nvnDeviceGetCurrentTimestampInNanoseconds;
static int nvn_trace_n_nvnDeviceSetIntermediateShaderCache;
static int nvn_trace_n_nvnDeviceGetTextureHandle;
static int nvn_trace_n_nvnDeviceGetTexelFetchHandle;
static int nvn_trace_n_nvnDeviceGetImageHandle;
static int nvn_trace_n_nvnDeviceInstallDebugCallback;
static int nvn_trace_n_nvnDeviceGenerateDebugDomainId;
static int nvn_trace_n_nvnDeviceSetWindowOriginMode;
static int nvn_trace_n_nvnDeviceSetDepthMode;
static int nvn_trace_n_nvnDeviceRegisterFastClearColor;
static int nvn_trace_n_nvnDeviceRegisterFastClearColori;
static int nvn_trace_n_nvnDeviceRegisterFastClearColorui;
static int nvn_trace_n_nvnDeviceRegisterFastClearDepth;
static int nvn_trace_n_nvnDeviceGetWindowOriginMode;
static int nvn_trace_n_nvnDeviceGetDepthMode;
static int nvn_trace_n_nvnDeviceGetTimestampInNanoseconds;
static int nvn_trace_n_nvnDeviceApplyDeferredFinalizes;
static int nvn_trace_n_nvnDeviceFinalizeCommandHandle;
static int nvn_trace_n_nvnDeviceWalkDebugDatabase;
static int nvn_trace_n_nvnDeviceGetSeparateTextureHandle;
static int nvn_trace_n_nvnDeviceGetSeparateSamplerHandle;
static int nvn_trace_n_nvnDeviceIsExternalDebuggerAttached;
static int nvn_trace_n_nvnQueueGetError;
static int nvn_trace_n_nvnQueueGetTotalCommandMemoryUsed;
static int nvn_trace_n_nvnQueueGetTotalControlMemoryUsed;
static int nvn_trace_n_nvnQueueGetTotalComputeMemoryUsed;
static int nvn_trace_n_nvnQueueResetMemoryUsageCounts;
static int nvn_trace_n_nvnQueueBuilderSetDevice;
static int nvn_trace_n_nvnQueueBuilderSetDefaults;
static int nvn_trace_n_nvnQueueBuilderSetFlags;
static int nvn_trace_n_nvnQueueBuilderSetCommandMemorySize;
static int nvn_trace_n_nvnQueueBuilderSetComputeMemorySize;
static int nvn_trace_n_nvnQueueBuilderSetControlMemorySize;
static int nvn_trace_n_nvnQueueBuilderGetQueueMemorySize;
static int nvn_trace_n_nvnQueueBuilderSetQueueMemory;
static int nvn_trace_n_nvnQueueBuilderSetCommandFlushThreshold;
static int nvn_trace_n_nvnQueueBuilderSetQueuePriority;
static int nvn_trace_n_nvnQueueBuilderGetQueuePriority;
static int nvn_trace_n_nvnQueueBuilderGetDevice;
static int nvn_trace_n_nvnQueueBuilderGetFlags;
static int nvn_trace_n_nvnQueueBuilderGetCommandMemorySize;
static int nvn_trace_n_nvnQueueBuilderGetComputeMemorySize;
static int nvn_trace_n_nvnQueueBuilderGetControlMemorySize;
static int nvn_trace_n_nvnQueueBuilderGetCommandFlushThreshold;
static int nvn_trace_n_nvnQueueBuilderGetMemorySize;
static int nvn_trace_n_nvnQueueBuilderGetMemory;
static int nvn_trace_n_nvnQueueInitialize;
static int nvn_trace_n_nvnQueueFinalize;
static int nvn_trace_n_nvnQueueSetDebugLabel;
static int nvn_trace_n_nvnQueueSubmitCommands;
static int nvn_trace_n_nvnQueueFlush;
static int nvn_trace_n_nvnQueueFinish;
static int nvn_trace_n_nvnQueuePresentTexture;
static int nvn_trace_n_nvnQueueAcquireTexture;
static int nvn_trace_n_nvnWindowBuilderSetDevice;
static int nvn_trace_n_nvnWindowBuilderSetDefaults;
static int nvn_trace_n_nvnWindowBuilderSetNativeWindow;
static int nvn_trace_n_nvnWindowBuilderSetTextures;
static int nvn_trace_n_nvnWindowBuilderSetPresentInterval;
static int nvn_trace_n_nvnWindowBuilderSetNumActiveTextures;
static int nvn_trace_n_nvnWindowBuilderGetDevice;
static int nvn_trace_n_nvnWindowBuilderGetNumTextures;
static int nvn_trace_n_nvnWindowBuilderGetTexture;
static int nvn_trace_n_nvnWindowBuilderGetNativeWindow;
static int nvn_trace_n_nvnWindowBuilderGetPresentInterval;
static int nvn_trace_n_nvnWindowBuilderGetNumActiveTextures;
static int nvn_trace_n_nvnWindowInitialize;
static int nvn_trace_n_nvnWindowFinalize;
static int nvn_trace_n_nvnWindowSetDebugLabel;
static int nvn_trace_n_nvnWindowAcquireTexture;
static int nvn_trace_n_nvnWindowGetNativeWindow;
static int nvn_trace_n_nvnWindowGetPresentInterval;
static int nvn_trace_n_nvnWindowSetPresentInterval;
static int nvn_trace_n_nvnWindowSetCrop;
static int nvn_trace_n_nvnWindowGetCrop;
static int nvn_trace_n_nvnWindowSetNumActiveTextures;
static int nvn_trace_n_nvnWindowGetNumActiveTextures;
static int nvn_trace_n_nvnWindowGetNumTextures;
static int nvn_trace_n_nvnProgramInitialize;
static int nvn_trace_n_nvnProgramFinalize;
static int nvn_trace_n_nvnProgramSetDebugLabel;
static int nvn_trace_n_nvnProgramSetShaders;
static int nvn_trace_n_nvnProgramSetShadersExt;
static int nvn_trace_n_nvnProgramSetSampleShading;
static int nvn_trace_n_nvnMemoryPoolBuilderSetDevice;
static int nvn_trace_n_nvnMemoryPoolBuilderSetDefaults;
static int nvn_trace_n_nvnMemoryPoolBuilderSetStorage;
static int nvn_trace_n_nvnMemoryPoolBuilderSetFlags;
static int nvn_trace_n_nvnMemoryPoolBuilderGetDevice;
static int nvn_trace_n_nvnMemoryPoolBuilderGetMemory;
static int nvn_trace_n_nvnMemoryPoolBuilderGetSize;
static int nvn_trace_n_nvnMemoryPoolBuilderGetFlags;
static int nvn_trace_n_nvnMemoryPoolInitialize;
static int nvn_trace_n_nvnMemoryPoolSetDebugLabel;
static int nvn_trace_n_nvnMemoryPoolFinalize;
static int nvn_trace_n_nvnMemoryPoolMap;
static int nvn_trace_n_nvnMemoryPoolFlushMappedRange;
static int nvn_trace_n_nvnMemoryPoolInvalidateMappedRange;
static int nvn_trace_n_nvnMemoryPoolGetBufferAddress;
static int nvn_trace_n_nvnMemoryPoolMapVirtual;
static int nvn_trace_n_nvnMemoryPoolGetSize;
static int nvn_trace_n_nvnMemoryPoolGetFlags;
static int nvn_trace_n_nvnTexturePoolInitialize;
static int nvn_trace_n_nvnTexturePoolSetDebugLabel;
static int nvn_trace_n_nvnTexturePoolFinalize;
static int nvn_trace_n_nvnTexturePoolRegisterTexture;
static int nvn_trace_n_nvnTexturePoolRegisterImage;
static int nvn_trace_n_nvnTexturePoolGetMemoryPool;
static int nvn_trace_n_nvnTexturePoolGetMemoryOffset;
static int nvn_trace_n_nvnTexturePoolGetSize;
static int nvn_trace_n_nvnSamplerPoolInitialize;
static int nvn_trace_n_nvnSamplerPoolSetDebugLabel;
static int nvn_trace_n_nvnSamplerPoolFinalize;
static int nvn_trace_n_nvnSamplerPoolRegisterSampler;
static int nvn_trace_n_nvnSamplerPoolRegisterSamplerBuilder;
static int nvn_trace_n_nvnSamplerPoolGetMemoryPool;
static int nvn_trace_n_nvnSamplerPoolGetMemoryOffset;
static int nvn_trace_n_nvnSamplerPoolGetSize;
static int nvn_trace_n_nvnBufferBuilderSetDevice;
static int nvn_trace_n_nvnBufferBuilderSetDefaults;
static int nvn_trace_n_nvnBufferBuilderSetStorage;
static int nvn_trace_n_nvnBufferBuilderGetDevice;
static int nvn_trace_n_nvnBufferBuilderGetMemoryPool;
static int nvn_trace_n_nvnBufferBuilderGetMemoryOffset;
static int nvn_trace_n_nvnBufferBuilderGetSize;
static int nvn_trace_n_nvnBufferInitialize;
static int nvn_trace_n_nvnBufferSetDebugLabel;
static int nvn_trace_n_nvnBufferFinalize;
static int nvn_trace_n_nvnBufferMap;
static int nvn_trace_n_nvnBufferGetAddress;
static int nvn_trace_n_nvnBufferFlushMappedRange;
static int nvn_trace_n_nvnBufferInvalidateMappedRange;
static int nvn_trace_n_nvnBufferGetMemoryPool;
static int nvn_trace_n_nvnBufferGetMemoryOffset;
static int nvn_trace_n_nvnBufferGetSize;
static int nvn_trace_n_nvnBufferGetDebugID;
static int nvn_trace_n_nvnTextureBuilderSetDevice;
static int nvn_trace_n_nvnTextureBuilderSetDefaults;
static int nvn_trace_n_nvnTextureBuilderSetFlags;
static int nvn_trace_n_nvnTextureBuilderSetTarget;
static int nvn_trace_n_nvnTextureBuilderSetWidth;
static int nvn_trace_n_nvnTextureBuilderSetHeight;
static int nvn_trace_n_nvnTextureBuilderSetDepth;
static int nvn_trace_n_nvnTextureBuilderSetSize1D;
static int nvn_trace_n_nvnTextureBuilderSetSize2D;
static int nvn_trace_n_nvnTextureBuilderSetSize3D;
static int nvn_trace_n_nvnTextureBuilderSetLevels;
static int nvn_trace_n_nvnTextureBuilderSetFormat;
static int nvn_trace_n_nvnTextureBuilderSetSamples;
static int nvn_trace_n_nvnTextureBuilderSetSwizzle;
static int nvn_trace_n_nvnTextureBuilderSetDepthStencilMode;
static int nvn_trace_n_nvnTextureBuilderGetStorageSize;
static int nvn_trace_n_nvnTextureBuilderGetStorageAlignment;
static int nvn_trace_n_nvnTextureBuilderSetStorage;
static int nvn_trace_n_nvnTextureBuilderSetPackagedTextureData;
static int nvn_trace_n_nvnTextureBuilderSetPackagedTextureLayout;
static int nvn_trace_n_nvnTextureBuilderSetStride;
static int nvn_trace_n_nvnTextureBuilderSetGLTextureName;
static int nvn_trace_n_nvnTextureBuilderGetStorageClass;
static int nvn_trace_n_nvnTextureBuilderGetDevice;
static int nvn_trace_n_nvnTextureBuilderGetFlags;
static int nvn_trace_n_nvnTextureBuilderGetTarget;
static int nvn_trace_n_nvnTextureBuilderGetWidth;
static int nvn_trace_n_nvnTextureBuilderGetHeight;
static int nvn_trace_n_nvnTextureBuilderGetDepth;
static int nvn_trace_n_nvnTextureBuilderGetLevels;
static int nvn_trace_n_nvnTextureBuilderGetFormat;
static int nvn_trace_n_nvnTextureBuilderGetSamples;
static int nvn_trace_n_nvnTextureBuilderGetSwizzle;
static int nvn_trace_n_nvnTextureBuilderGetDepthStencilMode;
static int nvn_trace_n_nvnTextureBuilderGetPackagedTextureData;
static int nvn_trace_n_nvnTextureBuilderGetPackagedTextureLayout;
static int nvn_trace_n_nvnTextureBuilderGetStride;
static int nvn_trace_n_nvnTextureBuilderGetSparseTileLayout;
static int nvn_trace_n_nvnTextureBuilderGetGLTextureName;
static int nvn_trace_n_nvnTextureBuilderGetZCullStorageSize;
static int nvn_trace_n_nvnTextureBuilderGetMemoryPool;
static int nvn_trace_n_nvnTextureBuilderGetMemoryOffset;
static int nvn_trace_n_nvnTextureBuilderGetRawStorageClass;
static int nvn_trace_n_nvnTextureViewSetDefaults;
static int nvn_trace_n_nvnTextureViewSetLevels;
static int nvn_trace_n_nvnTextureViewSetLayers;
static int nvn_trace_n_nvnTextureViewSetFormat;
static int nvn_trace_n_nvnTextureViewSetSwizzle;
static int nvn_trace_n_nvnTextureViewSetDepthStencilMode;
static int nvn_trace_n_nvnTextureViewSetTarget;
static int nvn_trace_n_nvnTextureViewGetLevels;
static int nvn_trace_n_nvnTextureViewGetLayers;
static int nvn_trace_n_nvnTextureViewGetFormat;
static int nvn_trace_n_nvnTextureViewGetSwizzle;
static int nvn_trace_n_nvnTextureViewGetDepthStencilMode;
static int nvn_trace_n_nvnTextureViewGetTarget;
static int nvn_trace_n_nvnTextureViewCompare;
static int nvn_trace_n_nvnTextureInitialize;
static int nvn_trace_n_nvnTextureGetZCullStorageSize;
static int nvn_trace_n_nvnTextureFinalize;
static int nvn_trace_n_nvnTextureSetDebugLabel;
static int nvn_trace_n_nvnTextureGetStorageClass;
static int nvn_trace_n_nvnTextureGetViewOffset;
static int nvn_trace_n_nvnTextureGetFlags;
static int nvn_trace_n_nvnTextureGetTarget;
static int nvn_trace_n_nvnTextureGetWidth;
static int nvn_trace_n_nvnTextureGetHeight;
static int nvn_trace_n_nvnTextureGetDepth;
static int nvn_trace_n_nvnTextureGetLevels;
static int nvn_trace_n_nvnTextureGetFormat;
static int nvn_trace_n_nvnTextureGetSamples;
static int nvn_trace_n_nvnTextureGetSwizzle;
static int nvn_trace_n_nvnTextureGetDepthStencilMode;
static int nvn_trace_n_nvnTextureGetStride;
static int nvn_trace_n_nvnTextureGetTextureAddress;
static int nvn_trace_n_nvnTextureGetSparseTileLayout;
static int nvn_trace_n_nvnTextureWriteTexels;
static int nvn_trace_n_nvnTextureWriteTexelsStrided;
static int nvn_trace_n_nvnTextureReadTexels;
static int nvn_trace_n_nvnTextureReadTexelsStrided;
static int nvn_trace_n_nvnTextureFlushTexels;
static int nvn_trace_n_nvnTextureInvalidateTexels;
static int nvn_trace_n_nvnTextureGetMemoryPool;
static int nvn_trace_n_nvnTextureGetMemoryOffset;
static int nvn_trace_n_nvnTextureGetStorageSize;
static int nvn_trace_n_nvnTextureCompare;
static int nvn_trace_n_nvnTextureGetDebugID;
static int nvn_trace_n_nvnTextureGetRawStorageClass;
static int nvn_trace_n_nvnSamplerBuilderSetDevice;
static int nvn_trace_n_nvnSamplerBuilderSetDefaults;
static int nvn_trace_n_nvnSamplerBuilderSetMinMagFilter;
static int nvn_trace_n_nvnSamplerBuilderSetWrapMode;
static int nvn_trace_n_nvnSamplerBuilderSetLodClamp;
static int nvn_trace_n_nvnSamplerBuilderSetLodBias;
static int nvn_trace_n_nvnSamplerBuilderSetCompare;
static int nvn_trace_n_nvnSamplerBuilderSetBorderColor;
static int nvn_trace_n_nvnSamplerBuilderSetBorderColori;
static int nvn_trace_n_nvnSamplerBuilderSetBorderColorui;
static int nvn_trace_n_nvnSamplerBuilderSetMaxAnisotropy;
static int nvn_trace_n_nvnSamplerBuilderSetReductionFilter;
static int nvn_trace_n_nvnSamplerBuilderSetLodSnap;
static int nvn_trace_n_nvnSamplerBuilderGetDevice;
static int nvn_trace_n_nvnSamplerBuilderGetMinMagFilter;
static int nvn_trace_n_nvnSamplerBuilderGetWrapMode;
static int nvn_trace_n_nvnSamplerBuilderGetLodClamp;
static int nvn_trace_n_nvnSamplerBuilderGetLodBias;
static int nvn_trace_n_nvnSamplerBuilderGetCompare;
static int nvn_trace_n_nvnSamplerBuilderGetBorderColor;
static int nvn_trace_n_nvnSamplerBuilderGetBorderColori;
static int nvn_trace_n_nvnSamplerBuilderGetBorderColorui;
static int nvn_trace_n_nvnSamplerBuilderGetMaxAnisotropy;
static int nvn_trace_n_nvnSamplerBuilderGetReductionFilter;
static int nvn_trace_n_nvnSamplerBuilderGetLodSnap;
static int nvn_trace_n_nvnSamplerInitialize;
static int nvn_trace_n_nvnSamplerFinalize;
static int nvn_trace_n_nvnSamplerSetDebugLabel;
static int nvn_trace_n_nvnSamplerGetMinMagFilter;
static int nvn_trace_n_nvnSamplerGetWrapMode;
static int nvn_trace_n_nvnSamplerGetLodClamp;
static int nvn_trace_n_nvnSamplerGetLodBias;
static int nvn_trace_n_nvnSamplerGetCompare;
static int nvn_trace_n_nvnSamplerGetBorderColor;
static int nvn_trace_n_nvnSamplerGetBorderColori;
static int nvn_trace_n_nvnSamplerGetBorderColorui;
static int nvn_trace_n_nvnSamplerGetMaxAnisotropy;
static int nvn_trace_n_nvnSamplerGetReductionFilter;
static int nvn_trace_n_nvnSamplerCompare;
static int nvn_trace_n_nvnSamplerGetDebugID;
static int nvn_trace_n_nvnBlendStateSetDefaults;
static int nvn_trace_n_nvnBlendStateSetBlendTarget;
static int nvn_trace_n_nvnBlendStateSetBlendFunc;
static int nvn_trace_n_nvnBlendStateSetBlendEquation;
static int nvn_trace_n_nvnBlendStateSetAdvancedMode;
static int nvn_trace_n_nvnBlendStateSetAdvancedOverlap;
static int nvn_trace_n_nvnBlendStateSetAdvancedPremultipliedSrc;
static int nvn_trace_n_nvnBlendStateSetAdvancedNormalizedDst;
static int nvn_trace_n_nvnBlendStateGetBlendTarget;
static int nvn_trace_n_nvnBlendStateGetBlendFunc;
static int nvn_trace_n_nvnBlendStateGetBlendEquation;
static int nvn_trace_n_nvnBlendStateGetAdvancedMode;
static int nvn_trace_n_nvnBlendStateGetAdvancedOverlap;
static int nvn_trace_n_nvnBlendStateGetAdvancedPremultipliedSrc;
static int nvn_trace_n_nvnBlendStateGetAdvancedNormalizedDst;
static int nvn_trace_n_nvnColorStateSetDefaults;
static int nvn_trace_n_nvnColorStateSetBlendEnable;
static int nvn_trace_n_nvnColorStateSetLogicOp;
static int nvn_trace_n_nvnColorStateSetAlphaTest;
static int nvn_trace_n_nvnColorStateGetBlendEnable;
static int nvn_trace_n_nvnColorStateGetLogicOp;
static int nvn_trace_n_nvnColorStateGetAlphaTest;
static int nvn_trace_n_nvnChannelMaskStateSetDefaults;
static int nvn_trace_n_nvnChannelMaskStateSetChannelMask;
static int nvn_trace_n_nvnChannelMaskStateGetChannelMask;
static int nvn_trace_n_nvnMultisampleStateSetDefaults;
static int nvn_trace_n_nvnMultisampleStateSetMultisampleEnable;
static int nvn_trace_n_nvnMultisampleStateSetSamples;
static int nvn_trace_n_nvnMultisampleStateSetAlphaToCoverageEnable;
static int nvn_trace_n_nvnMultisampleStateSetAlphaToCoverageDither;
static int nvn_trace_n_nvnMultisampleStateGetMultisampleEnable;
static int nvn_trace_n_nvnMultisampleStateGetSamples;
static int nvn_trace_n_nvnMultisampleStateGetAlphaToCoverageEnable;
static int nvn_trace_n_nvnMultisampleStateGetAlphaToCoverageDither;
static int nvn_trace_n_nvnMultisampleStateSetRasterSamples;
static int nvn_trace_n_nvnMultisampleStateGetRasterSamples;
static int nvn_trace_n_nvnMultisampleStateSetCoverageModulationMode;
static int nvn_trace_n_nvnMultisampleStateGetCoverageModulationMode;
static int nvn_trace_n_nvnMultisampleStateSetCoverageToColorEnable;
static int nvn_trace_n_nvnMultisampleStateGetCoverageToColorEnable;
static int nvn_trace_n_nvnMultisampleStateSetCoverageToColorOutput;
static int nvn_trace_n_nvnMultisampleStateGetCoverageToColorOutput;
static int nvn_trace_n_nvnMultisampleStateSetSampleLocationsEnable;
static int nvn_trace_n_nvnMultisampleStateGetSampleLocationsEnable;
static int nvn_trace_n_nvnMultisampleStateGetSampleLocationsGrid;
static int nvn_trace_n_nvnMultisampleStateSetSampleLocationsGridEnable;
static int nvn_trace_n_nvnMultisampleStateGetSampleLocationsGridEnable;
static int nvn_trace_n_nvnMultisampleStateSetSampleLocations;
static int nvn_trace_n_nvnPolygonStateSetDefaults;
static int nvn_trace_n_nvnPolygonStateSetCullFace;
static int nvn_trace_n_nvnPolygonStateSetFrontFace;
static int nvn_trace_n_nvnPolygonStateSetPolygonMode;
static int nvn_trace_n_nvnPolygonStateSetPolygonOffsetEnables;
static int nvn_trace_n_nvnPolygonStateGetCullFace;
static int nvn_trace_n_nvnPolygonStateGetFrontFace;
static int nvn_trace_n_nvnPolygonStateGetPolygonMode;
static int nvn_trace_n_nvnPolygonStateGetPolygonOffsetEnables;
static int nvn_trace_n_nvnDepthStencilStateSetDefaults;
static int nvn_trace_n_nvnDepthStencilStateSetDepthTestEnable;
static int nvn_trace_n_nvnDepthStencilStateSetDepthWriteEnable;
static int nvn_trace_n_nvnDepthStencilStateSetDepthFunc;
static int nvn_trace_n_nvnDepthStencilStateSetStencilTestEnable;
static int nvn_trace_n_nvnDepthStencilStateSetStencilFunc;
static int nvn_trace_n_nvnDepthStencilStateSetStencilOp;
static int nvn_trace_n_nvnDepthStencilStateGetDepthTestEnable;
static int nvn_trace_n_nvnDepthStencilStateGetDepthWriteEnable;
static int nvn_trace_n_nvnDepthStencilStateGetDepthFunc;
static int nvn_trace_n_nvnDepthStencilStateGetStencilTestEnable;
static int nvn_trace_n_nvnDepthStencilStateGetStencilFunc;
static int nvn_trace_n_nvnDepthStencilStateGetStencilOp;
static int nvn_trace_n_nvnVertexAttribStateSetDefaults;
static int nvn_trace_n_nvnVertexAttribStateSetFormat;
static int nvn_trace_n_nvnVertexAttribStateSetStreamIndex;
static int nvn_trace_n_nvnVertexAttribStateGetFormat;
static int nvn_trace_n_nvnVertexAttribStateGetStreamIndex;
static int nvn_trace_n_nvnVertexStreamStateSetDefaults;
static int nvn_trace_n_nvnVertexStreamStateSetStride;
static int nvn_trace_n_nvnVertexStreamStateSetDivisor;
static int nvn_trace_n_nvnVertexStreamStateGetStride;
static int nvn_trace_n_nvnVertexStreamStateGetDivisor;
static int nvn_trace_n_nvnCommandBufferInitialize;
static int nvn_trace_n_nvnCommandBufferFinalize;
static int nvn_trace_n_nvnCommandBufferSetDebugLabel;
static int nvn_trace_n_nvnCommandBufferSetMemoryCallback;
static int nvn_trace_n_nvnCommandBufferSetMemoryCallbackData;
static int nvn_trace_n_nvnCommandBufferSetCommandMemoryCallbackEnabled;
static int nvn_trace_n_nvnCommandBufferAddCommandMemory;
static int nvn_trace_n_nvnCommandBufferAddControlMemory;
static int nvn_trace_n_nvnCommandBufferGetCommandMemorySize;
static int nvn_trace_n_nvnCommandBufferGetCommandMemoryUsed;
static int nvn_trace_n_nvnCommandBufferGetCommandMemoryFree;
static int nvn_trace_n_nvnCommandBufferGetControlMemorySize;
static int nvn_trace_n_nvnCommandBufferGetControlMemoryUsed;
static int nvn_trace_n_nvnCommandBufferGetControlMemoryFree;
static int nvn_trace_n_nvnCommandBufferBeginRecording;
static int nvn_trace_n_nvnCommandBufferEndRecording;
static int nvn_trace_n_nvnCommandBufferCallCommands;
static int nvn_trace_n_nvnCommandBufferCopyCommands;
static int nvn_trace_n_nvnCommandBufferBindBlendState;
static int nvn_trace_n_nvnCommandBufferBindChannelMaskState;
static int nvn_trace_n_nvnCommandBufferBindColorState;
static int nvn_trace_n_nvnCommandBufferBindMultisampleState;
static int nvn_trace_n_nvnCommandBufferBindPolygonState;
static int nvn_trace_n_nvnCommandBufferBindDepthStencilState;
static int nvn_trace_n_nvnCommandBufferBindVertexAttribState;
static int nvn_trace_n_nvnCommandBufferBindVertexStreamState;
static int nvn_trace_n_nvnCommandBufferBindProgram;
static int nvn_trace_n_nvnCommandBufferBindVertexBuffer;
static int nvn_trace_n_nvnCommandBufferBindVertexBuffers;
static int nvn_trace_n_nvnCommandBufferBindUniformBuffer;
static int nvn_trace_n_nvnCommandBufferBindUniformBuffers;
static int nvn_trace_n_nvnCommandBufferBindTransformFeedbackBuffer;
static int nvn_trace_n_nvnCommandBufferBindTransformFeedbackBuffers;
static int nvn_trace_n_nvnCommandBufferBindStorageBuffer;
static int nvn_trace_n_nvnCommandBufferBindStorageBuffers;
static int nvn_trace_n_nvnCommandBufferBindTexture;
static int nvn_trace_n_nvnCommandBufferBindTextures;
static int nvn_trace_n_nvnCommandBufferBindImage;
static int nvn_trace_n_nvnCommandBufferBindImages;
static int nvn_trace_n_nvnCommandBufferSetPatchSize;
static int nvn_trace_n_nvnCommandBufferSetInnerTessellationLevels;
static int nvn_trace_n_nvnCommandBufferSetOuterTessellationLevels;
static int nvn_trace_n_nvnCommandBufferSetPrimitiveRestart;
static int nvn_trace_n_nvnCommandBufferBeginTransformFeedback;
static int nvn_trace_n_nvnCommandBufferEndTransformFeedback;
static int nvn_trace_n_nvnCommandBufferPauseTransformFeedback;
static int nvn_trace_n_nvnCommandBufferResumeTransformFeedback;
static int nvn_trace_n_nvnCommandBufferDrawTransformFeedback;
static int nvn_trace_n_nvnCommandBufferDrawArrays;
static int nvn_trace_n_nvnCommandBufferDrawElements;
static int nvn_trace_n_nvnCommandBufferDrawElementsBaseVertex;
static int nvn_trace_n_nvnCommandBufferDrawArraysInstanced;
static int nvn_trace_n_nvnCommandBufferDrawElementsInstanced;
static int nvn_trace_n_nvnCommandBufferDrawArraysIndirect;
static int nvn_trace_n_nvnCommandBufferDrawElementsIndirect;
static int nvn_trace_n_nvnCommandBufferMultiDrawArraysIndirectCount;
static int nvn_trace_n_nvnCommandBufferMultiDrawElementsIndirectCount;
static int nvn_trace_n_nvnCommandBufferClearColor;
static int nvn_trace_n_nvnCommandBufferClearColori;
static int nvn_trace_n_nvnCommandBufferClearColorui;
static int nvn_trace_n_nvnCommandBufferClearDepthStencil;
static int nvn_trace_n_nvnCommandBufferDispatchCompute;
static int nvn_trace_n_nvnCommandBufferDispatchComputeIndirect;
static int nvn_trace_n_nvnCommandBufferSetViewport;
static int nvn_trace_n_nvnCommandBufferSetViewports;
static int nvn_trace_n_nvnCommandBufferSetViewportSwizzles;
static int nvn_trace_n_nvnCommandBufferSetScissor;
static int nvn_trace_n_nvnCommandBufferSetScissors;
static int nvn_trace_n_nvnCommandBufferSetDepthRange;
static int nvn_trace_n_nvnCommandBufferSetDepthBounds;
static int nvn_trace_n_nvnCommandBufferSetDepthRanges;
static int nvn_trace_n_nvnCommandBufferSetTiledCacheAction;
static int nvn_trace_n_nvnCommandBufferSetTiledCacheTileSize;
static int nvn_trace_n_nvnCommandBufferBindSeparateTexture;
static int nvn_trace_n_nvnCommandBufferBindSeparateSampler;
static int nvn_trace_n_nvnCommandBufferBindSeparateTextures;
static int nvn_trace_n_nvnCommandBufferBindSeparateSamplers;
static int nvn_trace_n_nvnCommandBufferSetStencilValueMask;
static int nvn_trace_n_nvnCommandBufferSetStencilMask;
static int nvn_trace_n_nvnCommandBufferSetStencilRef;
static int nvn_trace_n_nvnCommandBufferSetBlendColor;
static int nvn_trace_n_nvnCommandBufferSetPointSize;
static int nvn_trace_n_nvnCommandBufferSetLineWidth;
static int nvn_trace_n_nvnCommandBufferSetPolygonOffsetClamp;
static int nvn_trace_n_nvnCommandBufferSetAlphaRef;
static int nvn_trace_n_nvnCommandBufferSetSampleMask;
static int nvn_trace_n_nvnCommandBufferSetRasterizerDiscard;
static int nvn_trace_n_nvnCommandBufferSetDepthClamp;
static int nvn_trace_n_nvnCommandBufferSetConservativeRasterEnable;
static int nvn_trace_n_nvnCommandBufferSetConservativeRasterDilate;
static int nvn_trace_n_nvnCommandBufferSetSubpixelPrecisionBias;
static int nvn_trace_n_nvnCommandBufferCopyBufferToTexture;
static int nvn_trace_n_nvnCommandBufferCopyTextureToBuffer;
static int nvn_trace_n_nvnCommandBufferCopyTextureToTexture;
static int nvn_trace_n_nvnCommandBufferCopyBufferToBuffer;
static int nvn_trace_n_nvnCommandBufferClearBuffer;
static int nvn_trace_n_nvnCommandBufferClearTexture;
static int nvn_trace_n_nvnCommandBufferClearTexturei;
static int nvn_trace_n_nvnCommandBufferClearTextureui;
static int nvn_trace_n_nvnCommandBufferUpdateUniformBuffer;
static int nvn_trace_n_nvnCommandBufferReportCounter;
static int nvn_trace_n_nvnCommandBufferResetCounter;
static int nvn_trace_n_nvnCommandBufferReportValue;
static int nvn_trace_n_nvnCommandBufferSetRenderEnable;
static int nvn_trace_n_nvnCommandBufferSetRenderEnableConditional;
static int nvn_trace_n_nvnCommandBufferSetRenderTargets;
static int nvn_trace_n_nvnCommandBufferDiscardColor;
static int nvn_trace_n_nvnCommandBufferDiscardDepthStencil;
static int nvn_trace_n_nvnCommandBufferDownsample;
static int nvn_trace_n_nvnCommandBufferTiledDownsample;
static int nvn_trace_n_nvnCommandBufferDownsampleTextureView;
static int nvn_trace_n_nvnCommandBufferTiledDownsampleTextureView;
static int nvn_trace_n_nvnCommandBufferBarrier;
static int nvn_trace_n_nvnCommandBufferWaitSync;
static int nvn_trace_n_nvnCommandBufferFenceSync;
static int nvn_trace_n_nvnCommandBufferSetTexturePool;
static int nvn_trace_n_nvnCommandBufferSetSamplerPool;
static int nvn_trace_n_nvnCommandBufferSetShaderScratchMemory;
static int nvn_trace_n_nvnCommandBufferSaveZCullData;
static int nvn_trace_n_nvnCommandBufferRestoreZCullData;
static int nvn_trace_n_nvnCommandBufferSetCopyRowStride;
static int nvn_trace_n_nvnCommandBufferSetCopyImageStride;
static int nvn_trace_n_nvnCommandBufferGetCopyRowStride;
static int nvn_trace_n_nvnCommandBufferGetCopyImageStride;
static int nvn_trace_n_nvnCommandBufferDrawTexture;
static int nvn_trace_n_nvnProgramSetSubroutineLinkage;
static int nvn_trace_n_nvnCommandBufferSetProgramSubroutines;
static int nvn_trace_n_nvnCommandBufferBindCoverageModulationTable;
static int nvn_trace_n_nvnCommandBufferResolveDepthBuffer;
static int nvn_trace_n_nvnCommandBufferSetColorReductionEnable;
static int nvn_trace_n_nvnCommandBufferSetColorReductionThresholds;
static int nvn_trace_n_nvnCommandBufferPushDebugGroupStatic;
static int nvn_trace_n_nvnCommandBufferPushDebugGroupDynamic;
static int nvn_trace_n_nvnCommandBufferPushDebugGroup;
static int nvn_trace_n_nvnCommandBufferPopDebugGroup;
static int nvn_trace_n_nvnCommandBufferPopDebugGroupId;
static int nvn_trace_n_nvnCommandBufferInsertDebugMarkerStatic;
static int nvn_trace_n_nvnCommandBufferInsertDebugMarkerDynamic;
static int nvn_trace_n_nvnCommandBufferInsertDebugMarker;
static int nvn_trace_n_nvnCommandBufferGetMemoryCallback;
static int nvn_trace_n_nvnCommandBufferGetMemoryCallbackData;
static int nvn_trace_n_nvnCommandBufferIsRecording;
static int nvn_trace_n_nvnSyncInitialize;
static int nvn_trace_n_nvnSyncFinalize;
static int nvn_trace_n_nvnSyncSetDebugLabel;
static int nvn_trace_n_nvnQueueFenceSync;
static int nvn_trace_n_nvnSyncWait;
static int nvn_trace_n_nvnQueueWaitSync;
static int nvn_trace_n_nvnSyncInitializeFromFencedGLSync;
static int nvn_trace_n_nvnSyncCreateGLSync;
static int nvn_trace_n_nvnEventBuilderSetDefaults;
static int nvn_trace_n_nvnEventBuilderSetStorage;
static int nvn_trace_n_nvnEventBuilderGetStorage;
static int nvn_trace_n_nvnEventBuilderGetMemoryPool;
static int nvn_trace_n_nvnEventBuilderGetMemoryOffset;
static int nvn_trace_n_nvnEventInitialize;
static int nvn_trace_n_nvnEventFinalize;
static int nvn_trace_n_nvnEventGetValue;
static int nvn_trace_n_nvnEventSignal;
static int nvn_trace_n_nvnEventGetMemoryPool;
static int nvn_trace_n_nvnEventGetMemoryOffset;
static int nvn_trace_n_nvnCommandBufferWaitEvent;
static int nvn_trace_n_nvnCommandBufferSignalEvent;
static int nvn_trace_n_nvnCommandBufferSetStencilCullCriteria;
static int nvn_trace_n_nvnCommandBufferInsertCheckpointStatic;
static int nvn_trace_n_nvnCommandBufferInsertCheckpointDynamic;
static int nvn_trace_n_nvnDeviceSetAutomaticCheckpointsEnable;
static int nvn_trace_n_nvnDeviceSetAutomaticCheckpointsSamplingInterval;
static int nvn_trace_n_nvnDeviceSetAutomaticCheckpointsByDebugGroup;
static int nvn_trace_n_nvnDeviceSetCheckpointCallStacksEnable;
static int nvn_trace_n_nvnDeviceSetCheckpointMaxCallStacksDepth;
static uint64_t nvn_trace_nvnDeviceBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceBuilderSetDefaults < 3) { nvn_trace_n_nvnDeviceBuilderSetDefaults++;
        DMLOG_W("NVN call nvnDeviceBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceBuilderSetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceBuilderSetFlags < 3) { nvn_trace_n_nvnDeviceBuilderSetFlags++;
        DMLOG_W("NVN call nvnDeviceBuilderSetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceBuilderGetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceBuilderGetFlags < 3) { nvn_trace_n_nvnDeviceBuilderGetFlags++;
        DMLOG_W("NVN call nvnDeviceBuilderGetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceInitialize < 3) { nvn_trace_n_nvnDeviceInitialize++;
        DMLOG_W("NVN call nvnDeviceInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceFinalize < 3) { nvn_trace_n_nvnDeviceFinalize++;
        DMLOG_W("NVN call nvnDeviceFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetDebugLabel < 3) { nvn_trace_n_nvnDeviceSetDebugLabel++;
        DMLOG_W("NVN call nvnDeviceSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetProcAddress(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetProcAddress < 3) { nvn_trace_n_nvnDeviceGetProcAddress++;
        DMLOG_W("NVN call nvnDeviceGetProcAddress x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetInteger(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetInteger < 3) { nvn_trace_n_nvnDeviceGetInteger++;
        DMLOG_W("NVN call nvnDeviceGetInteger x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetCurrentTimestampInNanoseconds(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetCurrentTimestampInNanoseconds < 3) { nvn_trace_n_nvnDeviceGetCurrentTimestampInNanoseconds++;
        DMLOG_W("NVN call nvnDeviceGetCurrentTimestampInNanoseconds x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetIntermediateShaderCache(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetIntermediateShaderCache < 3) { nvn_trace_n_nvnDeviceSetIntermediateShaderCache++;
        DMLOG_W("NVN call nvnDeviceSetIntermediateShaderCache x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetTextureHandle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetTextureHandle < 3) { nvn_trace_n_nvnDeviceGetTextureHandle++;
        DMLOG_W("NVN call nvnDeviceGetTextureHandle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetTexelFetchHandle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetTexelFetchHandle < 3) { nvn_trace_n_nvnDeviceGetTexelFetchHandle++;
        DMLOG_W("NVN call nvnDeviceGetTexelFetchHandle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetImageHandle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetImageHandle < 3) { nvn_trace_n_nvnDeviceGetImageHandle++;
        DMLOG_W("NVN call nvnDeviceGetImageHandle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceInstallDebugCallback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceInstallDebugCallback < 3) { nvn_trace_n_nvnDeviceInstallDebugCallback++;
        DMLOG_W("NVN call nvnDeviceInstallDebugCallback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGenerateDebugDomainId(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGenerateDebugDomainId < 3) { nvn_trace_n_nvnDeviceGenerateDebugDomainId++;
        DMLOG_W("NVN call nvnDeviceGenerateDebugDomainId x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetWindowOriginMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetWindowOriginMode < 3) { nvn_trace_n_nvnDeviceSetWindowOriginMode++;
        DMLOG_W("NVN call nvnDeviceSetWindowOriginMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetDepthMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetDepthMode < 3) { nvn_trace_n_nvnDeviceSetDepthMode++;
        DMLOG_W("NVN call nvnDeviceSetDepthMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceRegisterFastClearColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceRegisterFastClearColor < 3) { nvn_trace_n_nvnDeviceRegisterFastClearColor++;
        DMLOG_W("NVN call nvnDeviceRegisterFastClearColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceRegisterFastClearColori(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceRegisterFastClearColori < 3) { nvn_trace_n_nvnDeviceRegisterFastClearColori++;
        DMLOG_W("NVN call nvnDeviceRegisterFastClearColori x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceRegisterFastClearColorui(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceRegisterFastClearColorui < 3) { nvn_trace_n_nvnDeviceRegisterFastClearColorui++;
        DMLOG_W("NVN call nvnDeviceRegisterFastClearColorui x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceRegisterFastClearDepth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceRegisterFastClearDepth < 3) { nvn_trace_n_nvnDeviceRegisterFastClearDepth++;
        DMLOG_W("NVN call nvnDeviceRegisterFastClearDepth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetWindowOriginMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetWindowOriginMode < 3) { nvn_trace_n_nvnDeviceGetWindowOriginMode++;
        DMLOG_W("NVN call nvnDeviceGetWindowOriginMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetDepthMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetDepthMode < 3) { nvn_trace_n_nvnDeviceGetDepthMode++;
        DMLOG_W("NVN call nvnDeviceGetDepthMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetTimestampInNanoseconds(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetTimestampInNanoseconds < 3) { nvn_trace_n_nvnDeviceGetTimestampInNanoseconds++;
        DMLOG_W("NVN call nvnDeviceGetTimestampInNanoseconds x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceApplyDeferredFinalizes(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceApplyDeferredFinalizes < 3) { nvn_trace_n_nvnDeviceApplyDeferredFinalizes++;
        DMLOG_W("NVN call nvnDeviceApplyDeferredFinalizes x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceFinalizeCommandHandle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceFinalizeCommandHandle < 3) { nvn_trace_n_nvnDeviceFinalizeCommandHandle++;
        DMLOG_W("NVN call nvnDeviceFinalizeCommandHandle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceWalkDebugDatabase(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceWalkDebugDatabase < 3) { nvn_trace_n_nvnDeviceWalkDebugDatabase++;
        DMLOG_W("NVN call nvnDeviceWalkDebugDatabase x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetSeparateTextureHandle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetSeparateTextureHandle < 3) { nvn_trace_n_nvnDeviceGetSeparateTextureHandle++;
        DMLOG_W("NVN call nvnDeviceGetSeparateTextureHandle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceGetSeparateSamplerHandle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceGetSeparateSamplerHandle < 3) { nvn_trace_n_nvnDeviceGetSeparateSamplerHandle++;
        DMLOG_W("NVN call nvnDeviceGetSeparateSamplerHandle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceIsExternalDebuggerAttached(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceIsExternalDebuggerAttached < 3) { nvn_trace_n_nvnDeviceIsExternalDebuggerAttached++;
        DMLOG_W("NVN call nvnDeviceIsExternalDebuggerAttached x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueGetError(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueGetError < 3) { nvn_trace_n_nvnQueueGetError++;
        DMLOG_W("NVN call nvnQueueGetError x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueGetTotalCommandMemoryUsed(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueGetTotalCommandMemoryUsed < 3) { nvn_trace_n_nvnQueueGetTotalCommandMemoryUsed++;
        DMLOG_W("NVN call nvnQueueGetTotalCommandMemoryUsed x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueGetTotalControlMemoryUsed(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueGetTotalControlMemoryUsed < 3) { nvn_trace_n_nvnQueueGetTotalControlMemoryUsed++;
        DMLOG_W("NVN call nvnQueueGetTotalControlMemoryUsed x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueGetTotalComputeMemoryUsed(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueGetTotalComputeMemoryUsed < 3) { nvn_trace_n_nvnQueueGetTotalComputeMemoryUsed++;
        DMLOG_W("NVN call nvnQueueGetTotalComputeMemoryUsed x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueResetMemoryUsageCounts(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueResetMemoryUsageCounts < 3) { nvn_trace_n_nvnQueueResetMemoryUsageCounts++;
        DMLOG_W("NVN call nvnQueueResetMemoryUsageCounts x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetDevice < 3) { nvn_trace_n_nvnQueueBuilderSetDevice++;
        DMLOG_W("NVN call nvnQueueBuilderSetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetDefaults < 3) { nvn_trace_n_nvnQueueBuilderSetDefaults++;
        DMLOG_W("NVN call nvnQueueBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetFlags < 3) { nvn_trace_n_nvnQueueBuilderSetFlags++;
        DMLOG_W("NVN call nvnQueueBuilderSetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetCommandMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetCommandMemorySize < 3) { nvn_trace_n_nvnQueueBuilderSetCommandMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderSetCommandMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetComputeMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetComputeMemorySize < 3) { nvn_trace_n_nvnQueueBuilderSetComputeMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderSetComputeMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetControlMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetControlMemorySize < 3) { nvn_trace_n_nvnQueueBuilderSetControlMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderSetControlMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetQueueMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetQueueMemorySize < 3) { nvn_trace_n_nvnQueueBuilderGetQueueMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderGetQueueMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetQueueMemory(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetQueueMemory < 3) { nvn_trace_n_nvnQueueBuilderSetQueueMemory++;
        DMLOG_W("NVN call nvnQueueBuilderSetQueueMemory x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetCommandFlushThreshold(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetCommandFlushThreshold < 3) { nvn_trace_n_nvnQueueBuilderSetCommandFlushThreshold++;
        DMLOG_W("NVN call nvnQueueBuilderSetCommandFlushThreshold x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderSetQueuePriority(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderSetQueuePriority < 3) { nvn_trace_n_nvnQueueBuilderSetQueuePriority++;
        DMLOG_W("NVN call nvnQueueBuilderSetQueuePriority x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetQueuePriority(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetQueuePriority < 3) { nvn_trace_n_nvnQueueBuilderGetQueuePriority++;
        DMLOG_W("NVN call nvnQueueBuilderGetQueuePriority x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetDevice < 3) { nvn_trace_n_nvnQueueBuilderGetDevice++;
        DMLOG_W("NVN call nvnQueueBuilderGetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetFlags < 3) { nvn_trace_n_nvnQueueBuilderGetFlags++;
        DMLOG_W("NVN call nvnQueueBuilderGetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetCommandMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetCommandMemorySize < 3) { nvn_trace_n_nvnQueueBuilderGetCommandMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderGetCommandMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetComputeMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetComputeMemorySize < 3) { nvn_trace_n_nvnQueueBuilderGetComputeMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderGetComputeMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetControlMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetControlMemorySize < 3) { nvn_trace_n_nvnQueueBuilderGetControlMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderGetControlMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetCommandFlushThreshold(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetCommandFlushThreshold < 3) { nvn_trace_n_nvnQueueBuilderGetCommandFlushThreshold++;
        DMLOG_W("NVN call nvnQueueBuilderGetCommandFlushThreshold x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetMemorySize < 3) { nvn_trace_n_nvnQueueBuilderGetMemorySize++;
        DMLOG_W("NVN call nvnQueueBuilderGetMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueBuilderGetMemory(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueBuilderGetMemory < 3) { nvn_trace_n_nvnQueueBuilderGetMemory++;
        DMLOG_W("NVN call nvnQueueBuilderGetMemory x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueInitialize < 3) { nvn_trace_n_nvnQueueInitialize++;
        DMLOG_W("NVN call nvnQueueInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueFinalize < 3) { nvn_trace_n_nvnQueueFinalize++;
        DMLOG_W("NVN call nvnQueueFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueSetDebugLabel < 3) { nvn_trace_n_nvnQueueSetDebugLabel++;
        DMLOG_W("NVN call nvnQueueSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueSubmitCommands(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueSubmitCommands < 3) { nvn_trace_n_nvnQueueSubmitCommands++;
        DMLOG_W("NVN call nvnQueueSubmitCommands x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueFlush(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueFlush < 3) { nvn_trace_n_nvnQueueFlush++;
        DMLOG_W("NVN call nvnQueueFlush x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueFinish(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueFinish < 3) { nvn_trace_n_nvnQueueFinish++;
        DMLOG_W("NVN call nvnQueueFinish x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueuePresentTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueuePresentTexture < 3) { nvn_trace_n_nvnQueuePresentTexture++;
        DMLOG_W("NVN call nvnQueuePresentTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueAcquireTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueAcquireTexture < 3) { nvn_trace_n_nvnQueueAcquireTexture++;
        DMLOG_W("NVN call nvnQueueAcquireTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderSetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderSetDevice < 3) { nvn_trace_n_nvnWindowBuilderSetDevice++;
        DMLOG_W("NVN call nvnWindowBuilderSetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderSetDefaults < 3) { nvn_trace_n_nvnWindowBuilderSetDefaults++;
        DMLOG_W("NVN call nvnWindowBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderSetNativeWindow(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderSetNativeWindow < 3) { nvn_trace_n_nvnWindowBuilderSetNativeWindow++;
        DMLOG_W("NVN call nvnWindowBuilderSetNativeWindow x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderSetTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderSetTextures < 3) { nvn_trace_n_nvnWindowBuilderSetTextures++;
        DMLOG_W("NVN call nvnWindowBuilderSetTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderSetPresentInterval(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderSetPresentInterval < 3) { nvn_trace_n_nvnWindowBuilderSetPresentInterval++;
        DMLOG_W("NVN call nvnWindowBuilderSetPresentInterval x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderSetNumActiveTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderSetNumActiveTextures < 3) { nvn_trace_n_nvnWindowBuilderSetNumActiveTextures++;
        DMLOG_W("NVN call nvnWindowBuilderSetNumActiveTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderGetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderGetDevice < 3) { nvn_trace_n_nvnWindowBuilderGetDevice++;
        DMLOG_W("NVN call nvnWindowBuilderGetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderGetNumTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderGetNumTextures < 3) { nvn_trace_n_nvnWindowBuilderGetNumTextures++;
        DMLOG_W("NVN call nvnWindowBuilderGetNumTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderGetTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderGetTexture < 3) { nvn_trace_n_nvnWindowBuilderGetTexture++;
        DMLOG_W("NVN call nvnWindowBuilderGetTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderGetNativeWindow(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderGetNativeWindow < 3) { nvn_trace_n_nvnWindowBuilderGetNativeWindow++;
        DMLOG_W("NVN call nvnWindowBuilderGetNativeWindow x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderGetPresentInterval(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderGetPresentInterval < 3) { nvn_trace_n_nvnWindowBuilderGetPresentInterval++;
        DMLOG_W("NVN call nvnWindowBuilderGetPresentInterval x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowBuilderGetNumActiveTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowBuilderGetNumActiveTextures < 3) { nvn_trace_n_nvnWindowBuilderGetNumActiveTextures++;
        DMLOG_W("NVN call nvnWindowBuilderGetNumActiveTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowInitialize < 3) { nvn_trace_n_nvnWindowInitialize++;
        DMLOG_W("NVN call nvnWindowInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowFinalize < 3) { nvn_trace_n_nvnWindowFinalize++;
        DMLOG_W("NVN call nvnWindowFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowSetDebugLabel < 3) { nvn_trace_n_nvnWindowSetDebugLabel++;
        DMLOG_W("NVN call nvnWindowSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowAcquireTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowAcquireTexture < 3) { nvn_trace_n_nvnWindowAcquireTexture++;
        DMLOG_W("NVN call nvnWindowAcquireTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowGetNativeWindow(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowGetNativeWindow < 3) { nvn_trace_n_nvnWindowGetNativeWindow++;
        DMLOG_W("NVN call nvnWindowGetNativeWindow x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowGetPresentInterval(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowGetPresentInterval < 3) { nvn_trace_n_nvnWindowGetPresentInterval++;
        DMLOG_W("NVN call nvnWindowGetPresentInterval x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowSetPresentInterval(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowSetPresentInterval < 3) { nvn_trace_n_nvnWindowSetPresentInterval++;
        DMLOG_W("NVN call nvnWindowSetPresentInterval x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowSetCrop(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowSetCrop < 3) { nvn_trace_n_nvnWindowSetCrop++;
        DMLOG_W("NVN call nvnWindowSetCrop x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowGetCrop(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowGetCrop < 3) { nvn_trace_n_nvnWindowGetCrop++;
        DMLOG_W("NVN call nvnWindowGetCrop x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowSetNumActiveTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowSetNumActiveTextures < 3) { nvn_trace_n_nvnWindowSetNumActiveTextures++;
        DMLOG_W("NVN call nvnWindowSetNumActiveTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowGetNumActiveTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowGetNumActiveTextures < 3) { nvn_trace_n_nvnWindowGetNumActiveTextures++;
        DMLOG_W("NVN call nvnWindowGetNumActiveTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnWindowGetNumTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnWindowGetNumTextures < 3) { nvn_trace_n_nvnWindowGetNumTextures++;
        DMLOG_W("NVN call nvnWindowGetNumTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramInitialize < 3) { nvn_trace_n_nvnProgramInitialize++;
        DMLOG_W("NVN call nvnProgramInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramFinalize < 3) { nvn_trace_n_nvnProgramFinalize++;
        DMLOG_W("NVN call nvnProgramFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramSetDebugLabel < 3) { nvn_trace_n_nvnProgramSetDebugLabel++;
        DMLOG_W("NVN call nvnProgramSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramSetShaders(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramSetShaders < 3) { nvn_trace_n_nvnProgramSetShaders++;
        DMLOG_W("NVN call nvnProgramSetShaders x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramSetShadersExt(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramSetShadersExt < 3) { nvn_trace_n_nvnProgramSetShadersExt++;
        DMLOG_W("NVN call nvnProgramSetShadersExt x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramSetSampleShading(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramSetSampleShading < 3) { nvn_trace_n_nvnProgramSetSampleShading++;
        DMLOG_W("NVN call nvnProgramSetSampleShading x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderSetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderSetDevice < 3) { nvn_trace_n_nvnMemoryPoolBuilderSetDevice++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderSetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderSetDefaults < 3) { nvn_trace_n_nvnMemoryPoolBuilderSetDefaults++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderSetStorage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderSetStorage < 3) { nvn_trace_n_nvnMemoryPoolBuilderSetStorage++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderSetStorage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderSetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderSetFlags < 3) { nvn_trace_n_nvnMemoryPoolBuilderSetFlags++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderSetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderGetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderGetDevice < 3) { nvn_trace_n_nvnMemoryPoolBuilderGetDevice++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderGetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderGetMemory(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderGetMemory < 3) { nvn_trace_n_nvnMemoryPoolBuilderGetMemory++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderGetMemory x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderGetSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderGetSize < 3) { nvn_trace_n_nvnMemoryPoolBuilderGetSize++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderGetSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolBuilderGetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolBuilderGetFlags < 3) { nvn_trace_n_nvnMemoryPoolBuilderGetFlags++;
        DMLOG_W("NVN call nvnMemoryPoolBuilderGetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolInitialize < 3) { nvn_trace_n_nvnMemoryPoolInitialize++;
        DMLOG_W("NVN call nvnMemoryPoolInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolSetDebugLabel < 3) { nvn_trace_n_nvnMemoryPoolSetDebugLabel++;
        DMLOG_W("NVN call nvnMemoryPoolSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolFinalize < 3) { nvn_trace_n_nvnMemoryPoolFinalize++;
        DMLOG_W("NVN call nvnMemoryPoolFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolMap(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolMap < 3) { nvn_trace_n_nvnMemoryPoolMap++;
        DMLOG_W("NVN call nvnMemoryPoolMap x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolFlushMappedRange(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolFlushMappedRange < 3) { nvn_trace_n_nvnMemoryPoolFlushMappedRange++;
        DMLOG_W("NVN call nvnMemoryPoolFlushMappedRange x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolInvalidateMappedRange(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolInvalidateMappedRange < 3) { nvn_trace_n_nvnMemoryPoolInvalidateMappedRange++;
        DMLOG_W("NVN call nvnMemoryPoolInvalidateMappedRange x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolGetBufferAddress(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolGetBufferAddress < 3) { nvn_trace_n_nvnMemoryPoolGetBufferAddress++;
        DMLOG_W("NVN call nvnMemoryPoolGetBufferAddress x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolMapVirtual(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolMapVirtual < 3) { nvn_trace_n_nvnMemoryPoolMapVirtual++;
        DMLOG_W("NVN call nvnMemoryPoolMapVirtual x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolGetSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolGetSize < 3) { nvn_trace_n_nvnMemoryPoolGetSize++;
        DMLOG_W("NVN call nvnMemoryPoolGetSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMemoryPoolGetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMemoryPoolGetFlags < 3) { nvn_trace_n_nvnMemoryPoolGetFlags++;
        DMLOG_W("NVN call nvnMemoryPoolGetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolInitialize < 3) { nvn_trace_n_nvnTexturePoolInitialize++;
        DMLOG_W("NVN call nvnTexturePoolInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolSetDebugLabel < 3) { nvn_trace_n_nvnTexturePoolSetDebugLabel++;
        DMLOG_W("NVN call nvnTexturePoolSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolFinalize < 3) { nvn_trace_n_nvnTexturePoolFinalize++;
        DMLOG_W("NVN call nvnTexturePoolFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolRegisterTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolRegisterTexture < 3) { nvn_trace_n_nvnTexturePoolRegisterTexture++;
        DMLOG_W("NVN call nvnTexturePoolRegisterTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolRegisterImage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolRegisterImage < 3) { nvn_trace_n_nvnTexturePoolRegisterImage++;
        DMLOG_W("NVN call nvnTexturePoolRegisterImage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolGetMemoryPool < 3) { nvn_trace_n_nvnTexturePoolGetMemoryPool++;
        DMLOG_W("NVN call nvnTexturePoolGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolGetMemoryOffset < 3) { nvn_trace_n_nvnTexturePoolGetMemoryOffset++;
        DMLOG_W("NVN call nvnTexturePoolGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTexturePoolGetSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTexturePoolGetSize < 3) { nvn_trace_n_nvnTexturePoolGetSize++;
        DMLOG_W("NVN call nvnTexturePoolGetSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolInitialize < 3) { nvn_trace_n_nvnSamplerPoolInitialize++;
        DMLOG_W("NVN call nvnSamplerPoolInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolSetDebugLabel < 3) { nvn_trace_n_nvnSamplerPoolSetDebugLabel++;
        DMLOG_W("NVN call nvnSamplerPoolSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolFinalize < 3) { nvn_trace_n_nvnSamplerPoolFinalize++;
        DMLOG_W("NVN call nvnSamplerPoolFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolRegisterSampler(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolRegisterSampler < 3) { nvn_trace_n_nvnSamplerPoolRegisterSampler++;
        DMLOG_W("NVN call nvnSamplerPoolRegisterSampler x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolRegisterSamplerBuilder(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolRegisterSamplerBuilder < 3) { nvn_trace_n_nvnSamplerPoolRegisterSamplerBuilder++;
        DMLOG_W("NVN call nvnSamplerPoolRegisterSamplerBuilder x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolGetMemoryPool < 3) { nvn_trace_n_nvnSamplerPoolGetMemoryPool++;
        DMLOG_W("NVN call nvnSamplerPoolGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolGetMemoryOffset < 3) { nvn_trace_n_nvnSamplerPoolGetMemoryOffset++;
        DMLOG_W("NVN call nvnSamplerPoolGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerPoolGetSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerPoolGetSize < 3) { nvn_trace_n_nvnSamplerPoolGetSize++;
        DMLOG_W("NVN call nvnSamplerPoolGetSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderSetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderSetDevice < 3) { nvn_trace_n_nvnBufferBuilderSetDevice++;
        DMLOG_W("NVN call nvnBufferBuilderSetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderSetDefaults < 3) { nvn_trace_n_nvnBufferBuilderSetDefaults++;
        DMLOG_W("NVN call nvnBufferBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderSetStorage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderSetStorage < 3) { nvn_trace_n_nvnBufferBuilderSetStorage++;
        DMLOG_W("NVN call nvnBufferBuilderSetStorage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderGetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderGetDevice < 3) { nvn_trace_n_nvnBufferBuilderGetDevice++;
        DMLOG_W("NVN call nvnBufferBuilderGetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderGetMemoryPool < 3) { nvn_trace_n_nvnBufferBuilderGetMemoryPool++;
        DMLOG_W("NVN call nvnBufferBuilderGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderGetMemoryOffset < 3) { nvn_trace_n_nvnBufferBuilderGetMemoryOffset++;
        DMLOG_W("NVN call nvnBufferBuilderGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferBuilderGetSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferBuilderGetSize < 3) { nvn_trace_n_nvnBufferBuilderGetSize++;
        DMLOG_W("NVN call nvnBufferBuilderGetSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferInitialize < 3) { nvn_trace_n_nvnBufferInitialize++;
        DMLOG_W("NVN call nvnBufferInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferSetDebugLabel < 3) { nvn_trace_n_nvnBufferSetDebugLabel++;
        DMLOG_W("NVN call nvnBufferSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferFinalize < 3) { nvn_trace_n_nvnBufferFinalize++;
        DMLOG_W("NVN call nvnBufferFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferMap(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferMap < 3) { nvn_trace_n_nvnBufferMap++;
        DMLOG_W("NVN call nvnBufferMap x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferGetAddress(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferGetAddress < 3) { nvn_trace_n_nvnBufferGetAddress++;
        DMLOG_W("NVN call nvnBufferGetAddress x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferFlushMappedRange(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferFlushMappedRange < 3) { nvn_trace_n_nvnBufferFlushMappedRange++;
        DMLOG_W("NVN call nvnBufferFlushMappedRange x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferInvalidateMappedRange(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferInvalidateMappedRange < 3) { nvn_trace_n_nvnBufferInvalidateMappedRange++;
        DMLOG_W("NVN call nvnBufferInvalidateMappedRange x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferGetMemoryPool < 3) { nvn_trace_n_nvnBufferGetMemoryPool++;
        DMLOG_W("NVN call nvnBufferGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferGetMemoryOffset < 3) { nvn_trace_n_nvnBufferGetMemoryOffset++;
        DMLOG_W("NVN call nvnBufferGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferGetSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferGetSize < 3) { nvn_trace_n_nvnBufferGetSize++;
        DMLOG_W("NVN call nvnBufferGetSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBufferGetDebugID(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBufferGetDebugID < 3) { nvn_trace_n_nvnBufferGetDebugID++;
        DMLOG_W("NVN call nvnBufferGetDebugID x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetDevice < 3) { nvn_trace_n_nvnTextureBuilderSetDevice++;
        DMLOG_W("NVN call nvnTextureBuilderSetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetDefaults < 3) { nvn_trace_n_nvnTextureBuilderSetDefaults++;
        DMLOG_W("NVN call nvnTextureBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetFlags < 3) { nvn_trace_n_nvnTextureBuilderSetFlags++;
        DMLOG_W("NVN call nvnTextureBuilderSetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetTarget < 3) { nvn_trace_n_nvnTextureBuilderSetTarget++;
        DMLOG_W("NVN call nvnTextureBuilderSetTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetWidth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetWidth < 3) { nvn_trace_n_nvnTextureBuilderSetWidth++;
        DMLOG_W("NVN call nvnTextureBuilderSetWidth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetHeight(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetHeight < 3) { nvn_trace_n_nvnTextureBuilderSetHeight++;
        DMLOG_W("NVN call nvnTextureBuilderSetHeight x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetDepth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetDepth < 3) { nvn_trace_n_nvnTextureBuilderSetDepth++;
        DMLOG_W("NVN call nvnTextureBuilderSetDepth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetSize1D(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetSize1D < 3) { nvn_trace_n_nvnTextureBuilderSetSize1D++;
        DMLOG_W("NVN call nvnTextureBuilderSetSize1D x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetSize2D(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetSize2D < 3) { nvn_trace_n_nvnTextureBuilderSetSize2D++;
        DMLOG_W("NVN call nvnTextureBuilderSetSize2D x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetSize3D(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetSize3D < 3) { nvn_trace_n_nvnTextureBuilderSetSize3D++;
        DMLOG_W("NVN call nvnTextureBuilderSetSize3D x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetLevels < 3) { nvn_trace_n_nvnTextureBuilderSetLevels++;
        DMLOG_W("NVN call nvnTextureBuilderSetLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetFormat < 3) { nvn_trace_n_nvnTextureBuilderSetFormat++;
        DMLOG_W("NVN call nvnTextureBuilderSetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetSamples < 3) { nvn_trace_n_nvnTextureBuilderSetSamples++;
        DMLOG_W("NVN call nvnTextureBuilderSetSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetSwizzle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetSwizzle < 3) { nvn_trace_n_nvnTextureBuilderSetSwizzle++;
        DMLOG_W("NVN call nvnTextureBuilderSetSwizzle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetDepthStencilMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetDepthStencilMode < 3) { nvn_trace_n_nvnTextureBuilderSetDepthStencilMode++;
        DMLOG_W("NVN call nvnTextureBuilderSetDepthStencilMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetStorageSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetStorageSize < 3) { nvn_trace_n_nvnTextureBuilderGetStorageSize++;
        DMLOG_W("NVN call nvnTextureBuilderGetStorageSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetStorageAlignment(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetStorageAlignment < 3) { nvn_trace_n_nvnTextureBuilderGetStorageAlignment++;
        DMLOG_W("NVN call nvnTextureBuilderGetStorageAlignment x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetStorage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetStorage < 3) { nvn_trace_n_nvnTextureBuilderSetStorage++;
        DMLOG_W("NVN call nvnTextureBuilderSetStorage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetPackagedTextureData(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetPackagedTextureData < 3) { nvn_trace_n_nvnTextureBuilderSetPackagedTextureData++;
        DMLOG_W("NVN call nvnTextureBuilderSetPackagedTextureData x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetPackagedTextureLayout(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetPackagedTextureLayout < 3) { nvn_trace_n_nvnTextureBuilderSetPackagedTextureLayout++;
        DMLOG_W("NVN call nvnTextureBuilderSetPackagedTextureLayout x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetStride < 3) { nvn_trace_n_nvnTextureBuilderSetStride++;
        DMLOG_W("NVN call nvnTextureBuilderSetStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderSetGLTextureName(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderSetGLTextureName < 3) { nvn_trace_n_nvnTextureBuilderSetGLTextureName++;
        DMLOG_W("NVN call nvnTextureBuilderSetGLTextureName x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetStorageClass(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetStorageClass < 3) { nvn_trace_n_nvnTextureBuilderGetStorageClass++;
        DMLOG_W("NVN call nvnTextureBuilderGetStorageClass x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetDevice < 3) { nvn_trace_n_nvnTextureBuilderGetDevice++;
        DMLOG_W("NVN call nvnTextureBuilderGetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetFlags < 3) { nvn_trace_n_nvnTextureBuilderGetFlags++;
        DMLOG_W("NVN call nvnTextureBuilderGetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetTarget < 3) { nvn_trace_n_nvnTextureBuilderGetTarget++;
        DMLOG_W("NVN call nvnTextureBuilderGetTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetWidth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetWidth < 3) { nvn_trace_n_nvnTextureBuilderGetWidth++;
        DMLOG_W("NVN call nvnTextureBuilderGetWidth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetHeight(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetHeight < 3) { nvn_trace_n_nvnTextureBuilderGetHeight++;
        DMLOG_W("NVN call nvnTextureBuilderGetHeight x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetDepth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetDepth < 3) { nvn_trace_n_nvnTextureBuilderGetDepth++;
        DMLOG_W("NVN call nvnTextureBuilderGetDepth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetLevels < 3) { nvn_trace_n_nvnTextureBuilderGetLevels++;
        DMLOG_W("NVN call nvnTextureBuilderGetLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetFormat < 3) { nvn_trace_n_nvnTextureBuilderGetFormat++;
        DMLOG_W("NVN call nvnTextureBuilderGetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetSamples < 3) { nvn_trace_n_nvnTextureBuilderGetSamples++;
        DMLOG_W("NVN call nvnTextureBuilderGetSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetSwizzle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetSwizzle < 3) { nvn_trace_n_nvnTextureBuilderGetSwizzle++;
        DMLOG_W("NVN call nvnTextureBuilderGetSwizzle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetDepthStencilMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetDepthStencilMode < 3) { nvn_trace_n_nvnTextureBuilderGetDepthStencilMode++;
        DMLOG_W("NVN call nvnTextureBuilderGetDepthStencilMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetPackagedTextureData(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetPackagedTextureData < 3) { nvn_trace_n_nvnTextureBuilderGetPackagedTextureData++;
        DMLOG_W("NVN call nvnTextureBuilderGetPackagedTextureData x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetPackagedTextureLayout(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetPackagedTextureLayout < 3) { nvn_trace_n_nvnTextureBuilderGetPackagedTextureLayout++;
        DMLOG_W("NVN call nvnTextureBuilderGetPackagedTextureLayout x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetStride < 3) { nvn_trace_n_nvnTextureBuilderGetStride++;
        DMLOG_W("NVN call nvnTextureBuilderGetStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetSparseTileLayout(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetSparseTileLayout < 3) { nvn_trace_n_nvnTextureBuilderGetSparseTileLayout++;
        DMLOG_W("NVN call nvnTextureBuilderGetSparseTileLayout x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetGLTextureName(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetGLTextureName < 3) { nvn_trace_n_nvnTextureBuilderGetGLTextureName++;
        DMLOG_W("NVN call nvnTextureBuilderGetGLTextureName x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetZCullStorageSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetZCullStorageSize < 3) { nvn_trace_n_nvnTextureBuilderGetZCullStorageSize++;
        DMLOG_W("NVN call nvnTextureBuilderGetZCullStorageSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetMemoryPool < 3) { nvn_trace_n_nvnTextureBuilderGetMemoryPool++;
        DMLOG_W("NVN call nvnTextureBuilderGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetMemoryOffset < 3) { nvn_trace_n_nvnTextureBuilderGetMemoryOffset++;
        DMLOG_W("NVN call nvnTextureBuilderGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureBuilderGetRawStorageClass(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureBuilderGetRawStorageClass < 3) { nvn_trace_n_nvnTextureBuilderGetRawStorageClass++;
        DMLOG_W("NVN call nvnTextureBuilderGetRawStorageClass x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetDefaults < 3) { nvn_trace_n_nvnTextureViewSetDefaults++;
        DMLOG_W("NVN call nvnTextureViewSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetLevels < 3) { nvn_trace_n_nvnTextureViewSetLevels++;
        DMLOG_W("NVN call nvnTextureViewSetLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetLayers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetLayers < 3) { nvn_trace_n_nvnTextureViewSetLayers++;
        DMLOG_W("NVN call nvnTextureViewSetLayers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetFormat < 3) { nvn_trace_n_nvnTextureViewSetFormat++;
        DMLOG_W("NVN call nvnTextureViewSetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetSwizzle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetSwizzle < 3) { nvn_trace_n_nvnTextureViewSetSwizzle++;
        DMLOG_W("NVN call nvnTextureViewSetSwizzle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetDepthStencilMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetDepthStencilMode < 3) { nvn_trace_n_nvnTextureViewSetDepthStencilMode++;
        DMLOG_W("NVN call nvnTextureViewSetDepthStencilMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewSetTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewSetTarget < 3) { nvn_trace_n_nvnTextureViewSetTarget++;
        DMLOG_W("NVN call nvnTextureViewSetTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewGetLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewGetLevels < 3) { nvn_trace_n_nvnTextureViewGetLevels++;
        DMLOG_W("NVN call nvnTextureViewGetLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewGetLayers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewGetLayers < 3) { nvn_trace_n_nvnTextureViewGetLayers++;
        DMLOG_W("NVN call nvnTextureViewGetLayers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewGetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewGetFormat < 3) { nvn_trace_n_nvnTextureViewGetFormat++;
        DMLOG_W("NVN call nvnTextureViewGetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewGetSwizzle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewGetSwizzle < 3) { nvn_trace_n_nvnTextureViewGetSwizzle++;
        DMLOG_W("NVN call nvnTextureViewGetSwizzle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewGetDepthStencilMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewGetDepthStencilMode < 3) { nvn_trace_n_nvnTextureViewGetDepthStencilMode++;
        DMLOG_W("NVN call nvnTextureViewGetDepthStencilMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewGetTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewGetTarget < 3) { nvn_trace_n_nvnTextureViewGetTarget++;
        DMLOG_W("NVN call nvnTextureViewGetTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureViewCompare(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureViewCompare < 3) { nvn_trace_n_nvnTextureViewCompare++;
        DMLOG_W("NVN call nvnTextureViewCompare x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureInitialize < 3) { nvn_trace_n_nvnTextureInitialize++;
        DMLOG_W("NVN call nvnTextureInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetZCullStorageSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetZCullStorageSize < 3) { nvn_trace_n_nvnTextureGetZCullStorageSize++;
        DMLOG_W("NVN call nvnTextureGetZCullStorageSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureFinalize < 3) { nvn_trace_n_nvnTextureFinalize++;
        DMLOG_W("NVN call nvnTextureFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureSetDebugLabel < 3) { nvn_trace_n_nvnTextureSetDebugLabel++;
        DMLOG_W("NVN call nvnTextureSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetStorageClass(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetStorageClass < 3) { nvn_trace_n_nvnTextureGetStorageClass++;
        DMLOG_W("NVN call nvnTextureGetStorageClass x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetViewOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetViewOffset < 3) { nvn_trace_n_nvnTextureGetViewOffset++;
        DMLOG_W("NVN call nvnTextureGetViewOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetFlags(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetFlags < 3) { nvn_trace_n_nvnTextureGetFlags++;
        DMLOG_W("NVN call nvnTextureGetFlags x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetTarget < 3) { nvn_trace_n_nvnTextureGetTarget++;
        DMLOG_W("NVN call nvnTextureGetTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetWidth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetWidth < 3) { nvn_trace_n_nvnTextureGetWidth++;
        DMLOG_W("NVN call nvnTextureGetWidth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetHeight(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetHeight < 3) { nvn_trace_n_nvnTextureGetHeight++;
        DMLOG_W("NVN call nvnTextureGetHeight x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetDepth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetDepth < 3) { nvn_trace_n_nvnTextureGetDepth++;
        DMLOG_W("NVN call nvnTextureGetDepth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetLevels < 3) { nvn_trace_n_nvnTextureGetLevels++;
        DMLOG_W("NVN call nvnTextureGetLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetFormat < 3) { nvn_trace_n_nvnTextureGetFormat++;
        DMLOG_W("NVN call nvnTextureGetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetSamples < 3) { nvn_trace_n_nvnTextureGetSamples++;
        DMLOG_W("NVN call nvnTextureGetSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetSwizzle(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetSwizzle < 3) { nvn_trace_n_nvnTextureGetSwizzle++;
        DMLOG_W("NVN call nvnTextureGetSwizzle x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetDepthStencilMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetDepthStencilMode < 3) { nvn_trace_n_nvnTextureGetDepthStencilMode++;
        DMLOG_W("NVN call nvnTextureGetDepthStencilMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetStride < 3) { nvn_trace_n_nvnTextureGetStride++;
        DMLOG_W("NVN call nvnTextureGetStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetTextureAddress(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetTextureAddress < 3) { nvn_trace_n_nvnTextureGetTextureAddress++;
        DMLOG_W("NVN call nvnTextureGetTextureAddress x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetSparseTileLayout(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetSparseTileLayout < 3) { nvn_trace_n_nvnTextureGetSparseTileLayout++;
        DMLOG_W("NVN call nvnTextureGetSparseTileLayout x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureWriteTexels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureWriteTexels < 3) { nvn_trace_n_nvnTextureWriteTexels++;
        DMLOG_W("NVN call nvnTextureWriteTexels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureWriteTexelsStrided(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureWriteTexelsStrided < 3) { nvn_trace_n_nvnTextureWriteTexelsStrided++;
        DMLOG_W("NVN call nvnTextureWriteTexelsStrided x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureReadTexels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureReadTexels < 3) { nvn_trace_n_nvnTextureReadTexels++;
        DMLOG_W("NVN call nvnTextureReadTexels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureReadTexelsStrided(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureReadTexelsStrided < 3) { nvn_trace_n_nvnTextureReadTexelsStrided++;
        DMLOG_W("NVN call nvnTextureReadTexelsStrided x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureFlushTexels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureFlushTexels < 3) { nvn_trace_n_nvnTextureFlushTexels++;
        DMLOG_W("NVN call nvnTextureFlushTexels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureInvalidateTexels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureInvalidateTexels < 3) { nvn_trace_n_nvnTextureInvalidateTexels++;
        DMLOG_W("NVN call nvnTextureInvalidateTexels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetMemoryPool < 3) { nvn_trace_n_nvnTextureGetMemoryPool++;
        DMLOG_W("NVN call nvnTextureGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetMemoryOffset < 3) { nvn_trace_n_nvnTextureGetMemoryOffset++;
        DMLOG_W("NVN call nvnTextureGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetStorageSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetStorageSize < 3) { nvn_trace_n_nvnTextureGetStorageSize++;
        DMLOG_W("NVN call nvnTextureGetStorageSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureCompare(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureCompare < 3) { nvn_trace_n_nvnTextureCompare++;
        DMLOG_W("NVN call nvnTextureCompare x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetDebugID(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetDebugID < 3) { nvn_trace_n_nvnTextureGetDebugID++;
        DMLOG_W("NVN call nvnTextureGetDebugID x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnTextureGetRawStorageClass(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnTextureGetRawStorageClass < 3) { nvn_trace_n_nvnTextureGetRawStorageClass++;
        DMLOG_W("NVN call nvnTextureGetRawStorageClass x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetDevice < 3) { nvn_trace_n_nvnSamplerBuilderSetDevice++;
        DMLOG_W("NVN call nvnSamplerBuilderSetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetDefaults < 3) { nvn_trace_n_nvnSamplerBuilderSetDefaults++;
        DMLOG_W("NVN call nvnSamplerBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetMinMagFilter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetMinMagFilter < 3) { nvn_trace_n_nvnSamplerBuilderSetMinMagFilter++;
        DMLOG_W("NVN call nvnSamplerBuilderSetMinMagFilter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetWrapMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetWrapMode < 3) { nvn_trace_n_nvnSamplerBuilderSetWrapMode++;
        DMLOG_W("NVN call nvnSamplerBuilderSetWrapMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetLodClamp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetLodClamp < 3) { nvn_trace_n_nvnSamplerBuilderSetLodClamp++;
        DMLOG_W("NVN call nvnSamplerBuilderSetLodClamp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetLodBias(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetLodBias < 3) { nvn_trace_n_nvnSamplerBuilderSetLodBias++;
        DMLOG_W("NVN call nvnSamplerBuilderSetLodBias x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetCompare(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetCompare < 3) { nvn_trace_n_nvnSamplerBuilderSetCompare++;
        DMLOG_W("NVN call nvnSamplerBuilderSetCompare x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetBorderColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetBorderColor < 3) { nvn_trace_n_nvnSamplerBuilderSetBorderColor++;
        DMLOG_W("NVN call nvnSamplerBuilderSetBorderColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetBorderColori(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetBorderColori < 3) { nvn_trace_n_nvnSamplerBuilderSetBorderColori++;
        DMLOG_W("NVN call nvnSamplerBuilderSetBorderColori x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetBorderColorui(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetBorderColorui < 3) { nvn_trace_n_nvnSamplerBuilderSetBorderColorui++;
        DMLOG_W("NVN call nvnSamplerBuilderSetBorderColorui x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetMaxAnisotropy(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetMaxAnisotropy < 3) { nvn_trace_n_nvnSamplerBuilderSetMaxAnisotropy++;
        DMLOG_W("NVN call nvnSamplerBuilderSetMaxAnisotropy x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetReductionFilter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetReductionFilter < 3) { nvn_trace_n_nvnSamplerBuilderSetReductionFilter++;
        DMLOG_W("NVN call nvnSamplerBuilderSetReductionFilter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderSetLodSnap(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderSetLodSnap < 3) { nvn_trace_n_nvnSamplerBuilderSetLodSnap++;
        DMLOG_W("NVN call nvnSamplerBuilderSetLodSnap x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetDevice(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetDevice < 3) { nvn_trace_n_nvnSamplerBuilderGetDevice++;
        DMLOG_W("NVN call nvnSamplerBuilderGetDevice x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetMinMagFilter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetMinMagFilter < 3) { nvn_trace_n_nvnSamplerBuilderGetMinMagFilter++;
        DMLOG_W("NVN call nvnSamplerBuilderGetMinMagFilter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetWrapMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetWrapMode < 3) { nvn_trace_n_nvnSamplerBuilderGetWrapMode++;
        DMLOG_W("NVN call nvnSamplerBuilderGetWrapMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetLodClamp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetLodClamp < 3) { nvn_trace_n_nvnSamplerBuilderGetLodClamp++;
        DMLOG_W("NVN call nvnSamplerBuilderGetLodClamp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetLodBias(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetLodBias < 3) { nvn_trace_n_nvnSamplerBuilderGetLodBias++;
        DMLOG_W("NVN call nvnSamplerBuilderGetLodBias x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetCompare(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetCompare < 3) { nvn_trace_n_nvnSamplerBuilderGetCompare++;
        DMLOG_W("NVN call nvnSamplerBuilderGetCompare x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetBorderColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetBorderColor < 3) { nvn_trace_n_nvnSamplerBuilderGetBorderColor++;
        DMLOG_W("NVN call nvnSamplerBuilderGetBorderColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetBorderColori(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetBorderColori < 3) { nvn_trace_n_nvnSamplerBuilderGetBorderColori++;
        DMLOG_W("NVN call nvnSamplerBuilderGetBorderColori x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetBorderColorui(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetBorderColorui < 3) { nvn_trace_n_nvnSamplerBuilderGetBorderColorui++;
        DMLOG_W("NVN call nvnSamplerBuilderGetBorderColorui x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetMaxAnisotropy(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetMaxAnisotropy < 3) { nvn_trace_n_nvnSamplerBuilderGetMaxAnisotropy++;
        DMLOG_W("NVN call nvnSamplerBuilderGetMaxAnisotropy x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetReductionFilter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetReductionFilter < 3) { nvn_trace_n_nvnSamplerBuilderGetReductionFilter++;
        DMLOG_W("NVN call nvnSamplerBuilderGetReductionFilter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerBuilderGetLodSnap(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerBuilderGetLodSnap < 3) { nvn_trace_n_nvnSamplerBuilderGetLodSnap++;
        DMLOG_W("NVN call nvnSamplerBuilderGetLodSnap x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerInitialize < 3) { nvn_trace_n_nvnSamplerInitialize++;
        DMLOG_W("NVN call nvnSamplerInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerFinalize < 3) { nvn_trace_n_nvnSamplerFinalize++;
        DMLOG_W("NVN call nvnSamplerFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerSetDebugLabel < 3) { nvn_trace_n_nvnSamplerSetDebugLabel++;
        DMLOG_W("NVN call nvnSamplerSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetMinMagFilter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetMinMagFilter < 3) { nvn_trace_n_nvnSamplerGetMinMagFilter++;
        DMLOG_W("NVN call nvnSamplerGetMinMagFilter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetWrapMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetWrapMode < 3) { nvn_trace_n_nvnSamplerGetWrapMode++;
        DMLOG_W("NVN call nvnSamplerGetWrapMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetLodClamp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetLodClamp < 3) { nvn_trace_n_nvnSamplerGetLodClamp++;
        DMLOG_W("NVN call nvnSamplerGetLodClamp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetLodBias(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetLodBias < 3) { nvn_trace_n_nvnSamplerGetLodBias++;
        DMLOG_W("NVN call nvnSamplerGetLodBias x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetCompare(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetCompare < 3) { nvn_trace_n_nvnSamplerGetCompare++;
        DMLOG_W("NVN call nvnSamplerGetCompare x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetBorderColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetBorderColor < 3) { nvn_trace_n_nvnSamplerGetBorderColor++;
        DMLOG_W("NVN call nvnSamplerGetBorderColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetBorderColori(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetBorderColori < 3) { nvn_trace_n_nvnSamplerGetBorderColori++;
        DMLOG_W("NVN call nvnSamplerGetBorderColori x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetBorderColorui(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetBorderColorui < 3) { nvn_trace_n_nvnSamplerGetBorderColorui++;
        DMLOG_W("NVN call nvnSamplerGetBorderColorui x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetMaxAnisotropy(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetMaxAnisotropy < 3) { nvn_trace_n_nvnSamplerGetMaxAnisotropy++;
        DMLOG_W("NVN call nvnSamplerGetMaxAnisotropy x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetReductionFilter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetReductionFilter < 3) { nvn_trace_n_nvnSamplerGetReductionFilter++;
        DMLOG_W("NVN call nvnSamplerGetReductionFilter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerCompare(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerCompare < 3) { nvn_trace_n_nvnSamplerCompare++;
        DMLOG_W("NVN call nvnSamplerCompare x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSamplerGetDebugID(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSamplerGetDebugID < 3) { nvn_trace_n_nvnSamplerGetDebugID++;
        DMLOG_W("NVN call nvnSamplerGetDebugID x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetDefaults < 3) { nvn_trace_n_nvnBlendStateSetDefaults++;
        DMLOG_W("NVN call nvnBlendStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetBlendTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetBlendTarget < 3) { nvn_trace_n_nvnBlendStateSetBlendTarget++;
        DMLOG_W("NVN call nvnBlendStateSetBlendTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetBlendFunc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetBlendFunc < 3) { nvn_trace_n_nvnBlendStateSetBlendFunc++;
        DMLOG_W("NVN call nvnBlendStateSetBlendFunc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetBlendEquation(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetBlendEquation < 3) { nvn_trace_n_nvnBlendStateSetBlendEquation++;
        DMLOG_W("NVN call nvnBlendStateSetBlendEquation x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetAdvancedMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetAdvancedMode < 3) { nvn_trace_n_nvnBlendStateSetAdvancedMode++;
        DMLOG_W("NVN call nvnBlendStateSetAdvancedMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetAdvancedOverlap(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetAdvancedOverlap < 3) { nvn_trace_n_nvnBlendStateSetAdvancedOverlap++;
        DMLOG_W("NVN call nvnBlendStateSetAdvancedOverlap x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetAdvancedPremultipliedSrc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetAdvancedPremultipliedSrc < 3) { nvn_trace_n_nvnBlendStateSetAdvancedPremultipliedSrc++;
        DMLOG_W("NVN call nvnBlendStateSetAdvancedPremultipliedSrc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateSetAdvancedNormalizedDst(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateSetAdvancedNormalizedDst < 3) { nvn_trace_n_nvnBlendStateSetAdvancedNormalizedDst++;
        DMLOG_W("NVN call nvnBlendStateSetAdvancedNormalizedDst x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetBlendTarget(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetBlendTarget < 3) { nvn_trace_n_nvnBlendStateGetBlendTarget++;
        DMLOG_W("NVN call nvnBlendStateGetBlendTarget x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetBlendFunc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetBlendFunc < 3) { nvn_trace_n_nvnBlendStateGetBlendFunc++;
        DMLOG_W("NVN call nvnBlendStateGetBlendFunc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetBlendEquation(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetBlendEquation < 3) { nvn_trace_n_nvnBlendStateGetBlendEquation++;
        DMLOG_W("NVN call nvnBlendStateGetBlendEquation x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetAdvancedMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetAdvancedMode < 3) { nvn_trace_n_nvnBlendStateGetAdvancedMode++;
        DMLOG_W("NVN call nvnBlendStateGetAdvancedMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetAdvancedOverlap(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetAdvancedOverlap < 3) { nvn_trace_n_nvnBlendStateGetAdvancedOverlap++;
        DMLOG_W("NVN call nvnBlendStateGetAdvancedOverlap x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetAdvancedPremultipliedSrc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetAdvancedPremultipliedSrc < 3) { nvn_trace_n_nvnBlendStateGetAdvancedPremultipliedSrc++;
        DMLOG_W("NVN call nvnBlendStateGetAdvancedPremultipliedSrc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnBlendStateGetAdvancedNormalizedDst(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnBlendStateGetAdvancedNormalizedDst < 3) { nvn_trace_n_nvnBlendStateGetAdvancedNormalizedDst++;
        DMLOG_W("NVN call nvnBlendStateGetAdvancedNormalizedDst x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateSetDefaults < 3) { nvn_trace_n_nvnColorStateSetDefaults++;
        DMLOG_W("NVN call nvnColorStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateSetBlendEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateSetBlendEnable < 3) { nvn_trace_n_nvnColorStateSetBlendEnable++;
        DMLOG_W("NVN call nvnColorStateSetBlendEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateSetLogicOp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateSetLogicOp < 3) { nvn_trace_n_nvnColorStateSetLogicOp++;
        DMLOG_W("NVN call nvnColorStateSetLogicOp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateSetAlphaTest(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateSetAlphaTest < 3) { nvn_trace_n_nvnColorStateSetAlphaTest++;
        DMLOG_W("NVN call nvnColorStateSetAlphaTest x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateGetBlendEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateGetBlendEnable < 3) { nvn_trace_n_nvnColorStateGetBlendEnable++;
        DMLOG_W("NVN call nvnColorStateGetBlendEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateGetLogicOp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateGetLogicOp < 3) { nvn_trace_n_nvnColorStateGetLogicOp++;
        DMLOG_W("NVN call nvnColorStateGetLogicOp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnColorStateGetAlphaTest(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnColorStateGetAlphaTest < 3) { nvn_trace_n_nvnColorStateGetAlphaTest++;
        DMLOG_W("NVN call nvnColorStateGetAlphaTest x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnChannelMaskStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnChannelMaskStateSetDefaults < 3) { nvn_trace_n_nvnChannelMaskStateSetDefaults++;
        DMLOG_W("NVN call nvnChannelMaskStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnChannelMaskStateSetChannelMask(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnChannelMaskStateSetChannelMask < 3) { nvn_trace_n_nvnChannelMaskStateSetChannelMask++;
        DMLOG_W("NVN call nvnChannelMaskStateSetChannelMask x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnChannelMaskStateGetChannelMask(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnChannelMaskStateGetChannelMask < 3) { nvn_trace_n_nvnChannelMaskStateGetChannelMask++;
        DMLOG_W("NVN call nvnChannelMaskStateGetChannelMask x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetDefaults < 3) { nvn_trace_n_nvnMultisampleStateSetDefaults++;
        DMLOG_W("NVN call nvnMultisampleStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetMultisampleEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetMultisampleEnable < 3) { nvn_trace_n_nvnMultisampleStateSetMultisampleEnable++;
        DMLOG_W("NVN call nvnMultisampleStateSetMultisampleEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetSamples < 3) { nvn_trace_n_nvnMultisampleStateSetSamples++;
        DMLOG_W("NVN call nvnMultisampleStateSetSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetAlphaToCoverageEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetAlphaToCoverageEnable < 3) { nvn_trace_n_nvnMultisampleStateSetAlphaToCoverageEnable++;
        DMLOG_W("NVN call nvnMultisampleStateSetAlphaToCoverageEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetAlphaToCoverageDither(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetAlphaToCoverageDither < 3) { nvn_trace_n_nvnMultisampleStateSetAlphaToCoverageDither++;
        DMLOG_W("NVN call nvnMultisampleStateSetAlphaToCoverageDither x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetMultisampleEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetMultisampleEnable < 3) { nvn_trace_n_nvnMultisampleStateGetMultisampleEnable++;
        DMLOG_W("NVN call nvnMultisampleStateGetMultisampleEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetSamples < 3) { nvn_trace_n_nvnMultisampleStateGetSamples++;
        DMLOG_W("NVN call nvnMultisampleStateGetSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetAlphaToCoverageEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetAlphaToCoverageEnable < 3) { nvn_trace_n_nvnMultisampleStateGetAlphaToCoverageEnable++;
        DMLOG_W("NVN call nvnMultisampleStateGetAlphaToCoverageEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetAlphaToCoverageDither(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetAlphaToCoverageDither < 3) { nvn_trace_n_nvnMultisampleStateGetAlphaToCoverageDither++;
        DMLOG_W("NVN call nvnMultisampleStateGetAlphaToCoverageDither x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetRasterSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetRasterSamples < 3) { nvn_trace_n_nvnMultisampleStateSetRasterSamples++;
        DMLOG_W("NVN call nvnMultisampleStateSetRasterSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetRasterSamples(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetRasterSamples < 3) { nvn_trace_n_nvnMultisampleStateGetRasterSamples++;
        DMLOG_W("NVN call nvnMultisampleStateGetRasterSamples x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetCoverageModulationMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetCoverageModulationMode < 3) { nvn_trace_n_nvnMultisampleStateSetCoverageModulationMode++;
        DMLOG_W("NVN call nvnMultisampleStateSetCoverageModulationMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetCoverageModulationMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetCoverageModulationMode < 3) { nvn_trace_n_nvnMultisampleStateGetCoverageModulationMode++;
        DMLOG_W("NVN call nvnMultisampleStateGetCoverageModulationMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetCoverageToColorEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetCoverageToColorEnable < 3) { nvn_trace_n_nvnMultisampleStateSetCoverageToColorEnable++;
        DMLOG_W("NVN call nvnMultisampleStateSetCoverageToColorEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetCoverageToColorEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetCoverageToColorEnable < 3) { nvn_trace_n_nvnMultisampleStateGetCoverageToColorEnable++;
        DMLOG_W("NVN call nvnMultisampleStateGetCoverageToColorEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetCoverageToColorOutput(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetCoverageToColorOutput < 3) { nvn_trace_n_nvnMultisampleStateSetCoverageToColorOutput++;
        DMLOG_W("NVN call nvnMultisampleStateSetCoverageToColorOutput x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetCoverageToColorOutput(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetCoverageToColorOutput < 3) { nvn_trace_n_nvnMultisampleStateGetCoverageToColorOutput++;
        DMLOG_W("NVN call nvnMultisampleStateGetCoverageToColorOutput x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetSampleLocationsEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetSampleLocationsEnable < 3) { nvn_trace_n_nvnMultisampleStateSetSampleLocationsEnable++;
        DMLOG_W("NVN call nvnMultisampleStateSetSampleLocationsEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetSampleLocationsEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetSampleLocationsEnable < 3) { nvn_trace_n_nvnMultisampleStateGetSampleLocationsEnable++;
        DMLOG_W("NVN call nvnMultisampleStateGetSampleLocationsEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetSampleLocationsGrid(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetSampleLocationsGrid < 3) { nvn_trace_n_nvnMultisampleStateGetSampleLocationsGrid++;
        DMLOG_W("NVN call nvnMultisampleStateGetSampleLocationsGrid x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetSampleLocationsGridEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetSampleLocationsGridEnable < 3) { nvn_trace_n_nvnMultisampleStateSetSampleLocationsGridEnable++;
        DMLOG_W("NVN call nvnMultisampleStateSetSampleLocationsGridEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateGetSampleLocationsGridEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateGetSampleLocationsGridEnable < 3) { nvn_trace_n_nvnMultisampleStateGetSampleLocationsGridEnable++;
        DMLOG_W("NVN call nvnMultisampleStateGetSampleLocationsGridEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnMultisampleStateSetSampleLocations(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnMultisampleStateSetSampleLocations < 3) { nvn_trace_n_nvnMultisampleStateSetSampleLocations++;
        DMLOG_W("NVN call nvnMultisampleStateSetSampleLocations x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateSetDefaults < 3) { nvn_trace_n_nvnPolygonStateSetDefaults++;
        DMLOG_W("NVN call nvnPolygonStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateSetCullFace(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateSetCullFace < 3) { nvn_trace_n_nvnPolygonStateSetCullFace++;
        DMLOG_W("NVN call nvnPolygonStateSetCullFace x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateSetFrontFace(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateSetFrontFace < 3) { nvn_trace_n_nvnPolygonStateSetFrontFace++;
        DMLOG_W("NVN call nvnPolygonStateSetFrontFace x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateSetPolygonMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateSetPolygonMode < 3) { nvn_trace_n_nvnPolygonStateSetPolygonMode++;
        DMLOG_W("NVN call nvnPolygonStateSetPolygonMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateSetPolygonOffsetEnables(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateSetPolygonOffsetEnables < 3) { nvn_trace_n_nvnPolygonStateSetPolygonOffsetEnables++;
        DMLOG_W("NVN call nvnPolygonStateSetPolygonOffsetEnables x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateGetCullFace(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateGetCullFace < 3) { nvn_trace_n_nvnPolygonStateGetCullFace++;
        DMLOG_W("NVN call nvnPolygonStateGetCullFace x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateGetFrontFace(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateGetFrontFace < 3) { nvn_trace_n_nvnPolygonStateGetFrontFace++;
        DMLOG_W("NVN call nvnPolygonStateGetFrontFace x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateGetPolygonMode(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateGetPolygonMode < 3) { nvn_trace_n_nvnPolygonStateGetPolygonMode++;
        DMLOG_W("NVN call nvnPolygonStateGetPolygonMode x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnPolygonStateGetPolygonOffsetEnables(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnPolygonStateGetPolygonOffsetEnables < 3) { nvn_trace_n_nvnPolygonStateGetPolygonOffsetEnables++;
        DMLOG_W("NVN call nvnPolygonStateGetPolygonOffsetEnables x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetDefaults < 3) { nvn_trace_n_nvnDepthStencilStateSetDefaults++;
        DMLOG_W("NVN call nvnDepthStencilStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetDepthTestEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetDepthTestEnable < 3) { nvn_trace_n_nvnDepthStencilStateSetDepthTestEnable++;
        DMLOG_W("NVN call nvnDepthStencilStateSetDepthTestEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetDepthWriteEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetDepthWriteEnable < 3) { nvn_trace_n_nvnDepthStencilStateSetDepthWriteEnable++;
        DMLOG_W("NVN call nvnDepthStencilStateSetDepthWriteEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetDepthFunc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetDepthFunc < 3) { nvn_trace_n_nvnDepthStencilStateSetDepthFunc++;
        DMLOG_W("NVN call nvnDepthStencilStateSetDepthFunc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetStencilTestEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetStencilTestEnable < 3) { nvn_trace_n_nvnDepthStencilStateSetStencilTestEnable++;
        DMLOG_W("NVN call nvnDepthStencilStateSetStencilTestEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetStencilFunc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetStencilFunc < 3) { nvn_trace_n_nvnDepthStencilStateSetStencilFunc++;
        DMLOG_W("NVN call nvnDepthStencilStateSetStencilFunc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateSetStencilOp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateSetStencilOp < 3) { nvn_trace_n_nvnDepthStencilStateSetStencilOp++;
        DMLOG_W("NVN call nvnDepthStencilStateSetStencilOp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateGetDepthTestEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateGetDepthTestEnable < 3) { nvn_trace_n_nvnDepthStencilStateGetDepthTestEnable++;
        DMLOG_W("NVN call nvnDepthStencilStateGetDepthTestEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateGetDepthWriteEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateGetDepthWriteEnable < 3) { nvn_trace_n_nvnDepthStencilStateGetDepthWriteEnable++;
        DMLOG_W("NVN call nvnDepthStencilStateGetDepthWriteEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateGetDepthFunc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateGetDepthFunc < 3) { nvn_trace_n_nvnDepthStencilStateGetDepthFunc++;
        DMLOG_W("NVN call nvnDepthStencilStateGetDepthFunc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateGetStencilTestEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateGetStencilTestEnable < 3) { nvn_trace_n_nvnDepthStencilStateGetStencilTestEnable++;
        DMLOG_W("NVN call nvnDepthStencilStateGetStencilTestEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateGetStencilFunc(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateGetStencilFunc < 3) { nvn_trace_n_nvnDepthStencilStateGetStencilFunc++;
        DMLOG_W("NVN call nvnDepthStencilStateGetStencilFunc x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDepthStencilStateGetStencilOp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDepthStencilStateGetStencilOp < 3) { nvn_trace_n_nvnDepthStencilStateGetStencilOp++;
        DMLOG_W("NVN call nvnDepthStencilStateGetStencilOp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexAttribStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexAttribStateSetDefaults < 3) { nvn_trace_n_nvnVertexAttribStateSetDefaults++;
        DMLOG_W("NVN call nvnVertexAttribStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexAttribStateSetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexAttribStateSetFormat < 3) { nvn_trace_n_nvnVertexAttribStateSetFormat++;
        DMLOG_W("NVN call nvnVertexAttribStateSetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexAttribStateSetStreamIndex(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexAttribStateSetStreamIndex < 3) { nvn_trace_n_nvnVertexAttribStateSetStreamIndex++;
        DMLOG_W("NVN call nvnVertexAttribStateSetStreamIndex x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexAttribStateGetFormat(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexAttribStateGetFormat < 3) { nvn_trace_n_nvnVertexAttribStateGetFormat++;
        DMLOG_W("NVN call nvnVertexAttribStateGetFormat x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexAttribStateGetStreamIndex(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexAttribStateGetStreamIndex < 3) { nvn_trace_n_nvnVertexAttribStateGetStreamIndex++;
        DMLOG_W("NVN call nvnVertexAttribStateGetStreamIndex x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexStreamStateSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexStreamStateSetDefaults < 3) { nvn_trace_n_nvnVertexStreamStateSetDefaults++;
        DMLOG_W("NVN call nvnVertexStreamStateSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexStreamStateSetStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexStreamStateSetStride < 3) { nvn_trace_n_nvnVertexStreamStateSetStride++;
        DMLOG_W("NVN call nvnVertexStreamStateSetStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexStreamStateSetDivisor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexStreamStateSetDivisor < 3) { nvn_trace_n_nvnVertexStreamStateSetDivisor++;
        DMLOG_W("NVN call nvnVertexStreamStateSetDivisor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexStreamStateGetStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexStreamStateGetStride < 3) { nvn_trace_n_nvnVertexStreamStateGetStride++;
        DMLOG_W("NVN call nvnVertexStreamStateGetStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnVertexStreamStateGetDivisor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnVertexStreamStateGetDivisor < 3) { nvn_trace_n_nvnVertexStreamStateGetDivisor++;
        DMLOG_W("NVN call nvnVertexStreamStateGetDivisor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferInitialize < 3) { nvn_trace_n_nvnCommandBufferInitialize++;
        DMLOG_W("NVN call nvnCommandBufferInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferFinalize < 3) { nvn_trace_n_nvnCommandBufferFinalize++;
        DMLOG_W("NVN call nvnCommandBufferFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetDebugLabel < 3) { nvn_trace_n_nvnCommandBufferSetDebugLabel++;
        DMLOG_W("NVN call nvnCommandBufferSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetMemoryCallback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetMemoryCallback < 3) { nvn_trace_n_nvnCommandBufferSetMemoryCallback++;
        DMLOG_W("NVN call nvnCommandBufferSetMemoryCallback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetMemoryCallbackData(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetMemoryCallbackData < 3) { nvn_trace_n_nvnCommandBufferSetMemoryCallbackData++;
        DMLOG_W("NVN call nvnCommandBufferSetMemoryCallbackData x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetCommandMemoryCallbackEnabled(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetCommandMemoryCallbackEnabled < 3) { nvn_trace_n_nvnCommandBufferSetCommandMemoryCallbackEnabled++;
        DMLOG_W("NVN call nvnCommandBufferSetCommandMemoryCallbackEnabled x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferAddCommandMemory(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferAddCommandMemory < 3) { nvn_trace_n_nvnCommandBufferAddCommandMemory++;
        DMLOG_W("NVN call nvnCommandBufferAddCommandMemory x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferAddControlMemory(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferAddControlMemory < 3) { nvn_trace_n_nvnCommandBufferAddControlMemory++;
        DMLOG_W("NVN call nvnCommandBufferAddControlMemory x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetCommandMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetCommandMemorySize < 3) { nvn_trace_n_nvnCommandBufferGetCommandMemorySize++;
        DMLOG_W("NVN call nvnCommandBufferGetCommandMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetCommandMemoryUsed(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetCommandMemoryUsed < 3) { nvn_trace_n_nvnCommandBufferGetCommandMemoryUsed++;
        DMLOG_W("NVN call nvnCommandBufferGetCommandMemoryUsed x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetCommandMemoryFree(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetCommandMemoryFree < 3) { nvn_trace_n_nvnCommandBufferGetCommandMemoryFree++;
        DMLOG_W("NVN call nvnCommandBufferGetCommandMemoryFree x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetControlMemorySize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetControlMemorySize < 3) { nvn_trace_n_nvnCommandBufferGetControlMemorySize++;
        DMLOG_W("NVN call nvnCommandBufferGetControlMemorySize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetControlMemoryUsed(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetControlMemoryUsed < 3) { nvn_trace_n_nvnCommandBufferGetControlMemoryUsed++;
        DMLOG_W("NVN call nvnCommandBufferGetControlMemoryUsed x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetControlMemoryFree(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetControlMemoryFree < 3) { nvn_trace_n_nvnCommandBufferGetControlMemoryFree++;
        DMLOG_W("NVN call nvnCommandBufferGetControlMemoryFree x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBeginRecording(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBeginRecording < 3) { nvn_trace_n_nvnCommandBufferBeginRecording++;
        DMLOG_W("NVN call nvnCommandBufferBeginRecording x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferEndRecording(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferEndRecording < 3) { nvn_trace_n_nvnCommandBufferEndRecording++;
        DMLOG_W("NVN call nvnCommandBufferEndRecording x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferCallCommands(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferCallCommands < 3) { nvn_trace_n_nvnCommandBufferCallCommands++;
        DMLOG_W("NVN call nvnCommandBufferCallCommands x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferCopyCommands(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferCopyCommands < 3) { nvn_trace_n_nvnCommandBufferCopyCommands++;
        DMLOG_W("NVN call nvnCommandBufferCopyCommands x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindBlendState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindBlendState < 3) { nvn_trace_n_nvnCommandBufferBindBlendState++;
        DMLOG_W("NVN call nvnCommandBufferBindBlendState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindChannelMaskState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindChannelMaskState < 3) { nvn_trace_n_nvnCommandBufferBindChannelMaskState++;
        DMLOG_W("NVN call nvnCommandBufferBindChannelMaskState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindColorState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindColorState < 3) { nvn_trace_n_nvnCommandBufferBindColorState++;
        DMLOG_W("NVN call nvnCommandBufferBindColorState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindMultisampleState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindMultisampleState < 3) { nvn_trace_n_nvnCommandBufferBindMultisampleState++;
        DMLOG_W("NVN call nvnCommandBufferBindMultisampleState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindPolygonState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindPolygonState < 3) { nvn_trace_n_nvnCommandBufferBindPolygonState++;
        DMLOG_W("NVN call nvnCommandBufferBindPolygonState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindDepthStencilState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindDepthStencilState < 3) { nvn_trace_n_nvnCommandBufferBindDepthStencilState++;
        DMLOG_W("NVN call nvnCommandBufferBindDepthStencilState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindVertexAttribState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindVertexAttribState < 3) { nvn_trace_n_nvnCommandBufferBindVertexAttribState++;
        DMLOG_W("NVN call nvnCommandBufferBindVertexAttribState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindVertexStreamState(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindVertexStreamState < 3) { nvn_trace_n_nvnCommandBufferBindVertexStreamState++;
        DMLOG_W("NVN call nvnCommandBufferBindVertexStreamState x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindProgram(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindProgram < 3) { nvn_trace_n_nvnCommandBufferBindProgram++;
        DMLOG_W("NVN call nvnCommandBufferBindProgram x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindVertexBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindVertexBuffer < 3) { nvn_trace_n_nvnCommandBufferBindVertexBuffer++;
        DMLOG_W("NVN call nvnCommandBufferBindVertexBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindVertexBuffers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindVertexBuffers < 3) { nvn_trace_n_nvnCommandBufferBindVertexBuffers++;
        DMLOG_W("NVN call nvnCommandBufferBindVertexBuffers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindUniformBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindUniformBuffer < 3) { nvn_trace_n_nvnCommandBufferBindUniformBuffer++;
        DMLOG_W("NVN call nvnCommandBufferBindUniformBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindUniformBuffers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindUniformBuffers < 3) { nvn_trace_n_nvnCommandBufferBindUniformBuffers++;
        DMLOG_W("NVN call nvnCommandBufferBindUniformBuffers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindTransformFeedbackBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindTransformFeedbackBuffer < 3) { nvn_trace_n_nvnCommandBufferBindTransformFeedbackBuffer++;
        DMLOG_W("NVN call nvnCommandBufferBindTransformFeedbackBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindTransformFeedbackBuffers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindTransformFeedbackBuffers < 3) { nvn_trace_n_nvnCommandBufferBindTransformFeedbackBuffers++;
        DMLOG_W("NVN call nvnCommandBufferBindTransformFeedbackBuffers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindStorageBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindStorageBuffer < 3) { nvn_trace_n_nvnCommandBufferBindStorageBuffer++;
        DMLOG_W("NVN call nvnCommandBufferBindStorageBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindStorageBuffers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindStorageBuffers < 3) { nvn_trace_n_nvnCommandBufferBindStorageBuffers++;
        DMLOG_W("NVN call nvnCommandBufferBindStorageBuffers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindTexture < 3) { nvn_trace_n_nvnCommandBufferBindTexture++;
        DMLOG_W("NVN call nvnCommandBufferBindTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindTextures < 3) { nvn_trace_n_nvnCommandBufferBindTextures++;
        DMLOG_W("NVN call nvnCommandBufferBindTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindImage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindImage < 3) { nvn_trace_n_nvnCommandBufferBindImage++;
        DMLOG_W("NVN call nvnCommandBufferBindImage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindImages(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindImages < 3) { nvn_trace_n_nvnCommandBufferBindImages++;
        DMLOG_W("NVN call nvnCommandBufferBindImages x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetPatchSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetPatchSize < 3) { nvn_trace_n_nvnCommandBufferSetPatchSize++;
        DMLOG_W("NVN call nvnCommandBufferSetPatchSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetInnerTessellationLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetInnerTessellationLevels < 3) { nvn_trace_n_nvnCommandBufferSetInnerTessellationLevels++;
        DMLOG_W("NVN call nvnCommandBufferSetInnerTessellationLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetOuterTessellationLevels(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetOuterTessellationLevels < 3) { nvn_trace_n_nvnCommandBufferSetOuterTessellationLevels++;
        DMLOG_W("NVN call nvnCommandBufferSetOuterTessellationLevels x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetPrimitiveRestart(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetPrimitiveRestart < 3) { nvn_trace_n_nvnCommandBufferSetPrimitiveRestart++;
        DMLOG_W("NVN call nvnCommandBufferSetPrimitiveRestart x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBeginTransformFeedback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBeginTransformFeedback < 3) { nvn_trace_n_nvnCommandBufferBeginTransformFeedback++;
        DMLOG_W("NVN call nvnCommandBufferBeginTransformFeedback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferEndTransformFeedback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferEndTransformFeedback < 3) { nvn_trace_n_nvnCommandBufferEndTransformFeedback++;
        DMLOG_W("NVN call nvnCommandBufferEndTransformFeedback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferPauseTransformFeedback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferPauseTransformFeedback < 3) { nvn_trace_n_nvnCommandBufferPauseTransformFeedback++;
        DMLOG_W("NVN call nvnCommandBufferPauseTransformFeedback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferResumeTransformFeedback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferResumeTransformFeedback < 3) { nvn_trace_n_nvnCommandBufferResumeTransformFeedback++;
        DMLOG_W("NVN call nvnCommandBufferResumeTransformFeedback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawTransformFeedback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawTransformFeedback < 3) { nvn_trace_n_nvnCommandBufferDrawTransformFeedback++;
        DMLOG_W("NVN call nvnCommandBufferDrawTransformFeedback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawArrays(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawArrays < 3) { nvn_trace_n_nvnCommandBufferDrawArrays++;
        DMLOG_W("NVN call nvnCommandBufferDrawArrays x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawElements(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawElements < 3) { nvn_trace_n_nvnCommandBufferDrawElements++;
        DMLOG_W("NVN call nvnCommandBufferDrawElements x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawElementsBaseVertex(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawElementsBaseVertex < 3) { nvn_trace_n_nvnCommandBufferDrawElementsBaseVertex++;
        DMLOG_W("NVN call nvnCommandBufferDrawElementsBaseVertex x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawArraysInstanced(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawArraysInstanced < 3) { nvn_trace_n_nvnCommandBufferDrawArraysInstanced++;
        DMLOG_W("NVN call nvnCommandBufferDrawArraysInstanced x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawElementsInstanced(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawElementsInstanced < 3) { nvn_trace_n_nvnCommandBufferDrawElementsInstanced++;
        DMLOG_W("NVN call nvnCommandBufferDrawElementsInstanced x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawArraysIndirect(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawArraysIndirect < 3) { nvn_trace_n_nvnCommandBufferDrawArraysIndirect++;
        DMLOG_W("NVN call nvnCommandBufferDrawArraysIndirect x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawElementsIndirect(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawElementsIndirect < 3) { nvn_trace_n_nvnCommandBufferDrawElementsIndirect++;
        DMLOG_W("NVN call nvnCommandBufferDrawElementsIndirect x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferMultiDrawArraysIndirectCount(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferMultiDrawArraysIndirectCount < 3) { nvn_trace_n_nvnCommandBufferMultiDrawArraysIndirectCount++;
        DMLOG_W("NVN call nvnCommandBufferMultiDrawArraysIndirectCount x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferMultiDrawElementsIndirectCount(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferMultiDrawElementsIndirectCount < 3) { nvn_trace_n_nvnCommandBufferMultiDrawElementsIndirectCount++;
        DMLOG_W("NVN call nvnCommandBufferMultiDrawElementsIndirectCount x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearColor < 3) { nvn_trace_n_nvnCommandBufferClearColor++;
        DMLOG_W("NVN call nvnCommandBufferClearColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearColori(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearColori < 3) { nvn_trace_n_nvnCommandBufferClearColori++;
        DMLOG_W("NVN call nvnCommandBufferClearColori x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearColorui(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearColorui < 3) { nvn_trace_n_nvnCommandBufferClearColorui++;
        DMLOG_W("NVN call nvnCommandBufferClearColorui x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearDepthStencil(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearDepthStencil < 3) { nvn_trace_n_nvnCommandBufferClearDepthStencil++;
        DMLOG_W("NVN call nvnCommandBufferClearDepthStencil x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDispatchCompute(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDispatchCompute < 3) { nvn_trace_n_nvnCommandBufferDispatchCompute++;
        DMLOG_W("NVN call nvnCommandBufferDispatchCompute x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDispatchComputeIndirect(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDispatchComputeIndirect < 3) { nvn_trace_n_nvnCommandBufferDispatchComputeIndirect++;
        DMLOG_W("NVN call nvnCommandBufferDispatchComputeIndirect x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetViewport(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetViewport < 3) { nvn_trace_n_nvnCommandBufferSetViewport++;
        DMLOG_W("NVN call nvnCommandBufferSetViewport x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetViewports(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetViewports < 3) { nvn_trace_n_nvnCommandBufferSetViewports++;
        DMLOG_W("NVN call nvnCommandBufferSetViewports x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetViewportSwizzles(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetViewportSwizzles < 3) { nvn_trace_n_nvnCommandBufferSetViewportSwizzles++;
        DMLOG_W("NVN call nvnCommandBufferSetViewportSwizzles x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetScissor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetScissor < 3) { nvn_trace_n_nvnCommandBufferSetScissor++;
        DMLOG_W("NVN call nvnCommandBufferSetScissor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetScissors(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetScissors < 3) { nvn_trace_n_nvnCommandBufferSetScissors++;
        DMLOG_W("NVN call nvnCommandBufferSetScissors x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetDepthRange(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetDepthRange < 3) { nvn_trace_n_nvnCommandBufferSetDepthRange++;
        DMLOG_W("NVN call nvnCommandBufferSetDepthRange x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetDepthBounds(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetDepthBounds < 3) { nvn_trace_n_nvnCommandBufferSetDepthBounds++;
        DMLOG_W("NVN call nvnCommandBufferSetDepthBounds x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetDepthRanges(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetDepthRanges < 3) { nvn_trace_n_nvnCommandBufferSetDepthRanges++;
        DMLOG_W("NVN call nvnCommandBufferSetDepthRanges x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetTiledCacheAction(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetTiledCacheAction < 3) { nvn_trace_n_nvnCommandBufferSetTiledCacheAction++;
        DMLOG_W("NVN call nvnCommandBufferSetTiledCacheAction x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetTiledCacheTileSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetTiledCacheTileSize < 3) { nvn_trace_n_nvnCommandBufferSetTiledCacheTileSize++;
        DMLOG_W("NVN call nvnCommandBufferSetTiledCacheTileSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindSeparateTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindSeparateTexture < 3) { nvn_trace_n_nvnCommandBufferBindSeparateTexture++;
        DMLOG_W("NVN call nvnCommandBufferBindSeparateTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindSeparateSampler(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindSeparateSampler < 3) { nvn_trace_n_nvnCommandBufferBindSeparateSampler++;
        DMLOG_W("NVN call nvnCommandBufferBindSeparateSampler x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindSeparateTextures(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindSeparateTextures < 3) { nvn_trace_n_nvnCommandBufferBindSeparateTextures++;
        DMLOG_W("NVN call nvnCommandBufferBindSeparateTextures x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindSeparateSamplers(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindSeparateSamplers < 3) { nvn_trace_n_nvnCommandBufferBindSeparateSamplers++;
        DMLOG_W("NVN call nvnCommandBufferBindSeparateSamplers x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetStencilValueMask(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetStencilValueMask < 3) { nvn_trace_n_nvnCommandBufferSetStencilValueMask++;
        DMLOG_W("NVN call nvnCommandBufferSetStencilValueMask x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetStencilMask(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetStencilMask < 3) { nvn_trace_n_nvnCommandBufferSetStencilMask++;
        DMLOG_W("NVN call nvnCommandBufferSetStencilMask x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetStencilRef(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetStencilRef < 3) { nvn_trace_n_nvnCommandBufferSetStencilRef++;
        DMLOG_W("NVN call nvnCommandBufferSetStencilRef x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetBlendColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetBlendColor < 3) { nvn_trace_n_nvnCommandBufferSetBlendColor++;
        DMLOG_W("NVN call nvnCommandBufferSetBlendColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetPointSize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetPointSize < 3) { nvn_trace_n_nvnCommandBufferSetPointSize++;
        DMLOG_W("NVN call nvnCommandBufferSetPointSize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetLineWidth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetLineWidth < 3) { nvn_trace_n_nvnCommandBufferSetLineWidth++;
        DMLOG_W("NVN call nvnCommandBufferSetLineWidth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetPolygonOffsetClamp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetPolygonOffsetClamp < 3) { nvn_trace_n_nvnCommandBufferSetPolygonOffsetClamp++;
        DMLOG_W("NVN call nvnCommandBufferSetPolygonOffsetClamp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetAlphaRef(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetAlphaRef < 3) { nvn_trace_n_nvnCommandBufferSetAlphaRef++;
        DMLOG_W("NVN call nvnCommandBufferSetAlphaRef x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetSampleMask(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetSampleMask < 3) { nvn_trace_n_nvnCommandBufferSetSampleMask++;
        DMLOG_W("NVN call nvnCommandBufferSetSampleMask x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetRasterizerDiscard(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetRasterizerDiscard < 3) { nvn_trace_n_nvnCommandBufferSetRasterizerDiscard++;
        DMLOG_W("NVN call nvnCommandBufferSetRasterizerDiscard x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetDepthClamp(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetDepthClamp < 3) { nvn_trace_n_nvnCommandBufferSetDepthClamp++;
        DMLOG_W("NVN call nvnCommandBufferSetDepthClamp x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetConservativeRasterEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetConservativeRasterEnable < 3) { nvn_trace_n_nvnCommandBufferSetConservativeRasterEnable++;
        DMLOG_W("NVN call nvnCommandBufferSetConservativeRasterEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetConservativeRasterDilate(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetConservativeRasterDilate < 3) { nvn_trace_n_nvnCommandBufferSetConservativeRasterDilate++;
        DMLOG_W("NVN call nvnCommandBufferSetConservativeRasterDilate x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetSubpixelPrecisionBias(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetSubpixelPrecisionBias < 3) { nvn_trace_n_nvnCommandBufferSetSubpixelPrecisionBias++;
        DMLOG_W("NVN call nvnCommandBufferSetSubpixelPrecisionBias x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferCopyBufferToTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferCopyBufferToTexture < 3) { nvn_trace_n_nvnCommandBufferCopyBufferToTexture++;
        DMLOG_W("NVN call nvnCommandBufferCopyBufferToTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferCopyTextureToBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferCopyTextureToBuffer < 3) { nvn_trace_n_nvnCommandBufferCopyTextureToBuffer++;
        DMLOG_W("NVN call nvnCommandBufferCopyTextureToBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferCopyTextureToTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferCopyTextureToTexture < 3) { nvn_trace_n_nvnCommandBufferCopyTextureToTexture++;
        DMLOG_W("NVN call nvnCommandBufferCopyTextureToTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferCopyBufferToBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferCopyBufferToBuffer < 3) { nvn_trace_n_nvnCommandBufferCopyBufferToBuffer++;
        DMLOG_W("NVN call nvnCommandBufferCopyBufferToBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearBuffer < 3) { nvn_trace_n_nvnCommandBufferClearBuffer++;
        DMLOG_W("NVN call nvnCommandBufferClearBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearTexture < 3) { nvn_trace_n_nvnCommandBufferClearTexture++;
        DMLOG_W("NVN call nvnCommandBufferClearTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearTexturei(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearTexturei < 3) { nvn_trace_n_nvnCommandBufferClearTexturei++;
        DMLOG_W("NVN call nvnCommandBufferClearTexturei x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferClearTextureui(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferClearTextureui < 3) { nvn_trace_n_nvnCommandBufferClearTextureui++;
        DMLOG_W("NVN call nvnCommandBufferClearTextureui x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferUpdateUniformBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferUpdateUniformBuffer < 3) { nvn_trace_n_nvnCommandBufferUpdateUniformBuffer++;
        DMLOG_W("NVN call nvnCommandBufferUpdateUniformBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferReportCounter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferReportCounter < 3) { nvn_trace_n_nvnCommandBufferReportCounter++;
        DMLOG_W("NVN call nvnCommandBufferReportCounter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferResetCounter(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferResetCounter < 3) { nvn_trace_n_nvnCommandBufferResetCounter++;
        DMLOG_W("NVN call nvnCommandBufferResetCounter x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferReportValue(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferReportValue < 3) { nvn_trace_n_nvnCommandBufferReportValue++;
        DMLOG_W("NVN call nvnCommandBufferReportValue x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetRenderEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetRenderEnable < 3) { nvn_trace_n_nvnCommandBufferSetRenderEnable++;
        DMLOG_W("NVN call nvnCommandBufferSetRenderEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetRenderEnableConditional(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetRenderEnableConditional < 3) { nvn_trace_n_nvnCommandBufferSetRenderEnableConditional++;
        DMLOG_W("NVN call nvnCommandBufferSetRenderEnableConditional x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetRenderTargets(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetRenderTargets < 3) { nvn_trace_n_nvnCommandBufferSetRenderTargets++;
        DMLOG_W("NVN call nvnCommandBufferSetRenderTargets x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDiscardColor(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDiscardColor < 3) { nvn_trace_n_nvnCommandBufferDiscardColor++;
        DMLOG_W("NVN call nvnCommandBufferDiscardColor x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDiscardDepthStencil(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDiscardDepthStencil < 3) { nvn_trace_n_nvnCommandBufferDiscardDepthStencil++;
        DMLOG_W("NVN call nvnCommandBufferDiscardDepthStencil x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDownsample(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDownsample < 3) { nvn_trace_n_nvnCommandBufferDownsample++;
        DMLOG_W("NVN call nvnCommandBufferDownsample x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferTiledDownsample(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferTiledDownsample < 3) { nvn_trace_n_nvnCommandBufferTiledDownsample++;
        DMLOG_W("NVN call nvnCommandBufferTiledDownsample x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDownsampleTextureView(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDownsampleTextureView < 3) { nvn_trace_n_nvnCommandBufferDownsampleTextureView++;
        DMLOG_W("NVN call nvnCommandBufferDownsampleTextureView x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferTiledDownsampleTextureView(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferTiledDownsampleTextureView < 3) { nvn_trace_n_nvnCommandBufferTiledDownsampleTextureView++;
        DMLOG_W("NVN call nvnCommandBufferTiledDownsampleTextureView x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBarrier(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBarrier < 3) { nvn_trace_n_nvnCommandBufferBarrier++;
        DMLOG_W("NVN call nvnCommandBufferBarrier x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferWaitSync(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferWaitSync < 3) { nvn_trace_n_nvnCommandBufferWaitSync++;
        DMLOG_W("NVN call nvnCommandBufferWaitSync x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferFenceSync(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferFenceSync < 3) { nvn_trace_n_nvnCommandBufferFenceSync++;
        DMLOG_W("NVN call nvnCommandBufferFenceSync x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetTexturePool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetTexturePool < 3) { nvn_trace_n_nvnCommandBufferSetTexturePool++;
        DMLOG_W("NVN call nvnCommandBufferSetTexturePool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetSamplerPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetSamplerPool < 3) { nvn_trace_n_nvnCommandBufferSetSamplerPool++;
        DMLOG_W("NVN call nvnCommandBufferSetSamplerPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetShaderScratchMemory(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetShaderScratchMemory < 3) { nvn_trace_n_nvnCommandBufferSetShaderScratchMemory++;
        DMLOG_W("NVN call nvnCommandBufferSetShaderScratchMemory x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSaveZCullData(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSaveZCullData < 3) { nvn_trace_n_nvnCommandBufferSaveZCullData++;
        DMLOG_W("NVN call nvnCommandBufferSaveZCullData x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferRestoreZCullData(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferRestoreZCullData < 3) { nvn_trace_n_nvnCommandBufferRestoreZCullData++;
        DMLOG_W("NVN call nvnCommandBufferRestoreZCullData x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetCopyRowStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetCopyRowStride < 3) { nvn_trace_n_nvnCommandBufferSetCopyRowStride++;
        DMLOG_W("NVN call nvnCommandBufferSetCopyRowStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetCopyImageStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetCopyImageStride < 3) { nvn_trace_n_nvnCommandBufferSetCopyImageStride++;
        DMLOG_W("NVN call nvnCommandBufferSetCopyImageStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetCopyRowStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetCopyRowStride < 3) { nvn_trace_n_nvnCommandBufferGetCopyRowStride++;
        DMLOG_W("NVN call nvnCommandBufferGetCopyRowStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetCopyImageStride(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetCopyImageStride < 3) { nvn_trace_n_nvnCommandBufferGetCopyImageStride++;
        DMLOG_W("NVN call nvnCommandBufferGetCopyImageStride x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferDrawTexture(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferDrawTexture < 3) { nvn_trace_n_nvnCommandBufferDrawTexture++;
        DMLOG_W("NVN call nvnCommandBufferDrawTexture x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnProgramSetSubroutineLinkage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnProgramSetSubroutineLinkage < 3) { nvn_trace_n_nvnProgramSetSubroutineLinkage++;
        DMLOG_W("NVN call nvnProgramSetSubroutineLinkage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetProgramSubroutines(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetProgramSubroutines < 3) { nvn_trace_n_nvnCommandBufferSetProgramSubroutines++;
        DMLOG_W("NVN call nvnCommandBufferSetProgramSubroutines x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferBindCoverageModulationTable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferBindCoverageModulationTable < 3) { nvn_trace_n_nvnCommandBufferBindCoverageModulationTable++;
        DMLOG_W("NVN call nvnCommandBufferBindCoverageModulationTable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferResolveDepthBuffer(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferResolveDepthBuffer < 3) { nvn_trace_n_nvnCommandBufferResolveDepthBuffer++;
        DMLOG_W("NVN call nvnCommandBufferResolveDepthBuffer x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetColorReductionEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetColorReductionEnable < 3) { nvn_trace_n_nvnCommandBufferSetColorReductionEnable++;
        DMLOG_W("NVN call nvnCommandBufferSetColorReductionEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetColorReductionThresholds(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetColorReductionThresholds < 3) { nvn_trace_n_nvnCommandBufferSetColorReductionThresholds++;
        DMLOG_W("NVN call nvnCommandBufferSetColorReductionThresholds x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferPushDebugGroupStatic(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferPushDebugGroupStatic < 3) { nvn_trace_n_nvnCommandBufferPushDebugGroupStatic++;
        DMLOG_W("NVN call nvnCommandBufferPushDebugGroupStatic x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferPushDebugGroupDynamic(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferPushDebugGroupDynamic < 3) { nvn_trace_n_nvnCommandBufferPushDebugGroupDynamic++;
        DMLOG_W("NVN call nvnCommandBufferPushDebugGroupDynamic x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferPushDebugGroup(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferPushDebugGroup < 3) { nvn_trace_n_nvnCommandBufferPushDebugGroup++;
        DMLOG_W("NVN call nvnCommandBufferPushDebugGroup x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferPopDebugGroup(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferPopDebugGroup < 3) { nvn_trace_n_nvnCommandBufferPopDebugGroup++;
        DMLOG_W("NVN call nvnCommandBufferPopDebugGroup x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferPopDebugGroupId(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferPopDebugGroupId < 3) { nvn_trace_n_nvnCommandBufferPopDebugGroupId++;
        DMLOG_W("NVN call nvnCommandBufferPopDebugGroupId x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferInsertDebugMarkerStatic(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferInsertDebugMarkerStatic < 3) { nvn_trace_n_nvnCommandBufferInsertDebugMarkerStatic++;
        DMLOG_W("NVN call nvnCommandBufferInsertDebugMarkerStatic x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferInsertDebugMarkerDynamic(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferInsertDebugMarkerDynamic < 3) { nvn_trace_n_nvnCommandBufferInsertDebugMarkerDynamic++;
        DMLOG_W("NVN call nvnCommandBufferInsertDebugMarkerDynamic x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferInsertDebugMarker(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferInsertDebugMarker < 3) { nvn_trace_n_nvnCommandBufferInsertDebugMarker++;
        DMLOG_W("NVN call nvnCommandBufferInsertDebugMarker x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetMemoryCallback(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetMemoryCallback < 3) { nvn_trace_n_nvnCommandBufferGetMemoryCallback++;
        DMLOG_W("NVN call nvnCommandBufferGetMemoryCallback x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferGetMemoryCallbackData(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferGetMemoryCallbackData < 3) { nvn_trace_n_nvnCommandBufferGetMemoryCallbackData++;
        DMLOG_W("NVN call nvnCommandBufferGetMemoryCallbackData x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferIsRecording(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferIsRecording < 3) { nvn_trace_n_nvnCommandBufferIsRecording++;
        DMLOG_W("NVN call nvnCommandBufferIsRecording x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSyncInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSyncInitialize < 3) { nvn_trace_n_nvnSyncInitialize++;
        DMLOG_W("NVN call nvnSyncInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSyncFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSyncFinalize < 3) { nvn_trace_n_nvnSyncFinalize++;
        DMLOG_W("NVN call nvnSyncFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSyncSetDebugLabel(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSyncSetDebugLabel < 3) { nvn_trace_n_nvnSyncSetDebugLabel++;
        DMLOG_W("NVN call nvnSyncSetDebugLabel x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueFenceSync(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueFenceSync < 3) { nvn_trace_n_nvnQueueFenceSync++;
        DMLOG_W("NVN call nvnQueueFenceSync x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSyncWait(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSyncWait < 3) { nvn_trace_n_nvnSyncWait++;
        DMLOG_W("NVN call nvnSyncWait x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnQueueWaitSync(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnQueueWaitSync < 3) { nvn_trace_n_nvnQueueWaitSync++;
        DMLOG_W("NVN call nvnQueueWaitSync x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSyncInitializeFromFencedGLSync(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSyncInitializeFromFencedGLSync < 3) { nvn_trace_n_nvnSyncInitializeFromFencedGLSync++;
        DMLOG_W("NVN call nvnSyncInitializeFromFencedGLSync x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnSyncCreateGLSync(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnSyncCreateGLSync < 3) { nvn_trace_n_nvnSyncCreateGLSync++;
        DMLOG_W("NVN call nvnSyncCreateGLSync x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventBuilderSetDefaults(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventBuilderSetDefaults < 3) { nvn_trace_n_nvnEventBuilderSetDefaults++;
        DMLOG_W("NVN call nvnEventBuilderSetDefaults x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventBuilderSetStorage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventBuilderSetStorage < 3) { nvn_trace_n_nvnEventBuilderSetStorage++;
        DMLOG_W("NVN call nvnEventBuilderSetStorage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventBuilderGetStorage(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventBuilderGetStorage < 3) { nvn_trace_n_nvnEventBuilderGetStorage++;
        DMLOG_W("NVN call nvnEventBuilderGetStorage x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventBuilderGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventBuilderGetMemoryPool < 3) { nvn_trace_n_nvnEventBuilderGetMemoryPool++;
        DMLOG_W("NVN call nvnEventBuilderGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventBuilderGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventBuilderGetMemoryOffset < 3) { nvn_trace_n_nvnEventBuilderGetMemoryOffset++;
        DMLOG_W("NVN call nvnEventBuilderGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventInitialize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventInitialize < 3) { nvn_trace_n_nvnEventInitialize++;
        DMLOG_W("NVN call nvnEventInitialize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventFinalize(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventFinalize < 3) { nvn_trace_n_nvnEventFinalize++;
        DMLOG_W("NVN call nvnEventFinalize x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventGetValue(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventGetValue < 3) { nvn_trace_n_nvnEventGetValue++;
        DMLOG_W("NVN call nvnEventGetValue x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventSignal(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventSignal < 3) { nvn_trace_n_nvnEventSignal++;
        DMLOG_W("NVN call nvnEventSignal x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventGetMemoryPool(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventGetMemoryPool < 3) { nvn_trace_n_nvnEventGetMemoryPool++;
        DMLOG_W("NVN call nvnEventGetMemoryPool x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnEventGetMemoryOffset(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnEventGetMemoryOffset < 3) { nvn_trace_n_nvnEventGetMemoryOffset++;
        DMLOG_W("NVN call nvnEventGetMemoryOffset x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferWaitEvent(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferWaitEvent < 3) { nvn_trace_n_nvnCommandBufferWaitEvent++;
        DMLOG_W("NVN call nvnCommandBufferWaitEvent x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSignalEvent(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSignalEvent < 3) { nvn_trace_n_nvnCommandBufferSignalEvent++;
        DMLOG_W("NVN call nvnCommandBufferSignalEvent x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferSetStencilCullCriteria(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferSetStencilCullCriteria < 3) { nvn_trace_n_nvnCommandBufferSetStencilCullCriteria++;
        DMLOG_W("NVN call nvnCommandBufferSetStencilCullCriteria x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferInsertCheckpointStatic(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferInsertCheckpointStatic < 3) { nvn_trace_n_nvnCommandBufferInsertCheckpointStatic++;
        DMLOG_W("NVN call nvnCommandBufferInsertCheckpointStatic x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnCommandBufferInsertCheckpointDynamic(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnCommandBufferInsertCheckpointDynamic < 3) { nvn_trace_n_nvnCommandBufferInsertCheckpointDynamic++;
        DMLOG_W("NVN call nvnCommandBufferInsertCheckpointDynamic x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetAutomaticCheckpointsEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetAutomaticCheckpointsEnable < 3) { nvn_trace_n_nvnDeviceSetAutomaticCheckpointsEnable++;
        DMLOG_W("NVN call nvnDeviceSetAutomaticCheckpointsEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetAutomaticCheckpointsSamplingInterval(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetAutomaticCheckpointsSamplingInterval < 3) { nvn_trace_n_nvnDeviceSetAutomaticCheckpointsSamplingInterval++;
        DMLOG_W("NVN call nvnDeviceSetAutomaticCheckpointsSamplingInterval x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetAutomaticCheckpointsByDebugGroup(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetAutomaticCheckpointsByDebugGroup < 3) { nvn_trace_n_nvnDeviceSetAutomaticCheckpointsByDebugGroup++;
        DMLOG_W("NVN call nvnDeviceSetAutomaticCheckpointsByDebugGroup x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetCheckpointCallStacksEnable(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetCheckpointCallStacksEnable < 3) { nvn_trace_n_nvnDeviceSetCheckpointCallStacksEnable++;
        DMLOG_W("NVN call nvnDeviceSetCheckpointCallStacksEnable x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static uint64_t nvn_trace_nvnDeviceSetCheckpointMaxCallStacksDepth(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    if (nvn_trace_n_nvnDeviceSetCheckpointMaxCallStacksDepth < 3) { nvn_trace_n_nvnDeviceSetCheckpointMaxCallStacksDepth++;
        DMLOG_W("NVN call nvnDeviceSetCheckpointMaxCallStacksDepth x0=%{public}llx x1=%{public}llx x2=%{public}llx x3=%{public}llx",
                (unsigned long long)a, (unsigned long long)b, (unsigned long long)c, (unsigned long long)d); }
    return 0;
}
static void dm_nvn_register_traces(void) {
    dmNvnRegister("nvnDeviceBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnDeviceBuilderSetDefaults);
    dmNvnRegister("nvnDeviceBuilderSetFlags", (nvn_func_t)nvn_trace_nvnDeviceBuilderSetFlags);
    dmNvnRegister("nvnDeviceBuilderGetFlags", (nvn_func_t)nvn_trace_nvnDeviceBuilderGetFlags);
    dmNvnRegister("nvnDeviceInitialize", (nvn_func_t)nvn_trace_nvnDeviceInitialize);
    dmNvnRegister("nvnDeviceFinalize", (nvn_func_t)nvn_trace_nvnDeviceFinalize);
    dmNvnRegister("nvnDeviceSetDebugLabel", (nvn_func_t)nvn_trace_nvnDeviceSetDebugLabel);
    dmNvnRegister("nvnDeviceGetProcAddress", (nvn_func_t)nvn_trace_nvnDeviceGetProcAddress);
    dmNvnRegister("nvnDeviceGetInteger", (nvn_func_t)nvn_trace_nvnDeviceGetInteger);
    dmNvnRegister("nvnDeviceGetCurrentTimestampInNanoseconds", (nvn_func_t)nvn_trace_nvnDeviceGetCurrentTimestampInNanoseconds);
    dmNvnRegister("nvnDeviceSetIntermediateShaderCache", (nvn_func_t)nvn_trace_nvnDeviceSetIntermediateShaderCache);
    dmNvnRegister("nvnDeviceGetTextureHandle", (nvn_func_t)nvn_trace_nvnDeviceGetTextureHandle);
    dmNvnRegister("nvnDeviceGetTexelFetchHandle", (nvn_func_t)nvn_trace_nvnDeviceGetTexelFetchHandle);
    dmNvnRegister("nvnDeviceGetImageHandle", (nvn_func_t)nvn_trace_nvnDeviceGetImageHandle);
    dmNvnRegister("nvnDeviceInstallDebugCallback", (nvn_func_t)nvn_trace_nvnDeviceInstallDebugCallback);
    dmNvnRegister("nvnDeviceGenerateDebugDomainId", (nvn_func_t)nvn_trace_nvnDeviceGenerateDebugDomainId);
    dmNvnRegister("nvnDeviceSetWindowOriginMode", (nvn_func_t)nvn_trace_nvnDeviceSetWindowOriginMode);
    dmNvnRegister("nvnDeviceSetDepthMode", (nvn_func_t)nvn_trace_nvnDeviceSetDepthMode);
    dmNvnRegister("nvnDeviceRegisterFastClearColor", (nvn_func_t)nvn_trace_nvnDeviceRegisterFastClearColor);
    dmNvnRegister("nvnDeviceRegisterFastClearColori", (nvn_func_t)nvn_trace_nvnDeviceRegisterFastClearColori);
    dmNvnRegister("nvnDeviceRegisterFastClearColorui", (nvn_func_t)nvn_trace_nvnDeviceRegisterFastClearColorui);
    dmNvnRegister("nvnDeviceRegisterFastClearDepth", (nvn_func_t)nvn_trace_nvnDeviceRegisterFastClearDepth);
    dmNvnRegister("nvnDeviceGetWindowOriginMode", (nvn_func_t)nvn_trace_nvnDeviceGetWindowOriginMode);
    dmNvnRegister("nvnDeviceGetDepthMode", (nvn_func_t)nvn_trace_nvnDeviceGetDepthMode);
    dmNvnRegister("nvnDeviceGetTimestampInNanoseconds", (nvn_func_t)nvn_trace_nvnDeviceGetTimestampInNanoseconds);
    dmNvnRegister("nvnDeviceApplyDeferredFinalizes", (nvn_func_t)nvn_trace_nvnDeviceApplyDeferredFinalizes);
    dmNvnRegister("nvnDeviceFinalizeCommandHandle", (nvn_func_t)nvn_trace_nvnDeviceFinalizeCommandHandle);
    dmNvnRegister("nvnDeviceWalkDebugDatabase", (nvn_func_t)nvn_trace_nvnDeviceWalkDebugDatabase);
    dmNvnRegister("nvnDeviceGetSeparateTextureHandle", (nvn_func_t)nvn_trace_nvnDeviceGetSeparateTextureHandle);
    dmNvnRegister("nvnDeviceGetSeparateSamplerHandle", (nvn_func_t)nvn_trace_nvnDeviceGetSeparateSamplerHandle);
    dmNvnRegister("nvnDeviceIsExternalDebuggerAttached", (nvn_func_t)nvn_trace_nvnDeviceIsExternalDebuggerAttached);
    dmNvnRegister("nvnQueueGetError", (nvn_func_t)nvn_trace_nvnQueueGetError);
    dmNvnRegister("nvnQueueGetTotalCommandMemoryUsed", (nvn_func_t)nvn_trace_nvnQueueGetTotalCommandMemoryUsed);
    dmNvnRegister("nvnQueueGetTotalControlMemoryUsed", (nvn_func_t)nvn_trace_nvnQueueGetTotalControlMemoryUsed);
    dmNvnRegister("nvnQueueGetTotalComputeMemoryUsed", (nvn_func_t)nvn_trace_nvnQueueGetTotalComputeMemoryUsed);
    dmNvnRegister("nvnQueueResetMemoryUsageCounts", (nvn_func_t)nvn_trace_nvnQueueResetMemoryUsageCounts);
    dmNvnRegister("nvnQueueBuilderSetDevice", (nvn_func_t)nvn_trace_nvnQueueBuilderSetDevice);
    dmNvnRegister("nvnQueueBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnQueueBuilderSetDefaults);
    dmNvnRegister("nvnQueueBuilderSetFlags", (nvn_func_t)nvn_trace_nvnQueueBuilderSetFlags);
    dmNvnRegister("nvnQueueBuilderSetCommandMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderSetCommandMemorySize);
    dmNvnRegister("nvnQueueBuilderSetComputeMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderSetComputeMemorySize);
    dmNvnRegister("nvnQueueBuilderSetControlMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderSetControlMemorySize);
    dmNvnRegister("nvnQueueBuilderGetQueueMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderGetQueueMemorySize);
    dmNvnRegister("nvnQueueBuilderSetQueueMemory", (nvn_func_t)nvn_trace_nvnQueueBuilderSetQueueMemory);
    dmNvnRegister("nvnQueueBuilderSetCommandFlushThreshold", (nvn_func_t)nvn_trace_nvnQueueBuilderSetCommandFlushThreshold);
    dmNvnRegister("nvnQueueBuilderSetQueuePriority", (nvn_func_t)nvn_trace_nvnQueueBuilderSetQueuePriority);
    dmNvnRegister("nvnQueueBuilderGetQueuePriority", (nvn_func_t)nvn_trace_nvnQueueBuilderGetQueuePriority);
    dmNvnRegister("nvnQueueBuilderGetDevice", (nvn_func_t)nvn_trace_nvnQueueBuilderGetDevice);
    dmNvnRegister("nvnQueueBuilderGetFlags", (nvn_func_t)nvn_trace_nvnQueueBuilderGetFlags);
    dmNvnRegister("nvnQueueBuilderGetCommandMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderGetCommandMemorySize);
    dmNvnRegister("nvnQueueBuilderGetComputeMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderGetComputeMemorySize);
    dmNvnRegister("nvnQueueBuilderGetControlMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderGetControlMemorySize);
    dmNvnRegister("nvnQueueBuilderGetCommandFlushThreshold", (nvn_func_t)nvn_trace_nvnQueueBuilderGetCommandFlushThreshold);
    dmNvnRegister("nvnQueueBuilderGetMemorySize", (nvn_func_t)nvn_trace_nvnQueueBuilderGetMemorySize);
    dmNvnRegister("nvnQueueBuilderGetMemory", (nvn_func_t)nvn_trace_nvnQueueBuilderGetMemory);
    dmNvnRegister("nvnQueueInitialize", (nvn_func_t)nvn_trace_nvnQueueInitialize);
    dmNvnRegister("nvnQueueFinalize", (nvn_func_t)nvn_trace_nvnQueueFinalize);
    dmNvnRegister("nvnQueueSetDebugLabel", (nvn_func_t)nvn_trace_nvnQueueSetDebugLabel);
    dmNvnRegister("nvnQueueSubmitCommands", (nvn_func_t)nvn_trace_nvnQueueSubmitCommands);
    dmNvnRegister("nvnQueueFlush", (nvn_func_t)nvn_trace_nvnQueueFlush);
    dmNvnRegister("nvnQueueFinish", (nvn_func_t)nvn_trace_nvnQueueFinish);
    dmNvnRegister("nvnQueuePresentTexture", (nvn_func_t)nvn_trace_nvnQueuePresentTexture);
    dmNvnRegister("nvnQueueAcquireTexture", (nvn_func_t)nvn_trace_nvnQueueAcquireTexture);
    dmNvnRegister("nvnWindowBuilderSetDevice", (nvn_func_t)nvn_trace_nvnWindowBuilderSetDevice);
    dmNvnRegister("nvnWindowBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnWindowBuilderSetDefaults);
    dmNvnRegister("nvnWindowBuilderSetNativeWindow", (nvn_func_t)nvn_trace_nvnWindowBuilderSetNativeWindow);
    dmNvnRegister("nvnWindowBuilderSetTextures", (nvn_func_t)nvn_trace_nvnWindowBuilderSetTextures);
    dmNvnRegister("nvnWindowBuilderSetPresentInterval", (nvn_func_t)nvn_trace_nvnWindowBuilderSetPresentInterval);
    dmNvnRegister("nvnWindowBuilderSetNumActiveTextures", (nvn_func_t)nvn_trace_nvnWindowBuilderSetNumActiveTextures);
    dmNvnRegister("nvnWindowBuilderGetDevice", (nvn_func_t)nvn_trace_nvnWindowBuilderGetDevice);
    dmNvnRegister("nvnWindowBuilderGetNumTextures", (nvn_func_t)nvn_trace_nvnWindowBuilderGetNumTextures);
    dmNvnRegister("nvnWindowBuilderGetTexture", (nvn_func_t)nvn_trace_nvnWindowBuilderGetTexture);
    dmNvnRegister("nvnWindowBuilderGetNativeWindow", (nvn_func_t)nvn_trace_nvnWindowBuilderGetNativeWindow);
    dmNvnRegister("nvnWindowBuilderGetPresentInterval", (nvn_func_t)nvn_trace_nvnWindowBuilderGetPresentInterval);
    dmNvnRegister("nvnWindowBuilderGetNumActiveTextures", (nvn_func_t)nvn_trace_nvnWindowBuilderGetNumActiveTextures);
    dmNvnRegister("nvnWindowInitialize", (nvn_func_t)nvn_trace_nvnWindowInitialize);
    dmNvnRegister("nvnWindowFinalize", (nvn_func_t)nvn_trace_nvnWindowFinalize);
    dmNvnRegister("nvnWindowSetDebugLabel", (nvn_func_t)nvn_trace_nvnWindowSetDebugLabel);
    dmNvnRegister("nvnWindowAcquireTexture", (nvn_func_t)nvn_trace_nvnWindowAcquireTexture);
    dmNvnRegister("nvnWindowGetNativeWindow", (nvn_func_t)nvn_trace_nvnWindowGetNativeWindow);
    dmNvnRegister("nvnWindowGetPresentInterval", (nvn_func_t)nvn_trace_nvnWindowGetPresentInterval);
    dmNvnRegister("nvnWindowSetPresentInterval", (nvn_func_t)nvn_trace_nvnWindowSetPresentInterval);
    dmNvnRegister("nvnWindowSetCrop", (nvn_func_t)nvn_trace_nvnWindowSetCrop);
    dmNvnRegister("nvnWindowGetCrop", (nvn_func_t)nvn_trace_nvnWindowGetCrop);
    dmNvnRegister("nvnWindowSetNumActiveTextures", (nvn_func_t)nvn_trace_nvnWindowSetNumActiveTextures);
    dmNvnRegister("nvnWindowGetNumActiveTextures", (nvn_func_t)nvn_trace_nvnWindowGetNumActiveTextures);
    dmNvnRegister("nvnWindowGetNumTextures", (nvn_func_t)nvn_trace_nvnWindowGetNumTextures);
    dmNvnRegister("nvnProgramInitialize", (nvn_func_t)nvn_trace_nvnProgramInitialize);
    dmNvnRegister("nvnProgramFinalize", (nvn_func_t)nvn_trace_nvnProgramFinalize);
    dmNvnRegister("nvnProgramSetDebugLabel", (nvn_func_t)nvn_trace_nvnProgramSetDebugLabel);
    dmNvnRegister("nvnProgramSetShaders", (nvn_func_t)nvn_trace_nvnProgramSetShaders);
    dmNvnRegister("nvnProgramSetShadersExt", (nvn_func_t)nvn_trace_nvnProgramSetShadersExt);
    dmNvnRegister("nvnProgramSetSampleShading", (nvn_func_t)nvn_trace_nvnProgramSetSampleShading);
    dmNvnRegister("nvnMemoryPoolBuilderSetDevice", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderSetDevice);
    dmNvnRegister("nvnMemoryPoolBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderSetDefaults);
    dmNvnRegister("nvnMemoryPoolBuilderSetStorage", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderSetStorage);
    dmNvnRegister("nvnMemoryPoolBuilderSetFlags", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderSetFlags);
    dmNvnRegister("nvnMemoryPoolBuilderGetDevice", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderGetDevice);
    dmNvnRegister("nvnMemoryPoolBuilderGetMemory", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderGetMemory);
    dmNvnRegister("nvnMemoryPoolBuilderGetSize", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderGetSize);
    dmNvnRegister("nvnMemoryPoolBuilderGetFlags", (nvn_func_t)nvn_trace_nvnMemoryPoolBuilderGetFlags);
    dmNvnRegister("nvnMemoryPoolInitialize", (nvn_func_t)nvn_trace_nvnMemoryPoolInitialize);
    dmNvnRegister("nvnMemoryPoolSetDebugLabel", (nvn_func_t)nvn_trace_nvnMemoryPoolSetDebugLabel);
    dmNvnRegister("nvnMemoryPoolFinalize", (nvn_func_t)nvn_trace_nvnMemoryPoolFinalize);
    dmNvnRegister("nvnMemoryPoolMap", (nvn_func_t)nvn_trace_nvnMemoryPoolMap);
    dmNvnRegister("nvnMemoryPoolFlushMappedRange", (nvn_func_t)nvn_trace_nvnMemoryPoolFlushMappedRange);
    dmNvnRegister("nvnMemoryPoolInvalidateMappedRange", (nvn_func_t)nvn_trace_nvnMemoryPoolInvalidateMappedRange);
    dmNvnRegister("nvnMemoryPoolGetBufferAddress", (nvn_func_t)nvn_trace_nvnMemoryPoolGetBufferAddress);
    dmNvnRegister("nvnMemoryPoolMapVirtual", (nvn_func_t)nvn_trace_nvnMemoryPoolMapVirtual);
    dmNvnRegister("nvnMemoryPoolGetSize", (nvn_func_t)nvn_trace_nvnMemoryPoolGetSize);
    dmNvnRegister("nvnMemoryPoolGetFlags", (nvn_func_t)nvn_trace_nvnMemoryPoolGetFlags);
    dmNvnRegister("nvnTexturePoolInitialize", (nvn_func_t)nvn_trace_nvnTexturePoolInitialize);
    dmNvnRegister("nvnTexturePoolSetDebugLabel", (nvn_func_t)nvn_trace_nvnTexturePoolSetDebugLabel);
    dmNvnRegister("nvnTexturePoolFinalize", (nvn_func_t)nvn_trace_nvnTexturePoolFinalize);
    dmNvnRegister("nvnTexturePoolRegisterTexture", (nvn_func_t)nvn_trace_nvnTexturePoolRegisterTexture);
    dmNvnRegister("nvnTexturePoolRegisterImage", (nvn_func_t)nvn_trace_nvnTexturePoolRegisterImage);
    dmNvnRegister("nvnTexturePoolGetMemoryPool", (nvn_func_t)nvn_trace_nvnTexturePoolGetMemoryPool);
    dmNvnRegister("nvnTexturePoolGetMemoryOffset", (nvn_func_t)nvn_trace_nvnTexturePoolGetMemoryOffset);
    dmNvnRegister("nvnTexturePoolGetSize", (nvn_func_t)nvn_trace_nvnTexturePoolGetSize);
    dmNvnRegister("nvnSamplerPoolInitialize", (nvn_func_t)nvn_trace_nvnSamplerPoolInitialize);
    dmNvnRegister("nvnSamplerPoolSetDebugLabel", (nvn_func_t)nvn_trace_nvnSamplerPoolSetDebugLabel);
    dmNvnRegister("nvnSamplerPoolFinalize", (nvn_func_t)nvn_trace_nvnSamplerPoolFinalize);
    dmNvnRegister("nvnSamplerPoolRegisterSampler", (nvn_func_t)nvn_trace_nvnSamplerPoolRegisterSampler);
    dmNvnRegister("nvnSamplerPoolRegisterSamplerBuilder", (nvn_func_t)nvn_trace_nvnSamplerPoolRegisterSamplerBuilder);
    dmNvnRegister("nvnSamplerPoolGetMemoryPool", (nvn_func_t)nvn_trace_nvnSamplerPoolGetMemoryPool);
    dmNvnRegister("nvnSamplerPoolGetMemoryOffset", (nvn_func_t)nvn_trace_nvnSamplerPoolGetMemoryOffset);
    dmNvnRegister("nvnSamplerPoolGetSize", (nvn_func_t)nvn_trace_nvnSamplerPoolGetSize);
    dmNvnRegister("nvnBufferBuilderSetDevice", (nvn_func_t)nvn_trace_nvnBufferBuilderSetDevice);
    dmNvnRegister("nvnBufferBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnBufferBuilderSetDefaults);
    dmNvnRegister("nvnBufferBuilderSetStorage", (nvn_func_t)nvn_trace_nvnBufferBuilderSetStorage);
    dmNvnRegister("nvnBufferBuilderGetDevice", (nvn_func_t)nvn_trace_nvnBufferBuilderGetDevice);
    dmNvnRegister("nvnBufferBuilderGetMemoryPool", (nvn_func_t)nvn_trace_nvnBufferBuilderGetMemoryPool);
    dmNvnRegister("nvnBufferBuilderGetMemoryOffset", (nvn_func_t)nvn_trace_nvnBufferBuilderGetMemoryOffset);
    dmNvnRegister("nvnBufferBuilderGetSize", (nvn_func_t)nvn_trace_nvnBufferBuilderGetSize);
    dmNvnRegister("nvnBufferInitialize", (nvn_func_t)nvn_trace_nvnBufferInitialize);
    dmNvnRegister("nvnBufferSetDebugLabel", (nvn_func_t)nvn_trace_nvnBufferSetDebugLabel);
    dmNvnRegister("nvnBufferFinalize", (nvn_func_t)nvn_trace_nvnBufferFinalize);
    dmNvnRegister("nvnBufferMap", (nvn_func_t)nvn_trace_nvnBufferMap);
    dmNvnRegister("nvnBufferGetAddress", (nvn_func_t)nvn_trace_nvnBufferGetAddress);
    dmNvnRegister("nvnBufferFlushMappedRange", (nvn_func_t)nvn_trace_nvnBufferFlushMappedRange);
    dmNvnRegister("nvnBufferInvalidateMappedRange", (nvn_func_t)nvn_trace_nvnBufferInvalidateMappedRange);
    dmNvnRegister("nvnBufferGetMemoryPool", (nvn_func_t)nvn_trace_nvnBufferGetMemoryPool);
    dmNvnRegister("nvnBufferGetMemoryOffset", (nvn_func_t)nvn_trace_nvnBufferGetMemoryOffset);
    dmNvnRegister("nvnBufferGetSize", (nvn_func_t)nvn_trace_nvnBufferGetSize);
    dmNvnRegister("nvnBufferGetDebugID", (nvn_func_t)nvn_trace_nvnBufferGetDebugID);
    dmNvnRegister("nvnTextureBuilderSetDevice", (nvn_func_t)nvn_trace_nvnTextureBuilderSetDevice);
    dmNvnRegister("nvnTextureBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnTextureBuilderSetDefaults);
    dmNvnRegister("nvnTextureBuilderSetFlags", (nvn_func_t)nvn_trace_nvnTextureBuilderSetFlags);
    dmNvnRegister("nvnTextureBuilderSetTarget", (nvn_func_t)nvn_trace_nvnTextureBuilderSetTarget);
    dmNvnRegister("nvnTextureBuilderSetWidth", (nvn_func_t)nvn_trace_nvnTextureBuilderSetWidth);
    dmNvnRegister("nvnTextureBuilderSetHeight", (nvn_func_t)nvn_trace_nvnTextureBuilderSetHeight);
    dmNvnRegister("nvnTextureBuilderSetDepth", (nvn_func_t)nvn_trace_nvnTextureBuilderSetDepth);
    dmNvnRegister("nvnTextureBuilderSetSize1D", (nvn_func_t)nvn_trace_nvnTextureBuilderSetSize1D);
    dmNvnRegister("nvnTextureBuilderSetSize2D", (nvn_func_t)nvn_trace_nvnTextureBuilderSetSize2D);
    dmNvnRegister("nvnTextureBuilderSetSize3D", (nvn_func_t)nvn_trace_nvnTextureBuilderSetSize3D);
    dmNvnRegister("nvnTextureBuilderSetLevels", (nvn_func_t)nvn_trace_nvnTextureBuilderSetLevels);
    dmNvnRegister("nvnTextureBuilderSetFormat", (nvn_func_t)nvn_trace_nvnTextureBuilderSetFormat);
    dmNvnRegister("nvnTextureBuilderSetSamples", (nvn_func_t)nvn_trace_nvnTextureBuilderSetSamples);
    dmNvnRegister("nvnTextureBuilderSetSwizzle", (nvn_func_t)nvn_trace_nvnTextureBuilderSetSwizzle);
    dmNvnRegister("nvnTextureBuilderSetDepthStencilMode", (nvn_func_t)nvn_trace_nvnTextureBuilderSetDepthStencilMode);
    dmNvnRegister("nvnTextureBuilderGetStorageSize", (nvn_func_t)nvn_trace_nvnTextureBuilderGetStorageSize);
    dmNvnRegister("nvnTextureBuilderGetStorageAlignment", (nvn_func_t)nvn_trace_nvnTextureBuilderGetStorageAlignment);
    dmNvnRegister("nvnTextureBuilderSetStorage", (nvn_func_t)nvn_trace_nvnTextureBuilderSetStorage);
    dmNvnRegister("nvnTextureBuilderSetPackagedTextureData", (nvn_func_t)nvn_trace_nvnTextureBuilderSetPackagedTextureData);
    dmNvnRegister("nvnTextureBuilderSetPackagedTextureLayout", (nvn_func_t)nvn_trace_nvnTextureBuilderSetPackagedTextureLayout);
    dmNvnRegister("nvnTextureBuilderSetStride", (nvn_func_t)nvn_trace_nvnTextureBuilderSetStride);
    dmNvnRegister("nvnTextureBuilderSetGLTextureName", (nvn_func_t)nvn_trace_nvnTextureBuilderSetGLTextureName);
    dmNvnRegister("nvnTextureBuilderGetStorageClass", (nvn_func_t)nvn_trace_nvnTextureBuilderGetStorageClass);
    dmNvnRegister("nvnTextureBuilderGetDevice", (nvn_func_t)nvn_trace_nvnTextureBuilderGetDevice);
    dmNvnRegister("nvnTextureBuilderGetFlags", (nvn_func_t)nvn_trace_nvnTextureBuilderGetFlags);
    dmNvnRegister("nvnTextureBuilderGetTarget", (nvn_func_t)nvn_trace_nvnTextureBuilderGetTarget);
    dmNvnRegister("nvnTextureBuilderGetWidth", (nvn_func_t)nvn_trace_nvnTextureBuilderGetWidth);
    dmNvnRegister("nvnTextureBuilderGetHeight", (nvn_func_t)nvn_trace_nvnTextureBuilderGetHeight);
    dmNvnRegister("nvnTextureBuilderGetDepth", (nvn_func_t)nvn_trace_nvnTextureBuilderGetDepth);
    dmNvnRegister("nvnTextureBuilderGetLevels", (nvn_func_t)nvn_trace_nvnTextureBuilderGetLevels);
    dmNvnRegister("nvnTextureBuilderGetFormat", (nvn_func_t)nvn_trace_nvnTextureBuilderGetFormat);
    dmNvnRegister("nvnTextureBuilderGetSamples", (nvn_func_t)nvn_trace_nvnTextureBuilderGetSamples);
    dmNvnRegister("nvnTextureBuilderGetSwizzle", (nvn_func_t)nvn_trace_nvnTextureBuilderGetSwizzle);
    dmNvnRegister("nvnTextureBuilderGetDepthStencilMode", (nvn_func_t)nvn_trace_nvnTextureBuilderGetDepthStencilMode);
    dmNvnRegister("nvnTextureBuilderGetPackagedTextureData", (nvn_func_t)nvn_trace_nvnTextureBuilderGetPackagedTextureData);
    dmNvnRegister("nvnTextureBuilderGetPackagedTextureLayout", (nvn_func_t)nvn_trace_nvnTextureBuilderGetPackagedTextureLayout);
    dmNvnRegister("nvnTextureBuilderGetStride", (nvn_func_t)nvn_trace_nvnTextureBuilderGetStride);
    dmNvnRegister("nvnTextureBuilderGetSparseTileLayout", (nvn_func_t)nvn_trace_nvnTextureBuilderGetSparseTileLayout);
    dmNvnRegister("nvnTextureBuilderGetGLTextureName", (nvn_func_t)nvn_trace_nvnTextureBuilderGetGLTextureName);
    dmNvnRegister("nvnTextureBuilderGetZCullStorageSize", (nvn_func_t)nvn_trace_nvnTextureBuilderGetZCullStorageSize);
    dmNvnRegister("nvnTextureBuilderGetMemoryPool", (nvn_func_t)nvn_trace_nvnTextureBuilderGetMemoryPool);
    dmNvnRegister("nvnTextureBuilderGetMemoryOffset", (nvn_func_t)nvn_trace_nvnTextureBuilderGetMemoryOffset);
    dmNvnRegister("nvnTextureBuilderGetRawStorageClass", (nvn_func_t)nvn_trace_nvnTextureBuilderGetRawStorageClass);
    dmNvnRegister("nvnTextureViewSetDefaults", (nvn_func_t)nvn_trace_nvnTextureViewSetDefaults);
    dmNvnRegister("nvnTextureViewSetLevels", (nvn_func_t)nvn_trace_nvnTextureViewSetLevels);
    dmNvnRegister("nvnTextureViewSetLayers", (nvn_func_t)nvn_trace_nvnTextureViewSetLayers);
    dmNvnRegister("nvnTextureViewSetFormat", (nvn_func_t)nvn_trace_nvnTextureViewSetFormat);
    dmNvnRegister("nvnTextureViewSetSwizzle", (nvn_func_t)nvn_trace_nvnTextureViewSetSwizzle);
    dmNvnRegister("nvnTextureViewSetDepthStencilMode", (nvn_func_t)nvn_trace_nvnTextureViewSetDepthStencilMode);
    dmNvnRegister("nvnTextureViewSetTarget", (nvn_func_t)nvn_trace_nvnTextureViewSetTarget);
    dmNvnRegister("nvnTextureViewGetLevels", (nvn_func_t)nvn_trace_nvnTextureViewGetLevels);
    dmNvnRegister("nvnTextureViewGetLayers", (nvn_func_t)nvn_trace_nvnTextureViewGetLayers);
    dmNvnRegister("nvnTextureViewGetFormat", (nvn_func_t)nvn_trace_nvnTextureViewGetFormat);
    dmNvnRegister("nvnTextureViewGetSwizzle", (nvn_func_t)nvn_trace_nvnTextureViewGetSwizzle);
    dmNvnRegister("nvnTextureViewGetDepthStencilMode", (nvn_func_t)nvn_trace_nvnTextureViewGetDepthStencilMode);
    dmNvnRegister("nvnTextureViewGetTarget", (nvn_func_t)nvn_trace_nvnTextureViewGetTarget);
    dmNvnRegister("nvnTextureViewCompare", (nvn_func_t)nvn_trace_nvnTextureViewCompare);
    dmNvnRegister("nvnTextureInitialize", (nvn_func_t)nvn_trace_nvnTextureInitialize);
    dmNvnRegister("nvnTextureGetZCullStorageSize", (nvn_func_t)nvn_trace_nvnTextureGetZCullStorageSize);
    dmNvnRegister("nvnTextureFinalize", (nvn_func_t)nvn_trace_nvnTextureFinalize);
    dmNvnRegister("nvnTextureSetDebugLabel", (nvn_func_t)nvn_trace_nvnTextureSetDebugLabel);
    dmNvnRegister("nvnTextureGetStorageClass", (nvn_func_t)nvn_trace_nvnTextureGetStorageClass);
    dmNvnRegister("nvnTextureGetViewOffset", (nvn_func_t)nvn_trace_nvnTextureGetViewOffset);
    dmNvnRegister("nvnTextureGetFlags", (nvn_func_t)nvn_trace_nvnTextureGetFlags);
    dmNvnRegister("nvnTextureGetTarget", (nvn_func_t)nvn_trace_nvnTextureGetTarget);
    dmNvnRegister("nvnTextureGetWidth", (nvn_func_t)nvn_trace_nvnTextureGetWidth);
    dmNvnRegister("nvnTextureGetHeight", (nvn_func_t)nvn_trace_nvnTextureGetHeight);
    dmNvnRegister("nvnTextureGetDepth", (nvn_func_t)nvn_trace_nvnTextureGetDepth);
    dmNvnRegister("nvnTextureGetLevels", (nvn_func_t)nvn_trace_nvnTextureGetLevels);
    dmNvnRegister("nvnTextureGetFormat", (nvn_func_t)nvn_trace_nvnTextureGetFormat);
    dmNvnRegister("nvnTextureGetSamples", (nvn_func_t)nvn_trace_nvnTextureGetSamples);
    dmNvnRegister("nvnTextureGetSwizzle", (nvn_func_t)nvn_trace_nvnTextureGetSwizzle);
    dmNvnRegister("nvnTextureGetDepthStencilMode", (nvn_func_t)nvn_trace_nvnTextureGetDepthStencilMode);
    dmNvnRegister("nvnTextureGetStride", (nvn_func_t)nvn_trace_nvnTextureGetStride);
    dmNvnRegister("nvnTextureGetTextureAddress", (nvn_func_t)nvn_trace_nvnTextureGetTextureAddress);
    dmNvnRegister("nvnTextureGetSparseTileLayout", (nvn_func_t)nvn_trace_nvnTextureGetSparseTileLayout);
    dmNvnRegister("nvnTextureWriteTexels", (nvn_func_t)nvn_trace_nvnTextureWriteTexels);
    dmNvnRegister("nvnTextureWriteTexelsStrided", (nvn_func_t)nvn_trace_nvnTextureWriteTexelsStrided);
    dmNvnRegister("nvnTextureReadTexels", (nvn_func_t)nvn_trace_nvnTextureReadTexels);
    dmNvnRegister("nvnTextureReadTexelsStrided", (nvn_func_t)nvn_trace_nvnTextureReadTexelsStrided);
    dmNvnRegister("nvnTextureFlushTexels", (nvn_func_t)nvn_trace_nvnTextureFlushTexels);
    dmNvnRegister("nvnTextureInvalidateTexels", (nvn_func_t)nvn_trace_nvnTextureInvalidateTexels);
    dmNvnRegister("nvnTextureGetMemoryPool", (nvn_func_t)nvn_trace_nvnTextureGetMemoryPool);
    dmNvnRegister("nvnTextureGetMemoryOffset", (nvn_func_t)nvn_trace_nvnTextureGetMemoryOffset);
    dmNvnRegister("nvnTextureGetStorageSize", (nvn_func_t)nvn_trace_nvnTextureGetStorageSize);
    dmNvnRegister("nvnTextureCompare", (nvn_func_t)nvn_trace_nvnTextureCompare);
    dmNvnRegister("nvnTextureGetDebugID", (nvn_func_t)nvn_trace_nvnTextureGetDebugID);
    dmNvnRegister("nvnTextureGetRawStorageClass", (nvn_func_t)nvn_trace_nvnTextureGetRawStorageClass);
    dmNvnRegister("nvnSamplerBuilderSetDevice", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetDevice);
    dmNvnRegister("nvnSamplerBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetDefaults);
    dmNvnRegister("nvnSamplerBuilderSetMinMagFilter", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetMinMagFilter);
    dmNvnRegister("nvnSamplerBuilderSetWrapMode", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetWrapMode);
    dmNvnRegister("nvnSamplerBuilderSetLodClamp", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetLodClamp);
    dmNvnRegister("nvnSamplerBuilderSetLodBias", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetLodBias);
    dmNvnRegister("nvnSamplerBuilderSetCompare", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetCompare);
    dmNvnRegister("nvnSamplerBuilderSetBorderColor", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetBorderColor);
    dmNvnRegister("nvnSamplerBuilderSetBorderColori", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetBorderColori);
    dmNvnRegister("nvnSamplerBuilderSetBorderColorui", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetBorderColorui);
    dmNvnRegister("nvnSamplerBuilderSetMaxAnisotropy", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetMaxAnisotropy);
    dmNvnRegister("nvnSamplerBuilderSetReductionFilter", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetReductionFilter);
    dmNvnRegister("nvnSamplerBuilderSetLodSnap", (nvn_func_t)nvn_trace_nvnSamplerBuilderSetLodSnap);
    dmNvnRegister("nvnSamplerBuilderGetDevice", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetDevice);
    dmNvnRegister("nvnSamplerBuilderGetMinMagFilter", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetMinMagFilter);
    dmNvnRegister("nvnSamplerBuilderGetWrapMode", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetWrapMode);
    dmNvnRegister("nvnSamplerBuilderGetLodClamp", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetLodClamp);
    dmNvnRegister("nvnSamplerBuilderGetLodBias", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetLodBias);
    dmNvnRegister("nvnSamplerBuilderGetCompare", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetCompare);
    dmNvnRegister("nvnSamplerBuilderGetBorderColor", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetBorderColor);
    dmNvnRegister("nvnSamplerBuilderGetBorderColori", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetBorderColori);
    dmNvnRegister("nvnSamplerBuilderGetBorderColorui", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetBorderColorui);
    dmNvnRegister("nvnSamplerBuilderGetMaxAnisotropy", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetMaxAnisotropy);
    dmNvnRegister("nvnSamplerBuilderGetReductionFilter", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetReductionFilter);
    dmNvnRegister("nvnSamplerBuilderGetLodSnap", (nvn_func_t)nvn_trace_nvnSamplerBuilderGetLodSnap);
    dmNvnRegister("nvnSamplerInitialize", (nvn_func_t)nvn_trace_nvnSamplerInitialize);
    dmNvnRegister("nvnSamplerFinalize", (nvn_func_t)nvn_trace_nvnSamplerFinalize);
    dmNvnRegister("nvnSamplerSetDebugLabel", (nvn_func_t)nvn_trace_nvnSamplerSetDebugLabel);
    dmNvnRegister("nvnSamplerGetMinMagFilter", (nvn_func_t)nvn_trace_nvnSamplerGetMinMagFilter);
    dmNvnRegister("nvnSamplerGetWrapMode", (nvn_func_t)nvn_trace_nvnSamplerGetWrapMode);
    dmNvnRegister("nvnSamplerGetLodClamp", (nvn_func_t)nvn_trace_nvnSamplerGetLodClamp);
    dmNvnRegister("nvnSamplerGetLodBias", (nvn_func_t)nvn_trace_nvnSamplerGetLodBias);
    dmNvnRegister("nvnSamplerGetCompare", (nvn_func_t)nvn_trace_nvnSamplerGetCompare);
    dmNvnRegister("nvnSamplerGetBorderColor", (nvn_func_t)nvn_trace_nvnSamplerGetBorderColor);
    dmNvnRegister("nvnSamplerGetBorderColori", (nvn_func_t)nvn_trace_nvnSamplerGetBorderColori);
    dmNvnRegister("nvnSamplerGetBorderColorui", (nvn_func_t)nvn_trace_nvnSamplerGetBorderColorui);
    dmNvnRegister("nvnSamplerGetMaxAnisotropy", (nvn_func_t)nvn_trace_nvnSamplerGetMaxAnisotropy);
    dmNvnRegister("nvnSamplerGetReductionFilter", (nvn_func_t)nvn_trace_nvnSamplerGetReductionFilter);
    dmNvnRegister("nvnSamplerCompare", (nvn_func_t)nvn_trace_nvnSamplerCompare);
    dmNvnRegister("nvnSamplerGetDebugID", (nvn_func_t)nvn_trace_nvnSamplerGetDebugID);
    dmNvnRegister("nvnBlendStateSetDefaults", (nvn_func_t)nvn_trace_nvnBlendStateSetDefaults);
    dmNvnRegister("nvnBlendStateSetBlendTarget", (nvn_func_t)nvn_trace_nvnBlendStateSetBlendTarget);
    dmNvnRegister("nvnBlendStateSetBlendFunc", (nvn_func_t)nvn_trace_nvnBlendStateSetBlendFunc);
    dmNvnRegister("nvnBlendStateSetBlendEquation", (nvn_func_t)nvn_trace_nvnBlendStateSetBlendEquation);
    dmNvnRegister("nvnBlendStateSetAdvancedMode", (nvn_func_t)nvn_trace_nvnBlendStateSetAdvancedMode);
    dmNvnRegister("nvnBlendStateSetAdvancedOverlap", (nvn_func_t)nvn_trace_nvnBlendStateSetAdvancedOverlap);
    dmNvnRegister("nvnBlendStateSetAdvancedPremultipliedSrc", (nvn_func_t)nvn_trace_nvnBlendStateSetAdvancedPremultipliedSrc);
    dmNvnRegister("nvnBlendStateSetAdvancedNormalizedDst", (nvn_func_t)nvn_trace_nvnBlendStateSetAdvancedNormalizedDst);
    dmNvnRegister("nvnBlendStateGetBlendTarget", (nvn_func_t)nvn_trace_nvnBlendStateGetBlendTarget);
    dmNvnRegister("nvnBlendStateGetBlendFunc", (nvn_func_t)nvn_trace_nvnBlendStateGetBlendFunc);
    dmNvnRegister("nvnBlendStateGetBlendEquation", (nvn_func_t)nvn_trace_nvnBlendStateGetBlendEquation);
    dmNvnRegister("nvnBlendStateGetAdvancedMode", (nvn_func_t)nvn_trace_nvnBlendStateGetAdvancedMode);
    dmNvnRegister("nvnBlendStateGetAdvancedOverlap", (nvn_func_t)nvn_trace_nvnBlendStateGetAdvancedOverlap);
    dmNvnRegister("nvnBlendStateGetAdvancedPremultipliedSrc", (nvn_func_t)nvn_trace_nvnBlendStateGetAdvancedPremultipliedSrc);
    dmNvnRegister("nvnBlendStateGetAdvancedNormalizedDst", (nvn_func_t)nvn_trace_nvnBlendStateGetAdvancedNormalizedDst);
    dmNvnRegister("nvnColorStateSetDefaults", (nvn_func_t)nvn_trace_nvnColorStateSetDefaults);
    dmNvnRegister("nvnColorStateSetBlendEnable", (nvn_func_t)nvn_trace_nvnColorStateSetBlendEnable);
    dmNvnRegister("nvnColorStateSetLogicOp", (nvn_func_t)nvn_trace_nvnColorStateSetLogicOp);
    dmNvnRegister("nvnColorStateSetAlphaTest", (nvn_func_t)nvn_trace_nvnColorStateSetAlphaTest);
    dmNvnRegister("nvnColorStateGetBlendEnable", (nvn_func_t)nvn_trace_nvnColorStateGetBlendEnable);
    dmNvnRegister("nvnColorStateGetLogicOp", (nvn_func_t)nvn_trace_nvnColorStateGetLogicOp);
    dmNvnRegister("nvnColorStateGetAlphaTest", (nvn_func_t)nvn_trace_nvnColorStateGetAlphaTest);
    dmNvnRegister("nvnChannelMaskStateSetDefaults", (nvn_func_t)nvn_trace_nvnChannelMaskStateSetDefaults);
    dmNvnRegister("nvnChannelMaskStateSetChannelMask", (nvn_func_t)nvn_trace_nvnChannelMaskStateSetChannelMask);
    dmNvnRegister("nvnChannelMaskStateGetChannelMask", (nvn_func_t)nvn_trace_nvnChannelMaskStateGetChannelMask);
    dmNvnRegister("nvnMultisampleStateSetDefaults", (nvn_func_t)nvn_trace_nvnMultisampleStateSetDefaults);
    dmNvnRegister("nvnMultisampleStateSetMultisampleEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateSetMultisampleEnable);
    dmNvnRegister("nvnMultisampleStateSetSamples", (nvn_func_t)nvn_trace_nvnMultisampleStateSetSamples);
    dmNvnRegister("nvnMultisampleStateSetAlphaToCoverageEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateSetAlphaToCoverageEnable);
    dmNvnRegister("nvnMultisampleStateSetAlphaToCoverageDither", (nvn_func_t)nvn_trace_nvnMultisampleStateSetAlphaToCoverageDither);
    dmNvnRegister("nvnMultisampleStateGetMultisampleEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateGetMultisampleEnable);
    dmNvnRegister("nvnMultisampleStateGetSamples", (nvn_func_t)nvn_trace_nvnMultisampleStateGetSamples);
    dmNvnRegister("nvnMultisampleStateGetAlphaToCoverageEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateGetAlphaToCoverageEnable);
    dmNvnRegister("nvnMultisampleStateGetAlphaToCoverageDither", (nvn_func_t)nvn_trace_nvnMultisampleStateGetAlphaToCoverageDither);
    dmNvnRegister("nvnMultisampleStateSetRasterSamples", (nvn_func_t)nvn_trace_nvnMultisampleStateSetRasterSamples);
    dmNvnRegister("nvnMultisampleStateGetRasterSamples", (nvn_func_t)nvn_trace_nvnMultisampleStateGetRasterSamples);
    dmNvnRegister("nvnMultisampleStateSetCoverageModulationMode", (nvn_func_t)nvn_trace_nvnMultisampleStateSetCoverageModulationMode);
    dmNvnRegister("nvnMultisampleStateGetCoverageModulationMode", (nvn_func_t)nvn_trace_nvnMultisampleStateGetCoverageModulationMode);
    dmNvnRegister("nvnMultisampleStateSetCoverageToColorEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateSetCoverageToColorEnable);
    dmNvnRegister("nvnMultisampleStateGetCoverageToColorEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateGetCoverageToColorEnable);
    dmNvnRegister("nvnMultisampleStateSetCoverageToColorOutput", (nvn_func_t)nvn_trace_nvnMultisampleStateSetCoverageToColorOutput);
    dmNvnRegister("nvnMultisampleStateGetCoverageToColorOutput", (nvn_func_t)nvn_trace_nvnMultisampleStateGetCoverageToColorOutput);
    dmNvnRegister("nvnMultisampleStateSetSampleLocationsEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateSetSampleLocationsEnable);
    dmNvnRegister("nvnMultisampleStateGetSampleLocationsEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateGetSampleLocationsEnable);
    dmNvnRegister("nvnMultisampleStateGetSampleLocationsGrid", (nvn_func_t)nvn_trace_nvnMultisampleStateGetSampleLocationsGrid);
    dmNvnRegister("nvnMultisampleStateSetSampleLocationsGridEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateSetSampleLocationsGridEnable);
    dmNvnRegister("nvnMultisampleStateGetSampleLocationsGridEnable", (nvn_func_t)nvn_trace_nvnMultisampleStateGetSampleLocationsGridEnable);
    dmNvnRegister("nvnMultisampleStateSetSampleLocations", (nvn_func_t)nvn_trace_nvnMultisampleStateSetSampleLocations);
    dmNvnRegister("nvnPolygonStateSetDefaults", (nvn_func_t)nvn_trace_nvnPolygonStateSetDefaults);
    dmNvnRegister("nvnPolygonStateSetCullFace", (nvn_func_t)nvn_trace_nvnPolygonStateSetCullFace);
    dmNvnRegister("nvnPolygonStateSetFrontFace", (nvn_func_t)nvn_trace_nvnPolygonStateSetFrontFace);
    dmNvnRegister("nvnPolygonStateSetPolygonMode", (nvn_func_t)nvn_trace_nvnPolygonStateSetPolygonMode);
    dmNvnRegister("nvnPolygonStateSetPolygonOffsetEnables", (nvn_func_t)nvn_trace_nvnPolygonStateSetPolygonOffsetEnables);
    dmNvnRegister("nvnPolygonStateGetCullFace", (nvn_func_t)nvn_trace_nvnPolygonStateGetCullFace);
    dmNvnRegister("nvnPolygonStateGetFrontFace", (nvn_func_t)nvn_trace_nvnPolygonStateGetFrontFace);
    dmNvnRegister("nvnPolygonStateGetPolygonMode", (nvn_func_t)nvn_trace_nvnPolygonStateGetPolygonMode);
    dmNvnRegister("nvnPolygonStateGetPolygonOffsetEnables", (nvn_func_t)nvn_trace_nvnPolygonStateGetPolygonOffsetEnables);
    dmNvnRegister("nvnDepthStencilStateSetDefaults", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetDefaults);
    dmNvnRegister("nvnDepthStencilStateSetDepthTestEnable", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetDepthTestEnable);
    dmNvnRegister("nvnDepthStencilStateSetDepthWriteEnable", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetDepthWriteEnable);
    dmNvnRegister("nvnDepthStencilStateSetDepthFunc", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetDepthFunc);
    dmNvnRegister("nvnDepthStencilStateSetStencilTestEnable", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetStencilTestEnable);
    dmNvnRegister("nvnDepthStencilStateSetStencilFunc", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetStencilFunc);
    dmNvnRegister("nvnDepthStencilStateSetStencilOp", (nvn_func_t)nvn_trace_nvnDepthStencilStateSetStencilOp);
    dmNvnRegister("nvnDepthStencilStateGetDepthTestEnable", (nvn_func_t)nvn_trace_nvnDepthStencilStateGetDepthTestEnable);
    dmNvnRegister("nvnDepthStencilStateGetDepthWriteEnable", (nvn_func_t)nvn_trace_nvnDepthStencilStateGetDepthWriteEnable);
    dmNvnRegister("nvnDepthStencilStateGetDepthFunc", (nvn_func_t)nvn_trace_nvnDepthStencilStateGetDepthFunc);
    dmNvnRegister("nvnDepthStencilStateGetStencilTestEnable", (nvn_func_t)nvn_trace_nvnDepthStencilStateGetStencilTestEnable);
    dmNvnRegister("nvnDepthStencilStateGetStencilFunc", (nvn_func_t)nvn_trace_nvnDepthStencilStateGetStencilFunc);
    dmNvnRegister("nvnDepthStencilStateGetStencilOp", (nvn_func_t)nvn_trace_nvnDepthStencilStateGetStencilOp);
    dmNvnRegister("nvnVertexAttribStateSetDefaults", (nvn_func_t)nvn_trace_nvnVertexAttribStateSetDefaults);
    dmNvnRegister("nvnVertexAttribStateSetFormat", (nvn_func_t)nvn_trace_nvnVertexAttribStateSetFormat);
    dmNvnRegister("nvnVertexAttribStateSetStreamIndex", (nvn_func_t)nvn_trace_nvnVertexAttribStateSetStreamIndex);
    dmNvnRegister("nvnVertexAttribStateGetFormat", (nvn_func_t)nvn_trace_nvnVertexAttribStateGetFormat);
    dmNvnRegister("nvnVertexAttribStateGetStreamIndex", (nvn_func_t)nvn_trace_nvnVertexAttribStateGetStreamIndex);
    dmNvnRegister("nvnVertexStreamStateSetDefaults", (nvn_func_t)nvn_trace_nvnVertexStreamStateSetDefaults);
    dmNvnRegister("nvnVertexStreamStateSetStride", (nvn_func_t)nvn_trace_nvnVertexStreamStateSetStride);
    dmNvnRegister("nvnVertexStreamStateSetDivisor", (nvn_func_t)nvn_trace_nvnVertexStreamStateSetDivisor);
    dmNvnRegister("nvnVertexStreamStateGetStride", (nvn_func_t)nvn_trace_nvnVertexStreamStateGetStride);
    dmNvnRegister("nvnVertexStreamStateGetDivisor", (nvn_func_t)nvn_trace_nvnVertexStreamStateGetDivisor);
    dmNvnRegister("nvnCommandBufferInitialize", (nvn_func_t)nvn_trace_nvnCommandBufferInitialize);
    dmNvnRegister("nvnCommandBufferFinalize", (nvn_func_t)nvn_trace_nvnCommandBufferFinalize);
    dmNvnRegister("nvnCommandBufferSetDebugLabel", (nvn_func_t)nvn_trace_nvnCommandBufferSetDebugLabel);
    dmNvnRegister("nvnCommandBufferSetMemoryCallback", (nvn_func_t)nvn_trace_nvnCommandBufferSetMemoryCallback);
    dmNvnRegister("nvnCommandBufferSetMemoryCallbackData", (nvn_func_t)nvn_trace_nvnCommandBufferSetMemoryCallbackData);
    dmNvnRegister("nvnCommandBufferSetCommandMemoryCallbackEnabled", (nvn_func_t)nvn_trace_nvnCommandBufferSetCommandMemoryCallbackEnabled);
    dmNvnRegister("nvnCommandBufferAddCommandMemory", (nvn_func_t)nvn_trace_nvnCommandBufferAddCommandMemory);
    dmNvnRegister("nvnCommandBufferAddControlMemory", (nvn_func_t)nvn_trace_nvnCommandBufferAddControlMemory);
    dmNvnRegister("nvnCommandBufferGetCommandMemorySize", (nvn_func_t)nvn_trace_nvnCommandBufferGetCommandMemorySize);
    dmNvnRegister("nvnCommandBufferGetCommandMemoryUsed", (nvn_func_t)nvn_trace_nvnCommandBufferGetCommandMemoryUsed);
    dmNvnRegister("nvnCommandBufferGetCommandMemoryFree", (nvn_func_t)nvn_trace_nvnCommandBufferGetCommandMemoryFree);
    dmNvnRegister("nvnCommandBufferGetControlMemorySize", (nvn_func_t)nvn_trace_nvnCommandBufferGetControlMemorySize);
    dmNvnRegister("nvnCommandBufferGetControlMemoryUsed", (nvn_func_t)nvn_trace_nvnCommandBufferGetControlMemoryUsed);
    dmNvnRegister("nvnCommandBufferGetControlMemoryFree", (nvn_func_t)nvn_trace_nvnCommandBufferGetControlMemoryFree);
    dmNvnRegister("nvnCommandBufferBeginRecording", (nvn_func_t)nvn_trace_nvnCommandBufferBeginRecording);
    dmNvnRegister("nvnCommandBufferEndRecording", (nvn_func_t)nvn_trace_nvnCommandBufferEndRecording);
    dmNvnRegister("nvnCommandBufferCallCommands", (nvn_func_t)nvn_trace_nvnCommandBufferCallCommands);
    dmNvnRegister("nvnCommandBufferCopyCommands", (nvn_func_t)nvn_trace_nvnCommandBufferCopyCommands);
    dmNvnRegister("nvnCommandBufferBindBlendState", (nvn_func_t)nvn_trace_nvnCommandBufferBindBlendState);
    dmNvnRegister("nvnCommandBufferBindChannelMaskState", (nvn_func_t)nvn_trace_nvnCommandBufferBindChannelMaskState);
    dmNvnRegister("nvnCommandBufferBindColorState", (nvn_func_t)nvn_trace_nvnCommandBufferBindColorState);
    dmNvnRegister("nvnCommandBufferBindMultisampleState", (nvn_func_t)nvn_trace_nvnCommandBufferBindMultisampleState);
    dmNvnRegister("nvnCommandBufferBindPolygonState", (nvn_func_t)nvn_trace_nvnCommandBufferBindPolygonState);
    dmNvnRegister("nvnCommandBufferBindDepthStencilState", (nvn_func_t)nvn_trace_nvnCommandBufferBindDepthStencilState);
    dmNvnRegister("nvnCommandBufferBindVertexAttribState", (nvn_func_t)nvn_trace_nvnCommandBufferBindVertexAttribState);
    dmNvnRegister("nvnCommandBufferBindVertexStreamState", (nvn_func_t)nvn_trace_nvnCommandBufferBindVertexStreamState);
    dmNvnRegister("nvnCommandBufferBindProgram", (nvn_func_t)nvn_trace_nvnCommandBufferBindProgram);
    dmNvnRegister("nvnCommandBufferBindVertexBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferBindVertexBuffer);
    dmNvnRegister("nvnCommandBufferBindVertexBuffers", (nvn_func_t)nvn_trace_nvnCommandBufferBindVertexBuffers);
    dmNvnRegister("nvnCommandBufferBindUniformBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferBindUniformBuffer);
    dmNvnRegister("nvnCommandBufferBindUniformBuffers", (nvn_func_t)nvn_trace_nvnCommandBufferBindUniformBuffers);
    dmNvnRegister("nvnCommandBufferBindTransformFeedbackBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferBindTransformFeedbackBuffer);
    dmNvnRegister("nvnCommandBufferBindTransformFeedbackBuffers", (nvn_func_t)nvn_trace_nvnCommandBufferBindTransformFeedbackBuffers);
    dmNvnRegister("nvnCommandBufferBindStorageBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferBindStorageBuffer);
    dmNvnRegister("nvnCommandBufferBindStorageBuffers", (nvn_func_t)nvn_trace_nvnCommandBufferBindStorageBuffers);
    dmNvnRegister("nvnCommandBufferBindTexture", (nvn_func_t)nvn_trace_nvnCommandBufferBindTexture);
    dmNvnRegister("nvnCommandBufferBindTextures", (nvn_func_t)nvn_trace_nvnCommandBufferBindTextures);
    dmNvnRegister("nvnCommandBufferBindImage", (nvn_func_t)nvn_trace_nvnCommandBufferBindImage);
    dmNvnRegister("nvnCommandBufferBindImages", (nvn_func_t)nvn_trace_nvnCommandBufferBindImages);
    dmNvnRegister("nvnCommandBufferSetPatchSize", (nvn_func_t)nvn_trace_nvnCommandBufferSetPatchSize);
    dmNvnRegister("nvnCommandBufferSetInnerTessellationLevels", (nvn_func_t)nvn_trace_nvnCommandBufferSetInnerTessellationLevels);
    dmNvnRegister("nvnCommandBufferSetOuterTessellationLevels", (nvn_func_t)nvn_trace_nvnCommandBufferSetOuterTessellationLevels);
    dmNvnRegister("nvnCommandBufferSetPrimitiveRestart", (nvn_func_t)nvn_trace_nvnCommandBufferSetPrimitiveRestart);
    dmNvnRegister("nvnCommandBufferBeginTransformFeedback", (nvn_func_t)nvn_trace_nvnCommandBufferBeginTransformFeedback);
    dmNvnRegister("nvnCommandBufferEndTransformFeedback", (nvn_func_t)nvn_trace_nvnCommandBufferEndTransformFeedback);
    dmNvnRegister("nvnCommandBufferPauseTransformFeedback", (nvn_func_t)nvn_trace_nvnCommandBufferPauseTransformFeedback);
    dmNvnRegister("nvnCommandBufferResumeTransformFeedback", (nvn_func_t)nvn_trace_nvnCommandBufferResumeTransformFeedback);
    dmNvnRegister("nvnCommandBufferDrawTransformFeedback", (nvn_func_t)nvn_trace_nvnCommandBufferDrawTransformFeedback);
    dmNvnRegister("nvnCommandBufferDrawArrays", (nvn_func_t)nvn_trace_nvnCommandBufferDrawArrays);
    dmNvnRegister("nvnCommandBufferDrawElements", (nvn_func_t)nvn_trace_nvnCommandBufferDrawElements);
    dmNvnRegister("nvnCommandBufferDrawElementsBaseVertex", (nvn_func_t)nvn_trace_nvnCommandBufferDrawElementsBaseVertex);
    dmNvnRegister("nvnCommandBufferDrawArraysInstanced", (nvn_func_t)nvn_trace_nvnCommandBufferDrawArraysInstanced);
    dmNvnRegister("nvnCommandBufferDrawElementsInstanced", (nvn_func_t)nvn_trace_nvnCommandBufferDrawElementsInstanced);
    dmNvnRegister("nvnCommandBufferDrawArraysIndirect", (nvn_func_t)nvn_trace_nvnCommandBufferDrawArraysIndirect);
    dmNvnRegister("nvnCommandBufferDrawElementsIndirect", (nvn_func_t)nvn_trace_nvnCommandBufferDrawElementsIndirect);
    dmNvnRegister("nvnCommandBufferMultiDrawArraysIndirectCount", (nvn_func_t)nvn_trace_nvnCommandBufferMultiDrawArraysIndirectCount);
    dmNvnRegister("nvnCommandBufferMultiDrawElementsIndirectCount", (nvn_func_t)nvn_trace_nvnCommandBufferMultiDrawElementsIndirectCount);
    dmNvnRegister("nvnCommandBufferClearColor", (nvn_func_t)nvn_trace_nvnCommandBufferClearColor);
    dmNvnRegister("nvnCommandBufferClearColori", (nvn_func_t)nvn_trace_nvnCommandBufferClearColori);
    dmNvnRegister("nvnCommandBufferClearColorui", (nvn_func_t)nvn_trace_nvnCommandBufferClearColorui);
    dmNvnRegister("nvnCommandBufferClearDepthStencil", (nvn_func_t)nvn_trace_nvnCommandBufferClearDepthStencil);
    dmNvnRegister("nvnCommandBufferDispatchCompute", (nvn_func_t)nvn_trace_nvnCommandBufferDispatchCompute);
    dmNvnRegister("nvnCommandBufferDispatchComputeIndirect", (nvn_func_t)nvn_trace_nvnCommandBufferDispatchComputeIndirect);
    dmNvnRegister("nvnCommandBufferSetViewport", (nvn_func_t)nvn_trace_nvnCommandBufferSetViewport);
    dmNvnRegister("nvnCommandBufferSetViewports", (nvn_func_t)nvn_trace_nvnCommandBufferSetViewports);
    dmNvnRegister("nvnCommandBufferSetViewportSwizzles", (nvn_func_t)nvn_trace_nvnCommandBufferSetViewportSwizzles);
    dmNvnRegister("nvnCommandBufferSetScissor", (nvn_func_t)nvn_trace_nvnCommandBufferSetScissor);
    dmNvnRegister("nvnCommandBufferSetScissors", (nvn_func_t)nvn_trace_nvnCommandBufferSetScissors);
    dmNvnRegister("nvnCommandBufferSetDepthRange", (nvn_func_t)nvn_trace_nvnCommandBufferSetDepthRange);
    dmNvnRegister("nvnCommandBufferSetDepthBounds", (nvn_func_t)nvn_trace_nvnCommandBufferSetDepthBounds);
    dmNvnRegister("nvnCommandBufferSetDepthRanges", (nvn_func_t)nvn_trace_nvnCommandBufferSetDepthRanges);
    dmNvnRegister("nvnCommandBufferSetTiledCacheAction", (nvn_func_t)nvn_trace_nvnCommandBufferSetTiledCacheAction);
    dmNvnRegister("nvnCommandBufferSetTiledCacheTileSize", (nvn_func_t)nvn_trace_nvnCommandBufferSetTiledCacheTileSize);
    dmNvnRegister("nvnCommandBufferBindSeparateTexture", (nvn_func_t)nvn_trace_nvnCommandBufferBindSeparateTexture);
    dmNvnRegister("nvnCommandBufferBindSeparateSampler", (nvn_func_t)nvn_trace_nvnCommandBufferBindSeparateSampler);
    dmNvnRegister("nvnCommandBufferBindSeparateTextures", (nvn_func_t)nvn_trace_nvnCommandBufferBindSeparateTextures);
    dmNvnRegister("nvnCommandBufferBindSeparateSamplers", (nvn_func_t)nvn_trace_nvnCommandBufferBindSeparateSamplers);
    dmNvnRegister("nvnCommandBufferSetStencilValueMask", (nvn_func_t)nvn_trace_nvnCommandBufferSetStencilValueMask);
    dmNvnRegister("nvnCommandBufferSetStencilMask", (nvn_func_t)nvn_trace_nvnCommandBufferSetStencilMask);
    dmNvnRegister("nvnCommandBufferSetStencilRef", (nvn_func_t)nvn_trace_nvnCommandBufferSetStencilRef);
    dmNvnRegister("nvnCommandBufferSetBlendColor", (nvn_func_t)nvn_trace_nvnCommandBufferSetBlendColor);
    dmNvnRegister("nvnCommandBufferSetPointSize", (nvn_func_t)nvn_trace_nvnCommandBufferSetPointSize);
    dmNvnRegister("nvnCommandBufferSetLineWidth", (nvn_func_t)nvn_trace_nvnCommandBufferSetLineWidth);
    dmNvnRegister("nvnCommandBufferSetPolygonOffsetClamp", (nvn_func_t)nvn_trace_nvnCommandBufferSetPolygonOffsetClamp);
    dmNvnRegister("nvnCommandBufferSetAlphaRef", (nvn_func_t)nvn_trace_nvnCommandBufferSetAlphaRef);
    dmNvnRegister("nvnCommandBufferSetSampleMask", (nvn_func_t)nvn_trace_nvnCommandBufferSetSampleMask);
    dmNvnRegister("nvnCommandBufferSetRasterizerDiscard", (nvn_func_t)nvn_trace_nvnCommandBufferSetRasterizerDiscard);
    dmNvnRegister("nvnCommandBufferSetDepthClamp", (nvn_func_t)nvn_trace_nvnCommandBufferSetDepthClamp);
    dmNvnRegister("nvnCommandBufferSetConservativeRasterEnable", (nvn_func_t)nvn_trace_nvnCommandBufferSetConservativeRasterEnable);
    dmNvnRegister("nvnCommandBufferSetConservativeRasterDilate", (nvn_func_t)nvn_trace_nvnCommandBufferSetConservativeRasterDilate);
    dmNvnRegister("nvnCommandBufferSetSubpixelPrecisionBias", (nvn_func_t)nvn_trace_nvnCommandBufferSetSubpixelPrecisionBias);
    dmNvnRegister("nvnCommandBufferCopyBufferToTexture", (nvn_func_t)nvn_trace_nvnCommandBufferCopyBufferToTexture);
    dmNvnRegister("nvnCommandBufferCopyTextureToBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferCopyTextureToBuffer);
    dmNvnRegister("nvnCommandBufferCopyTextureToTexture", (nvn_func_t)nvn_trace_nvnCommandBufferCopyTextureToTexture);
    dmNvnRegister("nvnCommandBufferCopyBufferToBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferCopyBufferToBuffer);
    dmNvnRegister("nvnCommandBufferClearBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferClearBuffer);
    dmNvnRegister("nvnCommandBufferClearTexture", (nvn_func_t)nvn_trace_nvnCommandBufferClearTexture);
    dmNvnRegister("nvnCommandBufferClearTexturei", (nvn_func_t)nvn_trace_nvnCommandBufferClearTexturei);
    dmNvnRegister("nvnCommandBufferClearTextureui", (nvn_func_t)nvn_trace_nvnCommandBufferClearTextureui);
    dmNvnRegister("nvnCommandBufferUpdateUniformBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferUpdateUniformBuffer);
    dmNvnRegister("nvnCommandBufferReportCounter", (nvn_func_t)nvn_trace_nvnCommandBufferReportCounter);
    dmNvnRegister("nvnCommandBufferResetCounter", (nvn_func_t)nvn_trace_nvnCommandBufferResetCounter);
    dmNvnRegister("nvnCommandBufferReportValue", (nvn_func_t)nvn_trace_nvnCommandBufferReportValue);
    dmNvnRegister("nvnCommandBufferSetRenderEnable", (nvn_func_t)nvn_trace_nvnCommandBufferSetRenderEnable);
    dmNvnRegister("nvnCommandBufferSetRenderEnableConditional", (nvn_func_t)nvn_trace_nvnCommandBufferSetRenderEnableConditional);
    dmNvnRegister("nvnCommandBufferSetRenderTargets", (nvn_func_t)nvn_trace_nvnCommandBufferSetRenderTargets);
    dmNvnRegister("nvnCommandBufferDiscardColor", (nvn_func_t)nvn_trace_nvnCommandBufferDiscardColor);
    dmNvnRegister("nvnCommandBufferDiscardDepthStencil", (nvn_func_t)nvn_trace_nvnCommandBufferDiscardDepthStencil);
    dmNvnRegister("nvnCommandBufferDownsample", (nvn_func_t)nvn_trace_nvnCommandBufferDownsample);
    dmNvnRegister("nvnCommandBufferTiledDownsample", (nvn_func_t)nvn_trace_nvnCommandBufferTiledDownsample);
    dmNvnRegister("nvnCommandBufferDownsampleTextureView", (nvn_func_t)nvn_trace_nvnCommandBufferDownsampleTextureView);
    dmNvnRegister("nvnCommandBufferTiledDownsampleTextureView", (nvn_func_t)nvn_trace_nvnCommandBufferTiledDownsampleTextureView);
    dmNvnRegister("nvnCommandBufferBarrier", (nvn_func_t)nvn_trace_nvnCommandBufferBarrier);
    dmNvnRegister("nvnCommandBufferWaitSync", (nvn_func_t)nvn_trace_nvnCommandBufferWaitSync);
    dmNvnRegister("nvnCommandBufferFenceSync", (nvn_func_t)nvn_trace_nvnCommandBufferFenceSync);
    dmNvnRegister("nvnCommandBufferSetTexturePool", (nvn_func_t)nvn_trace_nvnCommandBufferSetTexturePool);
    dmNvnRegister("nvnCommandBufferSetSamplerPool", (nvn_func_t)nvn_trace_nvnCommandBufferSetSamplerPool);
    dmNvnRegister("nvnCommandBufferSetShaderScratchMemory", (nvn_func_t)nvn_trace_nvnCommandBufferSetShaderScratchMemory);
    dmNvnRegister("nvnCommandBufferSaveZCullData", (nvn_func_t)nvn_trace_nvnCommandBufferSaveZCullData);
    dmNvnRegister("nvnCommandBufferRestoreZCullData", (nvn_func_t)nvn_trace_nvnCommandBufferRestoreZCullData);
    dmNvnRegister("nvnCommandBufferSetCopyRowStride", (nvn_func_t)nvn_trace_nvnCommandBufferSetCopyRowStride);
    dmNvnRegister("nvnCommandBufferSetCopyImageStride", (nvn_func_t)nvn_trace_nvnCommandBufferSetCopyImageStride);
    dmNvnRegister("nvnCommandBufferGetCopyRowStride", (nvn_func_t)nvn_trace_nvnCommandBufferGetCopyRowStride);
    dmNvnRegister("nvnCommandBufferGetCopyImageStride", (nvn_func_t)nvn_trace_nvnCommandBufferGetCopyImageStride);
    dmNvnRegister("nvnCommandBufferDrawTexture", (nvn_func_t)nvn_trace_nvnCommandBufferDrawTexture);
    dmNvnRegister("nvnProgramSetSubroutineLinkage", (nvn_func_t)nvn_trace_nvnProgramSetSubroutineLinkage);
    dmNvnRegister("nvnCommandBufferSetProgramSubroutines", (nvn_func_t)nvn_trace_nvnCommandBufferSetProgramSubroutines);
    dmNvnRegister("nvnCommandBufferBindCoverageModulationTable", (nvn_func_t)nvn_trace_nvnCommandBufferBindCoverageModulationTable);
    dmNvnRegister("nvnCommandBufferResolveDepthBuffer", (nvn_func_t)nvn_trace_nvnCommandBufferResolveDepthBuffer);
    dmNvnRegister("nvnCommandBufferSetColorReductionEnable", (nvn_func_t)nvn_trace_nvnCommandBufferSetColorReductionEnable);
    dmNvnRegister("nvnCommandBufferSetColorReductionThresholds", (nvn_func_t)nvn_trace_nvnCommandBufferSetColorReductionThresholds);
    dmNvnRegister("nvnCommandBufferPushDebugGroupStatic", (nvn_func_t)nvn_trace_nvnCommandBufferPushDebugGroupStatic);
    dmNvnRegister("nvnCommandBufferPushDebugGroupDynamic", (nvn_func_t)nvn_trace_nvnCommandBufferPushDebugGroupDynamic);
    dmNvnRegister("nvnCommandBufferPushDebugGroup", (nvn_func_t)nvn_trace_nvnCommandBufferPushDebugGroup);
    dmNvnRegister("nvnCommandBufferPopDebugGroup", (nvn_func_t)nvn_trace_nvnCommandBufferPopDebugGroup);
    dmNvnRegister("nvnCommandBufferPopDebugGroupId", (nvn_func_t)nvn_trace_nvnCommandBufferPopDebugGroupId);
    dmNvnRegister("nvnCommandBufferInsertDebugMarkerStatic", (nvn_func_t)nvn_trace_nvnCommandBufferInsertDebugMarkerStatic);
    dmNvnRegister("nvnCommandBufferInsertDebugMarkerDynamic", (nvn_func_t)nvn_trace_nvnCommandBufferInsertDebugMarkerDynamic);
    dmNvnRegister("nvnCommandBufferInsertDebugMarker", (nvn_func_t)nvn_trace_nvnCommandBufferInsertDebugMarker);
    dmNvnRegister("nvnCommandBufferGetMemoryCallback", (nvn_func_t)nvn_trace_nvnCommandBufferGetMemoryCallback);
    dmNvnRegister("nvnCommandBufferGetMemoryCallbackData", (nvn_func_t)nvn_trace_nvnCommandBufferGetMemoryCallbackData);
    dmNvnRegister("nvnCommandBufferIsRecording", (nvn_func_t)nvn_trace_nvnCommandBufferIsRecording);
    dmNvnRegister("nvnSyncInitialize", (nvn_func_t)nvn_trace_nvnSyncInitialize);
    dmNvnRegister("nvnSyncFinalize", (nvn_func_t)nvn_trace_nvnSyncFinalize);
    dmNvnRegister("nvnSyncSetDebugLabel", (nvn_func_t)nvn_trace_nvnSyncSetDebugLabel);
    dmNvnRegister("nvnQueueFenceSync", (nvn_func_t)nvn_trace_nvnQueueFenceSync);
    dmNvnRegister("nvnSyncWait", (nvn_func_t)nvn_trace_nvnSyncWait);
    dmNvnRegister("nvnQueueWaitSync", (nvn_func_t)nvn_trace_nvnQueueWaitSync);
    dmNvnRegister("nvnSyncInitializeFromFencedGLSync", (nvn_func_t)nvn_trace_nvnSyncInitializeFromFencedGLSync);
    dmNvnRegister("nvnSyncCreateGLSync", (nvn_func_t)nvn_trace_nvnSyncCreateGLSync);
    dmNvnRegister("nvnEventBuilderSetDefaults", (nvn_func_t)nvn_trace_nvnEventBuilderSetDefaults);
    dmNvnRegister("nvnEventBuilderSetStorage", (nvn_func_t)nvn_trace_nvnEventBuilderSetStorage);
    dmNvnRegister("nvnEventBuilderGetStorage", (nvn_func_t)nvn_trace_nvnEventBuilderGetStorage);
    dmNvnRegister("nvnEventBuilderGetMemoryPool", (nvn_func_t)nvn_trace_nvnEventBuilderGetMemoryPool);
    dmNvnRegister("nvnEventBuilderGetMemoryOffset", (nvn_func_t)nvn_trace_nvnEventBuilderGetMemoryOffset);
    dmNvnRegister("nvnEventInitialize", (nvn_func_t)nvn_trace_nvnEventInitialize);
    dmNvnRegister("nvnEventFinalize", (nvn_func_t)nvn_trace_nvnEventFinalize);
    dmNvnRegister("nvnEventGetValue", (nvn_func_t)nvn_trace_nvnEventGetValue);
    dmNvnRegister("nvnEventSignal", (nvn_func_t)nvn_trace_nvnEventSignal);
    dmNvnRegister("nvnEventGetMemoryPool", (nvn_func_t)nvn_trace_nvnEventGetMemoryPool);
    dmNvnRegister("nvnEventGetMemoryOffset", (nvn_func_t)nvn_trace_nvnEventGetMemoryOffset);
    dmNvnRegister("nvnCommandBufferWaitEvent", (nvn_func_t)nvn_trace_nvnCommandBufferWaitEvent);
    dmNvnRegister("nvnCommandBufferSignalEvent", (nvn_func_t)nvn_trace_nvnCommandBufferSignalEvent);
    dmNvnRegister("nvnCommandBufferSetStencilCullCriteria", (nvn_func_t)nvn_trace_nvnCommandBufferSetStencilCullCriteria);
    dmNvnRegister("nvnCommandBufferInsertCheckpointStatic", (nvn_func_t)nvn_trace_nvnCommandBufferInsertCheckpointStatic);
    dmNvnRegister("nvnCommandBufferInsertCheckpointDynamic", (nvn_func_t)nvn_trace_nvnCommandBufferInsertCheckpointDynamic);
    dmNvnRegister("nvnDeviceSetAutomaticCheckpointsEnable", (nvn_func_t)nvn_trace_nvnDeviceSetAutomaticCheckpointsEnable);
    dmNvnRegister("nvnDeviceSetAutomaticCheckpointsSamplingInterval", (nvn_func_t)nvn_trace_nvnDeviceSetAutomaticCheckpointsSamplingInterval);
    dmNvnRegister("nvnDeviceSetAutomaticCheckpointsByDebugGroup", (nvn_func_t)nvn_trace_nvnDeviceSetAutomaticCheckpointsByDebugGroup);
    dmNvnRegister("nvnDeviceSetCheckpointCallStacksEnable", (nvn_func_t)nvn_trace_nvnDeviceSetCheckpointCallStacksEnable);
    dmNvnRegister("nvnDeviceSetCheckpointMaxCallStacksDepth", (nvn_func_t)nvn_trace_nvnDeviceSetCheckpointMaxCallStacksDepth);
}
