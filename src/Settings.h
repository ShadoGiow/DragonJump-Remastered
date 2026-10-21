#pragma once

namespace MJ
{
	// Tutte le impostazioni modificabili dal menu F1. Salvate in
	// Data/SKSE/Plugins/MagicalJumpingNative.ini
	struct Settings
	{
		// --- Generale ---
		bool enabled = true;   // mod attiva / disattiva
		bool messages = true;  // messaggi a schermo
		bool padEnabled = true;  // leggi i pulsanti del controller

		// --- Salti ---
		float jumpHeight = 86.25f;  // fJumpHeightMin (75 + 15%)
		float gravity = 1.35f;      // fInAirFallingCharGravityMult:Havok (1.35 = valore di Skyrim)
		bool  animatedLanding = true;
		bool  directionChange = true;
		float directionMinSpeed = 4.0f;  // velocita' minima (m/s di Havok, ~280 unita'/s) quando cambi direzione in aria da fermo
		float directionWeight = 0.0f;    // "peso" del cambio direzione: 0 = istantaneo, 100 = curva lenta (~1 s)
		bool  directionTurnBody = true;  // ruota anche il modello del personaggio nella nuova direzione (solo terza persona)
		bool  noFallDamage = false;
		bool  doubleJumpEnabled = true;  // false = niente salti in aria (solo il salto normale)
		bool  tripleJumpEnabled = true;  // false = al massimo il doppio salto
		float airControl = 100.0f;       // % del movimento orizzontale in aria (100 = invariato)

		// --- Super jump (pressione prolungata) ---
		bool  superJumpEnabled = true;
		float superJumpHeight = 750.0f;
		float superJumpHold = 1.5f;
		int   keySuperJump = 0x26;  // freccia su
		int   padSuperJump = 0x0004;  // controller: D-pad sinistra (maschera XInput, anche combinazioni)

		// --- Dash in aria ---
		bool dashEnabled = true;
		int  dashDistance = 1;   // 1 = corto, 2 = medio, 3 = lungo
		int  keyDash = 0x27;     // freccia destra
		int  padDash = 0x0008;   // controller: D-pad destra
		int  dashStyle = 0;      // 0 = fuoco (cerchio + scintille), 1 = effetto alternativo
		bool  dashSpeedLimit = true;    // dopo il dash limita la velocita' orizzontale (evita lo "scatto alla velocita' della luce")
		float dashExitMaxSpeed = 9.0f;  // m/s (9 m/s = ~630 unita'/s, poco sopra lo sprint)
		bool dashSound = true;        // suono del dash
		bool dashSoundRandom = false; // false = i suoni girano in ordine, true = a caso (mai lo stesso due volte di fila)

		// --- Slam a terra (pressione prolungata) ---
		bool  slamEnabled = true;
		float slamHold = 0.25f;
		bool  slamFreeMovement = false;  // muoviti liberamente durante la carica dello slam
		int   keySlam = 0x28;  // freccia giu'
		int   padSlam = 0x0108;  // controller: LB + D-pad destra
		bool  slamFireStorm = true;          // all'atterraggio lancia la magia Tempesta di Fuoco (invece dell'onda d'urto)
		float fireStormCooldown = 300.0f;    // secondi di attesa tra due Tempeste di Fuoco (300 = 5 minuti)
		bool  slamShockwaveWhileWaiting = true;  // durante l'attesa lo slam usa l'onda d'urto normale
		int   fireStormFormId = 0x0007A82B;  // FormID della magia in Skyrim.esm (cambiabile nell'ini)

		// --- Reset animazioni del personaggio (per quando resta bloccato) ---
		int   keyReset = 0;          // tastiera: nessuno di default
		int   padReset = 0x0320;     // controller: LB + RB + Back (tenuti premuti)
		float resetHold = 0.6f;      // secondi di pressione

		// --- Strascico del dash dopo un atterraggio a meta' dash (bug "supersonico") ---
		// Agisce SOLO nei 25 s dopo la fine di un dash e solo se la variabile Speed del grafo supera speedFixTrigger:
		// schivate e mosse speciali di altre mod (Speed fino a ~1500) non vengono mai toccate.
		bool  speedFix = true;          // limita il movimento mentre dura lo strascico del dash
		float speedFixMax = 12.0f;      // m/s: velocita' massima a terra mentre lo strascico e' in corso (una corsa normale e' ~5, lo sprint ~7)
		float speedFixTrigger = 3000.0f; // valore di Speed (m/s x 68) oltre il quale scatta: 3000 = 44 m/s, il doppio del massimo visto in schivate/mosse speciali (solo ini)
		bool  speedFixPin = true;       // durante lo strascico riporta indietro il personaggio se si sposta piu' del limite (dal primo frame)
		bool  speedFixDeepLog = true;   // nel log di un episodio anche gli effetti attivi, le variabili del grafo e i campi del controller (solo ini)
		bool  speedFixReset = false;    // se dopo 2 s e' ancora attivo, ripristina le animazioni (spento di default: non ha mai aiutato)
		bool  speedFixSource = true;    // cerca e azzera la velocita' "nascosta" da cui il motore ricava Speed e velocityMod (solo durante un episodio)
		bool  speedFixVars = true;      // durante l'episodio limita anche le altre variabili di velocita' del grafo (nome che contiene "speed")

		// --- Attacchi in salto (animazioni via OAR) ---
		// Le animazioni sono in OAR; qui ci sono i comandi e la logica.
		bool  airAttackEnabled = true;
		bool  airAttackAlways = true;    // false = solo durante la finestra di un salto magico
		bool  airAttackAutoPick = true;  // con l'arco l'attacco normale scocca una freccia; con sole magie in mano avvia la strega volante
		int   keyAirNormal = 0;
		int   padAirNormal = 0x20000;    // controller: RT
		int   keyAirPower = 0;
		int   padAirPower = 0x30000;     // controller: LT + RT
		int   keyAirCast = 0;
		int   padAirCast = 0x10000;      // controller: LT
		int   keyAirRanged = 0;
		int   padAirRanged = 0;          // nessuno: con l'arco basta l'attacco normale

		// --- Attacchi pesanti in salto (2.0) ---
		float airPowerMinHeight = 115.0f;  // unita' di gioco sopra il punto di partenza del salto: piu' in basso l'attacco pesante non parte (115 = poco sopra un salto normale)
		bool  heavyBurst = true;           // l'attacco pesante in salto con arma a due mani fa partire un piccolo burst di fuoco (nessuna attesa tra uno e l'altro)
		float heavyBurstDamage = 25.0f;    // danno da fuoco del burst
		float heavyBurstRadius = 10.0f;    // raggio del burst in piedi di gioco (10 = circa 3 metri)
		float heavyBurstSpeed = 1.5f;      // 2.6: velocita' dell'animazione degli effetti della Tempesta di Fuoco del burst (1 = come nel gioco, 1.5 = una volta e mezzo piu' veloce; stessa dimensione)

		// --- Modalita' strega volante (2.0): in aria, con le magie in mano, resti sospeso e lanci incantesimi ---
		bool  witchEnabled = true;
		float witchSpeed = 2.5f;           // velocita' orizzontale massima mentre sei sospeso (m/s; una camminata e' circa 1.9)
		int   keyWitchExit = 0x25;         // tasto per uscire (freccia sinistra); 0 = nessuno
		int   keyWitchExit2 = 0;           // secondo tasto opzionale: se diverso da 0 servono entrambi premuti
		int   padWitchExit = 0x0104;       // controller: LB + D-pad sinistra
		bool  witchFx = true;              // effetti visivi della strega volante
		bool  witchReequip = true;         // con le mani magiche gia' fuori: le rinfoda e le rimette, cosi' si vede l'animazione di equip
		bool  witchExitOnSheathe = true;   // rinfoderare le magie col comando del gioco fa uscire dalla modalita'
		float witchTimer = 0.0f;           // durata massima in secondi (30-300); 0 = illimitata
		float witchBob = 5.0f;             // ampiezza della fluttuazione su e giu' (unita' di gioco; 0 = ferma)
		bool  witchLegsLock = true;        // il gioco non riceve nessun movimento mentre sei sospeso (stick e tasti fermati nel gestore dei comandi): ti sposta solo il plugin, le gambe non camminano
		bool  witchGroundPose = true;      // all'attivazione manda al grafo gli eventi di atterraggio del gioco (esce dalla posa di caduta senza toccare la posa delle mani)
		bool  witchFallEvent = true;       // all'uscita rimanda il grafo in posa di caduta e da' una piccola spinta verso il basso
		bool  witchBlockActions = true;    // 2.7: durante la modalita' tutto cio' che non e' lancio di magie o uscita e' bloccato (salto, sprint, scatto di altre mod, schivata...): sia i tasti sia gli eventi mandati al grafo
		bool  witchBoneLock = true;        // 2.7/2.8: se la posa scende verso quella da terra (lanci, dual casting) bacino e gambe vengono riportati piano (correzione continua) verso la posa sospesa
		bool  witchHideLanding = true;     // 2.9: all'attivazione l'atterraggio in aria non si vede: bacino, gambe e busto restano nella posa sospesa memorizzata (salvata su file)
		bool  witchNoGlide = true;         // 3.0: dopo l'uscita la mod di planata non parte da sola: cadi con la gravita' normale fino all'atterraggio (e il grafo viene aiutato a uscire dalla posa di salto)
		bool  witchSmooth = true;          // 3.0: i cambi bruschi di posa di busto, spalle e braccia (inizio dei lanci, dual casting) vengono ammorbiditi: meno scatti alle maniche e ai vestiti

		// --- Compatibilita' con la mod di planata (mentre il personaggio plana) ---
		bool paraCompat = true;          // riconosce la planata e adatta la mod finche' dura
		bool paraClearDriven = true;     // tiene spento bAnimationDriven e i flag di caduta della mod mentre plani
		bool paraPause = true;           // niente dash / slam / super salto / cambio direzione / limitatore mentre plani
		bool paraVanillaGravity = true;  // gravita' di Skyrim (non la nostra) mentre plani
		bool paraSuspend = true;         // spegne l'interruttore delle animazioni (E91) per la durata della planata: OAR smette di sostituire le animazioni
		bool paraRemovePerks = false;    // in piu' toglie i perk (gate dei salti e animazioni) per la durata della planata: piu' invasivo, di solito inutile

		// --- Effetti visivi, uno per ogni movimento (spenti = niente luci, shader, particelle) ---
		bool fxJump1 = true;    // primo salto
		bool fxJump2 = true;    // doppio salto
		bool fxJump3 = true;    // triplo salto
		bool fxDash = true;
		bool fxSlam = true;
		bool fxLanding = true;  // polvere / bagliore all'atterraggio
		bool fxLandingRed = true;  // il bagliore di protezione dalla caduta e' rosso invece che blu
		bool fxLandingNoSparks = false;  // con il rosso acceso: toglie anche l'effetto di scintille del gioco (se restano blu)
		bool fxEdSuperJump = true;  // 2.5: super salto = scia di fuoco fino all'atterraggio + rune per 2 s (solo visive) + bagliore rosso mentre e' pronto
		bool fxEdHeavy = true;      // 2.5: attacco pesante in salto = scia di fuoco fino all'atterraggio + esplosione a terra (solo visive)
		bool fxEdFallSave = true;   // 2.6: salvataggio dalla caduta (insieme al bagliore rosso) = rune intorno al personaggio per 2 s
		bool  fxSjExplosion = true;      // 2.6: super salto da terra = piccola esplosione solo visiva (effetto e suono, nessun danno) sotto i piedi
		float fxSjExplosionSize = 0.45f; // 2.6: dimensione dell'esplosione del super salto rispetto a quella intera (1 = intera)
		int   fxTrailStyle = 0;          // 2.7: scia di fuoco del super salto e dell'attacco pesante: 0 = solo fiamme sul corpo, 1 = fiamme + arte di fiamme intorno al corpo (come nella 2.6), 2 = solo arte di fiamme

		static Settings& Get();

		void Load();
		void Save();
		void MarkDirty();

		bool   dirty = false;
		double dirtyAt = 0.0;
	};

	double Now();  // secondi (orologio monotono)
}
