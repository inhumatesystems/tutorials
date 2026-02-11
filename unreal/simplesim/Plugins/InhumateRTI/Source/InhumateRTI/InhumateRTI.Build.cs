// Copyright 2025 Inhumate AB. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class InhumateRTI : ModuleRules
{
	public InhumateRTI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        var ThirdPartyDir = Path.Combine(ModuleDirectory, "..", "ThirdParty");
        PublicIncludePaths.Add(Path.Combine(ThirdPartyDir, "Include"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
	        PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Win64", "libprotobuf.lib"));
	        PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Win64", "inhumaterti.lib"));
	        
	        // Oh but why isn't this picked up by the "OpenSSL" in PrivateDependencyModuleNames below??
	        // Here's copy-pasta from OpenSSL module build.cs instead:
	        
	        string OpenSSLPath = Path.Combine(Target.UEThirdPartySourceDirectory, "OpenSSL", "1.1.1t");
            string PlatformSubdir = Target.Platform.ToString();
            string ConfigFolder = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT) ? "Debug" : "Release";
			string VSVersion = "VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName();
	        PublicIncludePaths.Add(Path.Combine(OpenSSLPath, "include", PlatformSubdir, VSVersion));
	        string LibPath = Path.Combine(OpenSSLPath, "lib", PlatformSubdir, VSVersion, ConfigFolder);
	        PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libssl.lib"));
	        PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libcrypto.lib"));
	        PublicSystemLibraries.Add("crypt32.lib");
	        
	        /*

			Old DLL build version:

            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Win64", "libprotobuf.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Win64", "inhumaterti_shared_protos.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Win64", "inhumaterti_shared.lib"));

            var DllPath = Path.Combine(ThirdPartyDir, "Win64");
            foreach (var Dll in new string[] { "libprotobuf.dll", "inhumaterti_shared.dll", "inhumaterti_shared_protos.dll" })
            {
                // Unfortunately, we can't delay-load this because:
                // UATHelper: Packaging (Windows (64-bit)):     LINK : fatal error LNK1194: cannot delay-load 'inhumaterti_shared_protos.dll' due to import of data symbol '"__declspec(dllimport) private: static class inhumate::rti::proto::EntityPosition ...
                // Apparently, DLLs that use other DLLs that expose data can't be delay-loaded... 
                // So DLLs need to be copied to the binaries folder, which also messes up packaging.
                // See https://docs.unrealengine.com/en-US/Programming/BuildTools/UnrealBuildTool/ThirdPartyLibraries/index.html
                // PublicDelayLoadDLLs.Add(Dll);

                RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", Dll), Path.Combine(DllPath, Dll));
            }

            PublicDefinitions.Add("INHUMATE_RTI_SHARED");
            PublicDefinitions.Add("PROTOBUF_USE_DLLS");
            */
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Linux", "libprotobuf.a"));
            PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyDir, "Linux", "libinhumaterti.a"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string dylibPath = Path.Combine(ThirdPartyDir, "Mac");
            PublicAdditionalLibraries.Add(Path.Combine(dylibPath, "libprotobuf.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(dylibPath, "libinhumaterti_shared.dylib"));
            PublicAdditionalLibraries.Add(Path.Combine(dylibPath, "libinhumaterti_shared_protos.dylib"));
            RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "libprotobuf.dylib"), Path.Combine(dylibPath, "libprotobuf.dylib"));
            RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "libinhumaterti_shared.dylib"), Path.Combine(dylibPath, "libinhumaterti_shared.dylib"));
            RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "libinhumaterti_shared_protos.dylib"), Path.Combine(dylibPath, "libinhumaterti_shared_protos.dylib"));
        }

        PublicDefinitions.Add("GOOGLE_PROTOBUF_NO_RTTI=1");
        PublicDefinitions.Add("PROTOBUF_INLINE_NOT_IN_HEADERS=0");
        PublicDefinitions.Add("GOOGLE_PROTOBUF_USE_UNALIGNED=0");
        PublicDefinitions.Add("GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER=0");
        PublicDefinitions.Add("_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS=1");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
                "InputCore",
				"Projects",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "OpenSSL",
				// ... add private dependencies that you statically link with here ...	
			}
            );
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
