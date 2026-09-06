# Iron Strike 1981

Shoot'em up 2D de rolagem vertical, no estilo arcade dos anos 80. Você pilota um
helicóptero visto de cima: decola do chão, sobe o mapa metralhando tanques e
construções e, no fim da fase, pousa no ponto marcado.

Escrito em C++17 com [raylib](https://www.raylib.com/). As fases são desenhadas
no [Tiled](https://www.mapeditor.org/) e carregadas do JSON exportado por ele.

## Controles

| Tecla | Ação |
|---|---|
| `↑` `↓` `←` `→` ou `W` `A` `S` `D` | Voar |
| `Ctrl` | Metralhadora |
| `ESC` | Sair |

O helicóptero começa no chão: espere o motor pegar e as hélices ganharem
rotação. O controle só é liberado depois que ele termina de decolar — é a mesma
condição que faz o cenário começar a rolar.

## Compilar

Precisa de CMake 3.20+ e um compilador com C++17. raylib e nlohmann/json são
baixados automaticamente pelo `FetchContent`, então a primeira compilação exige
internet.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

No Linux, o raylib precisa dos headers de X11/OpenGL. No Debian/Ubuntu:

```bash
sudo apt install build-essential cmake libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

## Rodar

```bash
./build/IronStrike1981
```

O executável procura a pasta `assets/` subindo a partir do próprio diretório, então
pode ser chamado de qualquer lugar. Sem os assets ele ainda abre — mudo e sem
sprites — em vez de quebrar.

## Estrutura

```
src/
├── main.cpp            janela, dispositivo de áudio e loop principal
└── game/
    ├── Game            orquestrador: update, colisões e render em camadas
    ├── Player          helicóptero: física, decolagem, tiro, pouso, áudio
    ├── EnemyBase       base abstrata de inimigo + a física comum a todos
    ├── Tank            navegação por waypoints, IA de torreta
    ├── EnemyBullet     projétil inimigo com rastro
    ├── SoundPool       vozes múltiplas do mesmo som (LoadSoundAlias)
    ├── Explosion/Smoke Manager   efeitos
    └── map/MapManager  carrega e desenha os mapas do Tiled

assets/
├── audio/    música, motor, tiros, impactos, explosões
├── maps/     level1.json (a fase jogável) e os tilesets
└── sprites/  helicóptero, tanques, explosões, fumaça
```

`CLAUDE.md`, na raiz, tem a análise detalhada da arquitetura, as decisões de
projeto e a lista do que ainda está em aberto.

## Editar fases

Abra `assets/maps/IronStrike1981.tiled-project` no Tiled. O jogo entende:

- **tile layers** — o chão (grama, asfalto, água, estrada);
- **objetos com imagem** da classe `Building` — construções, com `HP` e
  `FrameDestroyed` (o PNG de ruína, sem extensão);
- **objetos `EnemyRoute`** com polilinha — a rota que o tanque percorre em
  vai-e-vem, com `HP`, `Speed` e `EnemyType`;
- **objetos `EnemyPatrol`** retangulares — uma cerca dentro da qual `Quantity`
  inimigos vagam sorteando destinos;
- **`PlayerStartPosition`** e **`PlayerFinishPosition`** — onde o helicóptero
  decola e onde ele pousa no fim da fase.

Os tilesets precisam estar **embutidos** no JSON da fase (o Tiled ainda não é
lido a partir de `.tsx`/`.tsj` externos).
