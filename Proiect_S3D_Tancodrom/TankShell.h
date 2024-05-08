#pragma once
#include "Model.h"

class TankShell
{
public:
	Model Shell;
	double spawnTime;
	glm::vec3 moveDir;

	TankShell(Model Shell, double spawnTime, glm::vec3 moveDir);

};

