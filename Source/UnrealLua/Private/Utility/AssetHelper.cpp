#include "Utility/AssetHelper.h"

FString FAssetHelper::ParseToFullPath(const FString& path)
{
	if(path.IsEmpty())
	{
		return {};
	}
	FString final = path;
	//Fix class path, in case user did not use /Game/dir/assetname.assetname, but /Game/dir/assetname  

	//get only part after last slash
			
	const int32 slashIndex = path.Find(TEXT("/"),ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	//if no slash was found, slashIndex is -1 -> .Mid would go from index 0
	const FString assetName = path.Mid(slashIndex + 1);

	//assetName may be 'assetname.assetname' or 'assetname'
	//if it does not have a dot, assume user only used 'assetname'
	//-> add dot and duplicate asset name
			
	const int32 foundDelimiter = assetName.Find(TEXT("."),ESearchCase::IgnoreCase, ESearchDir::FromEnd);;
	if(foundDelimiter == INDEX_NONE)
	{
		final.Append("." + assetName);
	}
	return final;
}