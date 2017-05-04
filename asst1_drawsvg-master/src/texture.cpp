#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

template <typename T>
inline T interpolate(T arg1, T arg2, float t)
{
	return arg1 + t * (arg2 - arg1);
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE(sky): 
  // The starter code allocates the mip levels and generates a level 
  // map simply fills each level with a color that differs from its
  // neighbours'. The reference solution uses trilinear filtering
  // and it will only work when you have mipmaps.

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    Color c = colors[i % 3];
    MipLevel& mip = tex.mipmap[i];

    for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
      float_to_uint8( &mip.texels[i], &c.r );
    }
  }

}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 6: Implement nearest neighbour interpolation
	MipLevel mipTex = tex.mipmap[level];
	float x = mipTex.width * u;
	float y = mipTex.height * v;
	int sx = floor(x);
	if (x - sx >= 0.5f) ++sx;
	int sy = floor(y);
	if (y - sy >= 0.5f) ++sy;

	int index = (sy * mipTex.width + sx) * 4;
	float f = 1.0f / 255;
	float r = mipTex.texels[index] * f;
	float g = mipTex.texels[index + 1] * f;
	float b = mipTex.texels[index + 2] * f;
	float a = mipTex.texels[index + 3] * f;
  // return magenta for invalid level
	return Color(r, g, b, a);
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 6: Implement bilinear filtering
	MipLevel mipTex = tex.mipmap[level];
	float x = mipTex.width * u;
	float y = mipTex.height * v;
	int sx = floor(x), sy = floor(y);
	float tx = x - sx, ty = y - sy;

	int r1 = interpolate(mipTex.texels[4 * (sy * mipTex.width + sx)], mipTex.texels[4 * (sy * mipTex.width + sx + 1)],tx);
	int g1 = interpolate(mipTex.texels[4 * (sy * mipTex.width + sx) + 1], mipTex.texels[4 * (sy * mipTex.width + sx + 1) + 1], tx);
	int b1 = interpolate(mipTex.texels[4 * (sy * mipTex.width + sx) + 2], mipTex.texels[4 * (sy * mipTex.width + sx + 1) + 2], tx);
	int a1 = interpolate(mipTex.texels[4 * (sy * mipTex.width + sx) + 3], mipTex.texels[4 * (sy * mipTex.width + sx + 1) + 3], tx);

	int r2 = interpolate(mipTex.texels[4 * ((sy + 1) * mipTex.width + sx)], mipTex.texels[4 * ((sy + 1) * mipTex.width + sx + 1)], tx);
	int g2 = interpolate(mipTex.texels[4 * ((sy + 1) * mipTex.width + sx) + 1], mipTex.texels[4 * ((sy + 1) * mipTex.width + sx + 1) + 1], tx);
	int b2 = interpolate(mipTex.texels[4 * ((sy + 1) * mipTex.width + sx) + 2], mipTex.texels[4 * ((sy + 1) * mipTex.width + sx + 1) + 2], tx);
	int a2 = interpolate(mipTex.texels[4 * ((sy + 1) * mipTex.width + sx) + 3], mipTex.texels[4 * ((sy + 1) * mipTex.width + sx + 1) + 3], tx);

	int r = interpolate(r1, r2, ty);
	int g = interpolate(g1, g2, ty);
	int b = interpolate(b1, b2, ty);
	int a = interpolate(a1, a2, ty);

	float f = 1.f / 255;
	
  // return magenta for invalid level
	return Color(r * f, g * f, b * f, a * f);
}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Task 7: Implement trilinear filtering

  // return magenta for invalid level
  return Color(1,0,1,1);

}

} // namespace CMU462
