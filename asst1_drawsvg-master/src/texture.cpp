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
inline T interpolate(const T& arg1, const T& arg2, float t)
{
	return arg1 + t * (arg2 - arg1);
}

inline Color sip(const MipLevel& tex, int x, int y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > tex.width - 1) x = tex.width - 1;
	if (y > tex.height - 1) y = tex.height - 1;
	int index = 4 * (y * tex.width + x);
	float f = 1.f / 255;
	return Color(tex.texels[index] * f, tex.texels[index+1] * f, tex.texels[index+2] * f, tex.texels[index+3] * f);
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
	
    for(size_t j = 0; j < 4 * mip.width * mip.height; j += 4) {
	  int index = j * 0.25f;
	  int x = index % mip.width;
	  int y = index / mip.width;
	  Color c = 0.25f * sip(tex.mipmap[i - 1], 2*x, 2*y) +
		  0.25f * sip(tex.mipmap[i - 1], 2*x + 1, 2*y) +
		  0.25f * sip(tex.mipmap[i - 1], 2*x, 2*y + 1) +
		  0.25f * sip(tex.mipmap[i - 1], 2*x + 1,2*y + 1);
	  float_to_uint8(&mip.texels[j], &c.r);
    }
  }
}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 6: Implement nearest neighbour interpolation
	if (level < 0 || level >= tex.mipmap.size())
		return Color(1, 0, 1, 1);

	MipLevel mipTex = tex.mipmap[level];
	float x = mipTex.width * u - 0.5f;
	float y = mipTex.height * v - 0.5f;
	int sx = floor(x), sy = floor(y);
	if (x - sx >= 0.5f) ++sx;
	if (y - sy >= 0.5f) ++sy;
  // return magenta for invalid level
	return sip(mipTex, sx, sy);
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  if (level < 0 || level >= tex.mipmap.size())
	  return Color(1, 0, 1, 1);

  // Task 6: Implement bilinear filtering
	MipLevel mipTex = tex.mipmap[level];
	float x = mipTex.width * u - 0.5f;
	float y = mipTex.height * v - 0.5f;
	int sx = floor(x), sy = floor(y);
	float tx = x - sx, ty = y - sy;

	Color c1 = interpolate(sip(mipTex,sx,sy), sip(mipTex, sx+1,sy),tx);
	Color c2 = interpolate(sip(mipTex, sx, sy+1), sip(mipTex, sx + 1, sy+1), tx);
	
  // return magenta for invalid level
	return interpolate(c1, c2, ty);
}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Task 7: Implement trilinear filtering
	float L = max(u_scale * tex.width, v_scale * tex.height);
	float d = log2f(L);
	int level = floor(d);
	if (level < 0) return sample_bilinear(tex, u, v, 0);
	if (level >= tex.mipmap.size() - 1) return sample_bilinear(tex, u, v, tex.mipmap.size() - 1);
	Color c1 = sample_bilinear(tex, u, v, level);
	Color c2 = sample_bilinear(tex, u, v, level + 1);

  // return magenta for invalid level
	return interpolate(c1, c2, d - level);
}

} // namespace CMU462
