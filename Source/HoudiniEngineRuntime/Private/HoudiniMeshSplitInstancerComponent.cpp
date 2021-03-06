/*
* Copyright (c) <2017> Side Effects Software Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include "HoudiniApi.h"
#include "Components/StaticMeshComponent.h"
#include "HoudiniMeshSplitInstancerComponent.h"
#include "HoudiniEngineRuntimePrivatePCH.h"
#if WITH_EDITOR
#include "LevelEditorViewport.h"
#endif

#include "Internationalization.h"
#define LOCTEXT_NAMESPACE HOUDINI_LOCTEXT_NAMESPACE 

UHoudiniMeshSplitInstancerComponent::UHoudiniMeshSplitInstancerComponent( const FObjectInitializer& ObjectInitializer )
: Super( ObjectInitializer )
, InstancedMesh( nullptr )
{
}

void
UHoudiniMeshSplitInstancerComponent::OnComponentDestroyed( bool bDestroyingHierarchy )
{
    ClearInstances();
    Super::OnComponentDestroyed( bDestroyingHierarchy );
}

void
UHoudiniMeshSplitInstancerComponent::Serialize( FArchive & Ar )
{
    Super::Serialize( Ar );
    Ar.UsingCustomVersion( FHoudiniCustomSerializationVersion::GUID );

    Ar << InstancedMesh;
    Ar << OverrideMaterial;
    Ar << Instances;
}

void 
UHoudiniMeshSplitInstancerComponent::AddReferencedObjects( UObject * InThis, FReferenceCollector & Collector )
{
    if ( UHoudiniMeshSplitInstancerComponent * This = Cast< UHoudiniMeshSplitInstancerComponent >( InThis ) )
    {
        Collector.AddReferencedObject( This->InstancedMesh, This );
	Collector.AddReferencedObject( This->OverrideMaterial, This );
        Collector.AddReferencedObjects( This->Instances, This );
    }
}

void 
UHoudiniMeshSplitInstancerComponent::SetInstances( const TArray<FTransform>& InstanceTransforms )
{
#if WITH_EDITOR
    if ( Instances.Num() || InstanceTransforms.Num() )
    {
        const FScopedTransaction Transaction( LOCTEXT( "UpdateInstances", "Update Instances" ) );
        GetOwner()->Modify();
        ClearInstances();

        if( InstancedMesh )
        {
            for( const FTransform& InstanceTransform : InstanceTransforms )
            {
                AddInstance( InstanceTransform );
            }
        }
        else
        {
            HOUDINI_LOG_ERROR( TEXT( "%s: Null InstancedMesh for split instanced mesh override" ), *GetOwner()->GetName() );
        }
    }
#endif
}

int32 
UHoudiniMeshSplitInstancerComponent::AddInstance( const FTransform& InstanceTransform )
{
    UStaticMeshComponent* SMC = NewObject< UStaticMeshComponent >(
	GetOwner(), UStaticMeshComponent::StaticClass(),
	NAME_None, RF_Transactional);

    SMC->SetRelativeTransform(InstanceTransform);
    // Attach created static mesh component to this thing
    SMC->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

    SMC->SetStaticMesh(InstancedMesh);
    SMC->SetVisibility(IsVisible());
    SMC->SetMobility(Mobility);
    if( OverrideMaterial )
    {
	int32 MeshMaterialCount = InstancedMesh->StaticMaterials.Num();
	for( int32 Idx = 0; Idx < MeshMaterialCount; ++Idx )
	    SMC->SetMaterial(Idx, OverrideMaterial);
    }
    SMC->RegisterComponent();

    return Instances.Add(SMC);
}

void 
UHoudiniMeshSplitInstancerComponent::ClearInstances()
{
    for ( auto&& Instance : Instances )
    {
        if ( Instance )
        {
            Instance->ConditionalBeginDestroy();
        }
    }
    Instances.Empty();
}

#undef LOCTEXT_NAMESPACE