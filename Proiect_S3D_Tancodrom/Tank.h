#pragma once

#include "Model.h"

class Tank
{
public:
	Model Head;
	Model Body;

	Tank(Model Body, Model Head);


	void SetIsSelected(bool isSelected);
	bool GetIsSelected();


private:
	bool isSelected;

};

