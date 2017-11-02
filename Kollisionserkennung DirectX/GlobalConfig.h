#pragma once

#include <fstream>
#include <iostream>

using namespace std;


// Dient zum Einlesen der Konfigurationsdatei, die Daten können mit Get_GPU() und Get_Scene() ausgelesen werden
class GlobalConfig
{
public:
	GlobalConfig();

	int Get_GPU() { return m_Gpu; }
	int Get_Scene() { return m_Scene; }

private:
	int m_Gpu;
	int m_Scene;
};



