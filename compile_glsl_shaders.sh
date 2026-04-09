glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/culling.comp             -o ./build/engine-next/Debug/culling_comp.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/reduceDepth.comp         -o ./build/engine-next/Debug/reduce_depth_comp.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/basicUpdated.frag        -o ./build/engine-next/Debug/basic_updated_frag.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/basicInstanced.vert      -o ./build/engine-next/Debug/basic_instanced_vert.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/culledInstanced.vert     -o ./build/engine-next/Debug/culled_instanced_vert.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/screenQuad.vert          -o ./build/engine-next/Debug/screen_quad_vert.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/gbuffer.frag             -o ./build/engine-next/Debug/gbuffer_frag.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/basicDeferred.frag       -o ./build/engine-next/Debug/basic_deferred_frag.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrCulledInstanced.vert  -o ./build/engine-next/Debug/pbr_culled_instanced_vert.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrScreenQuad.vert       -o ./build/engine-next/Debug/pbr_screen_quad_vert.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrGBuffer.frag          -o ./build/engine-next/Debug/pbr_gbuffer_frag.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrForward.frag          -o ./build/engine-next/Debug/pbr_forward_frag.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/pbrDeferred.frag         -o ./build/engine-next/Debug/pbr_deferred_frag.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/rtbasic.rgen             -o ./build/engine-next/Debug/rtbasic_rgen.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/rtbasic.rmiss            -o ./build/engine-next/Debug/rtbasic_rmiss.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/rtshadow.rmiss           -o ./build/engine-next/Debug/rtshadow_rmiss.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/rtshadow.rchit           -o ./build/engine-next/Debug/rtshadow_rchit.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/sky.comp                 -o ./build/engine-next/Debug/sky_comp.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/texturePreview.frag      -o ./build/engine-next/Debug/texture_preview_frag.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/finalOutput.frag         -o ./build/engine-next/Debug/final_output_frag.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/waveSpectrum.comp        -o ./build/engine-next/Debug/wave_spectrum_comp.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/waveTimeSpectrum.comp    -o ./build/engine-next/Debug/wave_time_spectrum_comp.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/fftBitReverse.comp       -o ./build/engine-next/Debug/fft_bit_reverse_comp.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/fftPingPong.comp         -o ./build/engine-next/Debug/fft_ping_pong_comp.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/waveSpectrumTransform.comp   -o ./build/engine-next/Debug/wave_spectrum_transform_comp.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/waveConstruct.comp       -o ./build/engine-next/Debug/wave_construct_comp.spv

glslc --target-spv=spv1.5 ./lib/rendering/vulkan/shader/wave.mesh                   -o ./build/engine-next/Debug/wave_mesh.spv
glslc --target-spv=spv1.5 ./lib/rendering/vulkan/shader/wavefft.mesh                -o ./build/engine-next/Debug/wave_fft_mesh.spv
glslc --target-spv=spv1.5 ./lib/rendering/vulkan/shader/wave.frag                   -o ./build/engine-next/Debug/wave_frag.spv

glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/transparentAccum.frag    -o ./build/engine-next/Debug/transparent_accum_frag.spv
glslc --target-env=vulkan1.3 ./lib/rendering/vulkan/shader/transparentComp.frag     -o ./build/engine-next/Debug/transparent_comp_frag.spv
