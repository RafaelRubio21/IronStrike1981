#include "Player.h"

void Player::Initialize(Vector2 startPos)
{
    position = startPos;
    velocity = {0.0f, 0.0f};
    acceleration = 1800.0f;
    friction = 6.0f;
    
    hasSprite = false;
    hasRotor = false;
    rotorOffsetY = -28.0f; // Valor ideal definido pelo jogador

    hp = 100000;
    hitTimer = 0.0f;
    isDestroyed = false;
    justHitGround = false;

    rotorRotation = 0.0f;
    currentRotorSpeed = 0.0f; // Comeca totalmente parada
    targetRotorSpeed = 1500.0f; // Velocidade final alvo
    
    scale = 0.5f; // Comeca no chao (menor)
    
    // Carrega o Corpo
    sprite = LoadTexture("assets/sprites/helicopter/helicopter.png");
    if (sprite.id == 0) sprite = LoadTexture("../../assets/sprites/helicopter/helicopter.png");
    if (sprite.id == 0) sprite = LoadTexture("../../../assets/sprites/helicopter/helicopter.png");
    destroyedSprite = LoadTexture("assets/sprites/helicopter/helicopter_brocken.png");
    if (destroyedSprite.id == 0) destroyedSprite = LoadTexture("../../assets/sprites/helicopter/helicopter_brocken.png");
    if (destroyedSprite.id == 0) destroyedSprite = LoadTexture("../../../assets/sprites/helicopter/helicopter_brocken.png");
    if (sprite.id != 0) hasSprite = true;

    // Carrega a Helice
    rotorSprite = LoadTexture("assets/sprites/helicopter/helice.png");
    if (rotorSprite.id == 0) rotorSprite = LoadTexture("../../assets/sprites/helicopter/helice.png");
    if (rotorSprite.id == 0) rotorSprite = LoadTexture("../../../assets/sprites/helicopter/helice.png");
    destroyedRotorSprite = LoadTexture("assets/sprites/helicopter/helice_brocken.png");
    if (destroyedRotorSprite.id == 0) destroyedRotorSprite = LoadTexture("../../assets/sprites/helicopter/helice_brocken.png");
    if (destroyedRotorSprite.id == 0) destroyedRotorSprite = LoadTexture("../../../assets/sprites/helicopter/helice_brocken.png");
    if (rotorSprite.id != 0) hasRotor = true;

    // Carrega o Fogo da Metralhadora
    hasMachineGun = false;
    isShooting = false;
    mgCurrentFrame = 0;
    mgFrameTimer = 0.0f;
    mgOffsetY = -114.0f; // Ajuste fino pro bico do helicoptero

    bullets.clear();
    bulletSpeed = 1200.0f; // Velocidade da bolinha
    mgFireRate = 0.08f;    // O intervalo entre um tiro e outro (quanto menor, mais rapido atira)

    machineGunSprite = LoadTexture("assets/sprites/helicopter/machine_gun.png");
    if (machineGunSprite.id == 0) machineGunSprite = LoadTexture("../../assets/sprites/helicopter/machine_gun.png");
    if (machineGunSprite.id == 0) machineGunSprite = LoadTexture("../../../assets/sprites/helicopter/machine_gun.png");
    if (machineGunSprite.id != 0) hasMachineGun = true;

    // Carrega os Sons
    engineLoopActive = false;
    
    engineStartingSound = LoadSound("assets/audio/helicopter/engine_starting.ogg");
    if (engineStartingSound.frameCount == 0) engineStartingSound = LoadSound("../../assets/audio/helicopter/engine_starting.ogg");
    if (engineStartingSound.frameCount == 0) engineStartingSound = LoadSound("../../../assets/audio/helicopter/engine_starting.ogg");

    engineLoopMusic = LoadMusicStream("assets/audio/helicopter/engine.ogg");
    if (engineLoopMusic.frameCount == 0) engineLoopMusic = LoadMusicStream("../../assets/audio/helicopter/engine.ogg");
    if (engineLoopMusic.frameCount == 0) engineLoopMusic = LoadMusicStream("../../../assets/audio/helicopter/engine.ogg");
    engineLoopMusic.looping = true; // Define que esse vai tocar pra sempre

    // -------------------------------------------------------------
    // AQUI VOCE AJUSTA O VOLUME DOS SONS (0.0f a 1.0f)
    // -------------------------------------------------------------
    if (engineStartingSound.frameCount != 0) SetSoundVolume(engineStartingSound, 0.3f); // 30%
    if (engineLoopMusic.frameCount != 0) SetMusicVolume(engineLoopMusic, 0.3f); // 30%

    // Carrega os sons da arma
    wasShooting = false;
    
    mgShootSound = LoadSound("assets/audio/helicopter/machine_gun.ogg");
    if (mgShootSound.frameCount == 0) mgShootSound = LoadSound("../../assets/audio/helicopter/machine_gun.ogg");
    if (mgShootSound.frameCount == 0) mgShootSound = LoadSound("../../../assets/audio/helicopter/machine_gun.ogg");

    mgFinalShotSound = LoadSound("assets/audio/helicopter/machine_gun_final_shot.ogg");
    if (mgFinalShotSound.frameCount == 0) mgFinalShotSound = LoadSound("../../assets/audio/helicopter/machine_gun_final_shot.ogg");
    if (mgFinalShotSound.frameCount == 0) mgFinalShotSound = LoadSound("../../../assets/audio/helicopter/machine_gun_final_shot.ogg");

    if (mgShootSound.frameCount != 0) SetSoundVolume(mgShootSound, 0.5f);
    if (mgFinalShotSound.frameCount != 0) SetSoundVolume(mgFinalShotSound, 0.5f);

    // O timer vai segurar a inicializacao do motor por meio segundo (0.5f)
    engineStartDelayTimer = 1.5f; 
}

void Player::Update(float deltaTime)
{
    // Atualiza a posicao de todos os tiros criados
    for (int i = 0; i < bullets.size(); i++)
    {
        bullets[i].y -= bulletSpeed * deltaTime; // Tiro sobe a tela
        
        if (bullets[i].y < -50) // Se saiu da tela por cima, apaga da memoria
        {
            bullets.erase(bullets.begin() + i);
            i--; 
        }
    }
    if (hitTimer > 0.0f) hitTimer -= deltaTime;
    if (isDestroyed) {
        isShooting = false; // Força parar de atirar ao morrer
        if (scale > 0.5f) {
            scale -= 0.5f * deltaTime;
            if (scale <= 0.5f) {
                scale = 0.5f;
                justHitGround = true;
            }
        }
        targetRotorSpeed = 0.0f;
        if (velocity.x > 0.0f) { velocity.x -= friction * 200.0f * deltaTime; if (velocity.x < 0.0f) velocity.x = 0.0f; }
        else if (velocity.x < 0.0f) { velocity.x += friction * 200.0f * deltaTime; if (velocity.x > 0.0f) velocity.x = 0.0f; }
        if (velocity.y > 0.0f) { velocity.y -= friction * 200.0f * deltaTime; if (velocity.y < 0.0f) velocity.y = 0.0f; }
        else if (velocity.y < 0.0f) { velocity.y += friction * 200.0f * deltaTime; if (velocity.y > 0.0f) velocity.y = 0.0f; }
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        currentRotorSpeed += (targetRotorSpeed - currentRotorSpeed) * 2.0f * deltaTime;
        rotorRotation += currentRotorSpeed * deltaTime;
        if (rotorRotation > 360.0f) rotorRotation -= 360.0f;
        return;
    }

    // LOGICA DO DELAY DE PARTIDA
    if (engineStartDelayTimer > 0.0f)
    {
        engineStartDelayTimer -= deltaTime;
        if (engineStartDelayTimer <= 0.0f)
        {
            // O timer zerou! Dá o Play no som de inicialização
            if (engineStartingSound.frameCount != 0) PlaySound(engineStartingSound);
        }
    }
    else
    {
        // LOGICA DO SOM DO MOTOR
        if (!engineLoopActive) 
        {
            // Se o som de inicializacao ja foi carregado E parou de tocar (acabou o audio)
            if (engineStartingSound.frameCount != 0 && !IsSoundPlaying(engineStartingSound)) 
            {
                engineLoopActive = true;
                if (engineLoopMusic.frameCount != 0) PlayMusicStream(engineLoopMusic); // Liga o Loop contínuo!
            }
        } 
        else 
        {
            // Regra do Raylib: Todo MusicStream precisa ser atualizado todo frame para continuar tocando
            if (engineLoopMusic.frameCount != 0) UpdateMusicStream(engineLoopMusic);
        }

        // Acelera a helice gradativamente ao longo do tempo (ganha 500 graus por segundo, a cada segundo)
        if (currentRotorSpeed < targetRotorSpeed)
        {
            currentRotorSpeed += 500.0f * deltaTime; 
            if (currentRotorSpeed > targetRotorSpeed) 
                currentRotorSpeed = targetRotorSpeed;
        }

        // Quando o motor pega forca (700 graus/s), o helicoptero "levanta voo"
        if (currentRotorSpeed > 700.0f)
        {
            if (scale < 1.0f)
            {
                scale += 0.1f * deltaTime; // Sobe suavemente
                if (scale > 1.0f) scale = 1.0f;
            }
        }
    }

    // Gira a helice usando a velocidade atual (sempre roda, mesmo que seja zero)
    rotorRotation += currentRotorSpeed * deltaTime;
    if (rotorRotation >= 360.0f) rotorRotation -= 360.0f;

    // O jogador so pode se mover quando o helicoptero terminar a decolagem (escala = 1.0)
    if (scale >= 1.0f)
    {
        Vector2 input = {0.0f, 0.0f};

        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  input.x -= 1.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.x += 1.0f;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))    input.y -= 1.0f;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))  input.y += 1.0f;

        // Acelera baseado no botão apertado
        velocity.x += input.x * acceleration * deltaTime;
        velocity.y += input.y * acceleration * deltaTime;
    }

    // O atrito (resistencia do ar) aplica independente do jogador estar decolando ou nao,
    // garantindo que ele sempre pare suavemente
    velocity.x -= velocity.x * friction * deltaTime;
    velocity.y -= velocity.y * friction * deltaTime;

    // Aplica a velocidade na posicao da tela
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Prende na tela
    if (position.x < 20) position.x = 20;
    if (position.x > 1024 - 20) position.x = 1024 - 20;
    if (position.y < 20) position.y = 20;
    if (position.y > 768 - 20) position.y = 768 - 20;

    // LOGICA DA METRALHADORA (So atira se o motor ja ligou)
    if (scale >= 1.0f) 
    {
        isShooting = (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL));
        if (isShooting && hasMachineGun)
        {
            mgFrameTimer += deltaTime;
            // Cria uma bala e roda a animacao a cada mgFireRate segundos
            if (mgFrameTimer >= mgFireRate) 
            {
                mgFrameTimer = 0.0f;
                mgCurrentFrame++;
                
                int maxFrames = machineGunSprite.width / 16;
                if (maxFrames <= 0) maxFrames = 1;

                if (mgCurrentFrame >= maxFrames) mgCurrentFrame = 0;

                // Spawna a bolinha bem na ponta do bico
                bullets.push_back({ position.x, position.y + (mgOffsetY * scale) });
                
                // Dispara o som a cada bala criada!
                if (mgShootSound.frameCount != 0) PlaySound(mgShootSound);
            }
        }
        else
        {
            mgCurrentFrame = 0; // Desliga a animacao se soltar o botão
            isShooting = false;
        }

        // Detecta exatamente o momento em que a tecla foi SOLTA
        if (wasShooting && !isShooting)
        {
            // Toca o som de eco/cauda do ultimo tiro
            if (mgFinalShotSound.frameCount != 0) PlaySound(mgFinalShotSound);
        }
        
        // Guarda o estado para o proximo frame
        wasShooting = isShooting;
    }


}

bool Player::CheckBulletHits(Rectangle targetRect)
{
    for (int i = 0; i < bullets.size(); i++)
    {
        // A bala é um projetil fino e alto. Criamos uma hitbox para ela
        Rectangle bulletRect = { bullets[i].x - 2.0f, bullets[i].y - 15.0f, 4.0f, 15.0f };
        
        if (CheckCollisionRecs(bulletRect, targetRect))
        {
            bullets.erase(bullets.begin() + i); // Apaga a bala da array (ela explode no tanque)
            return true; // Acertou!
        }
    }
    return false;
}

void Player::DrawShadows() const
{
    float altitude = scale - 0.6f;
    float shadowDistance = 15.0f + (altitude * 100.0f); 
    Vector2 shadowOffset = { shadowDistance, shadowDistance }; 

    if (hasSprite)
    {
        Texture2D tex = (isDestroyed && scale <= 0.5f && destroyedSprite.id != 0) ? destroyedSprite : sprite;
        Vector2 shadowDrawPos = { 
            position.x + shadowOffset.x - (tex.width * scale) / 2.0f, 
            position.y + shadowOffset.y - (tex.height * scale) / 2.0f 
        };
        // Atenção: O Alpha quem controla agora é o Game.cpp no Framebuffer Global
        // Aqui desenhamos preto 100% solido (BLACK)
        DrawTextureEx(tex, shadowDrawPos, 0.0f, scale, BLACK);
    }

    if (hasRotor)
    {
        Texture2D tex = (isDestroyed && scale <= 0.5f && destroyedRotorSprite.id != 0) ? destroyedRotorSprite : rotorSprite;
        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Vector2 origin = { (tex.width * scale) / 2.0f, (tex.height * scale) / 2.0f };
        Rectangle shadowDestRec = { position.x + shadowOffset.x, position.y + shadowOffset.y + (rotorOffsetY * scale), tex.width * scale, tex.height * scale };
        
        DrawTexturePro(tex, sourceRec, shadowDestRec, origin, rotorRotation, BLACK);
    }
}

void Player::DrawBody() const
{
    // -------------------------------------------------------------
    // ETAPA 3: DESENHA O HELICOPTERO REAL POR CIMA
    // -------------------------------------------------------------
    Color tintColor = WHITE;
    if (isDestroyed && scale > 0.5f) tintColor = GRAY; // Fica cinza apenas enquanto está caindo (fumaça/falha)
    else if (hitTimer > 0.0f && !isDestroyed) tintColor = RED;
    
    if (hasSprite)
    {
        Texture2D tex = (isDestroyed && scale <= 0.5f && destroyedSprite.id != 0) ? destroyedSprite : sprite;
        Vector2 drawPos = { 
            position.x - (tex.width * scale) / 2.0f, 
            position.y - (tex.height * scale) / 2.0f 
        };
        DrawTextureEx(tex, drawPos, 0.0f, scale, tintColor);
    }
    else
    {
        DrawRectangle(position.x - 15, position.y - 15, 30, 30, isDestroyed ? GRAY : tintColor);
    }

    // -------------------------------------------------------------
    // ETAPA 3.5: DESENHA O FOGO DA METRALHADORA
    // -------------------------------------------------------------
    if (hasMachineGun && isShooting)
    {
        // Pega o quadro atual multiplicando o indice por 16px (largura do quadro)
        Rectangle sourceRec = { (float)(mgCurrentFrame * 16), 0.0f, 16.0f, 24.0f };
        Vector2 origin = { (16.0f * scale) / 2.0f, (24.0f * scale) / 2.0f };
        
        // Coloca no bico usando mgOffsetY
        Rectangle destRec = { position.x, position.y + (mgOffsetY * scale), 16.0f * scale, 24.0f * scale };
        
        DrawTexturePro(machineGunSprite, sourceRec, destRec, origin, 0.0f, WHITE);
    }

    // -------------------------------------------------------------
    // ETAPA 4: DESENHA A HELICE POR CIMA DE TUDO
    // -------------------------------------------------------------
    if (hasRotor)
    {
        Texture2D tex = (isDestroyed && scale <= 0.5f && destroyedRotorSprite.id != 0) ? destroyedRotorSprite : rotorSprite;
        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Vector2 origin = { (tex.width * scale) / 2.0f, (tex.height * scale) / 2.0f };
        Rectangle destRec = { position.x, position.y + (rotorOffsetY * scale), tex.width * scale, tex.height * scale };
        
        DrawTexturePro(tex, sourceRec, destRec, origin, rotorRotation, isDestroyed && scale > 0.5f ? GRAY : tintColor);
    }
    else
    {
        DrawCircle(position.x, position.y - 15.0f, 10, DARKGRAY); 
    }

    // -------------------------------------------------------------
    // ETAPA 5: DESENHA AS BALAS (Efeito Tracer / Rajada incandescente)
    // -------------------------------------------------------------
    for (const auto& b : bullets)
    {
        // 1. Rastro da bala (uma linha que simula a velocidade)
        DrawLineEx({b.x, b.y + 15.0f}, {b.x, b.y}, 3.0f, Fade(ORANGE, 0.6f));
        
        // 2. Ponta da bala (brilho amarelo)
        DrawCircleV(b, 2.5f, YELLOW);
        
        // 3. Núcleo ultra-quente da bala (branco puro)
        DrawCircleV(b, 1.0f, WHITE);
    }
}



Rectangle Player::GetHitbox() const
{
    if (!hasSprite) return {0,0,0,0};
    float w = (float)sprite.width * scale * 0.4f;
    float h = (float)sprite.height * scale * 0.8f;
    return { position.x - w/2.0f, position.y - h/2.0f, w, h };
}

void Player::TakeDamage(int damage)
{
    if (isDestroyed) return;
    hp -= damage;
    hitTimer = 0.1f;
    if (hp <= 0)
    {
        hp = 0;
        isDestroyed = true;
        
        // Corta os sons do motor do helicóptero imediatamente (evita stuttering/loop quebrado)
        if (engineLoopMusic.frameCount != 0) StopMusicStream(engineLoopMusic);
        if (engineStartingSound.frameCount != 0) StopSound(engineStartingSound);
    }
}



void Player::Unload()
{
    if (sprite.id != 0) { UnloadTexture(sprite); sprite.id = 0; }
    if (destroyedSprite.id != 0) { UnloadTexture(destroyedSprite); destroyedSprite.id = 0; }
    if (rotorSprite.id != 0) { UnloadTexture(rotorSprite); rotorSprite.id = 0; }
    if (destroyedRotorSprite.id != 0) { UnloadTexture(destroyedRotorSprite); destroyedRotorSprite.id = 0; }
    if (machineGunSprite.id != 0) { UnloadTexture(machineGunSprite); machineGunSprite.id = 0; }
    hasSprite = false;
    hasRotor = false;
    hasMachineGun = false;

    if (engineStartingSound.frameCount != 0) { UnloadSound(engineStartingSound); engineStartingSound = {}; }
    if (mgShootSound.frameCount != 0) { UnloadSound(mgShootSound); mgShootSound = {}; }
    if (mgFinalShotSound.frameCount != 0) { UnloadSound(mgFinalShotSound); mgFinalShotSound = {}; }
    if (engineLoopMusic.frameCount != 0) { UnloadMusicStream(engineLoopMusic); engineLoopMusic = {}; }
    engineLoopActive = false;

    bullets.clear();
}
