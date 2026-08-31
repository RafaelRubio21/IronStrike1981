#include "Player.h"

void Player::Initialize(Vector2 startPos)
{
    position = startPos;
    velocity = {0.0f, 0.0f};
    acceleration = 1800.0f;
    friction = 6.0f;
    
    hasSprite = false;
    hasRotor = false;
    rotorRotation = 0.0f;
    currentRotorSpeed = 0.0f; // Comeca totalmente parada
    targetRotorSpeed = 1500.0f; // Velocidade final alvo
    
    scale = 0.5f; // Comeca no chao (menor)
    
    // Carrega o Corpo
    sprite = LoadTexture("assets/sprites/helicopter.png");
    if (sprite.id == 0) sprite = LoadTexture("../../assets/sprites/helicopter.png");
    if (sprite.id == 0) sprite = LoadTexture("../../../assets/sprites/helicopter.png");
    if (sprite.id != 0) hasSprite = true;

    // Carrega a Helice
    rotorSprite = LoadTexture("assets/sprites/helice.png");
    if (rotorSprite.id == 0) rotorSprite = LoadTexture("../../assets/sprites/helice.png");
    if (rotorSprite.id == 0) rotorSprite = LoadTexture("../../../assets/sprites/helice.png");
    if (rotorSprite.id != 0) hasRotor = true;

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

    // O timer vai segurar a inicializacao do motor por meio segundo (0.5f)
    engineStartDelayTimer = 1.5f; 
}

void Player::Update(float deltaTime)
{
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
}

void Player::Render() const
{
    if (hasSprite)
    {
        // Desenha o corpo do helicoptero
        Vector2 drawPos = { 
            position.x - (sprite.width * scale) / 2.0f, 
            position.y - (sprite.height * scale) / 2.0f 
        };
        DrawTextureEx(sprite, drawPos, 0.0f, scale, WHITE);
    }
    else
    {
        DrawRectangle(position.x - 15, position.y - 15, 30, 30, GRAY);
    }

    if (hasRotor)
    {
        // -------------------------------------------------------------
        // AQUI VOCE AJUSTA A POSICAO DA HELICE!
        // Esse valor se ajusta automaticamente a escala agora.
        // -------------------------------------------------------------
        float rotorOffsetY = -28.0f; 

        // Para girar uma imagem pelo centro dela, usamos DrawTexturePro
        Rectangle sourceRec = { 0.0f, 0.0f, (float)rotorSprite.width, (float)rotorSprite.height };
        
        // Multiplicamos o Offset pelo 'scale' para que a helice acompanhe o corpo perfeitamente
        Rectangle destRec = { position.x, position.y + (rotorOffsetY * scale), rotorSprite.width * scale, rotorSprite.height * scale };
        Vector2 origin = { (rotorSprite.width * scale) / 2.0f, (rotorSprite.height * scale) / 2.0f };

        // Desenha a helice rodando
        DrawTexturePro(rotorSprite, sourceRec, destRec, origin, rotorRotation, WHITE);
    }
    else
    {
        // Rotor generico rodando
        DrawCircle(position.x, position.y - 15.0f, 10, DARKGRAY); 
    }
}
