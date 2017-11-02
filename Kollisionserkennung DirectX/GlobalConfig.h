#pragma once

#include <fstream>
#include <iostream>

using namespace std;



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



