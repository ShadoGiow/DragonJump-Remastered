#include "Core.h"
#include "CrashTrace.h"
#include "Menu.h"
#include "Settings.h"

namespace
{
	void SetupLog()
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			return;
		}
		*path /= "MagicalJumpingNative.log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S] [%l] %v"s);
	}

	// una riga nel log per i momenti in cui il gioco legge o scrive il salvataggio: se il gioco si chiude proprio li', l'ultima riga lo dice
	void Breadcrumb(const char* a_what)
	{
		static int count = 0;
		if (++count > 80) {
			return;
		}
		logger::info("{} (ultimo frame del plugin {:.1f} s fa, ultimo passo: {})", a_what, MJ::Trace::SecondsSinceTick(), MJ::Trace::CurrentStep());
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			try {
				MJ::Settings::Get().Load();
				MJ::Core::Init();
				MJ::Core::InstallHook();
				MJ::Menu::Register();
			} catch (const std::exception& e) {
				logger::error("errore in kDataLoaded: {}", e.what());
			}
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
			MJ::Trace::Note("caricamento del salvataggio");
			Breadcrumb("caricamento del salvataggio in corso");
			break;
		case SKSE::MessagingInterface::kSaveGame:
			MJ::Trace::Note("salvataggio");
			Breadcrumb("salvataggio del gioco (manuale o automatico)");
			break;
		case SKSE::MessagingInterface::kDeleteGame:
			MJ::Trace::Note("salvataggio eliminato");
			break;
		case SKSE::MessagingInterface::kNewGame:
		case SKSE::MessagingInterface::kPostLoadGame:
			MJ::Trace::Note(a_msg->type == SKSE::MessagingInterface::kNewGame ? "nuova partita" : "salvataggio caricato");
			Breadcrumb(a_msg->type == SKSE::MessagingInterface::kNewGame ? "nuova partita" : "salvataggio caricato");
			MJ::Core::OnNewOrLoad();
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	logger::info("MJN 2.4 (uso personale) in avvio (build del {} {})", __DATE__, __TIME__);
	MJ::Trace::Install();

	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
