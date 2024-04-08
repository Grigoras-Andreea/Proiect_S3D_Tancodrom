#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32s.lib")

#define GLEW_STATIC
#include <GL/glew.h>

#include "KeyBinder.tpp"

// example usage
#define GLFW_KEY_W 46

void MoveForward()
{
	std::cout << "Moving forward 1" << std::endl;
}

void MoveForward2(int param)
{
	std::cout << "Moving forward 2 Param: " << param << std::endl;
}

class Test
{
public:
	int x = 1;
	void Method()
	{
		std::cout << "Test " << x << std::endl;
	}
};

int main()
{
	KeyBinder<int, void(), void(int), void(Test*), void(Test*, int)> a;

	// Metode normale

	a.ForceBind(GLFW_KEY_W, MoveForward);

	// Trebuie sa arunce o eroare
	a.Call(GLFW_KEY_W, 1);

	a.ForceBind(GLFW_KEY_W, MoveForward2);
	a.Call(GLFW_KEY_W, 1);

	// Metode din obiecte alocate pe stack

	Test test;

	a.ForceBind(GLFW_KEY_W, &Test::Method);
	a.Call(GLFW_KEY_W, &test);

	a.ForceBind(GLFW_KEY_W, [&test]() { test.Method(); });
	a.Call(GLFW_KEY_W);

	test.x = 2;
	a.Call(GLFW_KEY_W);

	// Metode din obiecte alocate pe heap

	Test* pTest = new Test();

	a.ForceBind(GLFW_KEY_W, &Test::Method);
	a.Call(GLFW_KEY_W, pTest);

	a.ForceBind(GLFW_KEY_W, [pTest]() { pTest->Method(); });
	a.Call(GLFW_KEY_W);

	pTest->x = 2;
	a.Call(GLFW_KEY_W);

	pTest = new Test();
	a.Call(GLFW_KEY_W);

	pTest = new Test();
	a.ForceBind(GLFW_KEY_W, [&pTest]() { pTest->Method(); });
	a.Call(GLFW_KEY_W);
	a.Unbind(GLFW_KEY_W);

	pTest->x = 2;
	a.Call(GLFW_KEY_W);

	pTest = new Test();
	a.Call(GLFW_KEY_W);

	a.Call(GLFW_KEY_W, pTest, 2);
	a.Get(GLFW_KEY_W);
}