#include "ObjMeshLoader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static bool ParseVector3(const FString& Line, FVector& OutVec)
{
    TArray<FString> Parts;
    Line.ParseIntoArrayWS(Parts);

    if (Parts.Num() < 4) return false;

    OutVec.X = FCString::Atof(*Parts[1]);
    OutVec.Y = FCString::Atof(*Parts[2]);
    OutVec.Z = FCString::Atof(*Parts[3]);

    return true;
}

static bool ParseVector2(const FString& Line, FVector2D& OutVec)
{
    TArray<FString> Parts;
    Line.ParseIntoArrayWS(Parts);

    if (Parts.Num() < 3) return false;

    OutVec.X = FCString::Atof(*Parts[1]);
    OutVec.Y = FCString::Atof(*Parts[2]);

    return true;
}

static void ParseFaceToken(const FString& Tok, int32& OutV, int32& OutVT, int32& OutVN)
{
    OutV = OutVT = OutVN = INDEX_NONE;

    TArray<FString> Parts;
    Tok.ParseIntoArray(Parts, TEXT("/"), true);

    if (Parts.Num() > 0 && !Parts[0].IsEmpty()) OutV = FCString::Atoi(*Parts[0]) - 1;
    if (Parts.Num() > 1 && !Parts[1].IsEmpty()) OutVT = FCString::Atoi(*Parts[1]) - 1;
    if (Parts.Num() > 2 && !Parts[2].IsEmpty()) OutVN = FCString::Atoi(*Parts[2]) - 1;
}

bool FObjMeshLoader::LoadFromFile(const FString& FilePath, FLoadedObjMesh& OutMesh)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *FilePath))
        return false;

    TArray<FVector> TempV;
    TArray<FVector> TempVN;
    TArray<FVector2D> TempVT;

    struct FFaceIndex { int32 V, VT, VN; };
    TArray<FFaceIndex> FaceStream;

    TArray<FString> Lines;
    Text.ParseIntoArrayLines(Lines);

    for (const FString& L : Lines)
    {
        FString Line = L.TrimStartAndEnd();
        if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
            continue;

        if (Line.StartsWith(TEXT("v ")))
        {
            FVector V;
            if (ParseVector3(Line, V)) TempV.Add(V);
        }
        else if (Line.StartsWith(TEXT("vn ")))
        {
            FVector N;
            if (ParseVector3(Line, N)) TempVN.Add(N);
        }
        else if (Line.StartsWith(TEXT("vt ")))
        {
            FVector2D UV;
            if (ParseVector2(Line, UV)) TempVT.Add(UV);
        }
        else if (Line.StartsWith(TEXT("f ")))
        {
            TArray<FString> Toks;
            Line.RightChop(2).ParseIntoArrayWS(Toks);

            if (Toks.Num() < 3) continue;

            int32 v0, vt0, vn0;
            ParseFaceToken(Toks[0], v0, vt0, vn0);

            for (int32 i = 1; i + 1 < Toks.Num(); i++)
            {
                int32 v1, vt1, vn1;
                int32 v2, vt2, vn2;

                ParseFaceToken(Toks[i], v1, vt1, vn1);
                ParseFaceToken(Toks[i + 1], v2, vt2, vn2);

                FaceStream.Add({ v0, vt0, vn0 });
                FaceStream.Add({ v1, vt1, vn1 });
                FaceStream.Add({ v2, vt2, vn2 });
            }
        }
    }

    // Build final buffers
    OutMesh.Vertices.Empty(FaceStream.Num());
    OutMesh.Triangles.Empty(FaceStream.Num());
    OutMesh.Normals.Empty(FaceStream.Num());
    OutMesh.UVs.Empty(FaceStream.Num());

    for (int32 i = 0; i < FaceStream.Num(); i++)
    {
        const auto& F = FaceStream[i];

        FVector P = TempV.IsValidIndex(F.V) ? TempV[F.V] : FVector::ZeroVector;
        FVector N = TempVN.IsValidIndex(F.VN) ? TempVN[F.VN] : FVector::UpVector;
        FVector2D UV = TempVT.IsValidIndex(F.VT) ? TempVT[F.VT] : FVector2D::ZeroVector;

        OutMesh.Vertices.Add(P);
        OutMesh.Normals.Add(N);
        OutMesh.UVs.Add(UV);
        OutMesh.Triangles.Add(i);
    }

    return OutMesh.Vertices.Num() > 0 && OutMesh.Triangles.Num() % 3 == 0;
}
