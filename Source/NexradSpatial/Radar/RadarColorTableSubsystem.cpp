// Copyright

#include "RadarColorTableSubsystem.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "TextureResource.h"
#include "Math/Color.h"

void URadarColorTableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FString RootDir = FPaths::ProjectDir() + TEXT("ColorTables/");
    FString ReflDir = RootDir + TEXT("Reflectivity/");
    FString VelDir = RootDir + TEXT("Velocity/");

    IFileManager& FileManager = IFileManager::Get();
    UE_LOG(LogTemp, Warning, TEXT("RadarColorTableSubsystem: Initializing... RootDir is %s"), *RootDir);

    if (!FileManager.DirectoryExists(*ReflDir))
    {
        FileManager.MakeDirectory(*ReflDir, true);
    }
    
    if (!FileManager.DirectoryExists(*VelDir))
    {
        FileManager.MakeDirectory(*VelDir, true);
    }

    ScanAndLoadFolder(ReflDir, ReflectivityTables);
    ScanAndLoadFolder(VelDir, VelocityTables);
}

void URadarColorTableSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void URadarColorTableSubsystem::ScanAndLoadFolder(const FString& FolderPath, TMap<FString, FCustomColorTableData>& OutTables)
{
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *(FolderPath + TEXT("*.*")), true, false);

    UE_LOG(LogTemp, Warning, TEXT("RadarColorTableSubsystem: Scanning folder: %s - Found %d files."), *FolderPath, Files.Num());

    for (const FString& File : Files)
    {
        if (File.EndsWith(TEXT(".csv"), ESearchCase::IgnoreCase) || File.EndsWith(TEXT(".pal"), ESearchCase::IgnoreCase))
        {
            FString FullPath = FolderPath + File;
            FCustomColorTableData TableData;
            if (ParseColorTableFile(FullPath, TableData))
            {
                FString Name = FPaths::GetBaseFilename(File);
                OutTables.Add(Name, TableData);
                UE_LOG(LogTemp, Warning, TEXT("RadarColorTableSubsystem: Successfully loaded color table '%s'"), *Name);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RadarColorTableSubsystem: Failed to parse color table '%s'"), *File);
            }
        }
    }
}

bool URadarColorTableSubsystem::ParseColorTableFile(const FString& FilePath, FCustomColorTableData& OutData)
{
    UE_LOG(LogTemp, Warning, TEXT("RadarColorTableSubsystem: Reading file %s"), *FilePath);
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("RadarColorTableSubsystem: Could not load file to string array."));
        return false;
    }

    struct FColorPoint {
        float Value;
        FLinearColor Color;
    };
    TArray<FColorPoint> Points;

    for (const FString& Line : Lines)
    {
        FString Trimmed = Line.TrimStartAndEnd();
        // Skip empty lines, standard comments (#), and AWIPS comments (;)
        if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT("#")) || Trimmed.StartsWith(TEXT(";")))
        {
            continue;
        }

        // Handle AWIPS .pal format which prefixes lines with "Color:"
        if (Trimmed.StartsWith(TEXT("Color:"), ESearchCase::IgnoreCase))
        {
            Trimmed = Trimmed.RightChop(6).TrimStartAndEnd();
        }

        // Replace any commas with spaces so we can unified-split by space
        Trimmed = Trimmed.Replace(TEXT(","), TEXT(" "));
        // Also replace tabs with spaces just in case
        Trimmed = Trimmed.Replace(TEXT("\t"), TEXT(" "));

        TArray<FString> Parts;
        // Split by spaces, removing empty elements
        Trimmed.ParseIntoArray(Parts, TEXT(" "), true);
        
        if (Parts.Num() >= 4)
        {
            float Val = FCString::Atof(*Parts[0]);
            float R = FCString::Atof(*Parts[1]);
            float G = FCString::Atof(*Parts[2]);
            float B = FCString::Atof(*Parts[3]);

            Points.Add({Val, FLinearColor(R, G, B, 1.0f)});
        }
    }

    if (Points.Num() < 2)
    {
        UE_LOG(LogTemp, Error, TEXT("RadarColorTableSubsystem: Not enough valid color points found. Only found %d"), Points.Num());
        return false;
    }

    Points.Sort([](const FColorPoint& A, const FColorPoint& B) {
        return A.Value < B.Value;
    });

    bool bNeedsNormalizeRGB = false;
    for (const auto& Pt : Points)
    {
        if (Pt.Color.R > 1.0f || Pt.Color.G > 1.0f || Pt.Color.B > 1.0f)
        {
            bNeedsNormalizeRGB = true;
            break;
        }
    }

    if (bNeedsNormalizeRGB)
    {
        for (auto& Pt : Points)
        {
            Pt.Color.R /= 255.0f;
            Pt.Color.G /= 255.0f;
            Pt.Color.B /= 255.0f;
        }
    }

    float MinVal = Points[0].Value;
    float MaxVal = Points.Last().Value;

    OutData.ColorData.SetNumUninitialized(16384 * 4);
    for (int32 i = 0; i < 16384; i++)
    {
        float T = (float)i / 16383.0f;
        float TargetVal = FMath::Lerp(MinVal, MaxVal, T);

        FLinearColor InterpColor = Points.Last().Color;
        for (int32 ptIdx = 0; ptIdx < Points.Num() - 1; ptIdx++)
        {
            if (TargetVal >= Points[ptIdx].Value && TargetVal <= Points[ptIdx + 1].Value)
            {
                float SegmentDiff = Points[ptIdx + 1].Value - Points[ptIdx].Value;
                if (SegmentDiff > KINDA_SMALL_NUMBER)
                {
                    float SegmentT = (TargetVal - Points[ptIdx].Value) / SegmentDiff;
                    InterpColor = Points[ptIdx].Color + (Points[ptIdx + 1].Color - Points[ptIdx].Color) * SegmentT;
                }
                break;
            }
        }

        // Store standard OpenStorm float array layout (R, G, B, Opacity)
        OutData.ColorData[i * 4 + 0] = InterpColor.R;
        OutData.ColorData[i * 4 + 1] = InterpColor.G;
        OutData.ColorData[i * 4 + 2] = InterpColor.B;
        OutData.ColorData[i * 4 + 3] = T * 2.0f; // Standard OpenStorm opacity ramp
    }

    OutData.MinValue = MinVal;
    OutData.MaxValue = MaxVal;
    OutData.bIsValid = true;

    return true;
}

void URadarColorTableSubsystem::SetRadarColorTableMaterial(UMaterialInstanceDynamic* Material, FName TextureParamName, FName MinParamName, FName MaxParamName, bool bIsReflectivity, FString TableName, bool& bOutSuccess)
{
    // Legacy Blueprint node disabled, logic is now natively in RadarVolumeRender.cpp
    bOutSuccess = false;
}

void URadarColorTableSubsystem::GetRadarColorTable(bool bIsReflectivity, FString TableName, TArray<float>& OutColorData, float& OutMinValue, float& OutMaxValue, bool& bOutIsValid)
{
    OutColorData.Empty();
    OutMinValue = 0.0f;
    OutMaxValue = 1.0f;
    bOutIsValid = false;

    const TMap<FString, FCustomColorTableData>& TargetMap = bIsReflectivity ? ReflectivityTables : VelocityTables;

    if (TableName.IsEmpty() && TargetMap.Num() > 0)
    {
        // If empty name, get the first available valid table
        for (auto& Pair : TargetMap)
        {
            if (Pair.Value.bIsValid)
            {
                OutColorData = Pair.Value.ColorData;
                OutMinValue = Pair.Value.MinValue;
                OutMaxValue = Pair.Value.MaxValue;
                bOutIsValid = true;
                return;
            }
        }
    }

    if (const FCustomColorTableData* FoundData = TargetMap.Find(TableName))
    {
        if (FoundData->bIsValid)
        {
            OutColorData = FoundData->ColorData;
            OutMinValue = FoundData->MinValue;
            OutMaxValue = FoundData->MaxValue;
            bOutIsValid = true;
        }
    }
}
