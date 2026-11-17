#include "StoryScript.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FStoryScript FStoryScript::Load(const FString& Name)
{
	FStoryScript Script;
	TArray<FString> FileLines;
	FFileHelper::LoadFileToStringArray(FileLines, *(FPaths::ProjectContentDir() / TEXT("Story") / Name + TEXT(".txt")));

	TArray<FStoryLine>* Current = nullptr;
	for (FString Line : FileLines)
	{
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		if (Line.StartsWith(TEXT("[")) && Line.EndsWith(TEXT("]")))
		{
			Current = &Script.Sections.FindOrAdd(Line.Mid(1, Line.Len() - 2));
			continue;
		}

		FString Speaker, Text;
		if (Current && Line.Split(TEXT(":"), &Speaker, &Text))
		{
			Current->Add({ Speaker.TrimStartAndEnd(), Text.TrimStartAndEnd() });
		}
	}
	return Script;
}

TArray<FStoryLine> FStoryScript::Get(const FString& Section) const
{
	if (const TArray<FStoryLine>* Lines = Sections.Find(Section))
	{
		return *Lines;
	}
	return { { TEXT("Script"), FString::Printf(TEXT("(no [%s] section written yet)"), *Section) } };
}
