#include "Tank.h"
#include <cmath>

// =================================================================
// CONFIGURAÇÕES GERAIS DOS TANQUES (Apenas áudios e partículas)
// =================================================================
static float TANK_VOL_EXPLOSION = 0.5f;   // Volume da explosão (0.0 a 1.0)
static float TANK_VOL_ENGINE = 0.15f;     // Volume do motor (0.0 a 1.0)
static float TANK_VOL_SHOOTING = 0.4f;    // Volume do tiro inimigo (0.0 a 1.0)

// =================================================================

// Suporte para 2 modelos de tanque (0 = Normal, 1 = Pesado)
static Texture2D tankFrames[2][4] = {0};
static Texture2D tankDestroyedFrame[2] = {0};
static Texture2D cannonFrames[2][3] = {0};
static Texture2D cannonDestroyedFrame[2] = {0};
static Texture2D fireFrames[2][3] = {0};
static bool tankTexturesLoaded = false;

static Sound tankExplodingSnd = {0};
static Sound tankMovingSnd = {0};
static Sound tankShootingSnd = {0};
static bool tankAudioLoaded = false;

void Tank::Initialize(Vector2 startPos, int spawnDirection, int tankType, std::vector<Vector2> path, Rectangle patrolArea)
{
    type = tankType;
    position = startPos;
    isDestroyed = false;
    isActive = true;
    hasFired = false;
    
    // Dimensões nativas das sprites de tanque (usado pela hitbox universal)
    width = 67.0f;
    height = 94.0f;
    
    // Status Individuais por Modelo
    if (type == 0) // Normal (Original)
    {
        speed = 60.0f;
        turretSpeed = 20.0f;
        hp = 50;
        cannonOffsetY = -25.0f;
        fireOffsetY = -110.0f; // Ajuste livremente esse valor! (Negativo = mais pra ponta do cano)
        scale = 1.0f; 
    }
    else // Tipo 1: Exemplo Pesado
    {
        speed = 30.0f;
        turretSpeed = 10.0f;
        hp = 12;
        cannonOffsetY = -25.0f;
        fireOffsetY = -50.0f;
        scale = 1.0f;
    }
    
    currentFrame = 0;
    frameTimer = 0.0f;
    currentSpeedMult = 1.0f;
    
    cannonFrame = 0;
    cannonAnimTimer = 0.0f;
    isCannonShooting = false;
    turretAngularVel = 0.0f;
    shootCooldown = (float)GetRandomValue(20, 50) / 10.0f; 
    
    hitTimer = 0.0f;
    
    // O Sprite aponta nativamente para BAIXO
    if (spawnDirection == 0)
    {
        velocity = { 0.0f, speed };
        rotation = 0.0f;
    }
    else if (spawnDirection == 1)
    {
        velocity = { speed, 0.0f };
        rotation = -90.0f;
    }
    else if (spawnDirection == 2)
    {
        velocity = { -speed, 0.0f };
        rotation = 90.0f;
    }
    
    waypoints = path;
    currentWaypoint = 0;
    pathDirection = 1;
    
    patrolBounds = patrolArea;
    isPatrolling = (patrolBounds.width > 0 && patrolBounds.height > 0);
    
    // Se tivermos waypoints, pulamos o primeiro (pois é onde nascemos)
    if (waypoints.size() > 1) {
        currentWaypoint = 1;
    } else if (isPatrolling) {
        // Se estamos em modo patrulha e não temos rota fixa, sorteia o primeiro ponto!
        float targetX = patrolBounds.x + GetRandomValue(0, (int)patrolBounds.width);
        float targetY = patrolBounds.y + GetRandomValue(0, (int)patrolBounds.height);
        waypoints.push_back({ targetX, targetY });
        currentWaypoint = 0;
    }
    
    cannonRotation = rotation;
    
    smokeFrame = GetRandomValue(0, 6);
    smokeAnimTimer = 0.0f;
    
    if (!tankTexturesLoaded)
    {
        const char* typeFolders[] = {
            "tank/",       // Tipo 0 (Pasta renomeada)
            "tank_heavy/"  // Tipo 1 (Pode ser qualquer pasta que você criar)
        };

        for (int t = 0; t < 2; t++) // Para cada modelo
        {
            const char* folder = typeFolders[t];

            for (int f = 0; f < 4; f++)
            {
                tankFrames[t][f] = LoadTexture(TextFormat("assets/sprites/enemies/%stank%d.png", folder, f + 1));
            }
            tankDestroyedFrame[t] = LoadTexture(TextFormat("assets/sprites/enemies/%stank_destroyed.png", folder));

            for (int f = 0; f < 3; f++)
            {
                cannonFrames[t][f] = LoadTexture(TextFormat("assets/sprites/enemies/%sturret%d.png", folder, f + 1));
                fireFrames[t][f] = LoadTexture(TextFormat("assets/sprites/enemies/%sfire%d.png", folder, f + 1));
            }
            cannonDestroyedFrame[t] = LoadTexture(TextFormat("assets/sprites/enemies/%sturret_destroyed.png", folder));
        }
        tankTexturesLoaded = true;
    }
    
    if (!tankAudioLoaded)
    {
        // Antes os três sons dividiam a checagem do exploding: se um deles
        // faltasse, nunca era tentado de novo.
        tankExplodingSnd = LoadSound("assets/audio/tank/exploding.ogg");
        tankMovingSnd = LoadSound("assets/audio/tank/moving.ogg");
        tankShootingSnd = LoadSound("assets/audio/tank/shotting.ogg");
        
        // Mantém os sons mais baixos já que os tanques estão lá embaixo no mapa
        if (tankExplodingSnd.frameCount != 0) SetSoundVolume(tankExplodingSnd, TANK_VOL_EXPLOSION);
        if (tankMovingSnd.frameCount != 0) SetSoundVolume(tankMovingSnd, TANK_VOL_ENGINE); 
        if (tankShootingSnd.frameCount != 0) SetSoundVolume(tankShootingSnd, TANK_VOL_SHOOTING);
        
        tankAudioLoaded = true;
    }
}



void Tank::Destroy()
{
    if (!isDestroyed)
    {
        isDestroyed = true;
        isCannonShooting = false; // Corta o fogo do canhão imediatamente
        cannonFrame = 0;
        
        // Toca o som de explosão ao ser destruído
        if (tankExplodingSnd.frameCount != 0) PlaySound(tankExplodingSnd);
    }
}



void Tank::UnloadSharedAssets()
{
    if (tankTexturesLoaded)
    {
        for (int t = 0; t < 2; t++)
        {
            for (int f = 0; f < 4; f++)
            {
                if (tankFrames[t][f].id != 0) { UnloadTexture(tankFrames[t][f]); tankFrames[t][f].id = 0; }
            }
            for (int f = 0; f < 3; f++)
            {
                if (cannonFrames[t][f].id != 0) { UnloadTexture(cannonFrames[t][f]); cannonFrames[t][f].id = 0; }
                if (fireFrames[t][f].id != 0) { UnloadTexture(fireFrames[t][f]); fireFrames[t][f].id = 0; }
            }
            if (tankDestroyedFrame[t].id != 0) { UnloadTexture(tankDestroyedFrame[t]); tankDestroyedFrame[t].id = 0; }
            if (cannonDestroyedFrame[t].id != 0) { UnloadTexture(cannonDestroyedFrame[t]); cannonDestroyedFrame[t].id = 0; }
        }
        tankTexturesLoaded = false;
    }

    if (tankAudioLoaded)
    {
        if (tankExplodingSnd.frameCount != 0) { UnloadSound(tankExplodingSnd); tankExplodingSnd = {}; }
        if (tankMovingSnd.frameCount != 0) { UnloadSound(tankMovingSnd); tankMovingSnd = {}; }
        if (tankShootingSnd.frameCount != 0) { UnloadSound(tankShootingSnd); tankShootingSnd = {}; }
        tankAudioLoaded = false;
    }
}


Rectangle Tank::GetHitbox() const
{
    // A imagem original é 67x94.
    // Vamos usar 45x70 para a hitbox ficar um pouco menor e mais focada no centro da lataria.
    float hitWidth = 45.0f * scale;
    float hitHeight = 70.0f * scale;

    // Se o sprite estiver mais perto da horizontal que da vertical, largura e altura invertem.
    // Usamos o ângulo módulo 180 porque 170 graus deixa o tanque tão "de pé" quanto 10 graus.
    float angle = fmodf(fabsf(rotation), 180.0f);
    if (angle > 45.0f && angle < 135.0f)
    {
        float temp = hitWidth;
        hitWidth = hitHeight;
        hitHeight = temp;
    }
    
    return { 
        position.x - (hitWidth / 2.0f), 
        position.y - (hitHeight / 2.0f), 
        hitWidth, 
        hitHeight 
    };
}

void Tank::Update(float deltaTime, Vector2 playerPos, bool playerDestroyed, float scrollSpeed)
{
    if (!isActive) return;
    
    // Lógica da Parada Estratégica
    // O tanque avisa que vai atirar, freia suavemente (inércia), e acelera suavemente depois.
    bool isAiming = (!isCannonShooting && shootCooldown <= 0.5f && !playerDestroyed);
    bool shouldStop = (isAiming || isCannonShooting || playerDestroyed);
    
    if (shouldStop) {
        currentSpeedMult -= 2.5f * deltaTime; // Freia em 0.4s
        if (currentSpeedMult < 0.0f) currentSpeedMult = 0.0f;
    } else {
        currentSpeedMult += 1.5f * deltaTime; // Acelera um pouco mais devagar
        if (currentSpeedMult > 1.0f) currentSpeedMult = 1.0f;
    }
    
    // Rola a cerca de patrulha
    if (isPatrolling) {
        patrolBounds.y += scrollSpeed * deltaTime;
    }

    // Rola os waypoints para baixo na tela, para que eles fiquem grudados no chão do mapa!
    for (auto& wp : waypoints) {
        wp.y += scrollSpeed * deltaTime;
    }

    // NAVEGAÇÃO POR WAYPOINTS
    if (!isDestroyed && currentWaypoint >= 0 && currentWaypoint < (int)waypoints.size()) {
        Vector2 target = waypoints[currentWaypoint];
        
        // Direção até o waypoint
        float dx = target.x - position.x;
        float dy = target.y - position.y;
        float dist = sqrt(dx*dx + dy*dy);
        
        if (dist < 5.0f) {
            // Chegamos no waypoint!
            if (isPatrolling) {
                // Sorteia um novo ponto dentro da cerca invisível
                float targetX = patrolBounds.x + GetRandomValue(0, (int)patrolBounds.width);
                float targetY = patrolBounds.y + GetRandomValue(0, (int)patrolBounds.height);
                waypoints.clear();
                waypoints.push_back({ targetX, targetY });
                currentWaypoint = 0;
            } else {
                // Efeito Ping-Pong (Bate e Volta) na Rota.
                // O cast para int é obrigatório: comparar um int negativo direto com
                // waypoints.size() (unsigned) promoveria o -1 para um número gigante,
                // e o ramo "currentWaypoint < 0" nunca seria alcançado.
                const int lastIndex = (int)waypoints.size() - 1;
                currentWaypoint += pathDirection;

                if (currentWaypoint > lastIndex) {
                    pathDirection = -1;
                    currentWaypoint = lastIndex - 1;
                } else if (currentWaypoint < 0) {
                    pathDirection = 1;
                    currentWaypoint = 1;
                }

                // Rotas de 1 ou 2 pontos: mantém o índice dentro da faixa válida
                if (currentWaypoint < 0) currentWaypoint = 0;
                if (currentWaypoint > lastIndex) currentWaypoint = lastIndex;
            }
        } else {
            // Usa a velocidade do modelo (o Pesado é mais lento que o Normal)
            velocity.x = (dx / dist) * speed;
            velocity.y = (dy / dist) * speed;
            
            // Gira o chassi do tanque pra apontar pro waypoint
            // atan2 retorna em radianos. Como o sprite nativo aponta pra BAIXO (0 graus), subtraímos 90 graus (-90)
            float targetRot = atan2(dy, dx) * 180.0f / PI - 90.0f;
            rotation = targetRot; 
        }
    }

    // Salva velocidade original e aplica o multiplicador de inércia antes do BaseUpdate
    Vector2 originalVel = velocity;
    velocity.x *= currentSpeedMult;
    velocity.y *= currentSpeedMult;
    
    // Chama o "Trabalho Sujo" universal (hitTimer, movimento, culling, atrito)
    EnemyBase::BaseUpdate(deltaTime);
    
    // Restaura a velocidade original APENAS se estiver vivo
    // (Se estiver destruído, o atrito do BaseUpdate precisa permanecer!)
    if (!isDestroyed) velocity = originalVel;
    
    // A animação de fumaça do tanque destruído roda no EnemyBase::BaseUpdate()
    if (!isDestroyed)
    {
        // ----------------------------------------------------
        // IA do Canhão: Mirar no Jogador
        // ----------------------------------------------------
        if (!playerDestroyed)
        {
            float dx = playerPos.x - position.x;
            float dy = playerPos.y - position.y;
            float targetAngle = atan2f(dy, dx) * (180.0f / PI);
            
            // Ajusta para o sprite que nativamente aponta para baixo (+90 graus)
            targetAngle -= 90.0f; 
            
            // Lógica de Giro Suave da Torreta para o targetAngle
            float diff = fmodf(targetAngle - cannonRotation, 360.0f);
            if (diff > 180.0f) diff -= 360.0f;
            if (diff < -180.0f) diff += 360.0f;
            
            // Inércia da Torreta: calcula a velocidade angular desejada
            float targetAngVel = 0.0f;
            if (fabs(diff) > 1.0f) // Zona morta de 1 grau
            {
                targetAngVel = (diff > 0.0f) ? turretSpeed : -turretSpeed;
                // Reduz a velocidade quando está perto do alvo (frenagem proporcional)
                if (fabs(diff) < 30.0f) targetAngVel *= (fabs(diff) / 30.0f);
            }
            
            // Acelera/desacelera suavemente a velocidade angular
            float turretAccel = turretSpeed * 4.0f; // Quão rápido a torreta ganha/perde velocidade
            if (turretAngularVel < targetAngVel) {
                turretAngularVel += turretAccel * deltaTime;
                if (turretAngularVel > targetAngVel) turretAngularVel = targetAngVel;
            } else {
                turretAngularVel -= turretAccel * deltaTime;
                if (turretAngularVel < targetAngVel) turretAngularVel = targetAngVel;
            }
            
            // Aplica a velocidade angular na rotação
            cannonRotation += turretAngularVel * deltaTime;
            
            // ----------------------------------------------------
            // IA do Canhão: Disparar
            // ----------------------------------------------------
            shootCooldown -= deltaTime;
            if (shootCooldown <= 0.0f)
            {
                // Só atira se a torreta estiver alinhada com o jogador (Tolerância de 15 graus)
                if (fabs(diff) <= 15.0f)
                {
                    isCannonShooting = true;
                    hasFired = true; // Avisa o mundo que a bala saiu!
                    cannonFrame = 1;
                    cannonAnimTimer = 0.0f;
                    shootCooldown = (float)GetRandomValue(30, 60) / 10.0f; // Proximo tiro em 3s a 6s
                    
                    if (tankShootingSnd.frameCount != 0) PlaySound(tankShootingSnd);
                }
            }
        }
        
        // Animação do Canhão (Recuo)
        if (isCannonShooting)
        {
            cannonAnimTimer += deltaTime;
            if (cannonAnimTimer >= 0.07f) // Velocidade da animação do tiro
            {
                cannonAnimTimer = 0.0f;
                cannonFrame++;
                if (cannonFrame > 2)
                {
                    cannonFrame = 0;
                    isCannonShooting = false;
                }
            }
        }
        
        // Lógica de Animação do Corpo (a velocidade da animação acompanha o freio do tanque)
        frameTimer += (deltaTime * currentSpeedMult);
        if (frameTimer >= 0.05f) // 20 FPS (Mais rápido e suave)
        {
            frameTimer = 0.0f;
            currentFrame++;
            if (currentFrame > 3) currentFrame = 0;
        }
        
        // Mantém o som do motor tocando apenas se o tanque estiver em movimento
        if (tankMovingSnd.frameCount != 0)
        {
            if (currentSpeedMult > 0.1f)
            {
                if (!IsSoundPlaying(tankMovingSnd))
                {
                    PlaySound(tankMovingSnd);
                }
            }
            else if (playerDestroyed)
            {
                // Força o corte imediato do som do tanque se o jogo "parou"
                if (IsSoundPlaying(tankMovingSnd)) StopSound(tankMovingSnd);
            }
        }
    }
}

void Tank::DrawShadows() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    Color shadowColor = BLACK; // Sombra opaca — a transparência é controlada pelo carimbo global
    
    // Sombra do corpo
    Texture2D tTex = isDestroyed ? tankDestroyedFrame[type] : tankFrames[type][currentFrame];
    if (tTex.id != 0)
    {
        float w = (float)tTex.width * scale;
        float h = (float)tTex.height * scale;
        Rectangle source = { 0.0f, 0.0f, (float)tTex.width, (float)tTex.height };
        Rectangle dest = { position.x + (8.0f * scale), position.y + (8.0f * scale), w, h };
        Vector2 origin = { w / 2.0f, h / 2.0f };
        
        DrawTexturePro(tTex, source, dest, origin, rotation, shadowColor);
    }
    
    // Sombra do canhão
    Texture2D cTex = isDestroyed ? cannonDestroyedFrame[type] : cannonFrames[type][cannonFrame];
    if (cTex.id != 0)
    {
        float cWidth = (float)cTex.width * scale;
        float cHeight = (float)cTex.height * scale;
        Rectangle cSource = { 0.0f, 0.0f, (float)cTex.width, (float)cTex.height };
        
        // A sombra do canhão fica um pouco mais alta (deslocamento maior) se ele estiver inteiro
        float shadowDistance = isDestroyed ? (8.0f * scale) : (12.0f * scale);
        Rectangle cDest = { position.x + shadowDistance, position.y + shadowDistance, cWidth, cHeight };
        
        Vector2 cOrigin = { cWidth / 2.0f, (cHeight / 2.0f) + (cannonOffsetY * scale) };
        DrawTexturePro(cTex, cSource, cDest, cOrigin, cannonRotation, shadowColor);
    }
}

void Tank::DrawBody() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    // DEBUG: Desenha os waypoints do tanque! (Descomente se precisar testar rotas no futuro)
    /*
    if (!isDestroyed && !waypoints.empty() && currentWaypoint < waypoints.size()) {
        for (int i = currentWaypoint; i < waypoints.size(); i++) {
            DrawCircle((int)waypoints[i].x, (int)waypoints[i].y, 5, RED);
            if (i > currentWaypoint) {
                DrawLineEx(waypoints[i-1], waypoints[i], 2.0f, RED);
            } else {
                DrawLineEx(position, waypoints[i], 2.0f, RED);
            }
        }
    }
    */
    
    // Corpo
    Texture2D tex = isDestroyed ? tankDestroyedFrame[type] : tankFrames[type][currentFrame];
    if (tex.id != 0)
    {
        float bodyWidth = (float)tex.width * scale;
        float bodyHeight = (float)tex.height * scale;

        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Rectangle destRec = { position.x, position.y, bodyWidth, bodyHeight };
        Vector2 origin = { bodyWidth / 2.0f, bodyHeight / 2.0f };
        
        Color tintColor = WHITE;
        if (hitTimer > 0.0f && !isDestroyed) {
            tintColor = RED;
        }
        
        DrawTexturePro(tex, sourceRec, destRec, origin, rotation, isDestroyed ? WHITE : tintColor);
    }
    
    // Canhão
    Texture2D cTex = isDestroyed ? cannonDestroyedFrame[type] : cannonFrames[type][cannonFrame];
    if (cTex.id != 0)
    {
        float cWidth = (float)cTex.width * scale;
        float cHeight = (float)cTex.height * scale;
        Rectangle cSource = { 0.0f, 0.0f, (float)cTex.width, (float)cTex.height };
        Rectangle cDest = { position.x, position.y, cWidth, cHeight };
        Vector2 cOrigin = { cWidth / 2.0f, (cHeight / 2.0f) + (cannonOffsetY * scale) };
        
        Color tintColor = WHITE;
        if (hitTimer > 0.0f && !isDestroyed) {
            tintColor = RED;
        }
        
        DrawTexturePro(cTex, cSource, cDest, cOrigin, cannonRotation, isDestroyed ? WHITE : tintColor);
        
        // Desenha o Tiro (Fogo na ponta do canhão) se estiver atirando E não estiver destruído
        if (!isDestroyed && isCannonShooting && fireFrames[type][0].id != 0)
        {
            // O tiro dura 0.2 segundos (cannonAnimTimer zera no 0.1)
            // Se cannonFrame == 1, se passou de 0 a 0.1s
            // Se cannonFrame == 2, se passou de 0.1 a 0.2s
            float elapsed = (cannonFrame == 1 ? 0.0f : 0.1f) + cannonAnimTimer;
            int fIndex = (int)(elapsed / (0.2f / 3.0f));
            if (fIndex > 2) fIndex = 2; // clamp de segurança
            
            Texture2D fTex = fireFrames[type][fIndex];
            if (fTex.id != 0)
            {
                float fWidth = (float)fTex.width * scale;
                float fHeight = (float)fTex.height * scale;
                Rectangle fSource = { 0.0f, 0.0f, (float)fTex.width, (float)fTex.height };
                Rectangle fDest = { position.x, position.y, fWidth, fHeight };
                
                // MAGIA AQUI: O Raylib rotaciona automaticamente usando o offset de origem!
                // É só mudar o fireOffsetY lá em cima pra empurrar a imagem pra ponta.
                Vector2 fOrigin = { fWidth / 2.0f, (fHeight / 2.0f) + (fireOffsetY * scale) };
                
                DrawTexturePro(fTex, fSource, fDest, fOrigin, cannonRotation, WHITE);
            }
        }
    }
}









