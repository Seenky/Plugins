// Fill out your copyright notice in the Description page of Project Settings.


#include "StaticLibraries/MaterialHelpersLibrary.h"

#include "Rendering/SkeletalMeshRenderData.h"

TArray<UMaterialInterface*> UMaterialHelpersLibrary::GetSkeletalMeshMaterialsFromLOD(const int32 LODIndex, const USkeletalMesh* SkeletalMesh)
{
	TArray<UMaterialInterface*> Materials;

	if (!IsValid(SkeletalMesh))
	{
		return Materials;
	}

	const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
	if (!RenderData || !RenderData->LODRenderData.IsValidIndex(LODIndex))
	{
		return Materials;
	}

	const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[LODIndex];
	const TArray<FSkeletalMaterial>& SkeletalMaterials = SkeletalMesh->GetMaterials();

	for (const FSkelMeshRenderSection& Section : LODRenderData.RenderSections)
	{
		UMaterialInterface* Material = nullptr;
		
		if (SkeletalMaterials.IsValidIndex(Section.MaterialIndex))
		{
			Material = SkeletalMaterials[Section.MaterialIndex].MaterialInterface;
		}
		
		if (IsValid(Material))
		{
			Materials.Add(Material);
		}
	}

	return Materials;
}
