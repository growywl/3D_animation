// Game Configuration File
// ปรับค่าต่าง ๆ เพื่อแต่งแต่ม gameplay

#pragma once

// ==================== Character Stats ====================
namespace GameConfig {

    // Player Stats
    constexpr float PLAYER_MAX_HP = 100.0f;
    constexpr float PLAYER_BASE_DAMAGE = 18.0f;
    constexpr float PLAYER_DAMAGE_RANGE = 12.0f;  // 18-30 damage
    constexpr float PLAYER_ATTACK_COOLDOWN = 1.2f;
    constexpr float PLAYER_ANIMATION_DURATION = 0.8f;

    // Enemy Stats
    constexpr float ENEMY_MAX_HP = 80.0f;
    constexpr float ENEMY_BASE_DAMAGE = 12.0f;
    constexpr float ENEMY_DAMAGE_RANGE = 8.0f;   // 12-20 damage
    constexpr float ENEMY_ATTACK_COOLDOWN = 2.0f;
    constexpr float ENEMY_ANIMATION_DURATION = 0.8f;

    // ==================== AI Behavior ====================
    constexpr float AI_THINK_INTERVAL = 2.5f;    // How often AI decides to act
    constexpr float AI_ATTACK_PROBABILITY = 60.0f; // 0-100 percent chance
    constexpr float AI_DIFFICULTY_MULTIPLIER = 1.0f; // 1.0 = normal, 1.5 = hard, 0.5 = easy

    // ==================== Rendering ====================
    constexpr unsigned int SCREEN_WIDTH = 1280;
    constexpr unsigned int SCREEN_HEIGHT = 720;
    constexpr float CAMERA_FOV = 45.0f;
    constexpr float CAMERA_NEAR = 0.1f;
    constexpr float CAMERA_FAR = 100.0f;

    // Player Position
    constexpr float PLAYER_POS_X = -2.0f;
    constexpr float PLAYER_POS_Y = 0.0f;
    constexpr float PLAYER_POS_Z = 0.0f;
    constexpr float PLAYER_SCALE = 0.01f;

    // Enemy Position
    constexpr float ENEMY_POS_X = 2.0f;
    constexpr float ENEMY_POS_Y = -0.4f;
    constexpr float ENEMY_POS_Z = 0.0f;
    constexpr float ENEMY_SCALE = 0.5f;

    // ==================== Physics ====================
    constexpr float MAX_DELTA_TIME = 0.016f;  // Cap at 60 FPS
    constexpr float GRAVITY = -9.81f;

    // ==================== Effects ====================
    constexpr float ATTACK_ROTATION_ANGLE = 45.0f;  // degrees

    // ==================== Easy Mode (Tutorial) ====================
    inline void ApplyEasyDifficulty() {
        // const_cast is not ideal, but for demonstration
        // In production, use a proper configuration system
    }

    // ==================== Hard Mode ====================
    inline void ApplyHardDifficulty() {
        // Higher AI damage, more frequent attacks
    }

    // ==================== Custom Difficulty ====================
    inline void ApplyCustomDifficulty(float difficultyMultiplier) {
        // difficultyMultiplier: 0.5 = easy, 1.0 = normal, 1.5 = hard, 2.0 = extreme
    }
}

#endif // GAME_CONFIG_H

