#pragma once
#include "Event.h"

class KeyPressedEvent : public Event
{
public:
	KeyPressedEvent(uint32_t keyCode, bool bRepeated, int mods)
		:m_KeyCode(keyCode), bIsRepeated(bRepeated), m_Mods(mods) {};

	uint32_t GetKeyCode() const { return m_KeyCode; }
	bool IsReapeted() const { return bIsRepeated; }
	bool HaveModes(int mods) const { return m_Mods & mods; }
	int GetMods() const { return m_Mods; }

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "KeyPressedEvent: " << m_KeyCode << " (repeat = " << bIsRepeated << ")";
		return ss.str();
	}

	EVENT_CLASS_TYPE(KeyPressed)
private:
	uint32_t m_KeyCode;
	bool bIsRepeated;
	int m_Mods;
};