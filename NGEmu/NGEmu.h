#pragma once

class Emulator
{
public:
	Emulator();
	bool initialize();
	void run();
	void emulate(u64 clock);

	bool running;
	bool emulating;

private:
	std::unique_ptr<Debugger> debugger;
	//std::unique_ptr<CPU> debugger;

	void pause();
	void stop();
};