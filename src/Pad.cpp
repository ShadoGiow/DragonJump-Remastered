#include "Pad.h"
#include "Settings.h"

namespace MJ::Pad
{
	namespace
	{
		// stessa struttura di XINPUT_STATE (16 byte)
		struct XState
		{
			std::uint32_t packet;
			std::uint16_t buttons;
			std::uint8_t  leftTrigger;
			std::uint8_t  rightTrigger;
			std::int16_t  lx;
			std::int16_t  ly;
			std::int16_t  rx;
			std::int16_t  ry;
		};
		static_assert(sizeof(XState) == 16);

		using GetStateFn = std::uint32_t(__stdcall*)(std::uint32_t, void*);
		using FgFn = void*(__stdcall*)();
		using WtpFn = std::uint32_t(__stdcall*)(void*, std::uint32_t*);
		using PidFn = std::uint32_t(__stdcall*)();

		constexpr std::uint8_t kTriggerThreshold = 90;  // su 255

		std::mutex    g_mutex;
		GetStateFn    g_getState = nullptr;
		bool          g_loadTried = false;
		int           g_slot = -1;
		double        g_nextScan = 0.0;
		double        g_lastPoll = -1.0;
		std::uint32_t g_last = 0;
		float         g_lx = 0.0f;
		float         g_ly = 0.0f;

		// zona morta radiale standard di XInput per lo stick sinistro (7849 su 32767)
		void StoreStick(std::int16_t a_x, std::int16_t a_y)
		{
			constexpr float kDead = 7849.0f / 32767.0f;
			float           x = std::clamp(static_cast<float>(a_x) / 32767.0f, -1.0f, 1.0f);
			float           y = std::clamp(static_cast<float>(a_y) / 32767.0f, -1.0f, 1.0f);
			const float     m = std::hypot(x, y);
			if (m <= kDead) {
				g_lx = g_ly = 0.0f;
				return;
			}
			const float k = std::min(1.0f, (m - kDead) / (1.0f - kDead)) / m;
			g_lx = x * k;
			g_ly = y * k;
		}

		void LoadXInput()
		{
			if (g_loadTried) {
				return;
			}
			g_loadTried = true;
			for (const char* name : { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" }) {
				auto mod = REX::W32::GetModuleHandleA(name);
				if (!mod) {
					mod = REX::W32::LoadLibraryA(name);
				}
				if (!mod) {
					continue;
				}
				if (auto* fn = REX::W32::GetProcAddress(mod, "XInputGetState")) {
					g_getState = reinterpret_cast<GetStateFn>(fn);
					logger::info("XInput caricato da {}", name);
					return;
				}
			}
			logger::warn("XInput non trovato: i pulsanti del controller non saranno letti");
		}

		const char* ButtonName(std::uint32_t a_bit)
		{
			switch (a_bit) {
			case kUp:
				return "D-pad Su";
			case kDown:
				return "D-pad Giu'";
			case kLeft:
				return "D-pad Sinistra";
			case kRight:
				return "D-pad Destra";
			case kStart:
				return "Start";
			case kBack:
				return "Back";
			case kL3:
				return "L3 (stick sinistro)";
			case kR3:
				return "R3 (stick destro)";
			case kLB:
				return "LB";
			case kRB:
				return "RB";
			case kA:
				return "A";
			case kB:
				return "B";
			case kX:
				return "X";
			case kY:
				return "Y";
			case kLT:
				return "LT";
			case kRT:
				return "RT";
			default:
				return "?";
			}
		}
	}

	std::uint32_t Poll()
	{
		std::lock_guard lk(g_mutex);
		const double now = Now();
		if (g_lastPoll >= 0.0 && (now - g_lastPoll) < 0.004) {
			return g_last;
		}
		g_lastPoll = now;

		LoadXInput();
		if (!g_getState) {
			g_last = 0;
			g_lx = g_ly = 0.0f;
			return 0;
		}

		if (g_slot < 0) {
			if (now < g_nextScan) {
				g_last = 0;
				g_lx = g_ly = 0.0f;
				return 0;
			}
			for (std::uint32_t i = 0; i < 4; ++i) {
				XState probe{};
				if (g_getState(i, &probe) == 0) {
					g_slot = static_cast<int>(i);
					break;
				}
			}
			if (g_slot < 0) {
				g_nextScan = now + 2.0;  // nessun controller: riprovo tra un po'
				g_last = 0;
				g_lx = g_ly = 0.0f;
				return 0;
			}
		}

		XState st{};
		if (g_getState(static_cast<std::uint32_t>(g_slot), &st) != 0) {
			g_slot = -1;  // scollegato
			g_nextScan = now + 1.0;
			g_last = 0;
			g_lx = g_ly = 0.0f;
			return 0;
		}
		StoreStick(st.lx, st.ly);

		std::uint32_t mask = st.buttons;
		if (st.leftTrigger > kTriggerThreshold) {
			mask |= kLT;
		}
		if (st.rightTrigger > kTriggerThreshold) {
			mask |= kRT;
		}
		g_last = mask & kAll;
		return g_last;
	}

	bool Connected()
	{
		std::lock_guard lk(g_mutex);
		return g_slot >= 0;
	}

	bool LeftStick(float& a_x, float& a_y)
	{
		Poll();  // aggiorna (al massimo ogni 4 ms)
		std::lock_guard lk(g_mutex);
		a_x = g_lx;
		a_y = g_ly;
		return g_slot >= 0;
	}

	bool GameFocused()
	{
		static FgFn   fg = nullptr;
		static WtpFn  wtp = nullptr;
		static PidFn  pid = nullptr;
		static bool   resolved = false;
		if (!resolved) {
			resolved = true;
			const auto user32 = REX::W32::GetModuleHandleA("user32.dll");
			const auto kernel32 = REX::W32::GetModuleHandleA("kernel32.dll");
			if (user32 && kernel32) {
				fg = reinterpret_cast<FgFn>(REX::W32::GetProcAddress(user32, "GetForegroundWindow"));
				wtp = reinterpret_cast<WtpFn>(REX::W32::GetProcAddress(user32, "GetWindowThreadProcessId"));
				pid = reinterpret_cast<PidFn>(REX::W32::GetProcAddress(kernel32, "GetCurrentProcessId"));
			}
		}
		if (!fg || !wtp || !pid) {
			return true;  // non so dirlo: non blocco nulla
		}
		void* hwnd = fg();
		if (!hwnd) {
			return false;
		}
		std::uint32_t owner = 0;
		wtp(hwnd, &owner);
		return owner == pid();
	}

	int CountBits(std::uint32_t a_mask)
	{
		int n = 0;
		for (; a_mask; a_mask &= (a_mask - 1)) {
			++n;
		}
		return n;
	}

	std::string Describe(std::uint32_t a_mask)
	{
		a_mask &= kAll;
		if (a_mask == 0) {
			return "nessuno";
		}
		static constexpr std::uint32_t order[] = { kLB, kRB, kLT, kRT, kL3, kR3, kA, kB, kX, kY, kUp, kDown, kLeft, kRight, kStart, kBack };
		std::string out;
		for (const auto bit : order) {
			if (a_mask & bit) {
				if (!out.empty()) {
					out += " + ";
				}
				out += ButtonName(bit);
			}
		}
		return out;
	}
}
