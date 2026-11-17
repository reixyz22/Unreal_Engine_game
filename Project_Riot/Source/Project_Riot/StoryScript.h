#pragma once

#include "CoreMinimal.h"

struct FStoryLine
{
	FString Speaker;
	FString Text;
};

// Dialog loaded from a plain text file in Content/Story, so the writing can be edited
// without compiling. Format:
//   # comment
//   [section_name]
//   Speaker: what they say
class FStoryScript
{
public:
	// Loads Content/Story/<Name>.txt. Missing files give an empty script.
	static FStoryScript Load(const FString& Name);

	// The lines of a section, or one line naming the missing section so gaps are easy to spot.
	TArray<FStoryLine> Get(const FString& Section) const;

private:
	TMap<FString, TArray<FStoryLine>> Sections;
};
