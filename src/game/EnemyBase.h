#pragma once
#include "raylib.h"
#include "Constants.h"
#include <cmath>

class EnemyBase
{
public:
    virtual ~EnemyBase() = default;

    // Métodos virtuais puros que todo inimigo OBRIGATORIAMENTE tem que implementar
    virtual void Update(float deltaTime, Vector2 playerPos, bool playerDestroyed, float scrollSpeed = 0.0f) = 0;
    virtual void DrawShadows() const = 0;
    virtual void DrawBody() const = 0;
    virtual void DrawGroundEffects() const {} // Rastros, esteiras d'água, etc.
    
    virtual Rectangle GetHitbox() const 
    {
        // Fator de redução (0.7 = 70% do tamanho da imagem)
        // Isso evita que tiros acertem os pixels transparentes nas bordas.
        float hitScale = 0.7f; 
        
        float scaledWidth = (width * scale) * hitScale;
        float scaledHeight = (height * scale) * hitScale;
        
        // Ângulo módulo 180: 170 graus deixa o sprite tão "de pé" quanto 10 graus
        float angle = std::fmod(std::fabs(rotation), 180.0f);
        if (angle > 45.0f && angle < 135.0f)
        {
            float temp = scaledWidth;
            scaledWidth = scaledHeight;
            scaledHeight = temp;
        }
        
        // Assume origin (pivot) is exactly in the center for all enemies
        return { 
            position.x - (scaledWidth / 2.0f), 
            position.y - (scaledHeight / 2.0f), 
            scaledWidth, 
            scaledHeight 
        };
    }
    
    // Método virtual que as classes filhas PODEM sobrescrever (ex: tocar som, limpar partículas)
    virtual void Destroy() 
    {
        isDestroyed = true;
    }

    // Método que todos os inimigos compartilham exatamente igual
    virtual void TakeDamage(int damage)
    {
        if (isDestroyed) return;
        
        hp -= damage;
        hitTimer = 0.05f; // Tempo piscando vermelho
        
        if (hp <= 0)
        {
            hp = 0;
            Destroy(); // Chama o Destroy do filho (Polimorfismo!)
        }
    }

    // =====================================================================
    // BASE UPDATE - "Trabalho Sujo" universal que TODOS os inimigos fazem
    // Chame EnemyBase::BaseUpdate(deltaTime) no início do Update() do filho.
    // =====================================================================
    void BaseUpdate(float deltaTime)
    {
        if (!isActive) return;
        
        // Timer de flash de dano (piscar vermelho)
        if (hitTimer > 0.0f) hitTimer -= deltaTime;
        
        // Física básica: aplica velocidade na posição
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        
        // Culling: desativa o inimigo se ele sair completamente da tela
        if (position.x < -Config::CULL_MARGIN || position.x > Config::SCREEN_WIDTH + Config::CULL_MARGIN ||
            position.y < -Config::CULL_MARGIN || position.y > Config::SCREEN_HEIGHT + Config::CULL_MARGIN)
        {
            isActive = false;
        }
        
        // Atrito ao ser destruído e Animação de Fumaça Universal
        if (isDestroyed)
        {
            if (velocity.x > 0.0f) { velocity.x -= friction * deltaTime; if (velocity.x < 0.0f) velocity.x = 0.0f; }
            else if (velocity.x < 0.0f) { velocity.x += friction * deltaTime; if (velocity.x > 0.0f) velocity.x = 0.0f; }
            
            if (velocity.y > 0.0f) { velocity.y -= friction * deltaTime; if (velocity.y < 0.0f) velocity.y = 0.0f; }
            else if (velocity.y < 0.0f) { velocity.y += friction * deltaTime; if (velocity.y > 0.0f) velocity.y = 0.0f; }
            
            // Fumaça animada universal para inimigos abatidos
            smokeAnimTimer += deltaTime;
            if (smokeAnimTimer >= 0.07f)
            {
                smokeAnimTimer = 0.0f;
                smokeFrame++;
                if (smokeFrame >= 7) smokeFrame = 0;
            }
        }
    }

    // Propriedades Universais
    int type = 0;
    int hp = 1;
    
    bool isDestroyed = false;
    bool isActive = false;
    bool hasFired = false;
    
    float hitTimer = 0.0f;
    float shootCooldown = 0.0f;
    float friction = 150.0f; // Força de frenagem ao ser destruído (derrapada)
    
    Vector2 position = {0, 0};
    Vector2 velocity = {0, 0};
    float rotation = 0.0f;
    float scale = 1.0f;
    float speed = 0.0f;
    float cannonRotation = 0.0f;
    float fireOffsetY = 0.0f;
    
    // Variáveis da animação de fumaça (inicia aleatório para inimigos não fumegarem em sincronia)
    int smokeFrame = 0;
    float smokeAnimTimer = 0.0f;
    
    // Dimensões do inimigo (usado pela hitbox universal)
    float width = 50.0f;
    float height = 50.0f;
};






