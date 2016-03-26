#include "stdafx.h"

Emulator emulator;

s32 main(s32 argc, char* argv[])
{
	std::string log_name = "NGEmu.log";
	std::string rom = "Rayman.app"; // If a ROM isn't specified on command line, then execute the ROM specified here (for development)

	if (argc >= 2)
	{
		rom = argv[1];
	}

	if (!init_logging(log_name))
	{
		log(ERROR, "Failed to initialize logging.");
		return exit(true);
	}

	if (!emulator.initialize(rom))
	{
		return exit(true);
	}

	emulator.run();

	return 0;
}

Emulator::Emulator()
{
	running = true;
	emulating = true;
}

bool Emulator::initialize(std::string path)
{
	debugger.reset(new Debugger());

	if (!loader::load(path, rom))
	{
		return false;
	}

	cpu.reset(new CPU());
	HLE::initialize();

	if (!debugger->initialize())
	{
		return false;
	}

	return true;
}

void Emulator::emulate(u64 clock)
{
	// For calculating instructions per second
	u64 ips = 0;
	u64 old_ips = 0;
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point end;

	while (running)
	{
		if (emulating)
		{
			cpu->execute();

			// Calculate instructions per second
			ips++;
			end = std::chrono::steady_clock::now();

			if (std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() >= 1)
			{
				u64 cur_ips = ips - old_ips;

				log(DEBUG, "ips: %d", cur_ips);

				old_ips = ips;
				begin = std::chrono::steady_clock::now();
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / clock)); // This should be improved, to be more accurate
	}
}

void Emulator::run()
{
	std::thread emulation_thread = std::thread([this]
	{
		// The clock speed
		const u32 clock_speed = 100;
		emulate(clock_speed);
	});

	while (running)
	{
		debugger->window->process_events();
		debugger->render();

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
	}

	emulation_thread.join();
}