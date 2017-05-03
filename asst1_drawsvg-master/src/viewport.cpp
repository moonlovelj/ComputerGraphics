#include "viewport.h"

#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 5 (part 2): 
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.
  this->x = x;
  this->y = y;
  this->span = span; 
  float scale = 1.0f / (2 * span);
  svg_2_norm(0, 0) = svg_2_norm(1, 1) = scale;
  svg_2_norm(0, 2) = -(x-span) * scale;
  svg_2_norm(1, 2) = -(y-span) * scale;
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CMU462