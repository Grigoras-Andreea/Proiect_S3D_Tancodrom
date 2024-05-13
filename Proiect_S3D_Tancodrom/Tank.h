#pragma once

#include "Model.h"

class Tank
{
public:
	Model Head;
	Model Body;
	bool isDestroyed;
	bool isDamaged;

	Tank(Model Body, Model Head);


	void SetIsSelected(bool isSelected);
	bool GetIsSelected();


private:
	bool isSelected;

};

