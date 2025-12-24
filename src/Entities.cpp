#include "Entities.hpp"
#include <algorithm>

Obstacle::Obstacle(Vector2 p, Vector2 s, ObstacleType t) 
    : pos(p), size(s), type(t) {
    if (type == ObstacleType::Circle) {
        radius = std::min(size.x, size.y) / 2.0f;
    }
    // Vary colors slightly for visual interest using unified RNG
    color.r = (unsigned char)(80 + RandomFloat(0, 30));
    color.g = (unsigned char)(80 + RandomFloat(0, 30));
    color.b = (unsigned char)(80 + RandomFloat(0, 30));
    color.a = 255;
}

bool Obstacle::Contains(Vector2 point) const {
    switch (type) {
        case ObstacleType::Circle: {
            Vector2 center = {pos.x + size.x / 2, pos.y + size.y / 2};
            return Vector2Distance(point, center) <= radius;
        }
        case ObstacleType::L_Shape: {
            bool inVertical = point.x >= pos.x && point.x <= pos.x + size.x * 0.3f &&
                             point.y >= pos.y && point.y <= pos.y + size.y;
            bool inHorizontal = point.x >= pos.x && point.x <= pos.x + size.x &&
                               point.y >= pos.y + size.y * 0.7f && point.y <= pos.y + size.y;
            return inVertical || inHorizontal;
        }
        case ObstacleType::Corridor: {
            bool inWall = point.x >= pos.x && point.x <= pos.x + size.x &&
                         point.y >= pos.y && point.y <= pos.y + size.y;
            float relX = (point.x - pos.x) / size.x;
            bool inGap = (relX > 0.35f && relX < 0.45f) || (relX > 0.55f && relX < 0.65f);
            return inWall && !inGap;
        }
        default: // Wall
            return point.x >= pos.x && point.x <= pos.x + size.x &&
                   point.y >= pos.y && point.y <= pos.y + size.y;
    }
}

bool Obstacle::Intersects(Vector2 point, float checkRadius) const {
    switch (type) {
        case ObstacleType::Circle: {
            Vector2 center = {pos.x + size.x / 2, pos.y + size.y / 2};
            return Vector2Distance(point, center) <= (radius + checkRadius);
        }
        case ObstacleType::L_Shape: {
            Rectangle vert = {pos.x, pos.y, size.x * 0.3f, size.y};
            Rectangle horiz = {pos.x, pos.y + size.y * 0.7f, size.x, size.y * 0.3f};
            return CheckCollisionCircleRec(point, checkRadius, vert) ||
                   CheckCollisionCircleRec(point, checkRadius, horiz);
        }
        case ObstacleType::Corridor: {
            Rectangle full = {pos.x, pos.y, size.x, size.y};
            if (!CheckCollisionCircleRec(point, checkRadius, full)) return false;
            float relX = (point.x - pos.x) / size.x;
            bool inGap = (relX > 0.35f && relX < 0.45f) || (relX > 0.55f && relX < 0.65f);
            return !inGap;
        }
        default: { // Wall
            float closestX = std::clamp(point.x, pos.x, pos.x + size.x);
            float closestY = std::clamp(point.y, pos.y, pos.y + size.y);
            float distX = point.x - closestX;
            float distY = point.y - closestY;
            return (distX * distX + distY * distY) < (checkRadius * checkRadius);
        }
    }
}

void Obstacle::Draw() const {
    Color outlineCol = { (unsigned char)(color.r + 40), (unsigned char)(color.g + 40), (unsigned char)(color.b + 40), 255 };
    
    switch (type) {
        case ObstacleType::Circle: {
            Vector2 center = {pos.x + size.x / 2, pos.y + size.y / 2};
            DrawCircleV(center, radius, color);
            DrawCircleLines(center.x, center.y, radius, outlineCol);
            break;
        }
        case ObstacleType::L_Shape: {
            DrawRectangle(pos.x, pos.y, size.x * 0.3f, size.y, color);
            DrawRectangleLines(pos.x, pos.y, size.x * 0.3f, size.y, outlineCol);
            DrawRectangle(pos.x, pos.y + size.y * 0.7f, size.x, size.y * 0.3f, color);
            DrawRectangleLines(pos.x, pos.y + size.y * 0.7f, size.x, size.y * 0.3f, outlineCol);
            break;
        }
        case ObstacleType::Corridor: {
            DrawRectangleV(pos, size, color);
            Color gapColor = {30, 30, 35, 255};
            float gapWidth = size.x * 0.1f;
            DrawRectangle(pos.x + size.x * 0.35f, pos.y, gapWidth, size.y, gapColor);
            DrawRectangle(pos.x + size.x * 0.55f, pos.y, gapWidth, size.y, gapColor);
            DrawRectangleLinesEx({pos.x, pos.y, size.x, size.y}, 2, outlineCol);
            break;
        }
        default: // Wall
            DrawRectangleV(pos, size, color);
            DrawRectangleLinesEx({pos.x, pos.y, size.x, size.y}, 2, outlineCol);
            break;
    }
}
