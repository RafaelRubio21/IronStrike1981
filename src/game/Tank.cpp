#include "Tank.h"
#include <cmath>

// =================================================================
// CONFIGURAÇÕES GERAIS DOS TANQUES (Fique à vontade para alterar!)
// =================================================================
static int TANK_MAX_HP = 5;               // Tiros necessarios para destruir
static float TANK_SPEED = 60.0f;          // Velocidade de movimento
static float TANK_FRICTION = 150.0f;      // Força de frenagem ao ser destruido (derrapada)
static float TANK_TURRET_SPEED = 20.0f;   // Velocidade de giro da torreta (graus por segundo)
static float TANK_VOL_EXPLOSION = 0.5f;   // Volume da explosão (0.0 a 1.0)
static float TANK_VOL_ENGINE = 0.15f;     // Volume do motor (0.0 a 1.0)
static float TANK_VOL_SHOOTING = 0.4f;    // Volume do tiro inimigo (0.0 a 1.0)
static float TANK_CANNON_OFFSET_Y = -25.0f; // Ajuste o encaixe do canhão (Pra frente / trás)
// =================================================================

// Variáveis globais/estáticas para carregar texturas e áudios apenas uma vez
static Texture2D tankFrames[4] = {0};
static Texture2D tankDestroyedFrame = {0};
static Texture2D cannonFrames[3] = {0};
static Texture2D cannonDestroyedFrame = {0};
static bool tankTexturesLoaded = false;

static Sound tankExplodingSnd = {0};
static Sound tankMovingSnd = {0};
static Sound tankShootingSnd = {0};
static bool tankAudioLoaded = false;

void Tank::Initialize(Vector2 startPos, int spawnDirection)
{
    position = startPos;
    isDestroyed = false;
    isActive = true;
    
    scale = 1.0f; // 1.0 = Tamanho original de 67x94
    currentFrame = 0;
    frameTimer = 0.0f;
    
    cannonFrame = 0;
    cannonAnimTimer = 0.0f;
    isCannonShooting = false;
    shootCooldown = (float)GetRandomValue(20, 50) / 10.0f; // 2 a 5 segundos pro primeiro tiro
    
    hp = TANK_MAX_HP; 
    hitTimer = 0.0f;
    
    // O Sprite aponta nativamente para BAIXO
    if (spawnDirection == 0)
    {
        velocity = { 0.0f, TANK_SPEED }; // Desce
        rotation = 0.0f;
    }
    else if (spawnDirection == 1)
    {
        velocity = { TANK_SPEED, 0.0f }; // Vai pra direita
        rotation = -90.0f; // Vira pra direita
    }
    else if (spawnDirection == 2)
    {
        velocity = { -TANK_SPEED, 0.0f }; // Vai pra esquerda
        rotation = 90.0f; // Vira pra esquerda
    }
    
    cannonRotation = rotation; // O canhão nasce alinhado com o chassi
    
    // Inicia a animação da fumaça em um frame aleatório para não sincronizar com outros tanques!
    smokeFrame = GetRandomValue(0, 6);
    smokeAnimTimer = 0.0f;
    
    if (!tankTexturesLoaded)
    {
        // Arrays com os possíveis caminhos de diretório para o Visual Studio
        const char* paths[] = {
            "assets/sprites/enemies/tank/",
            "../../assets/sprites/enemies/tank/",
            "../../../assets/sprites/enemies/tank/"
        };
        
        for (int i = 0; i < 3; i++)
        {
            if (tankFrames[0].id == 0) // Se ainda nao achou
            {
                tankFrames[0] = LoadTexture(TextFormat("%stank1.png", paths[i]));
                tankFrames[1] = LoadTexture(TextFormat("%stank2.png", paths[i]));
                tankFrames[2] = LoadTexture(TextFormat("%stank3.png", paths[i]));
                tankFrames[3] = LoadTexture(TextFormat("%stank4.png", paths[i]));
                tankDestroyedFrame = LoadTexture(TextFormat("%stank_destroyed.png", paths[i]));
                
                cannonFrames[0] = LoadTexture(TextFormat("%sturret1.png", paths[i]));
                cannonFrames[1] = LoadTexture(TextFormat("%sturret2.png", paths[i]));
                cannonFrames[2] = LoadTexture(TextFormat("%sturret3.png", paths[i]));
                cannonDestroyedFrame = LoadTexture(TextFormat("%sturret_destroyed.png", paths[i]));
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

void Tank::Update(float deltaTime, Vector2 playerPos)
{
    if (!isActive) return;
    
    if (hitTimer > 0.0f) hitTimer -= deltaTime;
    
    // Movimento
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
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
        float dx = playerPos.x - position.x;
        float dy = playerPos.y - position.y;
        float targetAngle = atan2f(dy, dx) * (180.0f / PI);
        
        // Ajusta para o sprite que nativamente aponta para baixo (+90 graus)
        targetAngle -= 90.0f; 
        
        // Lógica de Giro Suave da Torreta para o targetAngle
        float diff = fmodf(targetAngle - cannonRotation, 360.0f);
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;
        
        float turnSpeed = TANK_TURRET_SPEED * deltaTime;
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
                cannonFrame = 0;
                cannonAnimTimer = 0.0f;
                shootCooldown = (float)GetRandomValue(30, 60) / 10.0f; // Proximo tiro em 3s a 6s
                
                if (tankShootingSnd.frameCount != 0) PlaySound(tankShootingSnd);
                // Futuro: Instanciar uma bala inimiga aqui
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
        
        // Animação das lagartas
        frameTimer += deltaTime;
        if (frameTimer >= 0.05f) // 20 FPS (Mais rápido e suave)
        {
            frameTimer = 0.0f;
            currentFrame++;
            if (currentFrame >= 4) currentFrame = 0; 
        }
        
        // Mantém o som do motor tocando se o tanque estiver se movendo
        if (tankMovingSnd.frameCount != 0)
        {
            if (!IsSoundPlaying(tankMovingSnd))
            {
                PlaySound(tankMovingSnd);
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
    
    Texture2D tex = isDestroyed ? tankDestroyedFrame : tankFrames[currentFrame];
    
    float width = (float)tex.width * scale;
    float height = (float)tex.height * scale;
    
    Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    
    // Sombra do corpo
    Vector2 shadowOffset = { 8.0f * scale, 8.0f * scale };
    Rectangle destRec = { position.x + shadowOffset.x, position.y + shadowOffset.y, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };
    
    DrawTexturePro(tex, sourceRec, destRec, origin, rotation, BLACK);
    
    // Sombra do Canhão
    if (cannonFrames[0].id != 0)
    {
        Texture2D cTex = isDestroyed ? cannonDestroyedFrame : cannonFrames[cannonFrame];
        float cWidth = (float)cTex.width * scale;
        float cHeight = (float)cTex.height * scale;
        Rectangle cSource = { 0.0f, 0.0f, (float)cTex.width, (float)cTex.height };
        
        // Canhão é mais alto, então a sombra é projetada um pouquinho mais longe
        // Se estiver destruído, a sombra "cai" pro chão (mesma altura da carcaça)
        Vector2 cShadowOffset = isDestroyed ? shadowOffset : Vector2{ 12.0f * scale, 12.0f * scale };
        Rectangle cDest = { position.x + cShadowOffset.x, position.y + cShadowOffset.y, cWidth, cHeight };
        
        // Aplica o ajuste fino de encaixe preservando a rotação local
        Vector2 cOrigin = { cWidth / 2.0f, (cHeight / 2.0f) + (TANK_CANNON_OFFSET_Y * scale) };
        
        DrawTexturePro(cTex, cSource, cDest, cOrigin, cannonRotation, BLACK);
    }
}

void Tank::DrawBody() const
{
    if (!isActive || !tankTexturesLoaded) return;
    
    Texture2D tex = isDestroyed ? tankDestroyedFrame : tankFrames[currentFrame];
    
    float width = (float)tex.width * scale;
    float height = (float)tex.height * scale;
    
    Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    Rectangle destRec = { position.x, position.y, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };
    
    // Se tomou um tiro recentemente, pisca de vermelho!
    Color tintColor = (hitTimer > 0.0f && !isDestroyed) ? RED : WHITE;
    
    DrawTexturePro(tex, sourceRec, destRec, origin, rotation, tintColor);
    
    // Canhão (Desenhado por cima do corpo)
    if (cannonFrames[0].id != 0)
    {
        Texture2D cTex = isDestroyed ? cannonDestroyedFrame : cannonFrames[cannonFrame];
        float cWidth = (float)cTex.width * scale;
        float cHeight = (float)cTex.height * scale;
        Rectangle cSource = { 0.0f, 0.0f, (float)cTex.width, (float)cTex.height };
        Rectangle cDest = { position.x, position.y, cWidth, cHeight };
        
        // Aplica o ajuste fino de encaixe preservando a rotação local
        Vector2 cOrigin = { cWidth / 2.0f, (cHeight / 2.0f) + (TANK_CANNON_OFFSET_Y * scale) };
        
        // Se tomou tiro, o canhão também deve piscar junto com o corpo
        // Se destruído, a cor natural da imagem destruída prevalece (WHITE)
        DrawTexturePro(cTex, cSource, cDest, cOrigin, cannonRotation, isDestroyed ? WHITE : tintColor);
    }
}
