#ifndef NECROANIMATOR_H
#define NECROANIMATOR_H

#include <string>
#include <vector>
#include <map>

#include "Entity.h"
#include "AnimState.h"

namespace NECRO
{
namespace Client
{
	class Entity;

	class Animator
	{
	private:
		std::string m_name;
		std::string m_defaultStateName = "NULL";	// if specified and different than 'NULL' in the .nanim file, after loading the animator from the file we can call Animator.PlayDefaultIfNotNull()
												// this is done to allow automatic animation play from file definition for simple entities.

		float m_animTime = 0;

		Entity* m_owner;
		std::map<std::string, AnimState> m_states;

		AnimState*	m_curStatePlaying = nullptr;			// Keeps the state of the currently active state
		std::string m_curStateNamePlaying = "NULL";

	public:
		Animator& operator=(const Animator& other); //copy-assignment

	public:
		void Init(Entity* pOwner);

		bool LoadFromFile(const std::string& fullPath, bool clear = true); // Load an animator from a .nanim file

		void AddState(const std::string& sName, Image* sImg, float sSpeed);
		void Play(const std::string& sName);
		void PlayDefaultIfNotNull();

		void Update();

		std::string GetCurStateNamePlaying() const;
	};

	inline std::string Animator::GetCurStateNamePlaying() const
	{
		return m_curStateNamePlaying;
	}

}
}

#endif
