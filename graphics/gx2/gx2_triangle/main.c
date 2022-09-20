#include <gfd.h>
#include <gx2/draw.h>
#include <gx2/shaders.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2r/draw.h>
#include <gx2r/buffer.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <whb/proc.h>
#include <whb/gfx.h>
#include <whb/log.h>
#include <whb/log_udp.h>

#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/debug.h>

static const float sPositionData[] =
{
    1.0f, -1.0f,  0.0f, 1.0f,
    0.0f,  1.0f,  0.0f, 1.0f,
   -1.0f, -1.0f,  1.0f, 1.0f,
};

static const float sColourData[] =
{
   1.0f,  0.0f,  0.0f, 1.0f,
   0.0f,  1.0f,  0.0f, 1.0f,
   0.0f,  0.0f,  1.0f, 1.0f,
};

int main(int argc, char **argv)
{
   WHBLogUdpInit();
   WHBProcInit();
   WHBGfxInit();

   // Read the shader from the bundled content
   FILE *gshFile = fopen("/vol/content/pos_col_shader.gsh", "r");
   if (!gshFile) {
      OSFatal("Failed to open shader");
      return -1;
   }

   // Get the file size
   fseek(gshFile, 0, SEEK_END);
   uint32_t gshFileSize = ftell(gshFile);
   rewind(gshFile);

   void *gshFileData = malloc(gshFileSize);
   if (!gshFileData) {
      OSFatal("Out of memory");
      return -1;
   }

   if (fread(gshFileData, 1, gshFileSize, gshFile) != gshFileSize) {
      OSFatal("Failed to read shader data");
      return -1;
   }

   fclose(gshFile);

   WHBGfxShaderGroup group = { 0 };
   if (!WHBGfxLoadGFDShaderGroup(&group, 0, gshFileData)) {
      OSFatal("WHBGfxLoadGFDShaderGroup returned FALSE");
      return -1;
   }

   WHBGfxInitShaderAttribute(&group, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
   WHBGfxInitShaderAttribute(&group, "aColour", 1, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
   WHBGfxInitFetchShader(&group);

   free(gshFileData);

   // Set vertex position
   GX2RBuffer positionBuffer = { 0 };
   positionBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER |
                          GX2R_RESOURCE_USAGE_CPU_READ |
                          GX2R_RESOURCE_USAGE_CPU_WRITE |
                          GX2R_RESOURCE_USAGE_GPU_READ;
   positionBuffer.elemSize = 4 * 4;
   positionBuffer.elemCount = 3;
   GX2RCreateBuffer(&positionBuffer);
   void *buffer = GX2RLockBufferEx(&positionBuffer, 0);
   memcpy(buffer, sPositionData, positionBuffer.elemSize * positionBuffer.elemCount);
   GX2RUnlockBufferEx(&positionBuffer, 0);

   // Set vertex colour
   GX2RBuffer colourBuffer = { 0 };
   colourBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER |
                        GX2R_RESOURCE_USAGE_CPU_READ |
                        GX2R_RESOURCE_USAGE_CPU_WRITE |
                        GX2R_RESOURCE_USAGE_GPU_READ;
   colourBuffer.elemSize = 4 * 4;
   colourBuffer.elemCount = 3;
   GX2RCreateBuffer(&colourBuffer);
   buffer = GX2RLockBufferEx(&colourBuffer, 0);
   memcpy(buffer, sColourData, colourBuffer.elemSize * colourBuffer.elemCount);
   GX2RUnlockBufferEx(&colourBuffer, 0);

   WHBLogPrintf("Begin rendering...");
   while (WHBProcIsRunning()) {
      // Animate colours...
      float *colours = (float *)GX2RLockBufferEx(&colourBuffer, 0);
      colours[0] = 1.0f;
      colours[1] = colours[1] >= 1.0f ? 0.0f : (colours[1] + 0.01f);
      colours[2] = colours[2] >= 1.0f ? 0.0f : (colours[2] + 0.01f);
      colours[3] = 1.0f;

      colours[4] = colours[4] >= 1.0f ? 0.0f : (colours[4] + 0.01f);
      colours[5] = 1.0f;
      colours[6] = colours[6] >= 1.0f ? 0.0f : (colours[6] + 0.01f);
      colours[7] = 1.0f;

      colours[8] = colours[8] >= 1.0f ? 0.0f : (colours[8] + 0.01f);
      colours[9] = colours[9] >= 1.0f ? 0.0f : (colours[9] + 0.01f);
      colours[10] = 1.0f;
      colours[11] = 1.0f;
      GX2RUnlockBufferEx(&colourBuffer, 0);

      // Render!
      WHBGfxBeginRender();

      WHBGfxBeginRenderTV();
      WHBGfxClearColor(0.0f, 0.0f, 1.0f, 1.0f);
      GX2SetFetchShader(&group.fetchShader);
      GX2SetVertexShader(group.vertexShader);
      GX2SetPixelShader(group.pixelShader);
      GX2RSetAttributeBuffer(&positionBuffer, 0, positionBuffer.elemSize, 0);
      GX2RSetAttributeBuffer(&colourBuffer, 1, colourBuffer.elemSize, 0);
      GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
      WHBGfxFinishRenderTV();

      WHBGfxBeginRenderDRC();
      WHBGfxClearColor(1.0f, 0.0f, 1.0f, 1.0f);
      GX2SetFetchShader(&group.fetchShader);
      GX2SetVertexShader(group.vertexShader);
      GX2SetPixelShader(group.pixelShader);
      GX2RSetAttributeBuffer(&positionBuffer, 0, positionBuffer.elemSize, 0);
      GX2RSetAttributeBuffer(&colourBuffer, 1, colourBuffer.elemSize, 0);
      GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
      WHBGfxFinishRenderDRC();

      WHBGfxFinishRender();
   }

   WHBLogPrintf("Exiting...");
   GX2RDestroyBufferEx(&positionBuffer, 0);
   GX2RDestroyBufferEx(&colourBuffer, 0);

   WHBGfxShutdown();
   WHBProcShutdown();
   WHBLogUdpDeinit();
   return 0;
}
