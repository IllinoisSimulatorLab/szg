#ifndef __SELF_H__
#define __SELF_H__

/*
 * Self.h
 * 	Draws the red "skier" in SchpRel
 */

#include "SchprelScene.h"

/// Draws the red "skier" in SchpRel
class Self : public SchprelScene {
  public:
	Self();
	void drawAll();
	void updateAll(s_updateValues &updateValues);

  /// The original, constant vertices of the skier
  float self[120][3];
  /// The transformed vertices we use to draw
  float selfdraw[120][3];
};

#endif
