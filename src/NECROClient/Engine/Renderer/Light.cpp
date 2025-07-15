#include "Light.h"

#include "NECROEngine.h"

namespace NECRO
{
namespace Client
{
    //-----------------------------------------------------------------
    // Initializes max and min intensity
    //-----------------------------------------------------------------
    void Light::Init(Entity* pOwner)
    {
        m_maxIntensity = m_intensity;
        m_minIntensity = m_maxIntensity / m_minIntensityDivider;

        m_owner = pOwner;
    }

    //-----------------------------------------------------------------
    // Light Update
    //-----------------------------------------------------------------
    void Light::Update()
    {
        if (!m_enabled)
            return;

        if (m_doAnim)
            Animate();

        if (engine.GetInput().GetKeyDown(SDL_SCANCODE_X))
        {
            if (m_curPropagation == PropagationSetting::Raycast)
                m_curPropagation = PropagationSetting::Flat;
            else
                m_curPropagation = PropagationSetting::Raycast;
        }

        PropagateLight();
    }

    //----------------------------------------------------------------------
    // Propagates the light through the world in base of the curPropagation
    //----------------------------------------------------------------------
    void Light::PropagateLight()
    {
        switch (m_curPropagation)
        {
        case PropagationSetting::Flat:
            FlatLightPropagation();
            break;

        case PropagationSetting::Raycast:
            RaycastLightPropagation();
            break;

        default:
            FlatLightPropagation();
            break;
        }
    }

    //-------------------------------------------------------------------------------------------
    // Light propagation using raycast, can be expensive and should be used only when necessary.
    // 
    // TODO: The number of rays to cast can be < 360 for certain lights, so we can increment i
    // by more than just 1 per loop. Make it a setting.
    //-------------------------------------------------------------------------------------------
    void Light::RaycastLightPropagation()
    {
        // X and Y of the cell where the light is
        int lightCellX = m_owner->GetOwner()->GetCellX();
        int lightCellY = m_owner->GetOwner()->GetCellY();
        World* world = m_owner->GetOwner()->GetWorld();

        // TODO: Instead of having matrix allocation every frame for every light, we can keep a global matrix "touched" that has the size of the biggest light used (dynamically resize) or a predetermined max size
        // Touched is used to know which cells have already been hit by a ray of this light, so two rays cannot add up light in the same cell
        std::vector<std::vector<char>> touched;
        touched.resize(2 * m_radius + 1, std::vector<char>(2 * m_radius + 1, 0)); // 2 * radius + 1 so it covers the whole square area + the center cell

        // Propagate light through world using Raycasting
        // This is done every frame for visible every light, we can surely optimize this
        for (int i = 0; i < 360; i++)
        {
            // Get the angle
            float angle = (i * M_PI) / 180.f;

            // Get the direction
            float dirX = cos(angle);
            float dirY = sin(angle);

            // Start the ray
            float rayX = lightCellX * CELL_WIDTH + HALF_CELL_WIDTH;
            float rayY = lightCellY * CELL_HEIGHT + HALF_CELL_HEIGHT;

            // Step through the propagation for each angle
            float curBlock = 0.0f;
            for (int step = 0; step < m_radius; step++)
            {
                // Calculate the grid position of the ray
                int curGridX = rayX / CELL_WIDTH;
                int curGridY = rayY / CELL_HEIGHT;

                // Calculate the touched position
                int touchedX = curGridX - (lightCellX - m_radius);
                int touchedY = curGridY - (lightCellY - m_radius);

                // Get the cell
                Cell* curCell = world->GetCellAt(curGridX, curGridY);
                if (curCell)
                {
                    // Get Light block value
                    float cellBlock = curCell->GetLightBlockPercent();
                    if (cellBlock > curBlock)
                        curBlock = cellBlock;

                    // If the Cell wasn't already touched, lit it
                    if (touched[touchedX][touchedY] == false)
                    {
                        curCell->SetLightingInfluence(this, std::abs(curGridX - lightCellX) + std::abs(curGridY - lightCellY), curBlock);
                        touched[touchedX][touchedY] = true;
                    }
                }

                // Extend the ray
                rayX += dirX * CELL_WIDTH;
                rayY += dirY * CELL_HEIGHT;
            }
        }
    }

    //---------------------------------------------------------------------------
    // Lightweight light propagation for lights that do not need to be occluded.
    //---------------------------------------------------------------------------
    void Light::FlatLightPropagation()
    {
        int lightCellX = m_owner->GetOwner()->GetCellX();
        int lightCellY = m_owner->GetOwner()->GetCellY();
        World* world = m_owner->GetOwner()->GetWorld();

        for (int i = -m_radius; i < m_radius; i++)
            for (int j = -m_radius; j < m_radius; j++)
            {
                // Do lighting in a circle instead of a square, so it looks beter and not so much different from raycasting method
                if (i * i + j * j <= m_radius * m_radius)
                    if (world->IsInWorldBounds(lightCellX + i, lightCellY + j))
                        world->GetCellAt(lightCellX + i, lightCellY + j)->SetLightingInfluence(this, std::abs(i) + std::abs(j), 0.0f); // No occlusion supported for flat lights
            }
    }

    //-----------------------------------------------------------------
    // Animates the light 
    //-----------------------------------------------------------------
    void Light::Animate()
    {
        float dTime = engine.GetDeltaTime();

        float direction = m_goUp ? 1.0f : -1.0f;
        m_intensity += direction * m_animSpeed * dTime;

        // Toggle directions (pulse light)
        if (m_intensity <= m_minIntensity)
        {
            m_intensity = m_minIntensity;
            m_goUp = true;
            m_goDown = false;
        }
        else if (m_intensity >= m_maxIntensity)
        {
            m_intensity = m_maxIntensity;
            m_goUp = false;
            m_goDown = true;
        }
    }

    //-----------------------------------------------------------------
    // Sets doAnim 
    //-----------------------------------------------------------------
    void Light::SetAnim(bool v)
    {
        m_doAnim = v;
    }

}
}
