#pragma once

namespace MJ::Core
{
	struct Status
	{
		bool          formsOk = false;
		bool          applied = false;
		bool          airborne = false;
		bool          bjFileFound = false;
		bool          bjMatch = false;
		bool          bjFixedNow = false;
		std::uint32_t bjPerkInFile = 0;
		std::uint32_t bjPerkExpected = 0;
		const char*   state = "";  // testo di stato per il menu (sempre una stringa fissa: la legge anche il thread del menu)
	};

	void InstallHook();   // aggancia l'update del giocatore
	void Init();          // a kDataLoaded: trova i form, controlla il file del gate dei salti
	void OnNewOrLoad();   // a kNewGame / kPostLoadGame: azzera lo stato

	const Status& GetStatus();
	bool          FixJumpGate();  // scrive il FormID giusto in BetterJumpingSE.txt
	void          MenuTouched();       // chiamato dal menu a ogni frame disegnato
	void          RequestReset();      // reset animazioni del personaggio (eseguito al prossimo frame di gioco)
	void          RequestLegacyCleanup();  // disattiva i vecchi script e il vecchio plugin delle impostazioni (al prossimo frame di gioco)
	bool          LegacyBusy();            // la disattivazione e' in corso
	std::string   LegacyStatus();          // esito dell'ultima disattivazione (vuoto = mai eseguita)
	std::string   AirAttackStatus();       // stato degli attacchi in salto per il menu (globali trovate, ultimo attacco...)
	std::string   WitchStatus();           // stato della strega volante per il menu (variabili trovate, ultima volta...)
	std::string   GlideStatus();       // stato della mod di planata (rilevato, in planata...) per il menu
	float         VanillaGravity();        // gravita' originale del gioco catturata all'avvio (0 = non ancora nota)
	bool          FireStormFound();           // la magia Tempesta di Fuoco dello slam e' stata trovata
	double        FireStormRemaining();       // secondi che mancano alla prossima Tempesta di Fuoco (0 = pronta)
	void          FireStormResetCooldown();   // azzera l'attesa
}
