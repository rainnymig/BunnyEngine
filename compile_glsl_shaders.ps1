glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/culling.comp            -o ./build/engine-next/Debug/culling_comp.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/reduceDepth.comp        -o ./build/engine-next/Debug/reduce_depth_comp.spv

glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/basicUpdated.frag       -o ./build/engine-next/Debug/basic_updated_frag.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/basicInstanced.vert     -o ./build/engine-next/Debug/basic_instanced_vert.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/culledInstanced.vert    -o ./build/engine-next/Debug/culled_instanced_vert.spv

glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/screenQuad.vert         -o ./build/engine-next/Debug/screen_quad_vert.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/gbuffer.frag            -o ./build/engine-next/Debug/gbuffer_frag.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/basicDeferred.frag      -o ./build/engine-next/Debug/basic_deferred_frag.spv

glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrCulledInstanced.vert -o ./build/engine-next/Debug/pbr_culled_instanced_vert.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrScreenQuad.vert      -o ./build/engine-next/Debug/pbr_screen_quad_vert.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrGBuffer.frag         -o ./build/engine-next/Debug/pbr_gbuffer_frag.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrForward.frag         -o ./build/engine-next/Debug/pbr_forward_frag.spv
glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrDeferred.frag        -o ./build/engine-next/Debug/pbr_deferred_frag.spv

glslc.exe --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/rtbasic.rgen            -o ./build/engine-next/Debug/rtbasic_rgen.spv