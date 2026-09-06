# Iron Strike 1981 — Análise e memória do projeto

> Documento de contexto do repositório. Escrito em 2026-09-05 a partir da leitura
> completa de `src/`, `assets/`, `CMakeLists.txt` e do histórico do Git (34 commits),
> e revisado no mesmo dia depois da faxina de dívida técnica (seção 7).
> Serve como memória: quem abrir o projeto (pessoa ou agente) deve conseguir
> entender a arquitetura, as convenções e os pontos em aberto sem reler tudo.

---

## 1. O que é

Shoot'em up 2D de rolagem vertical (scrolling de baixo para cima), no estilo arcade
dos anos 80 tipo *Choplifter* / *Raid on Bungeling Bay*. O jogador pilota um
helicóptero visto de cima, decola do chão, sobe o mapa metralhando tanques e
construções, e no fim da fase pousa sozinho num ponto marcado no mapa.

- **Linguagem:** C++17
- **Engine:** [raylib 5.0](https://www.raylib.com/) (baixado via `FetchContent`, link estático)
- **JSON:** nlohmann/json 3.11.3 (via `FetchContent`)
- **Build:** CMake ≥ 3.20 (o dev usa CLion + Ninja; `cmake-build-debug/` é local e ignorado)
- **Plataforma testada:** Linux; nada no código é específico de SO
- **Resolução:** fixa em 1024×768, 60 FPS alvo
- **Remoto:** `https://github.com/RafaelRubio21/IronStrike1981.git` (branch principal: `master`)
- **Idioma do código:** comentários e nomes de domínio em **português**; API do raylib em inglês. Manter esse padrão.

---

## 2. Como compilar e rodar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j
```

O executável procura a pasta `assets/` subindo até 6 níveis a partir do diretório
do binário (`LocateAssetsRoot()` em [AssetPath.h](src/game/AssetPath.h)), então
pode ser rodado de qualquer lugar. Se não achar, o jogo **roda mesmo assim**, mudo
e sem sprites — todo carregamento é defensivo (`if (tex.id != 0)`, `if (snd.frameCount != 0)`).

Controles: setas ou `WASD` para voar, `Ctrl` para metralhar, `ESC` fecha.

---

## 3. Mapa do código

```
src/
├── main.cpp                    # 37 linhas: janela, áudio, loop, escopo de vida do Game
└── game/
    ├── Constants.h             # Config::SCREEN_WIDTH/HEIGHT, CULL_MARGIN
    ├── AssetPath.h             # LocateAssetsRoot(): resolve a raiz de assets uma vez só
    ├── Game.h/.cpp             # 438 linhas — orquestrador: update, colisões, render em camadas
    ├── Player.h/.cpp           # 482 linhas — helicóptero: física, decolagem, tiro, pouso, áudio
    ├── EnemyBase.h             # classe-base abstrata + BaseUpdate() comum a todo inimigo
    ├── Tank.h/.cpp             # 600 linhas — único inimigo concreto: navegação, IA de torreta
    ├── EnemyBullet.h/.cpp      # projétil inimigo com rastro de fumaça
    ├── SoundPool.h             # header-only: N vozes do mesmo som via LoadSoundAlias
    ├── ExplosionManager.h/.cpp # tabela de tipos de explosão (sprite + duração + som)
    ├── SmokeManager.h/.cpp     # fumaça de 7 frames dos destroços
    └── map/MapManager.h/.cpp   # 542 linhas — carrega e desenha mapas do Tiled (JSON)
```

Total: ~3.100 linhas. Há `README.md`; ainda não há testes automatizados nem CI.

### Dependências entre módulos

`main` → `Game` → { `Player`, `Tank`(via `EnemyBase`), `MapManager`, `ExplosionManager`, `SmokeManager`, `EnemyBullet` }

O `Game` é o único ponto que conhece todos. Os módulos **não se conhecem entre si**:
o `Tank` não sabe do `ExplosionManager`, ele levanta a flag `hasFired` / `isDestroyed`
e o `Game` reage. Essa é a convenção central do projeto — **comunicação por flag,
não por chamada direta**. Vale manter.

---

## 4. Decisões de arquitetura que importam

### 4.1 O mundo é a tela; quem se move é o cenário

Não existe câmera de verdade — nem o objeto `Camera2D`, que era vestigial e foi
removido. Todas as posições de entidades já são **coordenadas de tela**.

O `MapManager` guarda um `scrollY` que começa em `alturaDoMapa - 768` e **decresce**
até 0 (o mapa rola de baixo para cima). A cada frame:

1. `mapManager.Update(dt, speed)` devolve **quantos pixels o cenário andou de fato**;
2. `Game` converte isso em `worldScroll` e empurra manualmente inimigos, waypoints,
   cercas de patrulha e explosões para baixo pela mesma quantidade.

O detalhe crítico é o *de fato*: no fim do mapa o `scrollY` trava em 0, e se os
inimigos continuassem descendo sairiam do traçado desenhado no Tiled. Por isso
`Update` retorna o delta real, e não a velocidade pedida.

**Consequência:** balas do jogador e do inimigo **não** são roladas — voam no ar,
não estão coladas no chão. É intencional.

### 4.2 Pipeline de render em 6 etapas, com sombras unificadas

As sombras não são desenhadas por objeto direto na tela: vão todas para um
`RenderTexture2D` global (`globalShadowTarget`), pintadas de **preto 100% sólido**,
e depois esse canvas é carimbado de uma vez com alpha 80/255 (`StampShadows()`).
Isso evita que sombras sobrepostas fiquem mais escuras umas sobre as outras.

Ordem em `Game::Render()`:

| # | Etapa | Por quê |
|---|---|---|
| 1 | `mapManager.RenderGround()` | chão, água, asfalto — só tile layers |
| 2 | canvas de sombras ← construções + inimigos | tudo que está *no chão* |
| 3 | `StampShadows()` | carimba com alpha |
| 3.5 | `mapManager.RenderObjects()` | as construções, **por cima da própria sombra** |
| 4 | `e->DrawBody()` | os tanques |
| 4.5 | canvas limpo ← só o helicóptero, e carimba de novo | ele voa: a sombra dele cai **sobre** casas e tanques |
| 4.6 | `player.DrawBody()` | o helicóptero |
| 5 | fumaça, balas inimigas, explosões, FPS | sempre por cima |

O canvas é reutilizado (limpo com `BLANK` e usado duas vezes por frame). O
`sourceRec` do carimbo tem **altura negativa** de propósito: render target de
OpenGL vem invertido.

A distância da sombra codifica altitude: helicóptero usa `15 + (scale-0.6)*100`,
tanque usa 8 px fixos, construção 16 px.

### 4.3 Herança rasa, com "trabalho sujo" na base

`EnemyBase` é abstrata (`Update`, `DrawShadows`, `DrawBody` puros) e concentra o
que todo inimigo faz igual: timer de flash de dano, integração da velocidade,
culling fora da tela (`CULL_MARGIN` = 300 px), derrapada por atrito quando
destruído e animação de fumaça. O filho chama `EnemyBase::BaseUpdate(dt)` no meio
do seu próprio `Update`.

`Tank` é a **única** subclasse hoje. `Game.cpp:146-153` já tem o esqueleto da
fábrica de inimigos: quando surgir uma segunda classe, é ali (olhando
`spawn.enemyType`) que ela entra.

### 4.4 Assets compartilhados são estáticos de arquivo

As texturas e sons do tanque são `static` no `Tank.cpp` (`tankFrames[2][4]`,
`tankMovingSnd`, …), carregados preguiçosamente no primeiro `Initialize` e
liberados por `Tank::UnloadSharedAssets()`. Cem tanques na tela compartilham as
mesmas 12 texturas. Um modelo cuja pasta de arte não existe **empresta** as
texturas do tipo 0 (e o `Unload` pula esses slots, porque não é dono deles).

É estado global mutável — funciona, mas é o principal candidato a virar um
`AssetManager` se o número de inimigos crescer.

### 4.5 Um som que pode soar duas vezes ao mesmo tempo precisa de pool

Um `Sound` do raylib é **um canal**: chamar `PlaySound` nele de novo reinicia o
mesmo buffer. `SoundPool` ([SoundPool.h](src/game/SoundPool.h)) carrega o arquivo
uma vez e cria vozes extras com `LoadSoundAlias`, que compartilham as amostras —
N vozes não custam N vezes a memória do áudio. `Play(volume)` procura a primeira
voz livre e, com todas ocupadas, reaproveita a próxima da fila.

Usam pool: metralhadora do jogador (6 vozes), tiro de tanque (4), impactos
metálicos (3 cada) e explosões (4). O volume vai **por chamada**, porque tipos de
explosão diferentes dividem o mesmo pool com volumes diferentes.

Continua sendo `Sound` simples o que nunca soa em paralelo: partida e shutdown do
motor, o eco do último tiro, e o motor do tanque — este último é de propósito uma
ambiência única compartilhada por todos os tanques.

### 4.6 Ordem de destruição é frágil por natureza

Todo `Unload*` do raylib precisa rodar **enquanto** a janela e o dispositivo de
áudio ainda existem. Por isso o `Game` vive num escopo `{}` explícito dentro do
`main`, e `Game::Shutdown()` libera tudo antes de `CloseAudioDevice()`/`CloseWindow()`.
**Não mexer nessa ordem sem entender o motivo.** Todo `SoundPool` também precisa
do seu `Unload()` aí (os aliases saem antes do som de origem).

---

## 5. Sistemas, um a um

### 5.1 Player (helicóptero)

Máquina de estados implícita, controlada por `scale` (0.5 = no chão, 1.0 = no ar):

```
delay 1.5s → som de partida → hélice acelera 500°/s²
   → passa de 700°/s: scale sobe 0.5→1.0 a 0.1/s   (decolagem, ~5s)
   → scale == 1.0: IsAirborne() → libera controle, tiro e o scroll do mapa
   → fim do mapa: StartLanding() → voa sozinho até o alvo, desce, desliga
   → destruído: cai (scale→0.5), vira GRAY, justHitGround dispara a explosão
```

- **Física:** aceleração 1800 px/s² + atrito proporcional (`v -= v*6*dt`) — dá a
  inércia característica de helicóptero. Preso na tela com margem de 20 px.
- **Tiro:** `Ctrl` gera uma bala a cada 0.08 s a partir do bico (`mgOffsetY = -114`),
  1200 px/s, desenhada como tracer (linha laranja + miolo amarelo + núcleo branco).
- **Áudio:** `engine_starting.ogg` (Sound, uma vez) → `engine.ogg` (Music, em loop,
  precisa de `UpdateMusicStream` todo frame) → `engine_shutdown.ogg` no pouso. A
  desaceleração da hélice é calculada para casar exatamente com os 8.1 s do som
  de shutdown (`rotorShutdownRate = currentRotorSpeed / ENGINE_SHUTDOWN_TIME`).
  Há também um som de "cauda" do último tiro, disparado na borda de subida de
  soltar o `Ctrl`.

### 5.2 Tank

- **Navegação por waypoints** vindos da polyline do Tiled, com efeito ping-pong
  (vai e volta na rota). O tanque **gira parado**: `hullTurnSpeed` graus/s, e só
  volta a acelerar quando o chassi está alinhado dentro de `TANK_ALIGN_TOLERANCE`
  (6°). É isso que faz ele seguir o traçado em vez de cortar caminho.
- **Inércia de arranque/freada** via `currentSpeedMult` (freia em 0.4 s, acelera
  em 0.67 s). Ele para para atirar, para curvar, para ceder passagem e quando o
  jogador morre.
- **Torreta independente** do chassi, com velocidade angular própria e frenagem
  proporcional nos últimos 30°. Só dispara com o alvo dentro de 15°; cooldown
  aleatório de 3–6 s.
- **Modelos:** tipo 0 (normal, 60 px/s, 50 HP) e tipo 1 (pesado, 30 px/s, 12 HP).
  HP e velocidade vindos do Tiled sobrescrevem o padrão do modelo.
- O sprite aponta nativamente **para baixo**, então rotação 0° = Y+. Todo `atan2`
  no código leva `-90` por causa disso. O vetor "frente" é `{-sin(rad), cos(rad)}`.

### 5.3 Anti-trombada (`Game.cpp:158-196`)

Par a par entre inimigos vivos: se as hitboxes colidem, projeta a velocidade de
cada um no eixo que liga os dois centros e marca `mustYield` **só em quem está
avançando mais**. Ninguém é empurrado — quem cede apenas freia. A escolha é
deliberada: empurrar tirava o tanque do traçado do Tiled e ele acabava
atravessando lugar por onde a rota não passa. E marcar os dois numa batida de
frente travava os dois colados para sempre.

Custo: O(n²) por frame. Irrelevante com poucos tanques.

### 5.4 MapManager (Tiled)

Lê o JSON exportado do Tiled ([assets/maps/level1.json](assets/maps/level1.json)).
Suporta:

- **Tile layers** — desenhados com culling por linha, ancorados no canto **inferior**
  esquerdo da célula (o Tiled ancora pela base: uma imagem mais alta que o grid
  cresce para cima). A margem de culling sai da altura do tileset mais alto.
- **Tilesets embutidos** em dois formatos: spritesheet única (`image` + `columns`)
  ou coleção de imagens soltas (`tiles[]`, cada uma com sua textura).
- **Object layers**, discriminadas por ter ou não `gid`:
  - **com `gid`** → cenário (`MapObject`): construções da classe `Building` do
    Tiled, com `HP` e `FrameDestroyed` (nome do PNG de ruína em `Buildings/`).
    HP 0 = indestrutível, tiro passa direto. As texturas de ruína são carregadas
    uma vez por nome e compartilhadas.
  - **sem `gid`** → spawn (`EnemySpawnData`): `EnemyRoute` (com polyline),
    `EnemyPatrol` (retângulo + `Quantity`, sorteia pontos dentro da cerca),
    `PlayerStartPosition` e `PlayerFinishPosition`.
- **Spawn preguiçoso:** `PopReadySpawns()` só entrega o inimigo quando o ponto
  dele entra pelo topo da tela (`y >= scrollY - 50`).

Cuidados já resolvidos e comentados no código (não regredir):
- gid mascarado com `& 0x0FFFFFFF` (os bits altos são flags de espelhamento);
- `startX`/`startY` clampados em 0, senão o índice do tile fica negativo;
- camada com contagem de tiles diferente de `width*height` é redimensionada;
- a origem de um spawn com polyline é forçada ao **primeiro vértice** — o Tiled
  não garante que ele seja (0,0), e o `Tank::Initialize` pula o waypoint 0
  justamente por assumir que nasceu nele.

### 5.5 ExplosionManager

Tabela `EXPLOSION_CONFIGS` no `.cpp` define, por tipo, a pasta de frames, quantos
frames, o som e o volume. Tipos que apontam para o mesmo `.ogg` **compartilham o
handle** (carregado uma vez, descarregado uma vez, com os slots zerados juntos).
Para criar um efeito novo, é só acrescentar uma linha na tabela.

| Tipo | Uso | Sprite | Som |
|---|---|---|---|
| `TYPE_0` | construção derrubada, bala no player | Explosion0 (10f) | explosion1 |
| `TYPE_1` | helicóptero batendo no chão | Explosion1 (10f) | explosion1 |
| `TYPE_2` | faísca de bala na blindagem | FireElement1 (7f) | — (o impacto metálico é do `Game`) |
| `TYPE_3` | tanque explodindo | Explosion0 (10f) | tank/exploding |

### 5.6 HUD (`Game::RenderHUD()`)

Desenhado por cima de tudo, na última etapa do `Render()`, depois de `DrawFPS`
(que ocupa os primeiros ~30 px do canto superior esquerdo — a barra de vida
começa em y=55 de propósito, pra não sobrepor o contador). Três elementos, sem
estado próprio: leem `player`/`mapManager`/`score` a cada frame.

- **Vida**: barra proporcional a `Player::GetHpRatio()` (`hp / maxHp`). `maxHp`
  é guardado no `Initialize()` do player, não é uma constante, porque o `hp`
  inicial ainda é o valor de teste (100000) — quando ele virar um número real
  a barra continua funcionando sem mudança nenhuma. Cor muda de verde para
  amarelo (≤50%) e vermelho (≤25%).
- **Pontuação**: `Game::score`, um `int` simples somado direto onde o dano
  acontece — +100 quando um tanque morre (`Tank::hp <= 0 && isDestroyed`), +50
  quando `MapManager::DamageObject` derruba uma construção. Não sobrevive a um
  `Initialize()` novo (zera de propósito).
- **Progresso da fase**: barra fina no topo, de `MapManager::GetProgress()`.
  Usa o `scrollY` inicial (guardado em `scrollYInicial` no `Load()`) como
  referência: `1 - scrollY/scrollYInicial`. Mapa mais curto que a tela
  (`scrollYInicial <= 0`) devolve 1.0 — já "chegou".

---

## 6. Assets

```
assets/
├── audio/        bgmusic(1) explosions(1) helicopter(5) metal_impact(5) tank(3)
├── maps/         level1.json, level2.json, .tiled-project, propertytypes.json, tank.tx
│   ├── Asphalt(26) Dirt(24) Grass(26) Road(6) Sand(25) Water(29)   # coleções de tiles
│   └── Buildings(12)                                                # 3 casas × 3 cores + 3 ruínas
└── sprites/
    ├── helicopter/   helicopter, helicopter_brocken, helice, helice_brocken (90×206 / 180×177), machine_gun (48×24 = 3 frames)
    ├── enemies/tank/ tank1-4, tank_destroyed (67×94), turret1-3, turret_destroyed (55×115), fire1-3
    ├── explosions/   Explosion0(10) Explosion1(10) FireElement1(7)
    └── smokes/       smoke1(7) smoke2(10)
```

5,5 MB, 250 arquivos versionados. `level1` é a única fase jogável: 16×100 tiles de
64 px = mundo de 1024×6400, 5 tile layers, 8 construções, **1 rota de tanque** de
4 vértices, start em (482, 6177) e finish em (479, 293).

---

## 7. Dívida técnica: o que foi pago e o que sobrou

Faxina feita em 2026-09-05. Verificação: build com `-Wall -Wextra` (**zero
warnings**, antes eram ~20) e uma sessão de 110 s simulados rodando headless sob
**AddressSanitizer + UndefinedBehaviorSanitizer**, com o gatilho preso e varredura
lateral, até o pouso de fim de fase e o `Shutdown` completo — sem nenhum erro.
Nessa sessão os caminhos de tanque destruído, construção derrubada, faísca,
impacto metálico e bala inimiga acertando o jogador foram todos exercitados.

### Corrigido

| # | O que era | O que foi feito |
|---|---|---|
| 1 | `HEAVY_TANK` nascia **invisível e mesmo assim letal** — a pasta `tank_heavy/` não existe | Modelo sem arte própria **empresta** as texturas do tipo 0, com aviso no log. O `Unload` pula os slots emprestados, para não liberar o mesmo handle duas vezes |
| 8 | `tankSpawnTimer` escrito uma vez e nunca lido | Removido |
| 9 | `DrawGroundEffects()` chamado todo frame sem nenhuma implementação | Removido, junto com o `BeginMode2D`/`EndMode2D` que só existia para ele — e com o `Camera2D`, que ficou sem uso |
| 10 | `hitTimer` de inimigo destruído decrementado duas vezes por frame | Removida a segunda subtração, no `Game` |
| 11 | `deltaTime` sem teto: um travamento teleportava tudo | `Config::MAX_DELTA_TIME` (50 ms) aplicado no loop do `main` — se o jogo engasga, ele fica lento em vez de dar salto |
| 12 | Bala do jogador testada só na posição do frame: a 1200 px/s ela anda 20 px por quadro e a caixa tinha 15 px | A caixa passou a cobrir **o trecho percorrido no frame** (`15 + bulletTravel`). A bala inimiga trocou o teste de ponto por círculo do raio com que ela é desenhada (`EnemyBullet::RADIUS`) |
| 13 | Um `Sound` por efeito: dois tanques atirando juntos se cortavam, e a metralhadora (12 tiros/s) cortava a si mesma | `SoundPool` com `LoadSoundAlias` (ver 4.5). De quebra, o `ExplosionManager` perdeu a comparação frágil de `stream.buffer` que o `Unload` fazia para desduplicar sons |
| 15 | `.gitignore` não cobria `.idea/` nem `cmake-build-*/` | Cobre, e separou as seções de build e de IDE |
| 16 | Sem README | [README.md](README.md): o que é, controles, como compilar (com as libs de X11/GL), estrutura e como editar fases no Tiled |
| 17 | ~20 warnings de `-Wmissing-field-initializers` vindos de `= {0}` em structs da raylib | Trocado por `= {}` |

Dedup de brinde: os dois blocos idênticos que sorteavam e tocavam som de impacto
viraram `Game::PlayImpactSound()`.

### Em aberto — e por quê

**Decisão de jogo, não de código:**

- **`hp = 100000` no player** ([Player.cpp:26](src/game/Player.cpp:26)). É valor de
  teste e deixa o jogador praticamente imortal, mas baixar isso muda a
  dificuldade do jogo — é chamada sua, não faxina. Balas de tanque tiram 20.
- **Sem estados de jogo** (menu, game over, vitória, reinício).
  `Player::HasShutDown()` existe para sinalizar "pousou e as hélices pararam" e
  continua sem chamador: o fim de fase termina com o helicóptero parado na tela.
- **Só um inimigo na fase 1.** A infraestrutura de spawn e patrulha aguenta muito
  mais; falta conteúdo, não código.

**Feature, não dívida:**

- **`level2.json` não carrega** — usa tilesets externos (`.tsx`), e o `MapManager`
  só lê tileset embutido. Suportar `.tsx`/`.tsj` é implementar um parser novo. O
  caminho da fase também é fixo em `Game.cpp`; não há lista de níveis.
- **`level2` tem a propriedade escrita como `Speedy`** em vez de `Speed`. Não
  corrigi no JSON de propósito: quem manda nesse nome é a classe `EnemyRoute` do
  Tiled, e o editor reescreveria o arquivo na próxima gravação. O conserto é
  renomear a propriedade **no Tiled**.

**Grande demais para entrar junto:**

- **Resolução fixa em 1024×768.** O jeito certo é renderizar numa `RenderTexture`
  e escalar com letterbox — mas o pipeline atual usa `BeginTextureMode` no meio
  do `Render` para o canvas de sombras, e o raylib **não deixa aninhar** modos de
  textura. Daria para fazer construindo os dois passes de sombra *antes* de abrir
  o alvo da cena, e isso é uma reestruturação do render que eu não conseguiria
  conferir visualmente daqui. Ficou registrado, não feito.
- **Sem testes automatizados e sem CI.** Hoje nenhuma lógica pura está separada da
  raylib: para testar de verdade seria preciso extrair a matemática (ângulos,
  hitbox, navegação por waypoints) para funções livres — refatoração saudável,
  mas que é um trabalho por si só. O harness com sanitizers usado nesta faxina
  está descrito na seção 10 e dá para transformar em teste de fumaça de CI.

## 8. Convenções a respeitar ao mexer aqui

- **Comentário explica o *porquê*, não o *o quê*.** O código já é assim, e vários
  comentários registram armadilhas reais (ancoragem do Tiled, cast do índice
  negativo, inversão do render target, ordem de `Unload`). Não apagar esses.
- **Português** nos comentários e em nomes de domínio (`avancoI`, `usandoRuina`, `centro`).
- **Carregamento defensivo:** sempre checar `tex.id != 0` / `snd.frameCount != 0`.
  O jogo tem que abrir mesmo sem assets.
- **Números mágicos ficam no topo do arquivo**, em `static const` nomeado e com
  comentário dizendo o efeito de aumentar/diminuir (ver `TANK_ALIGN_TOLERANCE`,
  `LANDING_BRAKE_RADIUS`, `BUILDING_SHADOW_OFFSET`).
- **Constantes de tela vêm de `Config`**, nunca hardcoded — foi exatamente o que o
  commit `412fa25` foi corrigir.
- **Módulo não chama módulo:** levanta flag, o `Game` reage.
- **Todo `Load*` novo precisa do `Unload*` correspondente** em `Game::Shutdown()`,
  antes de fechar janela e áudio.

---

## 9. Histórico

34 commits, de "Initial commit: 2D Base Engine with Helicopter Liftoff" até
"implementação do motor do jogo". A trajetória é: decolagem → movimento → tiro →
sombras → tanque → explosões/fumaça → refatoração para `EnemyBase` → mapas do
Tiled → rolagem → rotas e patrulha → curvas suaves do chassi → construções
destrutíveis → pouso de fim de fase. Cerca de um terço dos commits são
refatorações — o projeto vem sendo limpo conforme cresce, e vale manter esse ritmo.

---

## 10. Como verificar uma mudança sem o CLion

O `ninja` que o CLion usa não fica no `PATH`, mas dá para compilar direto contra a
`libraylib.a` que já está em `cmake-build-debug/_deps` — e rodar headless com
`xvfb-run`, o que serve tanto para conferir localmente quanto para virar CI:

```bash
g++ -std=c++17 -Wall -Wextra -g -Isrc \
  -Icmake-build-debug/_deps/raylib-src/src \
  -Icmake-build-debug/_deps/nlohmann_json-src/include \
  src/main.cpp src/game/*.cpp src/game/map/*.cpp \
  cmake-build-debug/_deps/raylib-build/raylib/libraylib.a \
  -lGL -lm -lpthread -ldl -lrt -lX11 -o /tmp/IronStrike1981
```

Para exercitar o ciclo de vida inteiro — incluindo `Shutdown`, que é onde um
`Unload` errado estoura —, compile os fontes do jogo com um `main` de teste que
roda N segundos de `deltaTime` fixo e sai limpo, com `-fsanitize=address,undefined`.
Dois truques que valeram a pena:

- **injetar input** com `-DIsKeyDown=MinhaFuncao`: a macro renomeia as chamadas
  do `Player` e você controla o helicóptero pelo teste, sem tocar no código;
- **instrumentar uma cópia** de `src/` em vez do original, quando precisar de
  contadores (quantos tiros acertaram, quantas construções caíram) só para a
  verificação.

Rodar com `LIBGL_ALWAYS_SOFTWARE=1` e `ASAN_OPTIONS=detect_leaks=0` — o vazamento
que sobra é do driver de GL por software, não do jogo.
