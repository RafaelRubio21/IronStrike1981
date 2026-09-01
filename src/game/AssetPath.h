#pragma once
#include "raylib.h"

// Sobe a partir do diretório do executável até encontrar a pasta "assets" e
// entra no diretório que a contém. Com isso todo LoadTexture/LoadSound do
// projeto usa um caminho único ("assets/..."), sem a cascata de "../../"
// que antes estava repetida em cada carregamento.
// Retorna false se a pasta não for encontrada.
inline bool LocateAssetsRoot()
{
    if (DirectoryExists("assets")) return true;

    ChangeDirectory(GetApplicationDirectory());

    for (int up = 0; up < 6; up++)
    {
        if (DirectoryExists("assets")) return true;
        if (!ChangeDirectory("..")) break;
    }

    return false;
}
