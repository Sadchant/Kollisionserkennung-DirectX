#ifndef _INPUTCLASS_H_
#define _INPUTCLASS_H_

// ***Quelle***: http://www.rastertek.com/tutdx11.html



class InputClass
{
public:
	InputClass();
	InputClass(const InputClass&);
	~InputClass();

	void Initialize();

	void KeyDown(unsigned int);
	void KeyUp(unsigned int);

	bool IsKeyDown(unsigned int);

private:
	bool m_keys[256];
};

#endif