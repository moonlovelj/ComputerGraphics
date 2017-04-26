#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


	inline int round(float x)
	{
		return int(x + 0.5f);
	}

	inline float fpart(float x)
	{
		if (x < 0.0f)
			return 1 - (x - floor(x));

		return x - floor(x);
	}

	inline float rfpart(float x)
	{
		return 1 - fpart(x);
	}

// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = canvas_to_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y++;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y++;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y--;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y--;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  if (nullptr != supersample_target)
  {
	  delete supersample_target;
	  supersample_target = nullptr;
  }
  supersample_target = reinterpret_cast<unsigned char*>(::operator new(4 * target_w * target_h * sample_rate * sample_rate));
  memset(supersample_target, 255, 4 * target_w * target_h * sample_rate * sample_rate);
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  if (nullptr != supersample_target)
  {
	  delete supersample_target;
	  supersample_target = nullptr;
  }
  supersample_target = reinterpret_cast<unsigned char*>(::operator new(4 * target_w * target_h * sample_rate * sample_rate));
  memset(supersample_target, 255, 4 * target_w * target_h * sample_rate * sample_rate);
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  // fill sample - NOT doing alpha blending!
//   render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
//   render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
//   render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
//   render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);
  int square = sample_rate * sample_rate;
  int start = (sy * target_w + sx)*square;
  int end = start + square;
  for (int i = start; i < end; i++)
  {
	  supersample_target[4 * i] = (uint8_t)(color.r * 255);
	  supersample_target[4 * i + 1] = (uint8_t)(color.g * 255);
	  supersample_target[4 * i + 2] = (uint8_t)(color.b * 255);
	  supersample_target[4 * i + 3] = (uint8_t)(color.a * 255);
  }
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
  // Task 2: 
  // Implement line rasterization
	bool steep = abs(y1 - y0) > abs(x1 - x0);

	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1)
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	float dx = x1 - x0;
	float dy = y1 - y0;
	float gradient = 1.0f;
	if (dx != 0.0f)
		gradient = dy / dx;
	
	int xend = round(x0);
	float yend = y0 + gradient*(xend - x0);
	float xgap = rfpart(x0 + 0.5f);
	int xpx11 = xend;
	int ypx11 = int(yend);

	if (steep)
	{
		rasterize_point(ypx11, xpx11, rfpart(yend)*xgap*color);
		rasterize_point(ypx11+1, xpx11, fpart(yend)*xgap*color);
	}
	else
	{
		rasterize_point(xpx11, ypx11, rfpart(yend)*xgap*color);
		rasterize_point(xpx11, ypx11+1, fpart(yend)*xgap*color);
	}

	float intery = yend + gradient;

	xend = round(x1);
	yend = y1 + gradient*(xend - x1);
	xgap = fpart(x1 + 0.5f);
	int xpx12 = xend;

	int ypx12 = int(yend);
	if (steep)
	{
		rasterize_point(ypx12, xpx12, rfpart(yend)*xgap*color);
		rasterize_point(ypx12 + 1, xpx12, fpart(yend)*xgap*color);
	}
	else
	{
		rasterize_point(xpx12, ypx12, rfpart(yend)*xgap*color);
		rasterize_point(xpx12, ypx12 + 1, fpart(yend)*xgap*color);
	}

	if (steep)
	{
		for (int x = xpx11+1; x < xpx12; x++)
		{
			rasterize_point(int(intery), x, rfpart(intery)*color);
			rasterize_point(int(intery)+1, x, fpart(intery)*color);
			intery = intery + gradient;
		}
	}
	else
	{
		for (int x = xpx11+1; x < xpx12; x++)
		{
			rasterize_point(x, int(intery), rfpart(intery)*color);
			rasterize_point(x, int(intery) + 1, fpart(intery)*color);
			intery = intery + gradient;
		}
	}
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization
	float dY0 = y1 - y0, dY1 = y2 - y1, dY2 = y0 - y2;
	float dX0 = x1 - x0, dX1 = x2 - x1, dX2 = x0 - x2;
	float minX = x0 < x1 ? x0 : x1; minX = minX < x2 ? minX : x2;
	float maxX = x0 > x1 ? x0 : x1; maxX = maxX > x2 ? maxX : x2;
	float minY = y0 < y1 ? y0 : y1; minY = minY < y2 ? minY : y2;
	float maxY = y0 > y1 ? y0 : y1; maxY = maxY > y2 ? maxY : y2;
	float gap = 1.0f / sample_rate;

	for (float y = minY; y < maxY; y++)
	{
		for (float x = minX; x <= maxX; x++)
		{
			int sx = (int)floor(x);
			int sy = (int)floor(y);
			if (sx < 0 || sx >= target_w) continue;
			if (sy < 0 || sy >= target_h) continue;
			for (int iy = 0; iy < sample_rate; iy++)
			{
				for (int ix = 0; ix < sample_rate; ix++)
				{
					float fx = sx * sample_rate + 0.5 + ix;
					float fy = sy * sample_rate + 0.5 + iy;
					if ((fx*gap - x0)*dY0 - (fy*gap - y0)*dX0 <= 0 &&
						(fx*gap - x1)*dY1 - (fy*gap - y1)*dX1 <= 0 &&
						(fx*gap - x2)*dY2 - (fy*gap - y2)*dX2 <= 0)
					{
						int index = (sx + sy * target_w) * sample_rate * sample_rate + iy * sample_rate + ix;
						supersample_target[4 * index] = (uint8_t)(color.r * 255);
						supersample_target[4 * index + 1] = (uint8_t)(color.g * 255);
						supersample_target[4 * index + 2] = (uint8_t)(color.b * 255);
						supersample_target[4 * index + 3] = (uint8_t)(color.a * 255);
					}
				}
			}

		}
	}
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  int square = sample_rate * sample_rate ;
  float denominator = 1.0f / square;

  for (int y = 0; y < target_h; y++)
  {
	  for (int x = 0; x < target_w; x++)
	  {
		  unsigned short r = 0, g = 0, b = 0, a = 0;
		  int start = (y * target_w  + x)*square;
		  int end = start + square;
		  for (int i = start;i < end; i++)
		  {
			  r += supersample_target[i*4];
			  g += supersample_target[i*4 + 1];
			  b += supersample_target[i*4 + 2];
			  a += supersample_target[i*4 + 3];
		  }
		  r *= denominator; g *= denominator; b *= denominator; a *= denominator;
		  render_target[4 * (x + y * target_w)] = r;
		  render_target[4 * (x + y * target_w) + 1] = g;
		  render_target[4 * (x + y * target_w) + 2] = b;
		  render_target[4 * (x + y * target_w) + 3] = a;
	  }
  }
}


} // namespace CMU462
