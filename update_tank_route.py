import re

with open('src/game/Tank.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the previous messed up block and implement full AI
old_block = '''    // LOGICA DE WAYPOINTS (SE EXISTIR UMA ROTA)
    if (!waypoints.empty() && currentWaypoint < waypoints.size()) {
        // Converte a posição do próximo waypoint de WorldCoordinates para ScreenCoordinates
        Vector2 targetPos = waypoints[currentWaypoint];
        targetPos.y -= scrollY; // scrollY é passado para o Tank::Update via BaseUpdate? Não, recebemos scrollSpeed...
        // Espera, no Update do Tank a gente recebe scrollSpeed, mas não a posição da câmera!
        // Na verdade, os waypoints na Tiled são WorldCoordinates, assim como a 'position' original do tanque.
        // O tanque se move na TELA. Sua 'position' é em Tela.
        // Para calcular a direção, precisamos transformar o waypoint na Tela também!
        // Como não temos a câmera aqui, a gente pode fazer os waypoints "descerem" pela tela na mesma velocidade!
        // Ou seja, aplicamos o scrollSpeed neles também!
    }

    // Salva velocidade original e aplica o multiplicador de inércia antes do BaseUpdate
    Vector2 originalVel = velocity;
    velocity.x *= currentSpeedMult;
    velocity.y *= currentSpeedMult;'''

if old_block in content:
    pass # found
else:
    # try the original
    old_block = '''    // Salva velocidade original e aplica o multiplicador de inércia antes do BaseUpdate
    Vector2 originalVel = velocity;
    velocity.x *= currentSpeedMult;
    velocity.y *= currentSpeedMult;'''

new_block = '''    // Rola os waypoints para baixo na tela, para que eles fiquem grudados no chão do mapa!
    for (int i = 0; i < waypoints.size(); i++) {
        waypoints[i].y += scrollSpeed * deltaTime;
    }

    // NAVEGAÇÃO POR WAYPOINTS
    if (!waypoints.empty() && currentWaypoint < waypoints.size()) {
        Vector2 target = waypoints[currentWaypoint];
        
        // Direção até o waypoint
        float dx = target.x - position.x;
        float dy = target.y - position.y;
        float dist = sqrt(dx*dx + dy*dy);
        
        if (dist < 5.0f) {
            // Chegamos no waypoint! Pula pro próximo
            currentWaypoint++;
        } else {
            // Ajusta a velocidade para ir direto pro waypoint na velocidade máxima (60.0f)
            float speed = 60.0f;
            velocity.x = (dx / dist) * speed;
            velocity.y = (dy / dist) * speed;
            
            // Gira o chassi do tanque pra apontar pro waypoint
            // atan2 retorna em radianos, converte pra graus. +90 pq o tanque base aponta pra cima
            float targetRot = atan2(dy, dx) * 180.0f / PI + 90.0f;
            rotation = targetRot; 
        }
    }

    // Salva velocidade original e aplica o multiplicador de inércia antes do BaseUpdate
    Vector2 originalVel = velocity;
    velocity.x *= currentSpeedMult;
    velocity.y *= currentSpeedMult;'''

content = content.replace(old_block, new_block)

with open('src/game/Tank.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
