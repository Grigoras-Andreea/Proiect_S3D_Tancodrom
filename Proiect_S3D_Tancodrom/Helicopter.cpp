#include "Helicopter.h"

Helicopter::Helicopter(Model Body, Model ProppelerUp, Model ProppelerBack)
{
	this->Body = Body;
	this->ProppelerUp = ProppelerUp;
	this->ProppelerBack = ProppelerBack;
	isSelected = false;
	isDestroyed = false;
}

void Helicopter::SetIsSelected(bool isSelected)
{
	this->isSelected = isSelected;
}

bool Helicopter::GetIsSelected()
{
	return isSelected;
}
