#include "Tank.h"
#include <cmath>

// =================================================================
// CONFIGURAÇÕES GERAIS DOS TANQUES (Apenas áudios e partículas)
// =================================================================
static float TANK_FRICTION = 150.0f;      // Força de frenagem ao ser destruido (derrapada)
static float TANK_VOL_EXPLOSION = 0.5f;   // Volume da explosão (0.0 a 1.0)
static float TANK_VOL_ENGINE = 0.15f;     // Volume do motor (0.0 a 1.0)
static float TANK_VOL_SHOOTING = 0.4f;    // Volume do tiro inimigo (0.0 a 1.0)

// Configuração das Partículas (Poeira e Rastros)
static float TANK_DUST_RATE = 0.05f;       // Frequencia que a poeira nasce (menor = mais denso)
static float TANK_DUST_OFFSET = 20.0f;     // Distancia do centro do tanque até a lagarta (traseira)
static float TANK_DUST_SPREAD = 15.0f;     // O quão caótico/largo é o espalhamento
static float TANK_DUST_MIN_RADIUS = 4.0f;  // Tamanho inicial mínimo da poeira
static float TANK_DUST_MAX_RADIUS = 10.0f; // Tamanho inicial máximo
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

void Tank::Initialize(Vector2 startPos, int spawnDirection, int tankType)
{
    type = tankType;
    position = startPos;
    isDestroyed = false;
    isActive = true;
    hasFired = false;
    
    // Status Individuais por Modelo
    if (type == 0) // Normal (Original)
    {
        speed = 60.0f;
        turretSpeed = 20.0f;
        hp = 5;
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
    
    cannonRotation = rotation;
    
    smokeFrame = GetRandomValue(0, 6);
    smokeAnimTimer = 0.0f;
    
    tracks.clear();
    trackSpawnTimer = 0.0f;
    dustParticles.clear();
    dustSpawnTimer = 0.0f;
    
    if (!tankTexturesLoaded)
    {
        const char* basePaths[] = {
            "assets/sprites/enemies/",
            "../../assets/sprites/enemies/",
            "../../../assets/sprites/enemies/"
        };
        const char* typeFolders[] = {
            "tank/",     // Tipo 0 (Pasta renomeada)
            "tank_heavy/"  // Tipo 1 (Pode ser qualquer pasta que você criar)
        };
        
        for (int p = 0; p < 3; p++)
        {
            if (tankFrames[0][0].id == 0)
            {
                for (int t = 0; t < 2; t++) // Para cada modelo
                {
                    tankFrames[t][0] = LoadTexture(TextFormat("%s%stank1.png", basePaths[p], typeFolders[t]));
                    tankFrames[t][1] = LoadTexture(TextFormat("%s%stank2.png", basePaths[p], typeFolders[t]));
                    tankFrames[t][2] = LoadTexture(TextFormat("%s%stank3.png", basePaths[p], typeFolders[t]));
                    tankFrames[t][3] = LoadTexture(TextFormat("%s%stank4.png", basePaths[p], typeFolders[t]));
                    tankDestroyedFrame[t] = LoadTexture(TextFormat("%s%stank_destroyed.png", basePaths[p], typeFolders[t]));
                    
                    cannonFrames[t][0] = LoadTexture(TextFormat("%s%sturret1.png", basePaths[p], typeFolders[t]));
                    cannonFrames[t][1] = LoadTexture(TextFormat("%s%sturret2.png", basePaths[p], typeFolders[t]));
                    cannonFrames[t][2] = LoadTexture(TextFormat("%s%sturret3.png", basePaths[p], typeFolders[t]));
                    cannonDestroyedFrame[t] = LoadTexture(TextFormat("%s%sturret_destroyed.png", basePaths[p], typeFolders[t]));
                    
                    fireFrames[t][0] = LoadTexture(TextFormat("%s%sfire1.png", basePaths[p], typeFolders[t]));
                    fireFrames[t][1] = LoadTexture(TextFormat("%s%sfire2.png", basePaths[p], typeFolders[t]));
                    fireFrames[t][2] = LoadTexture(TextFormat("%s%sfire3.png", basePaths[p], typeFolders[t]));
                }
            }
        }
        tankTexturesLoaded = true;
    }
    
    if (!tankAudioLoaded)
    {
        const char* audioPaths[] = {
            "assets/audio/tank/",
            "../../assets/audio/tank/",
            "../../../assets/audio/tank/"
        };
        
        for (int i = 0; i < 3; i++)
        {
            if (tankExplodingSnd.frameCount == 0)
            {
                tankExplodingSnd = LoadSound(TextFormat("%sexploding.ogg", audioPaths[i]));
                tankMovingSnd = LoadSound(TextFormat("%smoving.ogg", audioPaths[i]));
                tankShootingSnd = LoadSound(TextFormat("%sshotting.ogg", audioPaths[i]));
            }
        }
        
        // Mantém os sons mais baixos já que os tanques estão lá embaixo no mapa
        if (tankExplodingSnd.frameCount != 0) SetSoundVolume(tankExplodingSnd, TANK_VOL_EXPLOSION);
        if (tankMovingSnd.frameCount != 0) SetSoundVolume(tankMovingSnd, TANK_VOL_ENGINE); 
        if (tankShootingSnd.frameCount != 0) SetSoundVolume(tankShootingSnd, TANK_VOL_SHOOTING);
        
        tankAudioLoaded = true;
    }
}

void Tank::TakeDamage(int damage)
{
    if (isDestroyed) return;
    
    hp -= damage;
    hitTimer = 0.05f; // Pisca de vermelho por 0.05 segundos ao ser alvejado
    
    if (hp <= 0)
    {
        Destroy();
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

Rectangle Tank::GetHitbox() const
{
    float width = 67.0f * scale;
    float height = 94.0f * scale;
    // Como a nossa origin é no meio, subtraímos metade para gerar o retângulo a partir do canto superior esquerdo
    return { position.x - (width / 2.0f), position.y - (height / 2.0f), width, height };
}

void Tank::DrawTracksAndDust() const
{
    /* --- EFEITOS DESATIVADOS (Para uso no futuro) ---
    // Desenha rastros (marcas de pneu na grama)
    for (const auto& tr : tracks)
    {
        float alpha = tr.lifeTime / 4.0f; // Fada de 1.0 para 0.0
        unsigned char a = (unsigned char)(alpha * 60.0f); // Max alpha = 60 (bem suave)
        
        // Retângulo simulando a lagarta esmagando o mato
        Rectangle dest = { tr.position.x, tr.position.y, 45.0f * scale, 30.0f * scale };
        Vector2 orig = { dest.width / 2.0f, dest.height / 2.0f };
        DrawRectanglePro(dest, orig, tr.rotation, { 20, 15, 5, a }); // Marrom escuro transparente
    }
    
    // Desenha poeira
    for (const auto& dp : dustParticles)
    {
        float alpha = dp.lifeTime / dp.maxLife;
        unsigned char a = (unsigned char)(alpha * 120.0f);
        DrawCircleV(dp.position, dp.radius, { 190, 160, 120, a }); // Cor de terra/areia
    }
    --------------------------------------------------- */
}

void Tank::Update(float deltaTime, Vector2 playerPos, bool playerDestroyed)
{
    if (!isActive) return;
    
    if (hitTimer > 0.0f) hitTimer -= deltaTime;
    
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
    
    float curVelX = velocity.x * currentSpeedMult;
    float curVelY = velocity.y * currentSpeedMult;

    // Movimento
    position.x += curVelX * deltaTime;
    position.y += curVelY * deltaTime;
    
    // Atualiza vida das partículas
    for (int i = 0; i < tracks.size(); i++) {
        tracks[i].lifeTime -= deltaTime;
        if (tracks[i].lifeTime <= 0) { tracks.erase(tracks.begin() + i); i--; }
    }
    
    for (int i = 0; i < dustParticles.size(); i++) {
        dustParticles[i].lifeTime -= deltaTime;
        dustParticles[i].position.x += dustParticles[i].velocity.x * deltaTime;
        dustParticles[i].position.y += dustParticles[i].velocity.y * deltaTime;
        dustParticles[i].radius += 10.0f * deltaTime; // Poeira espalha
        if (dustParticles[i].lifeTime <= 0) { dustParticles.erase(dustParticles.begin() + i); i--; }
    }
    
    // Spawna partículas se o tanque estiver se movendo rápido o suficiente
    float speedSqr = (curVelX * curVelX) + (curVelY * curVelY);
    //if (speedSqr > 10.0f)
    //{
        /* --- EFEITOS DESATIVADOS (Para uso no futuro) ---
        trackSpawnTimer -= deltaTime;
        if (trackSpawnTimer <= 0.0f) {
            trackSpawnTimer = 0.1f; // Frequência do rastro no chão
            TrackMark tm;
            tm.position = position;
            tm.rotation = rotation;
            tm.lifeTime = 4.0f; // Dura 4 segundos e some
            tracks.push_back(tm);
        }
        
        dustSpawnTimer -= deltaTime;
        if (dustSpawnTimer <= 0.0f) {
            dustSpawnTimer = TANK_DUST_RATE; // Usa a configuração de densidade
            
            // Poeira sai na direção oposta ao movimento
            float len = sqrtf(speedSqr);
            Vector2 dir = { -velocity.x / len, -velocity.y / len };
            
            DustParticle dp;
            // Usa as configurações de Offsets e Espalhamento (Spread)
            dp.position.x = position.x + (dir.x * TANK_DUST_OFFSET) + GetRandomValue(-(int)TANK_DUST_SPREAD, (int)TANK_DUST_SPREAD);
            dp.position.y = position.y + (dir.y * TANK_DUST_OFFSET) + GetRandomValue(-(int)TANK_DUST_SPREAD, (int)TANK_DUST_SPREAD);
            
            dp.velocity.x = dir.x * 15.0f + GetRandomValue(-10, 10);
            dp.velocity.y = dir.y * 15.0f + GetRandomValue(-10, 10);
            dp.maxLife = 0.5f + ((float)GetRandomValue(0, 5) / 10.0f); // Vive entre 0.5 e 1.0 seg
            dp.lifeTime = dp.maxLife;
            dp.radius = (float)GetRandomValue((int)TANK_DUST_MIN_RADIUS, (int)TANK_DUST_MAX_RADIUS);
            dustParticles.push_back(dp);
        }
        --------------------------------------------------- */
    //}
    
    if (isDestroyed)
    {
        // Atrito pesado: o tanque derrapa no chão (ferro com terra) até parar 
        // Freia o eixo X
        if (velocity.x > 0.0f) { velocity.x -= TANK_FRICTION * deltaTime; if (velocity.x < 0.0f) velocity.x = 0.0f; }
        else if (velocity.x < 0.0f) { velocity.x += TANK_FRICTION * deltaTime; if (velocity.x > 0.0f) velocity.x = 0.0f; }

        // Freia o eixo Y
        if (velocity.y > 0.0f) { velocity.y -= TANK_FRICTION * deltaTime; if (velocity.y < 0.0f) velocity.y = 0.0f; }
        else if (velocity.y < 0.0f) { velocity.y += TANK_FRICTION * deltaTime; if (velocity.y > 0.0f) velocity.y = 0.0f; }
        
        // Atualiza a fumaça local do tanque
        smokeAnimTimer += deltaTime;
        if (smokeAnimTimer >= 0.07f)
        {
            smokeAnimTimer = 0.0f;
            smokeFrame++;
            if (smokeFrame >= 7) smokeFrame = 0;
        }
    }
    else
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
            
            // Interpola a rotação do canhão para o alvo suavemente
            float turnSpeed = turretSpeed * deltaTime;
            if (fabs(diff) < turnSpeed) cannonRotation = targetAngle;
            else if (diff > 0.0f) cannonRotation += turnSpeed;
            else cannonRotation -= turnSpeed;
            
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
                    // Futuro: Instanciar uma bala inimiga aqui
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
    
    // Desativa se sair muito da tela
    if (position.x < -100 || position.x > 1200 || position.y > 900)
    {
        isActive = false;
    }
}

void Tank::DrawShadows() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    Color shadowColor = { 0, 0, 0, 100 }; // Sombra preta transparente
    
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
    
    // Corpo
    Texture2D tex = isDestroyed ? tankDestroyedFrame[type] : tankFrames[type][currentFrame];
    if (tex.id != 0)
    {
        float width = (float)tex.width * scale;
        float height = (float)tex.height * scale;
        
        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Rectangle destRec = { position.x, position.y, width, height };
        Vector2 origin = { width / 2.0f, height / 2.0f };
        
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




