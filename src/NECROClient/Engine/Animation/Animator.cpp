#include "Animator.h"
#include "AssetsManager.h"
#include "NECROClient.h"

#include <fstream>

// -----------------------------------------------------------------------
// Used for Copy-Assignment. When Animators are created they are copied
// from the ones loaded in the AssetsManager.
// -----------------------------------------------------------------------
Animator& Animator::operator=(const Animator& other)
{
    // Check for self-assignment
    if (this == &other)
        return *this;

    // Copy the data from the other Animator
    name = other.name;
    defaultStateName = other.defaultStateName;
    states = other.states;

    // Return the current object
    return *this;
}

//------------------------------------------------------------------
// Initializes the Animator and set the owner of this Animator
//------------------------------------------------------------------
void Animator::Init(Entity* pOwner)
{
    owner = pOwner;
    animTime = SDL_GetTicks();
}

//------------------------------------------------------------------
// Adds a state in the Animator
//------------------------------------------------------------------
void Animator::AddState(const std::string& sName, Image* sImg, float sSpeed)
{
	// Add a new state in the states map
	states.emplace(sName, AnimState(sName, sImg, sSpeed));
}

//-------------------------------------------------------------------------------------
// Updates the animator, updates the tilesetXOffset in order to select the right frame
//-------------------------------------------------------------------------------------
void Animator::Update()
{
    if (owner && curStatePlaying && curStatePlaying->GetImg()->IsTileset())
        owner->tilesetXOff = ((int)floor((SDL_GetTicks() - animTime) / curStatePlaying->GetSpeed()) % curStatePlaying->GetImg()->GetTileset()->tileXNum);
}

//------------------------------------------------------------------
// Plays the state passed as parameter (if exists)
//------------------------------------------------------------------
void Animator::Play(const std::string& sName)
{
    // Check if the state exists in the map
    if (states.find(sName) != states.end())
    {
        // Check if the img pointer is not null
        if (states.at(sName).GetImg() != nullptr)
        {
            // Set state
            curStatePlaying = &states.at(sName);
            curStateNamePlaying = sName;

            // Set anim
            owner->SetImg(curStatePlaying->GetImg()); // Set IMG of the entity
            animTime = SDL_GetTicks();
        }
        else
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ANIMATOR Error: Img PTR is null for state: %s ", sName.c_str());
    }
    else // State doesn't exist in map
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ANIMATOR Error: State %s not found in the states map", sName.c_str());
}

//------------------------------------------------------------------
// Plays the defaultState if set
//------------------------------------------------------------------
void Animator::PlayDefaultIfNotNull()
{
    if (!defaultStateName.empty() && defaultStateName.compare("NULL") != 0)
        Play(defaultStateName);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// Load an animator from a .nanim file
//--------------------------------------------------------------------------------------------------------------------------------------------
bool Animator::LoadFromFile(const std::string& fullPath, bool clear)
{
    if (clear)
        states.clear();

    SDL_Log("Loading Animator: '%s'\n", fullPath.c_str());

    std::ifstream stream(fullPath);

    if (!stream.is_open())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load animator at: %s\n", fullPath.c_str());
        return false;
    }

    std::string curLine;
    std::string curValStr;

    // Animator name
    std::getline(stream, curLine);
    curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
    curValStr = curValStr.substr(0, curValStr.find(";"));
    name = curValStr;

    // Default State
    std::getline(stream, curLine);
    curValStr = curLine.substr(curLine.find("=") + 2); // key = value;
    curValStr = curValStr.substr(0, curValStr.find(";"));
    defaultStateName = curValStr;

    // Load Animator States
    bool doneLoadingStates = false;
    while (!doneLoadingStates)
    {
        std::getline(stream, curLine);

        if(curLine.find("ENDSTATES") != std::string::npos)
            doneLoadingStates = true;
        else
        {
            // Load States
            //                                                            AnimSpeed
            // STATE:("state_name", "animation_file_name_for_this_state", 75);
            // SDL_Log("%s\n", curLine.c_str());

            std::string stateName;
            std::string animFile;
            float animSpeed;

            // Get state_name
            int startPos = curLine.find("\"");
            int endPos = curLine.find("\"", startPos + 1);
            stateName = curLine.substr(startPos + 1, endPos - startPos - 1);

            // Get animation_file_name_for_this_state
            startPos = curLine.find("\"", endPos + 1);
            endPos = curLine.find("\"", startPos + 1);
            animFile = curLine.substr(startPos + 1, endPos - startPos - 1);

            // Get animSpeed
            startPos = curLine.find_last_of(",") + 1;
            endPos = curLine.find(")", startPos + 1);
            animSpeed = ClientUtility::TryParseFloat(curLine.substr(startPos + 1, endPos - startPos - 1));

            AddState(stateName, engine.GetAssetsManager().GetImage(animFile), animSpeed);
        }
    }

    std::getline(stream, curLine); //; end of file 

    return true;
}
