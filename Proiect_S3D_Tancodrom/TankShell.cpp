#include "TankShell.h"

TankShell::TankShell(Model Shell, double spawnTime, glm::vec3 moveDir)
{
	this->Shell = Shell;
	this->spawnTime = spawnTime;
	this->moveDir = moveDir;
}
