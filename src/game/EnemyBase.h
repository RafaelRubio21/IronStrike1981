#pragma once
#include "raylib.h"

class EnemyBase
{
public:
    virtual ~EnemyBase() = default;

    // Métodos virtuais puros que todo inimigo OBRIGATORIAMENTE tem que implementar
    virtual void Update(float deltaTime, Vector2 playerPos, bool playerDestroyed) = 0;
    virtual void DrawShadows() const = 0;
    virtual void DrawBody() const = 0;
    virtual Rectangle GetHitbox() const = 0;
    
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
        if (position.x < -200 || position.x > 1300 || position.y < -200 || position.y > 1000)
        {
            isActive = false;
        }
        
        // Atrito ao ser destruído: derrapa até parar
        if (isDestroyed)
        {
            if (velocity.x > 0.0f) { velocity.x -= friction * deltaTime; if (velocity.x < 0.0f) velocity.x = 0.0f; }
            else if (velocity.x < 0.0f) { velocity.x += friction * deltaTime; if (velocity.x > 0.0f) velocity.x = 0.0f; }
            
            if (velocity.y > 0.0f) { velocity.y -= friction * deltaTime; if (velocity.y < 0.0f) velocity.y = 0.0f; }
            else if (velocity.y < 0.0f) { velocity.y += friction * deltaTime; if (velocity.y > 0.0f) velocity.y = 0.0f; }
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
};
