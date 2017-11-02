#include "GlobalConfig.h"



GlobalConfig::GlobalConfig()
{
	string fileName = "../Kollisionserkennung DirectX/config/config.txt";
	ifstream config(fileName);
	if (!config.is_open())
	{
		printf("/Kollisionserkennung DirectX/config/config.txt konnte nicht geoeffnet werden!");
		//cout << fileName << " konnte nicht geoeffnet werden!" << endl;
		return;
	}
	config >> m_Gpu;
	config >> m_Scene;

	config.close();
}