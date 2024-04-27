#pragma once

#include "Model.h"

class Helicopter
{
public:
	Model ProppelerUp;
	Model ProppelerBack;
	Model Body;


	Helicopter(Model Body, Model ProppelerUp, Model ProppelerBack);


	void SetIsSelected(bool isSelected);
	bool GetIsSelected();


private:
	bool isSelected;


};

