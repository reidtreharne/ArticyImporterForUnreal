//  
// Copyright (c) articy Software GmbH & Co. KG. All rights reserved.  
// Licensed under the MIT license. See LICENSE file in the project root for full license information.  
//


#include "ArticyImporter.h"
#include "ArticyPluginSettings.h"
#include "ArticyPluginSettingsCustomization.h"
#include "ArticyRefCustomization.h"
#include "Developer/Settings/Public/ISettingsModule.h"
#include "Developer/Settings/Public/ISettingsSection.h"
#include "Developer/Settings/Public/ISettingsContainer.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "ArticyImporterHelpers.h"
#include <Dialogs.h>
#include <SWindow.h>
#include "ArticyImporterFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogArticyImporter)

#define LOCTEXT_NAMESPACE "FArticyImporterModule"

void FArticyImporterModule::StartupModule()
{
	RegisterPluginSettings();

	// register custom details for ArticyRef struct
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("ArticyRef", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FArticyRefCustomization::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FArticyImporterModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UnregisterPluginSettings();
	}
}

void FArticyImporterModule::RegisterPluginSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		ISettingsSectionPtr SettingsSectionPtr = SettingsModule->RegisterSettings("Project", "Plugins", "ArticyImporter",
			LOCTEXT("Name", "Articy Importer"),
			LOCTEXT("Description", "Articy Importer Configuration."),
			GetMutableDefault<UArticyPluginSettings>()
		);
	}

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("ArticyPluginSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FArticyPluginSettingsCustomization::MakeInstance));
}

void FArticyImporterModule::UnregisterPluginSettings()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "ArticyImporter");
	}
}

bool FArticyImporterModule::IsImportQueued()
{
	return bIsImportQueued;
}

void FArticyImporterModule::QueueImport()
{
	bIsImportQueued = true;
	FOnMsgDlgResult OnDialogClosed;
	FText Message = LOCTEXT("ImportWhilePlaying", "To import articy:draft data, the play mode has to be quit. Import will begin after exiting play.");
	FText Title = LOCTEXT("ImportWhilePlaying_Title", "Import not possible");
	TSharedRef<SWindow> window = OpenMsgDlgInt_NonModal(EAppMsgType::Ok, Message, Title, OnDialogClosed);
	window->BringToFront(true);
	QueuedImportHandle = FEditorDelegates::EndPIE.AddRaw(this, &FArticyImporterModule::TriggerQueuedImport);
}

void FArticyImporterModule::UnqueueImport()
{
	FEditorDelegates::EndPIE.Remove(QueuedImportHandle);
	QueuedImportHandle.Reset();
	bIsImportQueued = false;
}

void FArticyImporterModule::TriggerQueuedImport(bool b)
{
	UArticyImportData* importData = nullptr;
	FArticyImporterFunctionLibrary::ForceCompleteReimport(importData);
	// important to unqueue in the end to reset the state
	UnqueueImport();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FArticyImporterModule, ArticyImporter)