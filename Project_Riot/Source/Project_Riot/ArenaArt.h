#pragma once

#include "CoreMinimal.h"

namespace ArenaArt
{
	// Sprites import at 1 pixel per unit; the arena is drawn at 4 units per pixel.
	constexpr float PixelScale = 4.f;

	// The flail art is drawn larger than the characters; shrink it to fit her hand.
	constexpr float FlailScale = 0.6f;
}
