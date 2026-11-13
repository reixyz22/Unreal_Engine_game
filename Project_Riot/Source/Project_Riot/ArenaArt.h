#pragma once

#include "CoreMinimal.h"

namespace ArenaArt
{
	// Sprites import at 1 pixel per unit; the arena is drawn at 4 units per pixel.
	constexpr float PixelScale = 4.f;
	// The floor art is 768 px square with a 16 px wall band, so these follow from PixelScale.
	constexpr float ArenaHalfSize = 768.f * PixelScale * 0.5f;
	constexpr float WallThickness = 16.f * PixelScale;

	// The flail art is drawn larger than the characters; shrink it to fit her hand.
	constexpr float FlailScale = 0.6f;

	// M_ArenaSprite draws a 1px outline in this color; alpha 0 hides it.
	inline const FName OutlineParam(TEXT("OutlineColor"));
	inline const FLinearColor HoverOutline(4.f, 0.15f, 0.1f, 1.f);
}
