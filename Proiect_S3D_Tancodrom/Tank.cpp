#include "Tank.h"

Tank::Tank(Model Body, Model Head)
{
	this->Body = Body;
	this->Head = Head;
	isSelected = false;
	isDestroyed = false;
}

void Tank::SetIsSelected(bool isSelected)
{
	this->isSelected = isSelected;
}

bool Tank::GetIsSelected()
{
	return isSelected;
}
