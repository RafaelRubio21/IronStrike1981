#include "Player.h"

void Player::Initialize(Vector2 startPos)
{
    position = startPos;
    speed = 300.0f; // Pixels por segundo
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
}

void Player::Update(float deltaTime)
{
    // O jogador so pode se mover quando o helicoptero terminar a decolagem (escala = 1.0)
    if (scale >= 0.7f)
    {
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  position.x -= speed * deltaTime;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) position.x += speed * deltaTime;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))    position.y -= speed * deltaTime;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))  position.y += speed * deltaTime;
    }

    // Acelera a helice gradativamente ao longo do tempo (ganha 500 graus por segundo, a cada segundo)
    if (currentRotorSpeed < targetRotorSpeed)
    {
        currentRotorSpeed += 500.0f * deltaTime; 
        if (currentRotorSpeed > targetRotorSpeed) 
            currentRotorSpeed = targetRotorSpeed;
    }

    // Gira a helice usando a velocidade atual
    rotorRotation += currentRotorSpeed * deltaTime;
    if (rotorRotation >= 360.0f) rotorRotation -= 360.0f;

    // Quando o motor pega forca (700 graus/s), o helicoptero "levanta voo"
    if (currentRotorSpeed > 700.0f)
    {
        if (scale < 1.0f)
        {
            scale += 0.15f * deltaTime; // Sobe suavemente
            if (scale > 1.0f) scale = 1.0f;
        }
    }

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
