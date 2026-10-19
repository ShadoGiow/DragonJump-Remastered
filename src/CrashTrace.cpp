#include "CrashTrace.h"

namespace MJ::Trace
{
	namespace
	{
		// layout x64 di EXCEPTION_RECORD / EXCEPTION_POINTERS (dichiarati qui per non includere windows.h)
		struct ExcRecord
		{
			std::uint32_t code;
			std::uint32_t flags;
			void*         next;
			void*         address;
			std::uint32_t paramCount;
			std::uint32_t pad;
			std::uint64_t info[15];
		};
		struct ExcPointers
		{
			ExcRecord* record;
			void*      context;
		};
		// CONTEXT x64: Rsp a 0x98, Rip a 0xF8
		constexpr std::size_t kCtxRsp = 0x98;

		using HandlerFn = long(__stdcall*)(ExcPointers*);
		using AddVehFn = void*(__stdcall*)(unsigned long, HandlerFn);
		using CreateFileFn = void*(__stdcall*)(const char*, unsigned long, unsigned long, void*, unsigned long, unsigned long, void*);
		using WriteFileFn = int(__stdcall*)(void*, const void*, unsigned long, unsigned long*, void*);
		using CloseFn = int(__stdcall*)(void*);
		using RpmFn = int(__stdcall*)(void*, const void*, void*, std::size_t, std::size_t*);
		using GetModExFn = int(__stdcall*)(unsigned long, const void*, void**);
		using GetModNameFn = unsigned long(__stdcall*)(void*, char*, unsigned long);
		using GetTidFn = unsigned long(__stdcall*)();

		std::atomic<const char*>   g_step{ "avvio" };
		char                       g_note[96] = "avvio";
		std::atomic<std::int64_t>  g_noteMs{ 0 };
		std::atomic<std::int64_t>  g_tickMs{ 0 };
		std::atomic<std::uint32_t> g_tickThread{ 0 };

		char           g_path[520] = {};
		std::uintptr_t g_selfBase = 0;
		std::uintptr_t g_selfEnd = 0;
		std::int64_t   g_t0 = 0;

		CreateFileFn g_createFile = nullptr;
		WriteFileFn  g_writeFile = nullptr;
		CloseFn      g_close = nullptr;
		RpmFn        g_rpm = nullptr;
		GetModExFn   g_getModEx = nullptr;
		GetModNameFn g_getModName = nullptr;
		GetTidFn     g_tid = nullptr;

		std::atomic_flag g_busy;
		std::uintptr_t   g_seen[16] = {};
		std::size_t      g_seenPos = 0;
		std::size_t      g_written = 0;
		std::uint8_t     g_stack[8192];
		char             g_line[1536];

		std::int64_t NowMs()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		// solo gli errori "duri": violazione di accesso, istruzione non valida, divisione per zero, heap corrotto...
		bool IsHardFault(std::uint32_t a_code)
		{
			switch (a_code) {
			case 0xC0000005:  // violazione di accesso
			case 0xC0000006:  // errore di paginazione
			case 0xC000001D:  // istruzione non valida
			case 0xC0000096:  // istruzione privilegiata
			case 0xC0000094:  // divisione intera per zero
			case 0xC000008C:  // indice fuori dai limiti
			case 0xC00000FD:  // stack esaurito
			case 0xC0000374:  // heap corrotto
				return true;
			default:
				return false;
			}
		}

		const char* FaultName(std::uint32_t a_code)
		{
			switch (a_code) {
			case 0xC0000005: return "violazione di accesso";
			case 0xC0000006: return "errore di paginazione";
			case 0xC000001D: return "istruzione non valida";
			case 0xC0000096: return "istruzione privilegiata";
			case 0xC0000094: return "divisione per zero";
			case 0xC000008C: return "indice fuori dai limiti";
			case 0xC00000FD: return "stack esaurito";
			case 0xC0000374: return "heap corrotto";
			default: return "errore";
			}
		}

		// nome del modulo (solo il nome del file) e base che contiene l'indirizzo
		void ModuleOf(std::uintptr_t a_addr, char* a_out, std::size_t a_size, std::uintptr_t& a_base)
		{
			a_out[0] = '?';
			a_out[1] = 0;
			a_base = 0;
			if (!g_getModEx || !g_getModName) {
				return;
			}
			void* mod = nullptr;
			if (!g_getModEx(0x4 | 0x2, reinterpret_cast<const void*>(a_addr), &mod) || !mod) {  // FROM_ADDRESS | UNCHANGED_REFCOUNT
				return;
			}
			char full[400] = {};
			if (g_getModName(mod, full, sizeof(full) - 1) == 0) {
				return;
			}
			const char* name = full;
			for (const char* p = full; *p; ++p) {
				if (*p == '\\' || *p == '/') {
					name = p + 1;
				}
			}
			std::snprintf(a_out, a_size, "%s", name);
			a_base = reinterpret_cast<std::uintptr_t>(mod);
		}

		void Report(ExcPointers* a_ep)
		{
			const auto* rec = a_ep->record;
			const auto  code = rec->code;
			const auto  rip = reinterpret_cast<std::uintptr_t>(rec->address);

			// stesso indirizzo gia' scritto da poco: non lo ripeto
			for (const auto s : g_seen) {
				if (s == rip) {
					return;
				}
			}
			g_seen[g_seenPos++ % 16] = rip;
			if (g_written > 300000 || !g_createFile || !g_writeFile || !g_close || g_path[0] == 0) {
				return;
			}

			std::uint64_t rsp = 0;
			if (a_ep->context) {
				std::memcpy(&rsp, static_cast<const std::uint8_t*>(a_ep->context) + kCtxRsp, sizeof(rsp));
			}

			// il plugin compare nello stack? cerco nello stack del thread valori che puntano dentro la nostra DLL
			const bool     inSelf = rip >= g_selfBase && rip < g_selfEnd;
			int            hits = 0;
			std::uint64_t  rvas[4] = {};
			if (code != 0xC00000FD && rsp != 0 && g_rpm && g_selfEnd > g_selfBase) {
				std::size_t got = 0;
				g_rpm(reinterpret_cast<void*>(static_cast<std::intptr_t>(-1)), reinterpret_cast<const void*>(rsp), g_stack, sizeof(g_stack), &got);
				const auto* q = reinterpret_cast<const std::uint64_t*>(g_stack);
				for (std::size_t i = 0; i < got / 8; ++i) {
					if (q[i] >= g_selfBase && q[i] < g_selfEnd) {
						if (hits < 4) {
							rvas[hits] = q[i] - g_selfBase;
						}
						++hits;
					}
				}
			}

			char         mod[96];
			std::uintptr_t base = 0;
			ModuleOf(rip, mod, sizeof(mod), base);

			char what[96] = {};
			if (code == 0xC0000005 && rec->paramCount >= 2) {
				const char* kind = rec->info[0] == 0 ? "lettura" : (rec->info[0] == 1 ? "scrittura" : "esecuzione");
				std::snprintf(what, sizeof(what), ", %s all'indirizzo 0x%llX", kind, static_cast<unsigned long long>(rec->info[1]));
			}

			const auto   now = NowMs();
			const auto   tick = g_tickMs.load();
			const auto   note = g_noteMs.load();
			const char*  step = g_step.load();
			const auto   tid = g_tid ? g_tid() : 0ul;

			int n = std::snprintf(g_line, sizeof(g_line),
				"[+%.1f s] 0x%08X (%s%s) in %s+0x%llX | il plugin e' in quell'errore: %s | passi del plugin nello stack: %d (RVA %llX %llX %llX %llX) | "
				"ultimo passo: %s | ultimo evento: %s (%.1f s fa) | ultimo frame del plugin: %s%.1f s fa | thread %lu (thread dei frame %lu)\r\n",
				static_cast<double>(now - g_t0) / 1000.0, code, FaultName(code), what, mod, static_cast<unsigned long long>(rip - base),
				inSelf ? "SI (l'errore avviene dentro la nostra DLL)" : (hits > 0 ? "forse (la DLL compare nello stack)" : "no"), hits,
				static_cast<unsigned long long>(rvas[0]), static_cast<unsigned long long>(rvas[1]), static_cast<unsigned long long>(rvas[2]),
				static_cast<unsigned long long>(rvas[3]), step ? step : "?", g_note, static_cast<double>(now - note) / 1000.0, tick == 0 ? "mai " : "",
				tick == 0 ? 0.0 : static_cast<double>(now - tick) / 1000.0, tid, static_cast<unsigned long>(g_tickThread.load()));
			if (n <= 0) {
				return;
			}
			if (n >= static_cast<int>(sizeof(g_line))) {
				n = static_cast<int>(sizeof(g_line)) - 1;
			}

			// FILE_APPEND_DATA, condivisione lettura+scrittura, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL
			void* h = g_createFile(g_path, 0x4, 0x3, nullptr, 4, 0x80, nullptr);
			if (h && h != reinterpret_cast<void*>(static_cast<std::intptr_t>(-1))) {
				unsigned long wrote = 0;
				g_writeFile(h, g_line, static_cast<unsigned long>(n), &wrote, nullptr);
				g_close(h);
				g_written += static_cast<std::size_t>(n);
			}
		}

		long __stdcall Handler(ExcPointers* a_ep)
		{
			if (!a_ep || !a_ep->record || !IsHardFault(a_ep->record->code)) {
				return 0;  // EXCEPTION_CONTINUE_SEARCH: non tocco niente
			}
			if (g_busy.test_and_set()) {
				return 0;
			}
			Report(a_ep);
			g_busy.clear();
			return 0;  // EXCEPTION_CONTINUE_SEARCH: l'errore prosegue come se non ci fossi
		}
	}

	void Install()
	{
		const auto k32 = REX::W32::GetModuleHandleA("kernel32.dll");
		if (!k32) {
			return;
		}
		auto sym = [&](const char* a_name) { return REX::W32::GetProcAddress(k32, a_name); };
		const auto addVeh = reinterpret_cast<AddVehFn>(sym("AddVectoredExceptionHandler"));
		g_createFile = reinterpret_cast<CreateFileFn>(sym("CreateFileA"));
		g_writeFile = reinterpret_cast<WriteFileFn>(sym("WriteFile"));
		g_close = reinterpret_cast<CloseFn>(sym("CloseHandle"));
		g_rpm = reinterpret_cast<RpmFn>(sym("ReadProcessMemory"));
		g_getModEx = reinterpret_cast<GetModExFn>(sym("GetModuleHandleExA"));
		g_getModName = reinterpret_cast<GetModNameFn>(sym("GetModuleFileNameA"));
		g_tid = reinterpret_cast<GetTidFn>(sym("GetCurrentThreadId"));
		if (!addVeh || !g_createFile || !g_writeFile || !g_close) {
			return;
		}

		auto dir = SKSE::log::log_directory();
		if (!dir) {
			return;
		}
		*dir /= "MagicalJumpingNative_crash.log";
		std::error_code ec;
		const auto      size = std::filesystem::file_size(*dir, ec);
		if (!ec && size > 262144) {
			std::filesystem::remove(*dir, ec);  // il file non cresce all'infinito
		}
		std::snprintf(g_path, sizeof(g_path), "%s", dir->string().c_str());

		// intervallo di memoria della nostra DLL (per riconoscerla nello stack)
		void* self = nullptr;
		if (g_getModEx && g_getModEx(0x4 | 0x2, reinterpret_cast<const void*>(&Install), &self) && self) {
			const auto* b = static_cast<const std::uint8_t*>(self);
			std::int32_t lfanew = 0;
			std::memcpy(&lfanew, b + 0x3C, sizeof(lfanew));
			if (lfanew > 0 && lfanew < 0x1000) {
				std::uint32_t imageSize = 0;
				std::memcpy(&imageSize, b + lfanew + 24 + 56, sizeof(imageSize));  // OptionalHeader.SizeOfImage
				g_selfBase = reinterpret_cast<std::uintptr_t>(self);
				g_selfEnd = g_selfBase + imageSize;
			}
		}

		g_t0 = NowMs();
		g_noteMs.store(g_t0);
		addVeh(1, &Handler);  // 1 = prima degli altri, cosi' vede l'errore prima di chi chiude il gioco
		logger::info("diagnostica crash attiva (scrive SKSE\\MagicalJumpingNative_crash.log solo se vede passare un errore grave di memoria)");
	}

	void Step(const char* a_what)
	{
		g_step.store(a_what, std::memory_order_relaxed);
	}

	void Note(const char* a_what)
	{
		std::snprintf(g_note, sizeof(g_note), "%s", a_what ? a_what : "?");
		g_noteMs.store(NowMs(), std::memory_order_relaxed);
	}

	void Tick()
	{
		g_tickMs.store(NowMs(), std::memory_order_relaxed);
		if (g_tid && g_tickThread.load(std::memory_order_relaxed) == 0) {
			g_tickThread.store(g_tid(), std::memory_order_relaxed);
		}
	}

	double SecondsSinceTick()
	{
		const auto t = g_tickMs.load();
		return t == 0 ? -1.0 : static_cast<double>(NowMs() - t) / 1000.0;
	}

	const char* CurrentStep()
	{
		const char* s = g_step.load();
		return s ? s : "?";
	}
}
