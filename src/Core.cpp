#include "Core.h"

#include "CrashTrace.h"
#include "Pad.h"
#include "Settings.h"

namespace MJ::Core
{
	namespace
	{
		constexpr auto kPlugin = "doublejump.esp"sv;

		// FormID locali di doublejump.esp
		constexpr std::uint32_t kPerkMain = 0x83A;
		constexpr std::uint32_t kPerkSoftLanding = 0x855;
		constexpr std::uint32_t kPerkAnim = 0xE93;          // "mod attiva" per le animazioni OAR (nuovo)
		constexpr std::uint32_t kAbilityMaster = 0x83E;
		constexpr std::uint32_t kSpellJump1 = 0x800;
		constexpr std::uint32_t kSpellJump2 = 0x803;
		constexpr std::uint32_t kSpellJump3 = 0x806;
		constexpr std::uint32_t kSpellDash = 0xDF4;
		constexpr std::uint32_t kSpellDashEnd = 0x876;
		constexpr std::uint32_t kEffJump1 = 0x802;
		constexpr std::uint32_t kEffJump2 = 0x804;
		constexpr std::uint32_t kEffJump3 = 0x805;
		constexpr std::uint32_t kGlobEnabled = 0xE91;
		constexpr std::uint32_t kGlobDirChanged = 0xE92;
		constexpr std::uint32_t kGlobInAir = 0x8A8;
		constexpr std::uint32_t kGlobSlam = 0x8A0;
		constexpr std::uint32_t kGlobDashLen = 0x8C9;
		constexpr std::uint32_t kGlobAnimLanding = 0x901;
		constexpr std::uint32_t kGlobDirOption = 0x8B5;
		constexpr std::uint32_t kGlobOne = 0x8AA;
		constexpr std::uint32_t kExplSlam = 0x87A;
		constexpr std::uint32_t kExplPower = 0xE63;
		constexpr std::uint32_t kSoundDashFirst = 0xE94;    // 5 suoni del dash (E94..E98)
		constexpr int           kSoundDashCount = 5;
		constexpr std::uint32_t kArtDashFire = 0xE99;       // effetto dash di fuoco (nuovo)
		constexpr std::uint32_t kArtDashOrig = 0x87C;       // effetto dash originale

		constexpr std::uint32_t kLegacyBJPerk = 0xF2CA6;

		// valori di riferimento
		constexpr double kDashDuration = 1.0;        // durata dello spell di dash
		constexpr double kDashEventDelay = 0.06;     // dal cast all'evento di animazione
		constexpr double kDashCooldown = 0.5;
		constexpr double kDashBuffer = 0.30;         // un tocco poco prima che il dash sia permesso vale lo stesso
		constexpr double kChordGrace = 0.07;         // attesa per capire se premi una combinazione piu' grande
		constexpr float  kSlowTime = 0.10f;
		constexpr double kSlamWindupThird = 0.75;
		constexpr double kSlamWindupFirst = 0.35;
		constexpr float  kSlamGravity = 50.0f;
		constexpr double kSuperJumpWindow = 5.0;     // quanto resta "carico" il super jump
		constexpr double kWaterWatch = 6.0;          // dopo un volo finito in acqua: per quanto tempo ripulisco lo stato
		constexpr double kDirMaxTurnTime = 1.0;      // slider "peso" al 100 %: secondi per una virata di 180 gradi
		constexpr double kBodyTurnRate = 14.0;       // quanto in fretta gira il modello del personaggio (rad/s, 180 gradi in ~0.22 s)
		constexpr float  kSpeedVarPerMps = 68.0f;    // la variabile "Speed" del grafo = velocita' del movimento in m/s x 68 (misurato nei log)
		constexpr double kAnomCalm = 0.6;            // secondi con Speed normale per dichiarare finito un episodio di velocita' anomala
		constexpr float  kRunMps = 5.287f;           // velocityMod = speedPct x 5.287 m/s (misurato nei log)
		constexpr double kLfWindow = 25.0;           // dopo la fine di un dash: per quanto una Speed altissima conta come strascico
		constexpr double kLfMax = 60.0;              // durata massima di un episodio di strascico
		constexpr int    kLfLogFrames = 30;          // frame di ogni episodio descritti nel log
		constexpr double kLfProbeEvery = 2.0;        // ogni quanti secondi, durante un episodio, scrivo una riga di controllo
		constexpr float  kLfPinMargin = 1.1f;        // il blocco dello spostamento lascia passare fino a cap x dt x 1.1
		constexpr double kGuardAfterDash = 6.0;      // per quanto sorveglio la velocita' dopo la fine del dash
		constexpr double kGuardAfterLand = 3.0;      // ...e dopo l'atterraggio di un volo con dash
		constexpr float  kHavokScale = 69.99125f;    // unita' di gioco per metro di Havok
		constexpr float  kGuardDispMin = 16.0f;      // spostamento reale in un frame oltre il quale e' uno "scatto" (m/s)
		constexpr float  kGuardMaxUp = 30.0f;        // velocita' verticale massima verso l'alto durante il limitatore (m/s)
		constexpr std::uint32_t kFireStormId = 0x0007A82B;  // magia Tempesta di Fuoco in Skyrim.esm
		constexpr auto          kSettingsPlugin = "SettingsManager.esp"sv;  // plugin che crea i riferimenti nel salvataggio
		constexpr int           kLegacyPasses = 3;          // passate della disattivazione dei vecchi script
		constexpr std::size_t   kLegacyMaxRefs = 300;       // sicurezza: non elimino mai piu' riferimenti di cosi'

		// attacchi in salto: variabili globali di doublejump.esp che le condizioni OAR leggono
		constexpr std::uint32_t kGlobAtkP1 = 0x8A6;         // MJ_MCM_InAirAttackP1: attacco in corso
		constexpr std::uint32_t kGlobAtkP2 = 0x8B0;         // MJ_MCM_InAirAttackP2: attacco finito (rilascio dell'arco)
		constexpr std::uint32_t kGlobAtkAirPower = 0x8B1;   // MJ_AirPowerAttacking: il secondo dell'attacco potente
		constexpr std::uint32_t kGlobAtkHand = 0x8B3;       // MJ_CastLeftRight: 1 = destra, 0 = sinistra
		constexpr std::uint32_t kGlobAtkNormal = 0x8B8;     // MJ_MCM_InAirNormalAttacking
		constexpr std::uint32_t kGlobAtkCast = 0x8B9;       // MJ_MCM_InAirCasting
		constexpr std::uint32_t kGlobAtkPower = 0x8C4;      // MJ_MCM_InAirPowerAttacking
		constexpr std::uint32_t kGlobAtkRanged = 0x8C5;     // MJ_MCM_InAirRangedAttacking
		constexpr double kAtkStartDelay = 0.10;             // dall'inizio dell'attacco all'evento di animazione (tempo fisso)
		constexpr double kAtkPowerSeconds = 1.0;            // quanto dura MJ_AirPowerAttacking e la gravita' forte
		constexpr float  kAtkPowerGravity = 30.0f;          // gravita' durante l'attacco potente (valore fisso)
		constexpr double kAtkLandKeep = 0.45;               // dopo l'atterraggio le variabili restano accese ancora un attimo (l'animazione di atterraggio le legge)
		constexpr double kAtkPowerLandKeep = 1.2;           // attacco potente: l'animazione di atterraggio e' piu' lunga
		constexpr double kAtkPowerLandMax = 2.5;
		constexpr double kAtkCastDelay = 0.30;              // dall'annotazione dell'animazione al lancio (0.3 s)
		constexpr double kAtkCastFallback = 0.70;           // senza l'annotazione (animazioni non installate) lancio comunque dopo questo tempo
		constexpr double kAtkCastAnimEnd = 1.0;             // dopo il lancio l'animazione resta accesa per questo tempo
		constexpr double kAtkCastLock = 2.0;                // un solo lancio ogni 2 s (un lancio alla volta)
		constexpr double kAtkBowMinDraw = 0.25;
		constexpr double kAtkBowMaxDraw = 4.0;
		constexpr double kAtkBowAnimEnd = 0.7;
		constexpr double kAtkNormalFailsafe = 1.8;          // se Collision_AttackEnd non arriva, l'animazione si spegne da sola
		constexpr double kAtkDup = 0.25;                    // gli eventi arrivano due volte (due grafi): ignoro i doppioni
		constexpr double kAtkMaxFlight = 8.0;
		constexpr int    kPadActions = 8;                   // super salto, dash, slam, attacco normale, potente, magia, arco, uscita dalla strega volante

		// strega volante (2.0): variabile globale per le animazioni (OAR), magie ed effetti
		constexpr std::uint32_t kGlobWitch = 0xE9A;         // MJN_Witch: 1 = modalita' attiva (le condizioni OAR del set di animazioni la leggono)
		constexpr std::uint32_t kEffBurstDamage = 0xE9D;    // effetto di danno del burst di fuoco
		constexpr std::uint32_t kSpellBurst = 0xE9E;        // burst di fuoco dell'attacco pesante in salto
		constexpr std::uint32_t kShaderRed = 0xE9F;         // shader rosso della protezione dalla caduta
		constexpr std::uint32_t kSpellWitchFx = 0xEA6;      // abilita' con gli effetti visivi della strega volante
		constexpr double kWitchUnequipMin = 1.2;            // durata dell'animazione di unequip (1.133 s) + un margine
		constexpr double kWitchEquipMax = 2.5;              // se lo stato "mani fuori" non arriva, vado avanti lo stesso dopo questo tempo
		constexpr double kWitchPhaseMax = 3.0;              // tempo massimo di attesa di ogni fase (mani riposte / unequip)
		constexpr float  kWitchAccel = 9.0f;                // quanto in fretta cambia la velocita' orizzontale (m/s al secondo)
		constexpr float  kWitchVertAccel = 60.0f;           // ...e quella verticale (frenata all'ingresso)
		constexpr float  kWitchHoldGain = 7.0f;             // correzione di quota: m/s di velocita' per ogni metro di scarto
		constexpr float  kWitchMaxVert = 4.0f;              // velocita' verticale massima della correzione (m/s)
		constexpr double kWitchPoseWait = 0.15;             // dopo l'evento di posa a terra aspetto un attimo prima di tirare fuori le mani
		constexpr float  kWitchDropSpeed = 45.0f;           // quanto in fretta scende l'ancora all'attivazione (unita' di gioco al secondo)
		constexpr double kWitchBobPeriod = 3.4;             // periodo della fluttuazione su e giu' (s)
		constexpr double kWitchBobRamp = 1.2;               // la fluttuazione sale piano dopo l'attivazione (s)
		constexpr float  kWitchFallKick = 3.0f;             // spinta verso il basso all'uscita (m/s)
		constexpr float  kWitchMinClear = 60.0f;            // l'abbassamento non porta mai il personaggio a meno di tanto dal terreno (unita' di gioco)

		struct Forms
		{
			RE::BGSPerk*       perk = nullptr;
			RE::BGSPerk*       perkSoft = nullptr;
			RE::BGSPerk*       perkAnim = nullptr;
			RE::SpellItem*     ability = nullptr;
			RE::SpellItem*     spJump1 = nullptr;
			RE::SpellItem*     spJump2 = nullptr;
			RE::SpellItem*     spJump3 = nullptr;
			RE::SpellItem*     spDash = nullptr;
			RE::SpellItem*     spDashEnd = nullptr;
			RE::SpellItem*     spFireStorm = nullptr;  // Tempesta di Fuoco (Skyrim.esm), per lo slam
			RE::EffectSetting* mgJump1 = nullptr;
			RE::EffectSetting* mgJump2 = nullptr;
			RE::EffectSetting* mgJump3 = nullptr;
			RE::TESGlobal*     gEnabled = nullptr;
			RE::TESGlobal*     gDirChanged = nullptr;
			RE::TESGlobal*     gInAir = nullptr;
			RE::TESGlobal*     gSlam = nullptr;
			RE::TESGlobal*     gDashLen = nullptr;
			RE::TESGlobal*     gAnimLanding = nullptr;
			RE::TESGlobal*     gDirOption = nullptr;
			RE::TESGlobal*     gOne = nullptr;
			RE::TESGlobal*     gAtkP1 = nullptr;
			RE::TESGlobal*     gAtkP2 = nullptr;
			RE::TESGlobal*     gAtkAirPower = nullptr;
			RE::TESGlobal*     gAtkHand = nullptr;
			RE::TESGlobal*     gAtkNormal = nullptr;
			RE::TESGlobal*     gAtkCast = nullptr;
			RE::TESGlobal*     gAtkPower = nullptr;
			RE::TESGlobal*     gAtkRanged = nullptr;
			RE::TESGlobal*     gWitch = nullptr;
			RE::SpellItem*     spBurst = nullptr;
			RE::SpellItem*     spWitchFx = nullptr;
			RE::EffectSetting* mgBurstDmg = nullptr;
			RE::TESEffectShader* shRed = nullptr;
			bool               atkOk = false;  // le 8 variabili esistono in doublejump.esp
			RE::BGSExplosion*  explSlam = nullptr;
			RE::BGSExplosion*  explPower = nullptr;
			RE::BGSArtObject*  artFire = nullptr;
			RE::BGSArtObject*  artOrig = nullptr;
			std::array<RE::BGSSoundDescriptorForm*, kSoundDashCount> sndDash{};
			bool               ok = false;
		};

		enum class Ev
		{
			JumpUp,
			SoftLanding,
			FastLanding,
			PowerAttackLanding,
			Parachute,
			ParaStart,  // il behavior della mod di planata ha iniziato la planata (SoundPlay.ParaglideEquip)
			ParaEnd,    // il behavior della mod di planata ha iniziato a ripiegare la planata (fine planata)
			AttackEnd,  // Collision_AttackEnd: l'animazione di un attacco in salto e' finita
			AirCast     // PIE.$MJ_InAirCast: l'animazione del lancio in salto e' al punto del lancio
		};

		struct PadTap
		{
			bool   pend = false;    // tocco in attesa di capire se e' una combinazione piu' grande
			double pendAt = 0.0;
			bool   cancel = false;  // la combinazione piu' grande e' stata premuta: niente azione
			bool   fired = false;   // azione gia' partita per questa pressione
		};

		struct Key
		{
			bool   down = false;
			double since = 0.0;
		};

		struct KeyEvt
		{
			bool   pressed = false;
			bool   released = false;
			bool   down = false;
			double held = 0.0;
		};

		constexpr double kTransitionQuiet = 1.5;     // dopo un cambio di zona / teletrasporto / caricamento: secondi in cui non tocco il personaggio
		constexpr float  kTeleportUnits = 4000.0f;   // spostamento in un solo frame oltre il quale e' un teletrasporto (1 m = ~70 unita')

		struct Delayed
		{
			double at;
			int    kind;  // 0 = bAnimationDriven false
		};

		struct Runtime
		{
			bool initialized = false;
			bool applied = false;

			// valori di Skyrim catturati prima di toccare qualunque cosa
			float vanillaJump = 76.0f;
			float vanillaGravity = 1.35f;
			bool  vanillaCaptured = false;

			// ultimi valori scritti (per non riscrivere ogni frame)
			float appliedJump = -1.0f;
			float appliedGravity = -1.0f;

			double lastMaintain = 0.0;
			double lastSink = 0.0;
			double lastAirAt = -100.0;
			double loadedAt = 0.0;
			bool   bjWarned = false;

			// cambio di zona / teletrasporto: per un attimo non tocco il personaggio
			const void* lastArea = nullptr;
			bool        had3D = false;
			bool        lastPosValid = false;
			float       lastPosX = 0.0f;
			float       lastPosY = 0.0f;
			double      quietUntil = 0.0;
			int         transitions = 0;
			double      paraPollAt = 0.0;

			// aria / atterraggio
			int    airTicks = 0;
			int    groundTicks = 0;
			bool   airborne = false;

			// conteggio dei salti (per disattivare doppio / triplo salto)
			int    jumpCount = 0;
			double lastJumpUpAt = -100.0;
			bool   jumpGateClosed = false;

			// controllo del movimento in aria
			bool   acValid = false;
			float  acX = 0.0f;
			float  acY = 0.0f;
			bool   acLogged = false;

			// acqua dopo un volo / dash
			bool   wasSwimming = false;
			double waterWatchUntil = 0.0;
			double waterSampleAt = 0.0;
			float  waterLastX = 0.0f;
			float  waterLastY = 0.0f;
			int    waterStuck = 0;
			int    waterRescues = 0;

			// suoni del dash
			int    dashSoundNext = 0;
			int    dashSoundLast = -1;

			// direzione al salto precedente
			bool   curValid = false;    // lo stick e' abbastanza inclinato da avere una direzione
			float  curAng = 0.0f;       // angolo dello stick (0 = avanti, + = destra), in radianti
			bool   prevValid = false;
			float  prevAng = 0.0f;      // angolo dello stick 0.12 s dopo l'ultimo salto
			bool   prevPending = false;
			double prevAt = 0.0;
			bool   dirChanged = false;  // la direzione dello stick e' diversa da quella del salto precedente

			// cambio direzione in aria: la direzione reale del movimento = angolo dello stick + camOff.
			// camOff si misura a terra (confronto tra stick e spostamento reale) cosi' non dipende da
			// come Skyrim orienta la camera.
			float  stickX = 0.0f;
			float  stickY = 0.0f;
			bool   stickFromPad = false;
			double stickStableSince = 0.0;
			float  lastStickAng = 0.0f;
			float  camOff = 0.0f;
			bool   camValid = false;
			bool   posValid = false;
			float  posX = 0.0f;
			float  posY = 0.0f;
			double posAt = 0.0;
			double dirUntil = 0.0;      // fino a quando forzo la nuova direzione
			float  dirVx = 0.0f;        // velocita' orizzontale voluta (unita' Havok, m/s)
			float  dirVy = 0.0f;
			int    dirApplied = 0;
			bool   dirReport = false;   // a fine finestra scrivo nel log cosa e' successo davvero
			float  dirStartX = 0.0f;
			float  dirStartY = 0.0f;
			double dirStartAt = 0.0;
			float  dirWantAng = 0.0f;
			float  dirCurAng = 0.0f;    // direzione attuale della virata graduale (radianti)
			float  dirSpeed = 0.0f;     // velocita' orizzontale tenuta durante la virata (m/s)
			double dirTurnTime = 0.0;   // secondi per una virata di 180 gradi (0 = istantanea)
			double dirLastAt = 0.0;
			bool   dirBody = false;     // ruoto anche il modello del personaggio
			float  dirBodyAng = 0.0f;   // direzione attuale del modello durante la virata

			// limitatore di velocita' dopo il dash
			double guardUntil = 0.0;
			bool   guardAfterDash = false;
			int    guardLogs = 0;
			bool   gdValid = false;      // posizione del frame precedente, per misurare lo spostamento reale
			double gdAt = 0.0;
			float  gdX = 0.0f;
			float  gdY = 0.0f;
			const void* gdCell = nullptr;

			// strascico del dash dopo un atterraggio a meta' dash ("bug supersonico")
			double      lfArmUntil = 0.0;     // fino a quando una Speed altissima conta come strascico del dash
			bool        lfClamp = false;      // episodio in corso: l'aggancio a UpdateAnimation limita il movimento
			bool        lfPosValid = false;   // posizione del frame prima (spostamento reale)
			float       lfX = 0.0f;
			float       lfY = 0.0f;
			double      lfAt = 0.0;
			const void* lfCell = nullptr;
			double      lfSince = -1.0;       // inizio dell'episodio in corso (-1 = nessuno)
			double      lfCalmSince = -1.0;   // da quando Speed e' tornata normale
			int         lfEpisodes = 0;
			int         lfFrames = 0;
			int         lfLogs = 0;
			float       lfPeak = 0.0f;
			int         lfHookCalls = 0;      // volte in cui l'aggancio ha dovuto limitare velocityMod
			float       lfHookMax = 0.0f;     // velocityMod massimo visto dall'aggancio (m/s)
			float       hkPreMod = -1.0f;     // velocityMod / speedPct / Speed del grafo ALL'INGRESSO di UpdateAnimation, prima del mio limite (solo log)
			float       hkPrePct = -1.0f;
			float       hkPreSpeed = -1.0f;
			float       hkPostMod = -1.0f;    // ...e all'uscita, prima del mio secondo limite (solo log)
			float       hkPostPct = -1.0f;
			float       hkPostSpeed = -1.0f;
			int         lfEntryCuts = 0;      // volte in cui il limite all'ingresso ha tolto qualcosa
			bool        lfPin = false;        // blocco dello spostamento per frame attivo
			int         lfPinFrames = 0;      // frame in cui il blocco ha riportato indietro il personaggio
			bool        lfPinValid = false;
			float       lfPinX = 0.0f;        // posizione (x, y, metri Havok) del controller dopo l'ultimo blocco
			float       lfPinY = 0.0f;
			double      lfPinAt = 0.0;        // ...e quando
			const void* lfPinCell = nullptr;
			float       lfStep = 0.0f;        // ultimo spostamento del controller PRIMA del blocco (m/s)
			float       lfStepMax = 0.0f;
			float       lfBack = 0.0f;        // di quanto l'ultimo blocco ha riportato indietro il personaggio (m)
			double      lfProbeAt = 0.0;
			int         lfProbes = 0;
			int         lfResets = 0;
			const char* dashEndWhy = nullptr; // perche' e' finito l'ultimo dash (solo log)
			double      dashEndAt = -100.0;

			// Mod di planata
			bool        paraHas = false;       // hasParaGlider != 0
			bool        paraSeen = false;      // visto almeno una volta in questa sessione
			bool        paraVar = false;       // bParaGliding acceso
			int         paraVarLogs = 0;
			int         paraEvLogs = 0;
			double      paraTextAt = 0.0;
			bool        glide = false;         // planata in corso
			bool        glideEvOn = false;     // il behavior ha detto "inizio planata" e non ancora "fine"
			const char* glideWhy = "";
			double      glideSince = 0.0;
			double      glideLogAt = 0.0;
			int         glideLogs = 0;
			int         glideCount = 0;
			int         glideFixes = 0;
			int         glideFixLogs = 0;
			bool        glideSuspended = false;  // perk e interruttore delle animazioni spenti da me per la durata della planata
			bool        glideRemovedPerk = false;
			bool        glideRemovedAnim = false;
			double      glideOffSince = -1.0;   // da quando la mod di planata non risulta piu' in planata (per non spegnere/accendere a ogni oscillazione)
			double      glideSuspendOffAt = 0.0;  // quando e' finita l'ultima planata (le animazioni tornano dopo una piccola attesa)
			bool        glideCapLogged = false;
			double      paraAirSince = -1.0;  // da quando e' in aria con la mod di planata senza che io riconosca una planata (solo log)
			double      paraAirAt = 0.0;
			int         paraAirLogs = 0;

			// disattivazione dei vecchi script (pulsante del menu)
			int    legacyPass = -1;  // -1 = nessuna operazione in corso
			double legacyNextAt = 0.0;
			int    legacyEffects = 0;
			int    legacySpells = 0;
			int    legacyQuests = 0;
			int    legacyRefs = 0;
			bool   legacyAbility = false;
			double abilityHoldUntil = 0.0;  // fino a quando non rimetto l'abilita' base (cosi' l'effetto vecchio con lo script sparisce davvero)

			// acqua: ripristino dell'animazione poco dopo l'ingresso
			bool   waterIdlePending = false;
			double waterIdleAt = 0.0;

			// reset animazioni
			Key    kReset;
			bool   resetFired = false;

			// controller: da quando ogni combinazione e' premuta (dash e attacchi in salto: piccola attesa per capire se premi una combinazione piu' grande)
			PadTap padTap[kPadActions];

			// attacchi in salto (animazioni via OAR)
			Key    kAirN, kAirP, kAirC, kAirR;
			bool   jaAnimOn = false;        // P1 acceso: l'animazione dell'attacco e' in corso
			int    jaType = -1;             // attacco in corso: 0 normale, 1 potente, 2 magia, 3 arco, -1 nessuno
			bool   jaFlightUsed = false;    // un attacco (normale, potente, arco) per volo
			double jaStartAt = 0.0;
			double jaEventAt = -1.0;        // quando mandare l'evento di animazione (-1 = gia' mandato)
			bool   jaPowerOn = false;       // MJ_AirPowerAttacking acceso
			double jaPowerOffAt = 0.0;
			bool   jaPowerGrav = false;     // la gravita' forte e' mia
			bool   jaDriven = false;        // bAnimationDriven l'ho acceso io
			double jaCleanAt = -1.0;        // dopo l'atterraggio: quando spegnere le variabili (-1 = niente da fare)
			double jaCleanMax = -1.0;
			double jaPowerLandAt = -1.0;    // MJPowerAttackLanding visto
			double jaAnimEndAt = -1.0;      // quando spegnere P1 (animazione di magia / arco / attacco senza Collision_AttackEnd)
			double jaCastAt = -100.0;       // ultimo lancio
			double jaCastFireAt = -1.0;     // lancio programmato
			bool   jaCastDone = false;
			int    jaBowKey = -1;           // 0 = tasto dell'attacco normale, 3 = tasto dell'arco
			bool   jaBowRelease = false;
			double jaBowReleaseAt = 0.0;
			bool   jaBowFired = false;
			double jaLastEvAt[2] = { -100.0, -100.0 };  // AttackEnd, AirCast: per i doppioni
			int    jaCount = 0;
			int    jaLogs = 0;
			int    jaRejLogs = 0;  // righe di rifiuto gia' scritte (limite a parte, cosi' non consumano il resto del log)
			std::string jaLast;             // ultimo attacco, per il menu (aggiornato dal thread di gioco, letto sotto g_atkMutex)

			// velocita' alla radice dello strascico 
			struct ProbeCand
			{
				std::uint8_t  region = 0;   // 0 = MiddleHighProcessData, 1 = HighProcessData
				std::uint16_t off = 0;
				std::uint8_t  scale = 0;
				int           hits = 0;
				bool          changed = false;
				float         last = 0.0f;
			};
			std::array<ProbeCand, 24> pbCands{};
			int         pbCandCount = 0;
			bool        pbLocked = false;
			ProbeCand   pbLock{};
			int         pbWrites = 0;
			float       pbWorst = 0.0f;     // il valore piu' alto che il campo agganciato aveva prima del mio limite
			int         pbFrames = 0;
			int         pbLogs = 0;
			std::vector<std::string> gvNames;  // variabili float del grafo con "speed" nel nome
			bool        gvBuilt = false;
			double      gvTryAt = 0.0;
			int         gvCuts = 0;
			int         gvLogs = 0;

			// super jump
			Key    kSuper;
			bool   sjFired = false;
			bool   sjArmed = false;
			double sjArmedAt = 0.0;
			double sjResetAt = -1.0;

			// dash
			Key    kDash;
			bool   dashActive = false;
			bool   dashEventSent = false;
			double dashStart = 0.0;
			double dashCooldownUntil = 0.0;
			bool   dashUsed = false;           // un solo dash per ogni volo
			double dashBufferUntil = 0.0;      // richiesta di dash ancora valida fino a questo istante
			const char* dashWhy = nullptr;     // perche' l'ultima richiesta non e' partita (solo per il log)

			// slam
			Key    kSlam;
			bool   slamConsumed = false;
			bool   slamActive = false;
			int    slamPhase = 0;  // 1 = carica, 2 = caduta, 3 = fine
			double slamStart = 0.0;
			double slamWindup = 0.75;
			double slamEndAt = 0.0;
			bool   slamCastWindow = false;
			bool   slamLanded = false;
			double slamCooldownUntil = 0.0;

			// sovrascritture
			bool  gravityOverride = false;
			float gravityValue = 1.35f;
			bool  timeScaled = false;
			float prevTimeScale = 1.0f;

			// strega volante (2.0)
			Key         kWitchExit;
			int         wmState = 0;            // 0 spenta, 1 equip, 2 attiva, 3 unequip
			double      wmSince = 0.0;          // inizio dello stato corrente
			double      wmStart = 0.0;          // inizio della modalita'
			double      wmLastAt = 0.0;
			float       wmAnchorZ = 0.0f;       // quota da tenere
			float       wmVx = 0.0f;            // velocita' che scrivo sul controller (m/s)
			float       wmVy = 0.0f;
			float       wmVz = 0.0f;
			int         wmEquipStep = 0;        // 0 niente, 1 aspetto le mani riposte, 2 aspetto le mani fuori
			double      wmStepAt = 0.0;
			double      wmUnequipMin = 0.0;     // attesa minima dell'unequip
			double      wmNoSpellSince = -1.0;
			bool        wmEverDrawn = false;    // le mani sono state viste "fuori" almeno una volta in questa modalita'
			bool        wmGravity = false;      // la gravita' a zero e' mia
			bool        witchSuspended = false; // perk e interruttore delle animazioni spenti da me
			bool        wmRemovedPerk = false;
			bool        wmRemovedAnim = false;
			const char* wmExitWhy = nullptr;
			double      wmLogAt = 0.0;
			double      wmFxAt = 0.0;
			int         wmLogs = 0;
			int         wmRejLogs = 0;
			int         wmCount = 0;
			float       wmStartZ = 0.0f;        // quota di attivazione
			float       wmAnchorGoal = 0.0f;    // quota finale dell'ancora (dopo l'abbassamento)
			double      wmBobAt = 0.0;          // inizio della fluttuazione
			double      wmPoseAt = 0.0;         // quando e' partito l'ultimo evento di uscita dalla posa di caduta
			int         wmPoseStage = 0;        // 0 nessuno, 1 JumpLandFailSafe, 2 JumpLand, 3 JumpLandEnd
			double      wmLegLogAt = 0.0;       // prossima riga di log delle gambe ferme
			int         wmLegLogs = 0;
			double      wmJumpSince = -1.0;     // da quando il grafo risulta in salto / caduta
			int         wmJumpFixes = 0;
			double      wmTextAt = 0.0;
			bool        wmWarned = false;       // avviso dei 10 secondi gia' dato
			double      wmFallDiagUntil = 0.0;  // diagnostica dopo l'uscita
			double      wmFallDiagAt = 0.0;
			bool        burstLogged = false;
			bool        wmLegSaved = false;     // aggancio di UpdateAnimation: velocityMod azzerato in ingresso, da rimettere in uscita
			float       wmLegMod[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			float       wmLegPct = 0.0f;
			int         wmLegCalls = 0;

			// altezza del volo (attacco pesante in salto)
			float       groundZ = 0.0f;
			bool        groundZValid = false;
			float       takeoffZ = 0.0f;
			bool        takeoffValid = false;
			bool        jaBurstDone = false;

			std::vector<Delayed> delayed;
		};

		Forms   F;
		Runtime R;
		Status  S;

		std::mutex          g_evMutex;
		std::vector<Ev>     g_events;
		std::atomic<double> g_menuUntil{ 0.0 };
		std::atomic<bool>   g_resetRequest{ false };  // il pulsante del menu non tocca il gioco: lascia una richiesta, la esegue Tick
		std::atomic<double> g_fireReadyAt{ 0.0 };     // quando la Tempesta di Fuoco dello slam e' di nuovo pronta (orologio di Now())
		std::atomic<bool>   g_legacyRequest{ false };  // disattivazione dei vecchi script, richiesta dal menu
		std::atomic<bool>   g_legacyBusy{ false };
		std::mutex          g_legacyMutex;
		std::string         g_legacyText;  // esito, mostrato nel menu
		std::mutex          g_paraMutex;
		std::string         g_paraText;    // stato della mod di planata, mostrato nel menu
		std::mutex          g_atkMutex;
		std::string         g_atkText;     // stato degli attacchi in salto, mostrato nel menu
		std::string         g_witchText;   // stato della strega volante, mostrato nel menu (sotto g_atkMutex)

		// ----------------------------------------------------------------
		// effetti visivi per movimento (luci, shader, particelle degli effetti magici)
		// Si spengono azzerando i puntatori grafici dell'effetto in memoria (il file .esp non
		// cambia) e si ripristinano quando li riaccendi.
		// ----------------------------------------------------------------
		enum FxField : std::uint32_t
		{
			kFxLight = 1,
			kFxShader = 2,
			kFxEnchShader = 4,
			kFxCastArt = 8,
			kFxHitArt = 16,
			kFxEnchArt = 32,
			kFxExplosion = 64,
			kFxVisuals = kFxLight | kFxShader | kFxEnchShader | kFxCastArt | kFxHitArt | kFxEnchArt
		};

		enum FxGroup : int
		{
			kGroupJump1,
			kGroupJump2,
			kGroupJump3,
			kGroupDash,
			kGroupLanding
		};

		struct FxEntry
		{
			int                  group;
			std::uint32_t        local;   // FormID locale dell'effetto magico
			std::uint32_t        fields;  // cosa spegnere
			RE::EffectSetting*   mgef = nullptr;
			RE::TESObjectLIGH*   light = nullptr;
			RE::TESEffectShader* shader = nullptr;
			RE::TESEffectShader* enchShader = nullptr;
			RE::BGSArtObject*    castArt = nullptr;
			RE::BGSArtObject*    hitArt = nullptr;
			RE::BGSArtObject*    enchArt = nullptr;
			RE::BGSExplosion*    explosion = nullptr;
			bool                 hidden = false;
			int                  style = -1;  // stile dell'effetto applicato (solo dash)
		};

		std::array<FxEntry, 6> g_fx{ {
			{ kGroupJump1, 0x802, kFxVisuals },
			{ kGroupJump2, 0x804, kFxVisuals },
			{ kGroupJump3, 0x805, kFxVisuals },
			{ kGroupDash, 0xDF0, kFxVisuals },
			{ kGroupLanding, 0x8EE, kFxExplosion },  // polvere (solo visiva)
			{ kGroupLanding, 0x911, kFxShader | kFxHitArt },  // bagliore (l'esplosione di stordimento resta)
		} };

		void InitFx(RE::TESDataHandler* a_dh)
		{
			for (auto& e : g_fx) {
				e.mgef = a_dh->LookupForm<RE::EffectSetting>(e.local, kPlugin);
				if (!e.mgef) {
					logger::warn("effetto {:X} non trovato (effetti visivi)", e.local);
					continue;
				}
				const auto& d = e.mgef->data;
				e.light = d.light;
				e.shader = d.effectShader;
				e.enchShader = d.enchantShader;
				e.castArt = d.castingArt;
				e.hitArt = d.hitEffectArt;
				e.enchArt = d.enchantEffectArt;
				e.explosion = d.explosion;
			}
		}

		// a_enabled = false: la mod e' spenta, rimetto tutto come in origine
		void ApplyFx(bool a_enabled)
		{
			const auto& s = Settings::Get();
			const bool  want[5] = { s.fxJump1, s.fxJump2, s.fxJump3, s.fxDash, s.fxLanding };
			for (auto& e : g_fx) {
				if (!e.mgef) {
					continue;
				}
				const bool hide = a_enabled && !want[e.group];
				const bool red = a_enabled && e.group == kGroupLanding && s.fxLandingRed && F.shRed != nullptr;
				const bool noSparks = red && s.fxLandingNoSparks;
				const int  style = (e.group == kGroupDash) ? s.dashStyle : (red ? (noSparks ? 2 : 1) : 0);
				if (hide == e.hidden && style == e.style) {
					continue;
				}
				auto& d = e.mgef->data;
				if (hide) {
					if (e.fields & kFxLight) {
						d.light = nullptr;
					}
					if (e.fields & kFxShader) {
						d.effectShader = nullptr;
					}
					if (e.fields & kFxEnchShader) {
						d.enchantShader = nullptr;
					}
					if (e.fields & kFxCastArt) {
						d.castingArt = nullptr;
					}
					if (e.fields & kFxHitArt) {
						d.hitEffectArt = nullptr;
					}
					if (e.fields & kFxEnchArt) {
						d.enchantEffectArt = nullptr;
					}
					if (e.fields & kFxExplosion) {
						d.explosion = nullptr;
					}
				} else {
					if (e.fields & kFxLight) {
						d.light = e.light;
					}
					if (e.fields & kFxShader) {
						d.effectShader = (red && e.shader) ? F.shRed : e.shader;  // bagliore della protezione dalla caduta: rosso invece di blu
					}
					if (e.fields & kFxEnchShader) {
						d.enchantShader = e.enchShader;
					}
					if (e.fields & kFxCastArt) {
						d.castingArt = e.castArt;
					}
					if (e.fields & kFxHitArt) {
						d.hitEffectArt = noSparks ? nullptr : e.hitArt;
						if (e.group == kGroupDash && F.artFire && F.artOrig) {
							d.hitEffectArt = (style == 1) ? F.artOrig : F.artFire;  // stile del dash: fuoco / originale
						}
					}
					if (e.fields & kFxEnchArt) {
						d.enchantEffectArt = e.enchArt;
					}
					if (e.fields & kFxExplosion) {
						d.explosion = e.explosion;
					}
				}
				e.hidden = hide;
				e.style = style;
			}
		}

		// ----------------------------------------------------------------
		// utilita'
		// ----------------------------------------------------------------
		bool KeyIsDown(int a_vk)
		{
			if (a_vk <= 0) {
				return false;
			}
			const auto st = static_cast<std::uint16_t>(REX::W32::GetKeyState(a_vk));
			return (st & 0x8000u) != 0;
		}

		// Valuta le combinazioni del controller (super salto, dash, slam, 4 attacchi in salto). Una combinazione conta solo
		// se tutti i suoi pulsanti sono premuti e non c'e' un'altra combinazione piu' grande che li contiene
		// (es. "D-pad destra" per il dash e "LB + D-pad destra" per lo slam non si pestano i piedi; RT = attacco normale e
		// LT + RT = attacco potente). Le azioni "a tocco" (dash e attacchi) se esiste una combinazione piu' grande aspettano
		// un attimo per capire cosa stai premendo, ma un tocco breve parte comunque, al rilascio.
		void EvalPad(std::uint32_t a_pad, double a_now, bool a_out[kPadActions])
		{
			const auto&         s = Settings::Get();
			const std::uint32_t m[kPadActions] = { static_cast<std::uint32_t>(s.padSuperJump), static_cast<std::uint32_t>(s.padDash), static_cast<std::uint32_t>(s.padSlam),
				static_cast<std::uint32_t>(s.padAirNormal), static_cast<std::uint32_t>(s.padAirPower), static_cast<std::uint32_t>(s.padAirCast), static_cast<std::uint32_t>(s.padAirRanged),
					static_cast<std::uint32_t>(s.padWitchExit) };
			const bool en[kPadActions] = { s.superJumpEnabled, s.dashEnabled, s.slamEnabled, s.airAttackEnabled, s.airAttackEnabled, s.airAttackEnabled, s.airAttackEnabled, s.witchEnabled };
			const bool tap[kPadActions] = { false, true, false, true, true, true, true, true };

			bool raw[kPadActions];
			bool hasBigger[kPadActions];
			bool biggerOn[kPadActions];
			for (int i = 0; i < kPadActions; ++i) {
				raw[i] = m[i] != 0 && (a_pad & m[i]) == m[i];
				hasBigger[i] = false;
				biggerOn[i] = false;
			}
			for (int i = 0; i < kPadActions; ++i) {
				for (int j = 0; j < kPadActions; ++j) {
					if (i != j && en[j] && m[j] != 0 && m[j] != m[i] && (m[j] & m[i]) == m[i]) {
						hasBigger[i] = true;
						if (raw[j]) {
							biggerOn[i] = true;
						}
					}
				}
				a_out[i] = raw[i] && !biggerOn[i];
			}

			for (int i = 0; i < kPadActions; ++i) {
				if (!tap[i] || !hasBigger[i]) {
					R.padTap[i] = PadTap{};
					continue;
				}
				auto& t = R.padTap[i];
				bool  out = false;
				if (!raw[i]) {
					if (t.pend && !t.cancel && !t.fired) {
						out = true;  // tocco breve: parte al rilascio
					}
					t.pend = false;
					t.cancel = false;
					t.fired = false;
				} else if (biggerOn[i]) {
					t.cancel = true;
					t.pend = false;
				} else if (!t.cancel) {
					if (!t.pend && !t.fired) {
						t.pend = true;
						t.pendAt = a_now;
					}
					if (t.pend && (a_now - t.pendAt) >= kChordGrace) {
						t.pend = false;
						t.fired = true;
					}
					out = t.fired;
				}
				a_out[i] = out;
			}
		}

		KeyEvt Poll(Key& a_key, bool a_raw, bool a_allowed, double a_now)
		{
			KeyEvt e;
			const bool d = a_allowed && a_raw;
			if (d && !a_key.down) {
				e.pressed = true;
				a_key.since = a_now;
			}
			if (!d && a_key.down) {
				e.released = true;
			}
			a_key.down = d;
			e.down = d;
			e.held = d ? (a_now - a_key.since) : 0.0;
			return e;
		}

		void SetG(RE::TESGlobal* a_g, float a_v)
		{
			if (a_g) {
				a_g->value = a_v;
			}
		}

		void Msg(const char* a_text)
		{
			if (Settings::Get().messages) {
				RE::SendHUDMessage::ShowHUDMessage(a_text);
			}
		}

		RE::Setting* GravitySetting()
		{
			static RE::Setting* s = nullptr;
			if (!s) {
				if (auto* c = RE::INISettingCollection::GetSingleton()) {
					s = c->GetSetting("fInAirFallingCharGravityMult:Havok");
				}
			}
			return s;
		}

		RE::Setting* JumpHeightSetting()
		{
			static RE::Setting* s = nullptr;
			if (!s) {
				if (auto* c = RE::GameSettingCollection::GetSingleton()) {
					s = c->GetSetting("fJumpHeightMin");
				}
			}
			return s;
		}

		void WriteGravity(float a_v)
		{
			if (auto* s = GravitySetting()) {
				s->data.f = a_v;
				R.appliedGravity = a_v;
			}
		}

		void WriteJump(float a_v)
		{
			if (auto* s = JumpHeightSetting()) {
				s->data.f = a_v;
				R.appliedJump = a_v;
			}
		}

		void SetTimeScale(float a_scale)
		{
			auto* t = RE::BSTimer::GetSingleton();
			if (!t) {
				return;
			}
			if (a_scale < 0.999f) {
				if (!R.timeScaled) {
					float prev = RE::BSTimer::QGlobalTimeMultiplierTarget();
					if (prev < 0.2f || prev > 1.0f) {
						prev = 1.0f;
					}
					R.prevTimeScale = prev;
					R.timeScaled = true;
				}
				t->SetGlobalTimeMultiplier(a_scale, true);
			} else if (R.timeScaled) {
				t->SetGlobalTimeMultiplier(R.prevTimeScale, true);
				R.timeScaled = false;
			}
		}

		void CastSelf(RE::PlayerCharacter* a_p, RE::SpellItem* a_spell)
		{
			if (!a_p || !a_spell) {
				return;
			}
			if (auto* caster = a_p->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
				caster->CastSpellImmediate(a_spell, false, a_p, 1.0f, false, 0.0f, a_p);
			}
		}

		void DispelSelf(RE::PlayerCharacter* a_p, RE::SpellItem* a_spell)
		{
			if (!a_p || !a_spell) {
				return;
			}
			if (auto* target = a_p->AsMagicTarget()) {
				auto handle = a_p->GetHandle();
				target->DispelEffect(a_spell, handle);
			}
		}

		bool HasEffect(RE::PlayerCharacter* a_p, RE::EffectSetting* a_e)
		{
			if (!a_p || !a_e) {
				return false;
			}
			auto* target = a_p->AsMagicTarget();
			return target && target->HasMagicEffect(a_e);
		}

		bool GraphBool(RE::PlayerCharacter* a_p, const char* a_name, bool a_default)
		{
			bool v = a_default;
			if (a_p->GetGraphVariableBool(a_name, v)) {
				return v;
			}
			return a_default;
		}

		void SetDriven(RE::PlayerCharacter* a_p, bool a_v)
		{
			a_p->SetGraphVariableBool("bAnimationDriven", a_v);
		}

		bool PlayerBusy(RE::PlayerCharacter* a_p)
		{
			if (a_p->IsOnMount() || a_p->IsInRagdollState() || a_p->IsInKillMove()) {
				return true;
			}
			// ATTENZIONE: con la libreria a piu' versioni del gioco il cast diretto (static_cast) a ActorState punta al posto sbagliato;
			// va usato l'accessore AsActorState(), che applica lo scostamento giusto per la versione in esecuzione.
			const auto* st = a_p->AsActorState();
			return st && st->IsSwimming();
		}

		bool IsSwimmingNow(RE::PlayerCharacter* a_p)
		{
			const auto* st = a_p->AsActorState();
			return st && st->IsSwimming();
		}

		// Variabili del grafo di animazione che le animazioni dei salti accendono e che si spengono
		// SOLO con un'animazione di atterraggio. In acqua l'atterraggio non c'e': restano accese e
		// (bAnimationDriven in particolare) il personaggio non risponde piu' ai comandi.
		// Restituisce quante variabili ha dovuto correggere.
		constexpr const char* kJumpVars[] = {
			"bAnimationDriven", "MagicalJumpingDashing", "bMJFallHeight1", "bMJFallHeight2", "bMJFastJump",
			"bMJStandingJump", "bMJVirtualJump", "bMJDirectionChange1", "bMJDirectionChange2", "bMJRollLand", "bMJGPMA"
		};

		// quali variabili dei salti sono accese adesso (solo per il log)
		std::string DescribeJumpVars(RE::PlayerCharacter* a_p)
		{
			std::string out;
			for (const auto* name : kJumpVars) {
				bool v = false;
				if (a_p->GetGraphVariableBool(name, v) && v) {
					if (!out.empty()) {
						out += ' ';
					}
					out += name;
				}
			}
			bool allowed = true;
			if (a_p->GetGraphVariableBool("bMJDashAllowed", allowed) && !allowed) {
				if (!out.empty()) {
					out += ' ';
				}
				out += "bMJDashAllowed=0";
			}
			return out.empty() ? std::string("nessuna") : out;
		}

		int CleanJumpGraphState(RE::PlayerCharacter* a_p)
		{
			int fixed = 0;
			for (const auto* name : kJumpVars) {
				bool v = false;
				if (a_p->GetGraphVariableBool(name, v) && v) {
					a_p->SetGraphVariableBool(name, false);
					++fixed;
				}
			}
			bool allowed = true;
			if (a_p->GetGraphVariableBool("bMJDashAllowed", allowed) && !allowed) {
				a_p->SetGraphVariableBool("bMJDashAllowed", true);
				++fixed;
			}
			return fixed;
		}

		// Riporta il personaggio in uno stato giocabile senza toccare i contatori di dash / slam / salti:
		// variabili di animazione spente, controlli riattivati, idle di reset (l'idle standard del gioco,
		// Idle 0xE6538 del gioco base). Restituisce quante variabili erano accese; a_idleOut = evento usato.
		int RecoverAnimState(RE::PlayerCharacter* a_p, const char** a_idleOut)
		{
			const int fixed = CleanJumpGraphState(a_p);
			SetDriven(a_p, false);

			if (auto* cm = RE::ControlMap::GetSingleton()) {
				using UE = RE::ControlMap::UEFlag;
				const auto flags = static_cast<UE>(
					static_cast<std::uint32_t>(UE::kMovement) | static_cast<std::uint32_t>(UE::kLooking) | static_cast<std::uint32_t>(UE::kActivate) |
					static_cast<std::uint32_t>(UE::kMenu) | static_cast<std::uint32_t>(UE::kPOVSwitch) | static_cast<std::uint32_t>(UE::kFighting) |
					static_cast<std::uint32_t>(UE::kSneaking) | static_cast<std::uint32_t>(UE::kMainFour));
				cm->ToggleControls(flags, true, false);
			}

			const char* ev = "IdleForceDefaultState";
			if (const auto* idle = RE::TESForm::LookupByID<RE::TESIdleForm>(0xE6538)) {
				if (idle->animEventName.size() > 0) {
					ev = idle->animEventName.c_str();
				}
			}
			a_p->NotifyAnimationGraph(ev);
			if (std::strcmp(ev, "IdleForceDefaultState") != 0) {
				a_p->NotifyAnimationGraph("IdleForceDefaultState");
			}
			if (a_idleOut) {
				*a_idleOut = ev;
			}
			return fixed;
		}

		bool InputAllowed()
		{
			if (Now() < g_menuUntil.load()) {
				return false;
			}
			if (const auto* ui = RE::UI::GetSingleton()) {
				if (const_cast<RE::UI*>(ui)->GameIsPaused()) {
					return false;
				}
			}
			return true;
		}

		// ----------------------------------------------------------------
		// ascolto degli eventi di animazione
		// ----------------------------------------------------------------
		class GraphSink final : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_evn, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override
			{
				if (!a_evn || !a_evn->holder) {
					return RE::BSEventNotifyControl::kContinue;
				}
				if (a_evn->holder != RE::PlayerCharacter::GetSingleton()) {
					return RE::BSEventNotifyControl::kContinue;
				}
				const char* tag = a_evn->tag.c_str();
				if (!tag) {
					return RE::BSEventNotifyControl::kContinue;
				}
				std::optional<Ev> ev;
				if (std::strcmp(tag, "JumpUp") == 0) {
					ev = Ev::JumpUp;
				} else if (std::strcmp(tag, "MJSoftLanding") == 0) {
					ev = Ev::SoftLanding;
				} else if (std::strcmp(tag, "MJFastLanding") == 0) {
					ev = Ev::FastLanding;
				} else if (std::strcmp(tag, "MJPowerAttackLanding") == 0) {
					ev = Ev::PowerAttackLanding;
				} else if (std::strcmp(tag, "Parachute") == 0) {
					ev = Ev::Parachute;
				} else if (std::strcmp(tag, "Collision_AttackEnd") == 0) {
					ev = Ev::AttackEnd;
				} else if (std::strcmp(tag, "PIE.$MJ_InAirCast") == 0 ||
						   (std::strcmp(tag, "PIE") == 0 && a_evn->payload.c_str() && std::strcmp(a_evn->payload.c_str(), "$MJ_InAirCast") == 0)) {
					ev = Ev::AirCast;
				} else if (std::strcmp(tag, "PIE.$MJPowerAttackLanding") == 0) {
					ev = Ev::PowerAttackLanding;
				} else if (std::strcmp(tag, "SoundPlay.ParaglideEquip") == 0) {
					ev = Ev::ParaStart;
				} else if (std::strcmp(tag, "SoundPlay.ParaglideUnequip") == 0 || std::strcmp(tag, "Para_UnequipOut") == 0 ||
						   std::strcmp(tag, "Para_Unequip_Air_Out") == 0) {
					ev = Ev::ParaEnd;
				}
				if (ev) {
					std::lock_guard lk(g_evMutex);
					if (g_events.size() < 64) {
						g_events.push_back(*ev);
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		GraphSink g_sink;

		void RegisterSink(RE::PlayerCharacter* a_p)
		{
			RE::BSAnimationGraphManagerPtr mgr;
			a_p->GetAnimationGraphManager(mgr);
			if (!mgr) {
				return;
			}
			for (auto& graph : mgr->graphs) {
				if (!graph) {
					continue;
				}
				auto* src = graph->GetEventSource<RE::BSAnimationGraphEvent>();
				if (!src) {
					continue;
				}
				// AddEventSink controlla da solo i doppioni (con il blocco dell'elenco): non scorro io l'elenco, potrebbe cambiare nel frattempo
				src->AddEventSink(&g_sink);
			}
		}

		// ----------------------------------------------------------------
		// gate dei salti: file di configurazione
		// ----------------------------------------------------------------
		constexpr auto kBJPath = "Data\\SKSE\\Plugins\\BetterJumpingSE.txt";

		struct BJValueLines
		{
			std::vector<std::string> lines;
			int                      perkLine = -1;
		};

		BJValueLines ReadBJ()
		{
			BJValueLines out;
			std::ifstream in(kBJPath);
			if (!in.is_open()) {
				return out;
			}
			std::string line;
			int         valueIndex = 0;
			while (std::getline(in, line)) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				const auto first = line.find_first_not_of(" \t");
				if (first != std::string::npos && line[first] != ';') {
					if (valueIndex == 1) {
						out.perkLine = static_cast<int>(out.lines.size());
					}
					++valueIndex;
				}
				out.lines.push_back(line);
			}
			return out;
		}

		void CheckBJ()
		{
			S.bjFileFound = false;
			S.bjMatch = false;
			S.bjPerkInFile = 0;
			if (!F.perk) {
				return;
			}
			S.bjPerkExpected = F.perk->GetFormID();
			const auto bj = ReadBJ();
			if (bj.perkLine < 0) {
				return;
			}
			S.bjFileFound = true;
			S.bjPerkInFile = static_cast<std::uint32_t>(std::strtoul(bj.lines[static_cast<std::size_t>(bj.perkLine)].c_str(), nullptr, 16));
			S.bjMatch = (S.bjPerkInFile == S.bjPerkExpected);
		}

		// ----------------------------------------------------------------
		// applica / rimuovi la mod
		// ----------------------------------------------------------------
		void ApplyBaselines()
		{
			const auto& s = Settings::Get();
			if (!R.applied) {
				return;
			}
			float g = R.gravityOverride ? R.gravityValue : s.gravity;
			if (R.glide && s.paraCompat && s.paraVanillaGravity && R.vanillaCaptured && !R.gravityOverride) {
				g = R.vanillaGravity;  // in planata il comportamento della mod di planata e' tarato sulla gravita' di Skyrim
			}
			if (std::fabs(g - R.appliedGravity) > 0.0001f) {
				WriteGravity(g);
			}
			const float j = R.sjArmed ? s.superJumpHeight : s.jumpHeight;
			if (std::fabs(j - R.appliedJump) > 0.0001f) {
				WriteJump(j);
			}
		}

		void RefreshGlobals()
		{
			const auto& s = Settings::Get();
			SetG(F.gEnabled, (R.glideSuspended || R.witchSuspended) ? 0.0f : 1.0f);
			SetG(F.gWitch, R.wmState != 0 ? 1.0f : 0.0f);
			SetG(F.gOne, 1.0f);
			SetG(F.gAnimLanding, s.animatedLanding ? 1.0f : 0.0f);
			SetG(F.gDirOption, s.directionChange ? 1.0f : 0.0f);
			SetG(F.gDashLen, 3000.0f * static_cast<float>(std::clamp(s.dashDistance, 1, 3)));
		}

		void Maintain(RE::PlayerCharacter* a_p)
		{
			const auto& s = Settings::Get();
			if (F.perk && !R.jumpGateClosed && !R.glideSuspended && !R.witchSuspended && !a_p->HasPerk(F.perk)) {
				a_p->AddPerk(F.perk);
			}
			if (F.perkAnim && !R.glideSuspended && !R.witchSuspended && !a_p->HasPerk(F.perkAnim)) {
				a_p->AddPerk(F.perkAnim);
			}
			if (F.spWitchFx && R.wmState == 0 && a_p->HasSpell(F.spWitchFx)) {
				a_p->RemoveSpell(F.spWitchFx);  // rimasta da un salvataggio fatto durante la strega volante
			}
			if (F.ability && !a_p->HasSpell(F.ability) && Now() >= R.abilityHoldUntil) {
				a_p->AddSpell(F.ability);
			}
			if (F.perkSoft) {
				const bool has = a_p->HasPerk(F.perkSoft);
				if (s.noFallDamage && !has) {
					a_p->AddPerk(F.perkSoft);
				} else if (!s.noFallDamage && has) {
					a_p->RemovePerk(F.perkSoft);
				}
			}
			RefreshGlobals();
		}

		void ResetActions(RE::PlayerCharacter* a_p);

		void Apply(RE::PlayerCharacter* a_p)
		{
			R.applied = true;
			R.appliedGravity = -1.0f;
			R.appliedJump = -1.0f;
			SetG(F.gInAir, 0.0f);
			SetG(F.gSlam, 0.0f);
			Maintain(a_p);
			ApplyBaselines();
			logger::info("mod attivata");
		}

		void Unapply(RE::PlayerCharacter* a_p)
		{
			ResetActions(a_p);
			R.applied = false;
			R.glide = false;
			R.glideEvOn = false;
			R.glideSuspended = false;
			R.glideRemovedPerk = false;
			R.glideRemovedAnim = false;
			R.witchSuspended = false;
			if (F.ability) {
				a_p->RemoveSpell(F.ability);
			}
			if (F.perk) {
				a_p->RemovePerk(F.perk);
			}
			if (F.perkAnim) {
				a_p->RemovePerk(F.perkAnim);
			}
			if (F.perkSoft) {
				a_p->RemovePerk(F.perkSoft);
			}
			R.jumpGateClosed = false;
			R.jumpCount = 0;
			SetG(F.gEnabled, 0.0f);
			SetG(F.gDirChanged, 0.0f);
			SetG(F.gInAir, 0.0f);
			SetG(F.gSlam, 0.0f);
			ApplyFx(false);
			if (R.vanillaCaptured) {
				WriteGravity(R.vanillaGravity);
				WriteJump(R.vanillaJump);
			}
			logger::info("mod disattivata");
		}

		// ----------------------------------------------------------------
		// limitatore di velocita' dopo il dash
		// Dopo un dash a volte il controller del personaggio si porta dietro la velocita' del movimento
		// incorporato nell'animazione e il personaggio parte a velocita' enorme (difetto noto di Skyrim).
		// Per qualche secondo dopo la fine del dash la velocita' orizzontale viene limitata.
		// ----------------------------------------------------------------
		void ArmLandingFix(double a_now);  // definita piu' avanti (strascico del dash)

		void ArmSpeedGuard(double a_now, double a_seconds)
		{
			if (!Settings::Get().dashSpeedLimit) {
				return;
			}
			if (R.guardUntil <= a_now) {
				R.guardLogs = 0;
			}
			R.guardUntil = std::max(R.guardUntil, a_now + a_seconds);
			R.guardAfterDash = true;
		}

		void LogCtrlSpeed(RE::PlayerCharacter* a_p, const char* a_label)
		{
			auto* ctrl = a_p->GetCharController();
			if (!ctrl) {
				return;
			}
			RE::hkVector4 v;
			ctrl->GetLinearVelocityImpl(v);
			const auto* c = reinterpret_cast<const float*>(&v.quad);
			logger::info("{}: velocita' orizzontale {:.1f} m/s, verticale {:.1f} m/s", a_label, std::hypot(c[0], c[1]), c[2]);
		}

		// Per qualche secondo dopo la fine del dash sorveglio il personaggio a ogni frame:
		//  1. misuro lo SPOSTAMENTO REALE (posizione di adesso meno quella del frame prima): e' quello che
		//     si vede, e non dipende da quale campo del controller porta l'energia in piu';
		//  2. se in un frame si e' spostato come a una velocita' assurda azzero tutta la velocita' (anche
		//     velocityMod e pushDelta del controller). La posizione NON la tocco: rimetterlo indietro non serviva a niente;
		//  3. in ogni caso limito la velocita' orizzontale del controller al massimo impostato.
		// true se in questo frame ha corretto qualcosa
		bool UpdateSpeedGuard(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			if (R.guardUntil <= a_now) {
				R.guardAfterDash = false;
				R.gdValid = false;
				return false;
			}
			if (!s.dashSpeedLimit || R.dashActive || R.slamActive || PlayerBusy(a_p)) {
				R.gdValid = false;
				return false;
			}
			auto* ctrl = a_p->GetCharController();
			if (!ctrl) {
				R.gdValid = false;
				return false;
			}

			// 1. spostamento reale dall'ultimo frame
			const float px = a_p->GetPositionX();
			const float py = a_p->GetPositionY();
			const void* cell = a_p->GetParentCell();
			float       disp = 0.0f;
			bool        jumped = false;
			if (R.gdValid && cell == R.gdCell) {
				const double dt = a_now - R.gdAt;
				const float  rawDist = std::hypot(px - R.gdX, py - R.gdY);
				if (rawDist > kTeleportUnits) {
					R.gdValid = false;  // e' un teletrasporto, non uno scatto: niente correzione di posizione
				} else if (dt >= 0.004 && dt <= 0.5) {
					disp = rawDist / static_cast<float>(dt) / kHavokScale;
					jumped = !std::isfinite(disp) || disp > std::max(kGuardDispMin, s.dashExitMaxSpeed * 2.0f);
				}
			}

			// 2. velocita' del controller
			RE::hkVector4 v;
			ctrl->GetLinearVelocityImpl(v);
			auto*       comp = reinterpret_cast<float*>(&v.quad);
			const float cap = std::max(s.dashExitMaxSpeed, 1.0f);
			const float h = std::hypot(comp[0], comp[1]);
			const float vz = comp[2];
			const char* action = "nessuna";
			bool        changed = false;
			if (!std::isfinite(h) || !std::isfinite(vz)) {
				comp[0] = 0.0f;
				comp[1] = 0.0f;
				if (!std::isfinite(vz)) {
					comp[2] = 0.0f;
				}
				changed = true;
				action = "velocita' non valida azzerata";
			} else {
				if (h > cap) {
					const float k = cap / h;
					comp[0] *= k;
					comp[1] *= k;
					changed = true;
					action = "velocita' limitata";
				}
				if (vz > kGuardMaxUp) {
					comp[2] = kGuardMaxUp;
					changed = true;
				}
			}

			const bool spike = jumped || changed;
			if (spike && R.guardLogs < 6) {
				++R.guardLogs;
				float speedVar = -1.0f;
				a_p->GetGraphVariableFloat("Speed", speedVar);
				const auto* mod = reinterpret_cast<const float*>(&ctrl->velocityMod.quad);
				const auto* push = reinterpret_cast<const float*>(&ctrl->pushDelta.quad);
				logger::info("scatto dopo il dash: spostamento reale {:.1f} m/s, velocita' del controller {:.1f} m/s (verticale {:.1f}), Speed del grafo {:.0f}, "
							 "velocityMod {:.2f}/{:.2f}, pushDelta {:.2f}/{:.2f}, in aria: {}, dopo {:.1f} s dall'ultimo dash, azione: {}",
					disp, h, vz, speedVar, mod[0], mod[1], push[0], push[1], R.airborne ? "si" : "no", a_now - R.dashStart, jumped ? "fermato" : action);
			}
			if (jumped) {
				// il personaggio e' stato lanciato: tolgo ogni velocita' (anche i campi "spinta" del controller)
				comp[0] = 0.0f;
				comp[1] = 0.0f;
				if (comp[2] > 0.0f) {
					comp[2] = 0.0f;
				}
				auto* mod = reinterpret_cast<float*>(&ctrl->velocityMod.quad);
				auto* push = reinterpret_cast<float*>(&ctrl->pushDelta.quad);
				mod[0] = mod[1] = mod[2] = 0.0f;
				push[0] = push[1] = push[2] = 0.0f;
				changed = true;
			}
			if (changed) {
				ctrl->SetLinearVelocityImpl(v);
				R.acValid = true;  // per il controllo del movimento in aria questa e' "la mia" velocita'
				R.acX = comp[0];
				R.acY = comp[1];
			}

			// posizione di riferimento per il frame dopo
			R.gdValid = true;
			R.gdAt = a_now;
			R.gdCell = cell;
			R.gdX = px;
			R.gdY = py;
			return changed;
		}

		// ----------------------------------------------------------------
		// dash
		// ----------------------------------------------------------------
		void EndDash(RE::PlayerCharacter* a_p, double a_now, const char* a_why)
		{
			if (!R.dashActive) {
				return;
			}
			logger::info("dash finito: {} (dopo {:.2f} s)", a_why, a_now - R.dashStart);
			LogCtrlSpeed(a_p, "dash finito");
			R.dashEndWhy = a_why;
			R.dashEndAt = a_now;
			ArmLandingFix(a_now);
			R.dashActive = false;
			R.dashEventSent = false;
			R.gravityOverride = false;
			R.dashCooldownUntil = a_now + kDashCooldown;
			CastSelf(a_p, F.spDashEnd);
			SetDriven(a_p, false);
			SetG(F.gInAir, 0.0f);
			ApplyBaselines();
			ArmSpeedGuard(a_now, kGuardAfterDash);
			UpdateSpeedGuard(a_p, a_now);
			if (IsSwimmingNow(a_p)) {
				// finito in acqua: niente atterraggio, quindi le variabili dei salti vanno spente a mano
				const int fixed = CleanJumpGraphState(a_p);
				DispelSelf(a_p, F.spDash);
				logger::info("dash finito in acqua: ripulite {} variabili di animazione", fixed);
			}
		}

		// Suono del dash: i 5 suoni girano uno dopo l'altro (o a caso, mai lo stesso due volte di fila)
		void PlayDashSound(RE::PlayerCharacter* a_p)
		{
			const auto& s = Settings::Get();
			if (!s.dashSound) {
				return;
			}
			int avail[kSoundDashCount];
			int n = 0;
			for (int i = 0; i < kSoundDashCount; ++i) {
				if (F.sndDash[static_cast<std::size_t>(i)]) {
					avail[n++] = i;
				}
			}
			if (n == 0) {
				return;
			}
			int pick = avail[0];
			if (s.dashSoundRandom && n > 1) {
				static std::mt19937 rng{ static_cast<std::mt19937::result_type>(std::chrono::steady_clock::now().time_since_epoch().count()) };
				std::uniform_int_distribution<int> dist(0, n - 1);
				do {
					pick = avail[dist(rng)];
				} while (pick == R.dashSoundLast);
			} else {
				pick = avail[R.dashSoundNext % n];
				R.dashSoundNext = (R.dashSoundNext + 1) % n;
			}
			R.dashSoundLast = pick;

			auto* am = RE::BSAudioManager::GetSingleton();
			auto* snd = F.sndDash[static_cast<std::size_t>(pick)];
			if (!am || !snd) {
				return;
			}
			auto* desc = static_cast<RE::BSISoundDescriptor*>(snd);
			RE::BSSoundHandle handle;
			if (am->GetSoundHandle(handle, desc) && handle.IsValid()) {
				handle.SetPosition(a_p->GetPositionX(), a_p->GetPositionY(), a_p->GetPositionZ());
				if (auto* node = a_p->Get3D()) {
					handle.SetObjectToFollow(node);
				}
				handle.Play();
			} else {
				am->Play(desc);
			}
		}

		void StartDash(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			R.dashActive = true;
			R.dashUsed = true;
			R.dashBufferUntil = 0.0;
			R.dashWhy = nullptr;
			R.dashEventSent = false;
			R.dashStart = a_now;

			SetG(F.gInAir, 1.0f);
			SetG(F.gDashLen, 3000.0f * static_cast<float>(std::clamp(s.dashDistance, 1, 3)));

			// gravita' durante il dash 
			float vz = 0.0f;
			a_p->GetGraphVariableFloat("VelocityZ", vz);
			vz = std::clamp(vz, -250.0f, 0.0f);
			R.gravityValue = -0.35f + vz / 250.0f;
			R.gravityOverride = true;
			ApplyBaselines();

			CastSelf(a_p, F.spDash);
			PlayDashSound(a_p);
		}

		void UpdateDash(RE::PlayerCharacter* a_p, double a_now, const KeyEvt& a_key, bool a_allowed)
		{
			const auto& s = Settings::Get();
			if (!R.dashActive) {
				if (a_key.pressed && a_allowed && s.dashEnabled) {
					if (R.airborne || R.airTicks > 0) {
						R.dashBufferUntil = a_now + kDashBuffer;
						R.dashWhy = nullptr;
					} else {
						logger::info("dash: premuto a terra, ignorato");
					}
				}
				if (R.dashBufferUntil <= 0.0) {
					return;
				}
				if (a_now > R.dashBufferUntil || !a_allowed || !s.dashEnabled || !F.spDash) {
					if (R.dashWhy) {
						logger::info("dash non partito: {}", R.dashWhy);
					}
					R.dashBufferUntil = 0.0;
					R.dashWhy = nullptr;
					return;
				}
				if (R.dashUsed) {
					logger::info("dash non partito: gia' usato in questo volo");
					R.dashBufferUntil = 0.0;
					R.dashWhy = nullptr;
					return;
				}
				if (!R.airborne) {
					R.dashWhy = "non ancora in aria";
					return;
				}
				if (R.slamActive) {
					R.dashWhy = "slam in corso";
					return;
				}
				if (a_now < R.dashCooldownUntil) {
					R.dashWhy = "attesa tra due dash";
					return;
				}
				if (PlayerBusy(a_p)) {
					R.dashWhy = "personaggio occupato (cavalcatura, nuoto, ragdoll)";
					return;
				}
				if (!GraphBool(a_p, "bMJDashAllowed", true)) {
					R.dashWhy = "l'animazione corrente non lo permette (bMJDashAllowed)";
					return;
				}
				StartDash(a_p, a_now);
				return;
			}

			// dash in corso: prima controllo se deve finire (acqua, atterraggio...), poi il resto
			const char* endWhy = nullptr;
			if (PlayerBusy(a_p)) {
				endWhy = IsSwimmingNow(a_p) ? "in acqua" : "personaggio occupato";
			} else if (!R.airborne) {
				endWhy = "atterrato";
			} else if ((a_now - R.dashStart) >= kDashDuration) {
				endWhy = "durata completata";
			}
			if (endWhy) {
				EndDash(a_p, a_now, endWhy);
				return;
			}
			if (!R.dashEventSent && (a_now - R.dashStart) >= kDashEventDelay) {
				R.dashEventSent = true;
				SetDriven(a_p, true);
				const bool fast = GraphBool(a_p, "bMJFastJump", false);
				a_p->NotifyAnimationGraph(fast ? "JumpStandingStart" : "JumpDirectionalStart");
			}
		}

		// ----------------------------------------------------------------
		// slam
		// ----------------------------------------------------------------
		void EndSlam(RE::PlayerCharacter* a_p, double a_now)
		{
			if (!R.slamActive) {
				return;
			}
			R.slamActive = false;
			R.slamPhase = 0;
			R.gravityOverride = false;
			SetTimeScale(1.0f);
			SetDriven(a_p, false);
			SetG(F.gSlam, 0.0f);
			if (R.slamCastWindow) {
				DispelSelf(a_p, F.spJump1);
				R.slamCastWindow = false;
			}
			R.slamCooldownUntil = a_now + 0.4;
			ArmLandingFix(a_now);  // dopo uno slam l'atterraggio puo' lasciare lo stesso strascico di velocita'
			ApplyBaselines();
		}

		void StartSlam(RE::PlayerCharacter* a_p, double a_now)
		{
			R.slamActive = true;
			R.slamPhase = 1;
			R.slamStart = a_now;
			R.slamLanded = false;
			R.slamCastWindow = false;

			// l'animazione dello slam richiede una "finestra di salto" attiva
			if (!HasEffect(a_p, F.mgJump1) && !HasEffect(a_p, F.mgJump2) && !HasEffect(a_p, F.mgJump3)) {
				CastSelf(a_p, F.spJump1);
				R.slamCastWindow = true;
			}

			SetG(F.gSlam, 1.0f);
			SetTimeScale(kSlowTime);

			bool firstPerson = false;
			if (const auto* cam = RE::PlayerCamera::GetSingleton()) {
				firstPerson = cam->IsInFirstPerson();
			}
			R.slamWindup = firstPerson ? kSlamWindupFirst : kSlamWindupThird;
			SetDriven(a_p, !firstPerson && !Settings::Get().slamFreeMovement);
			a_p->NotifyAnimationGraph("JumpFall");
		}

		// La magia "Tempesta di Fuoco" (Destruction, maestro): lancio su se stessi, area attorno al lanciatore.
		// Controllo la forma della magia (fuoco + area + su se stessi) cosi' un FormID sbagliato non lancia
		// altro; se non e' lei provo a cercarla per nome.
		bool LooksLikeFireStorm(const RE::SpellItem* a_sp)
		{
			if (!a_sp || a_sp->effects.empty() || a_sp->GetDelivery() != RE::MagicSystem::Delivery::kSelf) {
				return false;
			}
			for (const auto* e : a_sp->effects) {
				if (e && e->baseEffect && e->baseEffect->data.resistVariable == RE::ActorValue::kResistFire && e->GetArea() > 0) {
					return true;
				}
			}
			return false;
		}

		bool NameLooksLikeFireStorm(const RE::SpellItem* a_sp)
		{
			if (!a_sp || !a_sp->GetName()) {
				return false;
			}
			std::string n = a_sp->GetName();
			std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return n == "fire storm" || n == "tempesta di fuoco";
		}

		RE::SpellItem* ResolveFireStorm(RE::TESDataHandler* a_dh)
		{
			const auto& s = Settings::Get();
			const auto  id = static_cast<RE::FormID>(s.fireStormFormId > 0 ? s.fireStormFormId : kFireStormId);
			auto*       sp = RE::TESForm::LookupByID<RE::SpellItem>(id);
			if (sp) {
				const bool ok = LooksLikeFireStorm(sp) || NameLooksLikeFireStorm(sp);
				logger::info("Tempesta di Fuoco: form {:08X} '{}' ({})", id, sp->GetName() ? sp->GetName() : "", ok ? "ok" : "non sembra la magia giusta");
				if (ok) {
					return sp;
				}
			} else {
				logger::warn("Tempesta di Fuoco: nessuna magia con FormID {:08X}", id);
			}
			if (a_dh) {
				for (auto* cand : a_dh->GetFormArray<RE::SpellItem>()) {
					if (cand && NameLooksLikeFireStorm(cand) && cand->GetDelivery() == RE::MagicSystem::Delivery::kSelf) {
						logger::info("Tempesta di Fuoco: trovata per nome, form {:08X} '{}'", cand->GetFormID(), cand->GetName());
						return cand;
					}
				}
			}
			logger::warn("Tempesta di Fuoco: non trovata, lo slam usa solo l'onda d'urto (se serve cambia FireStormFormID nell'ini)");
			return nullptr;
		}

		void SlamLanded(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			R.slamLanded = true;
			R.slamPhase = 3;
			R.slamEndAt = a_now + 0.6;
			R.gravityOverride = false;
			SetTimeScale(1.0f);
			ApplyBaselines();

			// Tempesta di Fuoco al posto dell'onda d'urto, con attesa tra una e l'altra
			bool fire = false;
			if (s.slamFireStorm && F.spFireStorm) {
				const double ready = g_fireReadyAt.load();
				if (a_now >= ready) {
					CastSelf(a_p, F.spFireStorm);
					g_fireReadyAt.store(a_now + static_cast<double>(s.fireStormCooldown));
					fire = true;
					Msg("Tempesta di Fuoco!");
					logger::info("slam: Tempesta di Fuoco lanciata (la prossima tra {:.0f} s)", s.fireStormCooldown);
				} else {
					const int left = static_cast<int>(std::ceil(ready - a_now));
					logger::info("slam: Tempesta di Fuoco in attesa, mancano {} s", left);
					const auto txt = std::format("Tempesta di Fuoco pronta tra {}:{:02}", left / 60, left % 60);
					Msg(txt.c_str());
				}
			}

			// onda d'urto + stordimento ad area (creati da codice)
			const bool shock = !fire && (!s.slamFireStorm || !F.spFireStorm || s.slamShockwaveWhileWaiting);
			if (shock) {
				if (F.explPower) {
					a_p->PlaceObjectAtMe(F.explPower, false);
				}
				if (F.explSlam && s.fxSlam) {
					a_p->PlaceObjectAtMe(F.explSlam, false);  // anello d'onda: solo visivo, si puo' spegnere
				}
			}
		}

		void UpdateSlam(RE::PlayerCharacter* a_p, double a_now, const KeyEvt& a_key, bool a_allowed)
		{
			const auto& s = Settings::Get();
			if (!a_key.down) {
				R.slamConsumed = false;
			}
			if (!R.slamActive) {
				if (!a_allowed || !s.slamEnabled || !a_key.down || R.slamConsumed) {
					return;
				}
				if (a_key.held < s.slamHold) {
					return;
				}
				if (!R.airborne || R.dashActive || a_now < R.slamCooldownUntil || PlayerBusy(a_p)) {
					return;
				}
				R.slamConsumed = true;
				StartSlam(a_p, a_now);
				return;
			}

			if (PlayerBusy(a_p)) {
				EndSlam(a_p, a_now);
				return;
			}

			switch (R.slamPhase) {
			case 1:  // carica al rallentatore
				if (!R.airborne && (a_now - R.slamStart) > 0.1) {
					SlamLanded(a_p, a_now);  // sei gia' atterrato durante la carica
				} else if ((a_now - R.slamStart) >= R.slamWindup) {
					SetTimeScale(1.0f);
					if (a_key.down) {
						R.gravityValue = kSlamGravity;
						R.gravityOverride = true;
						ApplyBaselines();
						R.slamPhase = 2;
					} else {
						EndSlam(a_p, a_now);
					}
				} else if (!a_key.down && (a_now - R.slamStart) > 0.05) {
					// tasto rilasciato durante la carica: annulla
					EndSlam(a_p, a_now);
				}
				break;
			case 2:  // caduta
				if (!R.airborne) {
					SlamLanded(a_p, a_now);
				} else if (!a_key.down && (a_now - R.slamStart) > (R.slamWindup + 0.3)) {
					EndSlam(a_p, a_now);
				} else if ((a_now - R.slamStart) > 10.0) {
					EndSlam(a_p, a_now);
				}
				break;
			case 3:  // atterrato
				if (a_now >= R.slamEndAt) {
					EndSlam(a_p, a_now);
				}
				break;
			default:
				EndSlam(a_p, a_now);
				break;
			}
		}

		// ----------------------------------------------------------------
		// super jump
		// ----------------------------------------------------------------
		void DisarmSuperJump()
		{
			R.sjArmed = false;
			R.sjResetAt = -1.0;
			ApplyBaselines();
		}

		void UpdateSuperJump(double a_now, const KeyEvt& a_key, bool a_allowed)
		{
			const auto& s = Settings::Get();
			if (!a_key.down) {
				R.sjFired = false;
			}
			if (a_allowed && s.superJumpEnabled && a_key.down && !R.sjFired && a_key.held >= s.superJumpHold) {
				R.sjFired = true;
				R.sjArmed = true;
				R.sjArmedAt = a_now;
				R.sjResetAt = -1.0;
				ApplyBaselines();
				Msg("Super salto pronto");
			}
			if (R.sjArmed) {
				if (R.sjResetAt > 0.0 && a_now >= R.sjResetAt) {
					DisarmSuperJump();
				} else if ((a_now - R.sjArmedAt) > kSuperJumpWindow && R.sjResetAt < 0.0) {
					DisarmSuperJump();
				}
			}
			if (!s.superJumpEnabled && R.sjArmed) {
				DisarmSuperJump();
			}
		}

		// ----------------------------------------------------------------
		// direzione (cambio direzione tra un salto e l'altro)
		// ----------------------------------------------------------------
		float WrapAngle(float a)
		{
			constexpr float kPi = 3.14159265358979f;
			while (a > kPi) {
				a -= 2.0f * kPi;
			}
			while (a < -kPi) {
				a += 2.0f * kPi;
			}
			return a;
		}

		// Camera in terza persona con rotazione libera (arma rinfoderata), altrimenti nullptr.
		// In questa modalita' la direzione della camera e' angolo del personaggio + freeRotation.x:
		// se giro il personaggio senza compensare, la camera gira con lui.
		// Con l'arma sfoderata la camera E' la direzione del personaggio e non si puo' girare l'uno senza l'altra.
		RE::ThirdPersonState* FreeThirdPersonCamera()
		{
			auto* cam = RE::PlayerCamera::GetSingleton();
			if (!cam || !cam->IsCurrentState(RE::CameraState::kThirdPerson, RE::CameraState::kVRThirdPerson)) {
				return nullptr;
			}
			auto* tps = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
			return (tps && tps->freeRotationEnabled) ? tps : nullptr;
		}

		// Gira il modello del personaggio verso a_ang (0 = nord, + = verso est: stessa convenzione
		// della direzione del movimento sin(a), cos(a)) SENZA muovere la camera: la rotazione libera
		// della camera viene compensata dello stesso angolo (come fanno i sistemi di movimento direzionale).
		void FaceHeading(RE::PlayerCharacter* a_p, float a_ang, RE::ThirdPersonState* a_cam)
		{
			constexpr float kTwoPi = 6.28318530717959f;
			float           a = std::fmod(a_ang, kTwoPi);
			if (a < 0.0f) {
				a += kTwoPi;
			}
			const float before = a_p->GetAngleZ();
			a_p->SetHeading(a);
			if (a_cam) {
				const float delta = WrapAngle(a - before);
				a_cam->freeRotation.x = WrapAngle(a_cam->freeRotation.x - delta);
			}
		}

		// Stick di movimento. In terza persona Skyrim trasforma lo stick in "avanti + rotazione del
		// personaggio": PlayerControls::moveInputVec ha quasi sempre x = 0 e non dice mai destra/sinistra.
		// Come facevano gli script di base, leggo quindi lo stick sinistro direttamente dal
		// controller (XInput); senza controller, o con lo stick fermo, uso il valore di Skyrim.
		void ReadStick()
		{
			const auto& s = Settings::Get();
			float       x = 0.0f;
			float       y = 0.0f;
			bool        fromPad = false;
			if (s.padEnabled && Pad::GameFocused()) {
				float px = 0.0f;
				float py = 0.0f;
				if (Pad::LeftStick(px, py) && std::hypot(px, py) > 0.0f) {
					x = px;
					y = py;
					fromPad = true;
				}
			}
			if (!fromPad) {
				if (const auto* pc = RE::PlayerControls::GetSingleton()) {
					x = pc->data.moveInputVec.x;
					y = pc->data.moveInputVec.y;
				}
			}
			R.stickX = x;
			R.stickY = y;
			R.stickFromPad = fromPad;
		}

		// la direzione dello stick adesso e' diversa da quella del salto precedente?
		bool DirectionChangedNow()
		{
			if (!Settings::Get().directionChange) {
				return false;
			}
			if (std::hypot(R.stickX, R.stickY) <= 0.5f) {
				return false;
			}
			const float ang = std::atan2(R.stickX, R.stickY);
			return !R.prevValid || std::fabs(WrapAngle(ang - R.prevAng)) > 0.6f;
		}

		void UpdateDirection(double a_now)
		{
			const bool  valid = std::hypot(R.stickX, R.stickY) > 0.5f;
			const float ang = std::atan2(R.stickX, R.stickY);
			R.curValid = valid;
			R.curAng = ang;
			if (R.prevPending && a_now >= R.prevAt) {
				R.prevValid = valid;
				R.prevAng = ang;
				R.prevPending = false;
			}
			const bool changed = DirectionChangedNow();
			R.dirChanged = changed;
			SetG(F.gDirChanged, changed ? 1.0f : 0.0f);
		}

		// Legge lo stick e misura (a terra) quanto la direzione reale di movimento e' ruotata rispetto
		// allo stick: e' la direzione della camera, qualunque sia la convenzione degli assi di Skyrim.
		void UpdateHeadingCal(RE::PlayerCharacter* a_p, double a_now)
		{
			ReadStick();
			const float px = a_p->GetPositionX();
			const float py = a_p->GetPositionY();
			const bool  had = R.posValid;
			const float ox = R.posX;
			const float oy = R.posY;
			const double dt = a_now - R.posAt;
			R.posX = px;
			R.posY = py;
			R.posAt = a_now;
			R.posValid = true;

			const float mag = std::hypot(R.stickX, R.stickY);
			const float stickAng = std::atan2(R.stickX, R.stickY);
			const bool  steady = mag > 0.5f && std::fabs(WrapAngle(stickAng - R.lastStickAng)) < 0.08f;
			R.lastStickAng = stickAng;
			if (!steady) {
				R.stickStableSince = a_now;
			}

			if (!had || dt <= 0.0 || dt > 0.25) {
				return;
			}
			if (!steady || (a_now - R.stickStableSince) < 0.35) {
				return;  // aspetto che il personaggio abbia finito di girarsi verso lo stick
			}
			if (R.airborne || R.dashActive || R.slamActive || PlayerBusy(a_p) || a_p->IsInMidair()) {
				return;
			}
			const float dx = px - ox;
			const float dy = py - oy;
			const float spd = std::hypot(dx, dy) / static_cast<float>(dt);
			if (!(spd > 90.0f && spd < 2000.0f)) {
				return;  // fermo, o teletrasporto / caricamento
			}
			const float off = WrapAngle(std::atan2(dx, dy) - stickAng);
			if (!R.camValid) {
				R.camOff = off;
				R.camValid = true;
				logger::info("cambio direzione: camera misurata {:.0f} gradi (stick da {})", off * 57.29578f, R.stickFromPad ? "controller" : "gioco");
			} else {
				const float d = WrapAngle(off - R.camOff);
				R.camOff = (std::fabs(d) > 0.8f) ? off : WrapAngle(R.camOff + d * 0.35f);
			}
		}

		// Salto in aria con la direzione dello stick cambiata: porto la velocita' orizzontale del
		// personaggio nella nuova direzione (di norma lo fa il movimento incorporato
		// nelle animazioni, qui lo faccio direttamente sul controller del personaggio).
		void StartRedirect(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			const float mag = std::hypot(R.stickX, R.stickY);
			const bool  changed = DirectionChangedNow();
			logger::info("salto in aria n.{}: stick ({:.2f}, {:.2f}) da {}, cambio direzione {}, camera {}", R.jumpCount, R.stickX, R.stickY,
				R.stickFromPad ? "controller" : "gioco", changed ? "si" : "no", R.camValid ? "misurata" : "NON misurata");
			if (!changed || mag < 0.5f || !R.camValid || PlayerBusy(a_p)) {
				return;
			}
			auto* ctrl = a_p->GetCharController();
			if (!ctrl) {
				return;
			}
			RE::hkVector4 v;
			ctrl->GetLinearVelocityImpl(v);
			const auto* comp = reinterpret_cast<const float*>(&v.quad);
			float       cur = std::hypot(comp[0], comp[1]);
			if (!std::isfinite(cur)) {
				cur = 0.0f;
			}
			const float f = std::clamp(s.airControl / 100.0f, 0.0f, 2.0f);
			const float speed = std::clamp(std::max(cur, s.directionMinSpeed), 0.0f, 12.0f) * (f > 0.0f ? f : 1.0f);
			const float ang = WrapAngle(std::atan2(R.stickX, R.stickY) + R.camOff);
			// direzione di partenza = quella in cui il personaggio si sta muovendo adesso (se e' quasi fermo parte subito da quella nuova)
			const float from = (cur > 0.5f) ? std::atan2(comp[0], comp[1]) : ang;
			const float w = std::clamp(s.directionWeight / 100.0f, 0.0f, 1.0f);
			// il modello gira solo con la camera libera in terza persona: altrimenti girarlo girerebbe la visuale
			const bool bodyOk = s.directionTurnBody && FreeThirdPersonCamera() != nullptr;
			R.dirSpeed = speed;
			R.dirWantAng = ang;
			R.dirCurAng = from;
			R.dirTurnTime = static_cast<double>(w) * kDirMaxTurnTime;
			R.dirLastAt = a_now;
			R.dirBody = bodyOk;
			R.dirBodyAng = WrapAngle(a_p->GetAngleZ());
			R.dirVx = std::sin(from) * speed;
			R.dirVy = std::cos(from) * speed;
			R.dirUntil = a_now + std::max(0.35, R.dirTurnTime + 0.25);
			R.dirApplied = 0;
			R.dirReport = true;
			R.dirStartX = a_p->GetPositionX();
			R.dirStartY = a_p->GetPositionY();
			R.dirStartAt = a_now;
			logger::info("cambio direzione: velocita' {:.2f} -> {:.2f} m/s da {:.0f} verso {:.0f} gradi (peso {:.0f}%, camera {:.0f}, stick {:.0f}, rotazione modello {})", cur, speed,
				from * 57.29578f, ang * 57.29578f, s.directionWeight, R.camOff * 57.29578f, std::atan2(R.stickX, R.stickY) * 57.29578f,
				R.dirBody ? "si, camera compensata" : (s.directionTurnBody ? "no (prima persona o arma sfoderata: girerebbe la camera)" : "no (spenta)"));
		}

		// Per 0.35 s dopo il salto tengo la velocita' orizzontale sulla nuova direzione (il gioco
		// puo' riscriverla nei frame subito dopo il salto). Restituisce true se e' attivo.
		// A fine finestra scrivo nel log lo spostamento reale, cosi' si vede se ha funzionato.
		bool ApplyRedirect(RE::PlayerCharacter* a_p, double a_now)
		{
			if (R.dirUntil <= a_now) {
				if (R.dirReport) {
					R.dirReport = false;
					const float dx = a_p->GetPositionX() - R.dirStartX;
					const float dy = a_p->GetPositionY() - R.dirStartY;
					const double dt = std::max(a_now - R.dirStartAt, 0.05);
					logger::info("cambio direzione: risultato, spostamento verso {:.0f} gradi (voluti {:.0f}{}), {:.0f} unita'/s, applicato in {} frame",
						std::atan2(dx, dy) * 57.29578f, R.dirWantAng * 57.29578f, R.dirTurnTime > 0.02 ? ", virata graduale: spostamento medio" : "",
						std::hypot(dx, dy) / dt, R.dirApplied);
				}
				return false;
			}
			if (!R.airborne || R.dashActive || R.slamActive || PlayerBusy(a_p)) {
				R.dirUntil = 0.0;
				R.dirReport = false;
				return false;
			}
			auto* ctrl = a_p->GetCharController();
			if (!ctrl) {
				R.dirUntil = 0.0;
				R.dirReport = false;
				return false;
			}
			// direzione di questo frame: scatto immediato (peso 0) oppure virata graduale verso quella voluta
			const double dt = std::clamp(a_now - R.dirLastAt, 0.0, 0.05);
			R.dirLastAt = a_now;
			const bool gradual = R.dirTurnTime > 0.02;
			if (!gradual) {
				R.dirCurAng = R.dirWantAng;
			} else {
				constexpr float kPi = 3.14159265358979f;
				const float     step = static_cast<float>(kPi / R.dirTurnTime * dt);  // 180 gradi in dirTurnTime secondi
				R.dirCurAng = WrapAngle(R.dirCurAng + std::clamp(WrapAngle(R.dirWantAng - R.dirCurAng), -step, step));
			}
			R.dirVx = std::sin(R.dirCurAng) * R.dirSpeed;
			R.dirVy = std::cos(R.dirCurAng) * R.dirSpeed;
			if (R.dirBody) {
				// il modello gira in fretta ma non di scatto: segue la direzione del movimento
				const float bodyStep = static_cast<float>(kBodyTurnRate * dt);
				R.dirBodyAng = WrapAngle(R.dirBodyAng + std::clamp(WrapAngle(R.dirCurAng - R.dirBodyAng), -bodyStep, bodyStep));
				if (auto* tps = FreeThirdPersonCamera()) {
					FaceHeading(a_p, R.dirBodyAng, tps);
				} else {
					R.dirBody = false;  // la camera non e' piu' libera (cambio di vista o arma sfoderata): non giro piu' il modello
				}
			}

			RE::hkVector4 v;
			ctrl->GetLinearVelocityImpl(v);
			auto*       comp = reinterpret_cast<float*>(&v.quad);
			const float want = std::hypot(R.dirVx, R.dirVy);
			const float cur = std::hypot(comp[0], comp[1]);
			if (std::isfinite(cur) && want > 0.01f && cur > 0.05f) {
				const float cosA = (comp[0] * R.dirVx + comp[1] * R.dirVy) / (cur * want);
				if (cosA > (gradual ? 0.9998f : 0.995f) && std::fabs(cur - want) < 0.1f * want) {
					return true;  // gia' sulla direzione voluta
				}
			}
			comp[0] = R.dirVx;
			comp[1] = R.dirVy;
			ctrl->SetLinearVelocityImpl(v);
			R.acValid = true;  // per il controllo del movimento in aria questa e' "la mia" velocita'
			R.acX = R.dirVx;
			R.acY = R.dirVy;
			++R.dirApplied;
			return true;
		}

		// ----------------------------------------------------------------
		// attacchi in salto
		// Le animazioni sono in OAR: scelgono l'animazione leggendo le variabili globali di doublejump.esp
		// (P1 = attacco in corso, tipo di attacco, mano) e i bool/eventi del grafo. Prima a scrivere quelle
		// variabili e a mandare gli eventi era uno script: qui lo fa il plugin, in modo nativo e con le stesse regole:
		// un attacco (normale, potente o arco) per volo, attacco potente = 1 s di gravita' forte.
		// Dalla 2.0 la magia in salto non e' piu' un attacco: avvia la modalita' strega volante (vedi piu' sotto).
		// ----------------------------------------------------------------
		void SetAtkText(const std::string& a_text)
		{
			std::lock_guard lk(g_atkMutex);
			g_atkText = a_text;
		}

		void AtkLog(const char* a_what)
		{
			if (R.jaLogs < 300) {
				++R.jaLogs;
				logger::info("attacco in salto: {}", a_what);
			}
		}

		// stessa numerazione di GetEquippedItemType di Papyrus: 0 pugni, 1-6 armi da mischia, 7 arco, 8 bastone, 9 magia, 10 scudo, 11 torcia, 12 balestra
		int EquipType(RE::PlayerCharacter* a_p, bool a_left)
		{
			auto* o = a_p->GetEquippedObject(a_left);
			if (!o) {
				return 0;
			}
			switch (o->GetFormType()) {
			case RE::FormType::Weapon:
				if (const auto* w = o->As<RE::TESObjectWEAP>()) {
					switch (w->GetWeaponType()) {
					case RE::WEAPON_TYPE::kOneHandSword:
						return 1;
					case RE::WEAPON_TYPE::kOneHandDagger:
						return 2;
					case RE::WEAPON_TYPE::kOneHandAxe:
						return 3;
					case RE::WEAPON_TYPE::kOneHandMace:
						return 4;
					case RE::WEAPON_TYPE::kTwoHandSword:
						return 5;
					case RE::WEAPON_TYPE::kTwoHandAxe:
						return 6;
					case RE::WEAPON_TYPE::kBow:
						return 7;
					case RE::WEAPON_TYPE::kStaff:
						return 8;
					case RE::WEAPON_TYPE::kCrossbow:
						return 12;
					default:
						return 0;
					}
				}
				return 0;
			case RE::FormType::Spell:
			case RE::FormType::Scroll:
				return 9;
			case RE::FormType::Armor:
				return 10;
			case RE::FormType::Light:
				return 11;
			default:
				return 0;
			}
		}

		void AtkClearFlags()
		{
			SetG(F.gAtkP1, 0.0f);
			SetG(F.gAtkP2, 0.0f);
			SetG(F.gAtkAirPower, 0.0f);
			SetG(F.gAtkNormal, 0.0f);
			SetG(F.gAtkPower, 0.0f);
			SetG(F.gAtkCast, 0.0f);
			SetG(F.gAtkRanged, 0.0f);
		}

		// la gravita' forte dell'attacco potente: la tolgo solo se e' ancora la mia (lo slam usa lo stesso interruttore)
		void AtkReleaseGravity()
		{
			if (R.jaPowerGrav) {
				R.jaPowerGrav = false;
				if (!R.slamActive && R.gravityOverride && std::fabs(R.gravityValue - kAtkPowerGravity) < 0.01f) {
					R.gravityOverride = false;
				}
			}
		}

		// fine completa: tutte le variabili a zero, niente piu' in sospeso
		void EndAirAttack(RE::PlayerCharacter* a_p, const char* a_why)
		{
			const bool was = R.jaType >= 0 || R.jaFlightUsed || R.jaAnimOn || R.jaPowerOn || R.jaPowerGrav || R.jaDriven || R.jaCleanAt >= 0.0;
			AtkClearFlags();
			SetG(F.gAtkHand, -1.0f);
			AtkReleaseGravity();
			if (a_p && R.jaDriven) {
				if (!R.dashActive && !R.slamActive) {
					SetDriven(a_p, false);
				}
			}
			R.jaDriven = false;
			if (a_p && was) {
				a_p->SetGraphVariableBool("bMJGPMA", false);
			}
			R.jaType = -1;
			R.jaBurstDone = false;
			R.jaAnimOn = false;
			R.jaFlightUsed = false;
			R.jaEventAt = -1.0;
			R.jaPowerOn = false;
			R.jaCleanAt = -1.0;
			R.jaCleanMax = -1.0;
			R.jaPowerLandAt = -1.0;
			R.jaAnimEndAt = -1.0;
			R.jaCastFireAt = -1.0;
			R.jaCastDone = false;
			R.jaBowKey = -1;
			R.jaBowRelease = false;
			R.jaBowFired = false;
			if (was) {
				std::string t = std::format("ultimo attacco: {} ({})", R.jaLast.empty() ? "-" : R.jaLast, a_why);
				SetAtkText(t);
				AtkLog(a_why);
			}
		}

		// come Collision_AttackEnd nello script: gravita' normale, P1 spento, P2 acceso. Il tipo di attacco resta (serve fino all'atterraggio)
		void AirAttackAnimEnd(const char* a_why)
		{
			if (!R.jaAnimOn) {
				return;
			}
			R.jaAnimOn = false;
			R.jaAnimEndAt = -1.0;
			AtkReleaseGravity();
			SetG(F.gAtkP1, 0.0f);
			SetG(F.gAtkP2, 1.0f);
			AtkLog(a_why);
		}

		// ----------------------------------------------------------------
		// modalita' "strega volante" (2.0)
		// In aria, con una magia in mano, il tasto della magia in salto ti lascia sospeso nel punto in cui sei (niente discesa graduale).
		// Le animazioni di equip, lancio e unequip sono quelle del set dedicato (OAR: condizione sulla variabile globale MJN_Witch);
		// i lanci li fa il gioco, normalmente. Si esce col tasto (o la combinazione) di uscita, oppure rinfoderando le magie; il
		// personaggio comincia a cadere solo quando l'animazione di unequip e' finita.
		// ----------------------------------------------------------------
		constexpr const char* kWitchClearVars[] = {
			"bAnimationDriven", "bMJFallHeight1", "bMJFallHeight2", "bMJFastJump", "bMJStandingJump", "bMJVirtualJump", "bMJDirectionChange1", "bMJDirectionChange2", "bMJGPMA"
		};

		void WitchLog(const std::string& a_text)
		{
			if (R.wmLogs < 160) {
				++R.wmLogs;
				logger::info("strega volante: {}", a_text);
			}
		}

		void WitchSetText(const std::string& a_text)
		{
			std::lock_guard lk(g_atkMutex);
			g_witchText = a_text;
		}

		bool SpellInHand(RE::PlayerCharacter* a_p)
		{
			return EquipType(a_p, false) == 9 || EquipType(a_p, true) == 9;
		}

		RE::WEAPON_STATE WeaponStateOf(RE::PlayerCharacter* a_p)
		{
			const auto* ast = a_p->AsActorState();
			return ast ? ast->GetWeaponState() : RE::WEAPON_STATE::kSheathed;
		}

		// "come a mod spenta" per la durata della modalita': via i perk dei salti e delle animazioni e l'interruttore delle animazioni,
		// cosi' niente salto / dash / animazioni di caduta durante la sospensione
		void WitchSuspend(RE::PlayerCharacter* a_p)
		{
			if (R.witchSuspended) {
				return;
			}
			R.witchSuspended = true;
			R.wmRemovedPerk = false;
			R.wmRemovedAnim = false;
			if (F.perk && a_p->HasPerk(F.perk)) {
				a_p->RemovePerk(F.perk);
				R.wmRemovedPerk = true;
			}
			if (F.perkAnim && a_p->HasPerk(F.perkAnim)) {
				a_p->RemovePerk(F.perkAnim);
				R.wmRemovedAnim = true;
			}
			SetG(F.gEnabled, 0.0f);
		}

		void WitchResume(RE::PlayerCharacter* a_p)
		{
			if (!R.witchSuspended) {
				return;
			}
			R.witchSuspended = false;
			if (!R.glideSuspended) {
				if (R.wmRemovedPerk && F.perk && !R.jumpGateClosed && !a_p->HasPerk(F.perk)) {
					a_p->AddPerk(F.perk);
				}
				if (R.wmRemovedAnim && F.perkAnim && !a_p->HasPerk(F.perkAnim)) {
					a_p->AddPerk(F.perkAnim);
				}
				SetG(F.gEnabled, 1.0f);
			}
			R.wmRemovedPerk = false;
			R.wmRemovedAnim = false;
			R.lastMaintain = 0.0;  // controllo subito che sia tutto a posto
		}

		// effetti visivi della modalita': un'abilita' con tre effetti magici (corpo, mano sinistra, mano destra)
		void WitchFx(RE::PlayerCharacter* a_p, bool a_on)
		{
			if (!F.spWitchFx) {
				return;
			}
			const bool want = a_on && Settings::Get().witchFx;
			const bool has = a_p->HasSpell(F.spWitchFx);
			if (want && !has) {
				a_p->AddSpell(F.spWitchFx);
			} else if (!want && has) {
				a_p->RemoveSpell(F.spWitchFx);
			}
		}

		void WitchClearGraph(RE::PlayerCharacter* a_p)
		{
			for (const auto* name : kWitchClearVars) {
				bool v = false;
				if (a_p->GetGraphVariableBool(name, v) && v) {
					a_p->SetGraphVariableBool(name, false);
				}
			}
		}

		// ----------------------------------------------------------------
		// diagnostica: stato del controller e del grafo di animazione (solo per il log)
		// ----------------------------------------------------------------
		std::string WitchDiagText(RE::PlayerCharacter* a_p)
		{
			auto* c = a_p->GetCharController();
			if (!c) {
				return "nessun controller";
			}
			float sp = -1.0f;
			a_p->GetGraphVariableFloat("Speed", sp);
			const auto* m = reinterpret_cast<const float*>(&c->velocityMod.quad);
			return std::format("controller: stato {} (voluto {}), caduta {:.2f} s, IsInMidair {}, bInJumpState {}, Speed del grafo {:.0f}, velocityMod {:.2f} m/s", static_cast<int>(c->context.currentState),
				static_cast<int>(c->wantState), c->fallTime, a_p->IsInMidair() ? 1 : 0, GraphBool(a_p, "bInJumpState", false) ? 1 : 0, sp, std::hypot(m[0], m[1]));
		}

		// all'uscita normale in aria: il grafo di animazione riparte dalla posa di caduta (come quando cammini oltre un bordo) e il personaggio
		// riceve una piccola spinta verso il basso, cosi' non sembra "appoggiato" all'aria
		void WitchFallStart(RE::PlayerCharacter* a_p, double a_now)
		{
			const bool accepted = a_p->NotifyAnimationGraph("JumpFall");
			float      vz = 0.0f;
			bool       kicked = false;
			if (auto* ctrl = a_p->GetCharController()) {
				RE::hkVector4 v;
				ctrl->GetLinearVelocityImpl(v);
				auto* c = reinterpret_cast<float*>(&v.quad);
				if (std::isfinite(c[2])) {
					vz = c[2];
					if (c[2] > -kWitchFallKick) {
						c[2] = -kWitchFallKick;
						ctrl->SetLinearVelocityImpl(v);
						kicked = true;
					}
				}
			}
			R.wmFallDiagUntil = a_now + 2.0;
			R.wmFallDiagAt = a_now + 0.1;
			WitchLog(std::format("caduta: evento JumpFall {}, velocita' verticale {:.2f} m/s{}", accepted ? "accettato" : "rifiutato", vz, kicked ? " -> spinta verso il basso" : ""));
		}

		// fine completa: effetti spenti, gravita' e permessi come prima. Da qui in poi il personaggio cade.
		// a_fall = uscita normale in aria (parte la posa di caduta e la spinta verso il basso)
		void WitchFinish(RE::PlayerCharacter* a_p, const char* a_why, bool a_fall = false)
		{
			if (R.wmState == 0 && !R.witchSuspended) {
				return;
			}
			const double now = Now();
			WitchFx(a_p, false);
			SetG(F.gWitch, 0.0f);
			const int was = R.wmState;
			R.wmState = 0;
			R.wmEquipStep = 0;
			if (R.wmGravity) {
				R.wmGravity = false;
				R.gravityOverride = false;
			}
			WitchResume(a_p);
			ApplyBaselines();
			WitchClearGraph(a_p);
			R.acValid = false;
			R.gdValid = false;
			R.dirUntil = 0.0;
			if (a_fall && R.airborne && Settings::Get().witchFallEvent) {
				WitchFallStart(a_p, now);
			}
			WitchLog(std::format("finita: {} (stato {}, dopo {:.1f} s)", a_why ? a_why : "?", was, now - R.wmStart));
			WitchSetText(std::format("ultima volta: finita ({}) dopo {:.1f} s", a_why ? a_why : "?", now - R.wmStart));
		}

		// dopo l'uscita: per qualche secondo scrivo nel log cosa fanno controller e grafo (primi episodi)
		void WitchPostDiag(RE::PlayerCharacter* a_p, double a_now)
		{
			if (R.wmFallDiagUntil <= 0.0) {
				return;
			}
			if (a_now > R.wmFallDiagUntil || R.wmCount > 3) {
				R.wmFallDiagUntil = 0.0;
				return;
			}
			if (a_now >= R.wmFallDiagAt) {
				R.wmFallDiagAt = a_now + 0.4;
				WitchLog(std::format("dopo l'uscita: quota {:+.0f} dall'ancora, {}", a_p->GetPositionZ() - R.wmAnchorZ, WitchDiagText(a_p)));
			}
		}

		// cambio di zona / caricamento: niente chiamate al personaggio, rimetto solo lo stato; perk e abilita' li sistema Maintain
		void WitchAbortLight()
		{
			R.wmState = 0;
			R.wmEquipStep = 0;
			R.witchSuspended = false;
			R.wmRemovedPerk = false;
			R.wmRemovedAnim = false;
			SetG(F.gWitch, 0.0f);
			if (R.wmGravity) {
				R.wmGravity = false;
				R.gravityOverride = false;
			}
			ApplyBaselines();  // la gravita' a zero non deve restare durante la pausa di zona
			R.lastMaintain = 0.0;
		}

		// null = si puo' partire, altrimenti il motivo (solo per il log)
		const char* WitchBlock(RE::PlayerCharacter* a_p)
		{
			const auto& s = Settings::Get();
			if (!F.gWitch) {
				return "manca la variabile MJN_Witch in doublejump.esp (serve il doublejump.esp nuovo)";
			}
			if (!s.witchEnabled) {
				return "strega volante spenta";
			}
			if (!R.applied) {
				return "mod spenta";
			}
			if (!R.airborne) {
				return "non sei in aria";
			}
			if (PlayerBusy(a_p) || a_p->IsDead()) {
				return "personaggio occupato, in acqua o morto";
			}
			if (R.dashActive || R.slamActive) {
				return "dash o slam in corso";
			}
			if (R.glide && s.paraCompat) {
				return "stai planando";
			}
			if (!SpellInHand(a_p)) {
				return "nessuna magia in mano";
			}
			return nullptr;
		}

		// quota finale dell'ancora: un po' piu' in basso del punto di attivazione (compensa la telecamera), ma mai troppo vicino al terreno
		float WitchAnchorGoal(RE::PlayerCharacter* a_p, float a_z)
		{
			const float drop = Settings::Get().witchDrop;
			if (drop <= 0.0f) {
				return a_z;
			}
			float goal = a_z - drop;
			if (auto* tes = RE::TES::GetSingleton()) {
				float              land = 0.0f;
				const RE::NiPoint3 pos(a_p->GetPositionX(), a_p->GetPositionY(), a_z);
				if (tes->GetLandHeight(pos, land) && std::isfinite(land)) {
					const float minZ = land + kWitchMinClear;
					if (a_z <= minZ) {
						goal = a_z;  // gia' vicino al terreno: non scendo
					} else if (goal < minZ) {
						goal = minZ;
					}
				}
			}
			return goal;
		}

		// mani magiche: a mani riposte basta tirarle fuori; a mani gia' fuori (e se l'opzione e' accesa) le rinfodero e le rimetto
		void WitchEquipStart(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			const auto  ws = WeaponStateOf(a_p);
			R.wmEquipStep = 0;
			const char* equip = "nessuna (mani gia' fuori)";
			if (ws == RE::WEAPON_STATE::kSheathed || ws == RE::WEAPON_STATE::kWantToDraw) {
				a_p->DrawWeaponMagicHands(true);
				R.wmEquipStep = 2;
				R.wmStepAt = a_now;
				equip = "mani fuori";
			} else if (ws == RE::WEAPON_STATE::kDrawing) {
				R.wmEquipStep = 2;
				R.wmStepAt = a_now;
				equip = "mani gia' in uscita";
			} else if (s.witchReequip) {
				if (ws == RE::WEAPON_STATE::kDrawn) {
					a_p->DrawWeaponMagicHands(false);
				}
				R.wmEquipStep = 1;
				R.wmStepAt = a_now;
				equip = "rinfodero e rimetto le mani";
			}
			WitchLog(std::format("equip: {} (stato arma {})", equip, static_cast<int>(ws)));
		}

		void WitchPoseGuard(RE::PlayerCharacter* a_p, double a_now);

		void WitchEnter(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			if (R.wmState != 0) {
				return;
			}
			if (const char* why = WitchBlock(a_p)) {
				if (R.wmRejLogs < 10) {
					++R.wmRejLogs;
					WitchLog(std::format("non parte: {}", why));
				}
				return;
			}
			EndAirAttack(a_p, "strega volante");
			R.dirUntil = 0.0;
			R.dirReport = false;
			R.acValid = false;
			R.gdValid = false;
			WitchSuspend(a_p);

			R.wmState = 1;
			R.wmSince = a_now;
			R.wmStart = a_now;
			R.wmLastAt = a_now;
			const float z0 = a_p->GetPositionZ();
			R.wmStartZ = z0;
			R.wmAnchorZ = z0;
			R.wmAnchorGoal = WitchAnchorGoal(a_p, z0);
			R.wmBobAt = a_now;
			R.wmPoseAt = a_now;
			R.wmPoseStage = 0;
			R.wmJumpSince = -1.0;
			R.wmJumpFixes = 0;
			R.wmLegLogAt = a_now + 0.2;
			R.wmLegLogs = 0;
			R.wmTextAt = 0.0;
			R.wmWarned = false;
			R.wmFallDiagUntil = 0.0;
			R.wmVx = 0.0f;
			R.wmVy = 0.0f;
			R.wmVz = 0.0f;
			R.wmNoSpellSince = -1.0;
			R.wmEverDrawn = false;
			R.wmUnequipMin = 0.0;
			R.wmLogAt = a_now + 0.25;
			R.wmFxAt = a_now + 0.5;
			if (auto* ctrl = a_p->GetCharController()) {
				RE::hkVector4 v;
				ctrl->GetLinearVelocityImpl(v);
				const auto* c = reinterpret_cast<const float*>(&v.quad);
				if (std::isfinite(c[0]) && std::isfinite(c[1]) && std::isfinite(c[2])) {
					R.wmVx = c[0];
					R.wmVy = c[1];
					R.wmVz = c[2];
				}
			}
			R.gravityOverride = true;
			R.gravityValue = 0.0f;
			R.wmGravity = true;
			ApplyBaselines();
			SetG(F.gWitch, 1.0f);
			SetDriven(a_p, false);
			WitchClearGraph(a_p);
			WitchFx(a_p, true);

			++R.wmCount;
			Msg("Strega volante");
			WitchLog(std::format("partita (n. {}): quota {:.0f} (ancora finale {:.0f}), velocita' di partenza {:.1f}/{:.1f}/{:.1f} m/s; {}", R.wmCount, z0, R.wmAnchorGoal, R.wmVx, R.wmVy, R.wmVz,
				WitchDiagText(a_p)));
			WitchSetText("in corso (equip)");

			// le mani escono subito (la posa delle mani NON si tocca); l'uscita dalla posa di caduta la fa WitchPoseGuard
			WitchEquipStart(a_p, a_now);
			WitchPoseGuard(a_p, a_now);
		}

		// da attiva / equip a unequip: il personaggio resta fermo fino alla fine dell'animazione
		void WitchBeginExit(RE::PlayerCharacter* a_p, double a_now, const char* a_why, bool a_byGame)
		{
			if (R.wmState == 0 || R.wmState == 3) {
				return;
			}
			const auto ws = WeaponStateOf(a_p);
			double     minWait = 0.0;
			if (ws == RE::WEAPON_STATE::kSheathing || ws == RE::WEAPON_STATE::kWantToSheathe) {
				minWait = kWitchUnequipMin - 0.15;  // l'ha gia' iniziata il gioco
			} else if (ws != RE::WEAPON_STATE::kSheathed) {
				if (!a_byGame) {
					a_p->DrawWeaponMagicHands(false);
				}
				minWait = kWitchUnequipMin;
			}
			R.wmState = 3;
			R.wmSince = a_now;
			R.wmEquipStep = 0;
			R.wmUnequipMin = minWait;
			R.wmExitWhy = a_why;
			WitchFx(a_p, false);
			WitchLog(std::format("uscita: {} (stato arma {}, attesa unequip {:.2f} s)", a_why, static_cast<int>(ws), minWait));
			WitchSetText(std::format("in uscita ({})", a_why));
		}

		void WitchMove(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			auto*       ctrl = a_p->GetCharController();
			if (!ctrl) {
				return;
			}
			const float dt = static_cast<float>(std::clamp(a_now - R.wmLastAt, 0.0, 0.05));
			R.wmLastAt = a_now;

			auto approach = [](float a_cur, float a_target, float a_maxDelta) {
				const float d = a_target - a_cur;
				if (std::fabs(d) <= a_maxDelta) {
					return a_target;
				}
				return a_cur + (d > 0.0f ? a_maxDelta : -a_maxDelta);
			};

			// movimento orizzontale: solo con la modalita' attiva; la direzione e' quella dello stick rispetto a dove guarda il personaggio
			// (con le mani fuori il personaggio segue la camera)
			float tx = 0.0f;
			float ty = 0.0f;
			if (R.wmState == 2) {
				const float mag = std::hypot(R.stickX, R.stickY);
				if (std::isfinite(mag) && mag > 0.15f) {
					const float k = std::min(mag, 1.0f);
					const float h = a_p->GetAngleZ();
					const float sh = std::sin(h);
					const float ch = std::cos(h);
					const float dx = sh * R.stickY + ch * R.stickX;
					const float dy = ch * R.stickY - sh * R.stickX;
					const float dl = std::hypot(dx, dy);
					if (dl > 0.0001f) {
						tx = dx / dl * s.witchSpeed * k;
						ty = dy / dl * s.witchSpeed * k;
					}
				}
			}
			R.wmVx = approach(R.wmVx, tx, kWitchAccel * dt);
			R.wmVy = approach(R.wmVy, ty, kWitchAccel * dt);

			// quota: l'ancora scende piano fino alla quota finale (compensa la telecamera); sopra c'e' una leggera fluttuazione su e giu'
			const float prevAnchor = R.wmAnchorZ;
			R.wmAnchorZ = approach(R.wmAnchorZ, R.wmAnchorGoal, kWitchDropSpeed * dt);
			const float anchorV = dt > 0.0001f ? (R.wmAnchorZ - prevAnchor) / dt / kHavokScale : 0.0f;  // m/s
			float       bob = 0.0f;
			float       bobV = 0.0f;
			if (s.witchBob > 0.0f) {
				const double t = a_now - R.wmBobAt;
				const float  ramp = static_cast<float>(std::clamp(t / kWitchBobRamp, 0.0, 1.0));
				const float  w = static_cast<float>(6.283185307179586 / kWitchBobPeriod);
				bob = s.witchBob * ramp * std::sin(w * static_cast<float>(t));
				bobV = s.witchBob * ramp * w * std::cos(w * static_cast<float>(t)) / kHavokScale;
			}
			const float z = a_p->GetPositionZ();
			const float err = (R.wmAnchorZ + bob - z) / kHavokScale;  // metri
			const float hold = std::clamp(err * kWitchHoldGain + anchorV + bobV, -kWitchMaxVert, kWitchMaxVert);
			R.wmVz = approach(R.wmVz, hold, kWitchVertAccel * dt);

			RE::hkVector4 v;
			ctrl->GetLinearVelocityImpl(v);
			auto*       comp = reinterpret_cast<float*>(&v.quad);
			const float readX = comp[0];
			const float readY = comp[1];
			const float readZ = comp[2];
			if (!std::isfinite(R.wmVx) || !std::isfinite(R.wmVy) || !std::isfinite(R.wmVz)) {
				R.wmVx = 0.0f;
				R.wmVy = 0.0f;
				R.wmVz = 0.0f;
			}
			comp[0] = R.wmVx;
			comp[1] = R.wmVy;
			comp[2] = R.wmVz;
			ctrl->SetLinearVelocityImpl(v);
			R.acValid = false;

			// diagnostica: per i primi secondi delle prime attivazioni scrivo quota, velocita' e stato del grafo
			if (R.wmCount <= 3 && a_now >= R.wmLogAt && (a_now - R.wmStart) < 4.0) {
				R.wmLogAt = a_now + 0.5;
				WitchLog(std::format("a {:.1f} s: quota {:+.0f} dall'ancora iniziale (ancora ora {:+.0f}), velocita' letta {:.2f}/{:.2f}/{:.2f} m/s, scritta {:.2f}/{:.2f}/{:.2f} m/s; {}", a_now - R.wmStart,
					z - R.wmStartZ, R.wmAnchorZ - R.wmStartZ, readX, readY, readZ, R.wmVx, R.wmVy, R.wmVz, WitchDiagText(a_p)));
			}
		}

		// Il grafo di animazione resta nella posa di salto / caduta (bInJumpState) anche se la mod ti tiene sospeso. Ne esco con gli eventi di
		// atterraggio del gioco, uno alla volta (prima il piu' leggero) e solo finche' il grafo e' ancora in salto. NON si manda
		// IdleForceDefaultState: rimette a zero tutto il grafo e fa perdere la posa delle mani (visto nel log del 19/10).
		void WitchPoseGuard(RE::PlayerCharacter* a_p, double a_now)
		{
			static constexpr const char* kEv[] = { "JumpLandFailSafe", "JumpLand", "JumpLandEnd" };
			if (!Settings::Get().witchGroundPose || R.wmState == 0 || R.wmState == 3) {
				R.wmJumpSince = -1.0;
				return;
			}
			const bool jump = GraphBool(a_p, "bInJumpState", false);
			if (!jump) {
				if (R.wmPoseStage > 0) {
					WitchLog(std::format("posa: il grafo e' uscito dallo stato di salto / caduta dopo {:.2f} s dall'inizio (ultimo evento mandato: {})", a_now - R.wmStart, kEv[R.wmPoseStage - 1]));
					R.wmPoseStage = 0;
				}
				R.wmJumpSince = -1.0;
				return;
			}
			if (R.wmJumpSince < 0.0) {
				R.wmJumpSince = a_now;
			}
			if (R.wmPoseStage >= 3) {
				// provati tutti e tre e il grafo e' ancora in salto: dopo un po' ricomincio (al massimo tre giri in tutto)
				if ((a_now - R.wmPoseAt) > 1.5 && R.wmJumpFixes < 3) {
					R.wmPoseStage = 0;
				}
				return;
			}
			if (R.wmPoseStage == 0 && R.wmJumpFixes >= 3) {
				return;
			}
			const double since = R.wmPoseStage == 0 ? R.wmJumpSince : R.wmPoseAt;
			const double wait = R.wmPoseStage == 0 ? (R.wmJumpFixes == 0 ? 0.0 : 0.5) : 0.4;
			if ((a_now - since) < wait) {
				return;
			}
			const char* ev = kEv[R.wmPoseStage];
			const bool  ok = a_p->NotifyAnimationGraph(ev);
			if (R.wmPoseStage == 0) {
				++R.wmJumpFixes;
			}
			++R.wmPoseStage;
			R.wmPoseAt = a_now;
			WitchLog(std::format("posa: il grafo e' in salto / caduta (bInJumpState), mando {} ({}, giro {})", ev, ok ? "accettato" : "rifiutato", R.wmJumpFixes));
		}

		void UpdateWitch(RE::PlayerCharacter* a_p, double a_now, const KeyEvt& a_exit)
		{
			if (R.wmState == 0) {
				return;
			}
			const auto& s = Settings::Get();
			if (!s.witchEnabled || !R.applied) {
				WitchFinish(a_p, "spenta");
				return;
			}
			if (PlayerBusy(a_p) || a_p->IsDead()) {
				WitchFinish(a_p, "personaggio occupato, in acqua o morto");
				return;
			}
			if (!R.airborne) {
				WitchFinish(a_p, "atterrato");
				return;
			}
			if (R.glide && s.paraCompat) {
				WitchFinish(a_p, "planata");
				return;
			}

			const auto ws = WeaponStateOf(a_p);
			if (ws == RE::WEAPON_STATE::kDrawn) {
				R.wmEverDrawn = true;
			}
			if (R.wmState != 3 && a_exit.pressed && (a_now - R.wmStart) > 0.4) {
				WitchBeginExit(a_p, a_now, "tasto di uscita", false);
			}
			const double inState = a_now - R.wmSince;

			switch (R.wmState) {
			case 1:  // equip
				if (R.wmEquipStep == 1) {
					// aspetto che le mani siano riposte, poi le rimetto fuori
					if (ws == RE::WEAPON_STATE::kSheathed || (a_now - R.wmStepAt) >= kWitchPhaseMax) {
						a_p->DrawWeaponMagicHands(true);
						R.wmEquipStep = 2;
						R.wmStepAt = a_now;
					}
				} else if (R.wmEquipStep == 2) {
					const double e2 = a_now - R.wmStepAt;
					if ((ws == RE::WEAPON_STATE::kDrawn && e2 >= 0.5) || e2 >= kWitchEquipMax) {
						R.wmEquipStep = 0;
					}
				}
				if (R.wmEquipStep == 0) {
					R.wmState = 2;
					R.wmSince = a_now;
					WitchLog(std::format("attiva (equip finito dopo {:.2f} s, stato arma {}); {}", a_now - R.wmStart, static_cast<int>(ws), WitchDiagText(a_p)));
					WitchSetText("attiva: sospesa, puoi lanciare e muoverti");
				}
				break;
			case 2:  // attiva
				if (s.witchExitOnSheathe && R.wmEverDrawn && inState >= 0.5 &&
					(ws == RE::WEAPON_STATE::kWantToSheathe || ws == RE::WEAPON_STATE::kSheathing || ws == RE::WEAPON_STATE::kSheathed)) {
					WitchBeginExit(a_p, a_now, "magie rinfoderate", true);
					break;
				}
				if (s.witchTimer >= 30.0f) {
					const double left = static_cast<double>(s.witchTimer) - (a_now - R.wmStart);
					if (left <= 0.0) {
						Msg("Strega volante: tempo finito");
						WitchBeginExit(a_p, a_now, "tempo scaduto", false);
						break;
					}
					if (left <= 10.0 && !R.wmWarned) {
						R.wmWarned = true;
						Msg("Strega volante: ancora 10 secondi");
					}
					if (a_now >= R.wmTextAt) {
						R.wmTextAt = a_now + 1.0;
						WitchSetText(std::format("attiva: sospesa, puoi lanciare e muoverti (restano {:.0f} s)", std::ceil(left)));
					}
				}
				if (!SpellInHand(a_p)) {
					if (R.wmNoSpellSince < 0.0) {
						R.wmNoSpellSince = a_now;
					} else if ((a_now - R.wmNoSpellSince) > 0.4) {
						WitchBeginExit(a_p, a_now, "niente magie in mano", false);
					}
				} else {
					R.wmNoSpellSince = -1.0;
				}
				break;
			case 3:  // unequip: si resta fermi fino alla fine dell'animazione
				if ((inState >= R.wmUnequipMin && ws == RE::WEAPON_STATE::kSheathed) || inState >= kWitchPhaseMax) {
					WitchFinish(a_p, R.wmExitWhy ? R.wmExitWhy : "uscita", true);
					return;
				}
				break;
			default:
				break;
			}
			if (R.wmState == 0) {
				return;
			}

			WitchPoseGuard(a_p, a_now);
			WitchMove(a_p, a_now);
			WitchClearGraph(a_p);
			if (R.wmState != 3 && a_now >= R.wmFxAt) {
				R.wmFxAt = a_now + 0.5;
				WitchFx(a_p, true);  // rispetta anche l'interruttore delle impostazioni
			}
		}

		// ----------------------------------------------------------------
		// attacco pesante in salto (2.0): altezza minima e burst di fuoco con le armi a due mani
		// ----------------------------------------------------------------
		// altezza sopra il punto di partenza del salto; se sei sotto (caduto da un bordo) altezza sopra il terreno
		float AirHeight(RE::PlayerCharacter* a_p)
		{
			const float z = a_p->GetPositionZ();
			if (R.takeoffValid && z >= R.takeoffZ - 20.0f) {
				return z - R.takeoffZ;
			}
			if (auto* tes = RE::TES::GetSingleton()) {
				float          land = 0.0f;
				const RE::NiPoint3 pos(a_p->GetPositionX(), a_p->GetPositionY(), z);
				if (tes->GetLandHeight(pos, land) && std::isfinite(land)) {
					return z - land;
				}
			}
			return R.takeoffValid ? (z - R.takeoffZ) : 0.0f;
		}

		// Il burst usa gli stessi effetti della Tempesta di Fuoco dello slam (stessi effetti magici, stesso aspetto), ma con area e danno
		// ridotti: le voci della magia del burst (doublejump.esp) vengono riempite con quelle della Tempesta di Fuoco del gioco. Se la
		// Tempesta di Fuoco non si trova resta solo il danno da fuoco, senza effetto visivo.
		void ConfigureBurst()
		{
			const auto& s = Settings::Get();
			if (!F.spBurst) {
				return;
			}
			auto&          dst = F.spBurst->effects;
			const auto*    fs = F.spFireStorm;
			const auto     area = static_cast<std::uint32_t>(std::max(1.0f, std::round(s.heavyBurstRadius)));
			std::size_t    used = 0;
			if (fs && !fs->effects.empty()) {
				float maxMag = 0.0f;
				for (const auto* e : fs->effects) {
					if (e) {
						maxMag = std::max(maxMag, e->effectItem.magnitude);
					}
				}
				const float k = maxMag > 0.001f ? s.heavyBurstDamage / maxMag : 1.0f;
				for (const auto* e : fs->effects) {
					if (!e || !e->baseEffect || used >= dst.size()) {
						continue;
					}
					auto* d = dst[used];
					if (!d) {
						continue;
					}
					d->baseEffect = e->baseEffect;
					d->effectItem = e->effectItem;
					d->effectItem.magnitude = e->effectItem.magnitude * k;
					if (e->effectItem.area > 0) {
						d->effectItem.area = area;
					}
					++used;
				}
			}
			if (used == 0 && !dst.empty() && dst[0]) {
				// senza Tempesta di Fuoco: solo il danno da fuoco del plugin
				dst[0]->effectItem.magnitude = s.heavyBurstDamage;
				dst[0]->effectItem.area = area;
				used = 1;
			}
			for (std::size_t i = used; i < dst.size(); ++i) {
				if (auto* d = dst[i]) {
					d->effectItem.magnitude = 0.0f;
					d->effectItem.area = 0;
					d->effectItem.duration = 0;
				}
			}
			if (!R.burstLogged) {
				R.burstLogged = true;
				logger::info("burst di fuoco: {} voci dalla Tempesta di Fuoco del gioco ({}), area {} piedi, danno {:.0f}", used, fs ? "trovata" : "NON trovata, solo danno", area, s.heavyBurstDamage);
			}
		}

		void HeavyBurst(RE::PlayerCharacter* a_p)
		{
			const auto& s = Settings::Get();
			if (!s.heavyBurst || !F.spBurst || R.jaBurstDone) {
				return;
			}
			const int tr = EquipType(a_p, false);
			if (tr != 5 && tr != 6) {
				return;  // solo spade, asce e martelli a due mani
			}
			R.jaBurstDone = true;
			ConfigureBurst();
			CastSelf(a_p, F.spBurst);
			AtkLog(std::format("burst di fuoco (danno {:.0f}, raggio {:.0f})", s.heavyBurstDamage, s.heavyBurstRadius).c_str());
		}

		// null = si puo' attaccare, altrimenti il motivo (solo per il log)
		const char* AirAttackBlock(RE::PlayerCharacter* a_p)
		{
			const auto& s = Settings::Get();
			if (!F.atkOk) {
				return "mancano le variabili degli attacchi in doublejump.esp";
			}
			if (!s.airAttackEnabled) {
				return "attacchi in salto spenti";
			}
			if (!R.applied) {
				return "mod spenta";
			}
			if (!R.airborne) {
				return "non sei in aria";
			}
			if (PlayerBusy(a_p)) {
				return "personaggio occupato o in acqua";
			}
			if (R.dashActive || R.slamActive) {
				return "dash o slam in corso";
			}
			if (R.glide && s.paraCompat) {
				return "stai planando";
			}
			const auto* ast = a_p->AsActorState();
			if (!ast || !ast->IsWeaponDrawn()) {
				static int s_logged = 0;
				if (s_logged < 3) {
					++s_logged;
					logger::info("attacco in salto: stato dell'arma letto dal gioco = {} (0 rifoderata, 1-2 in sfoderamento, 3 sfoderata, 4-5 in rifoderamento)", ast ? static_cast<int>(ast->GetWeaponState()) : -1);
				}
				return "arma non sfoderata";
			}
			if (!s.airAttackAlways && !(HasEffect(a_p, F.mgJump1) || HasEffect(a_p, F.mgJump2) || HasEffect(a_p, F.mgJump3))) {
				return "fuori dalla finestra del salto magico";
			}
			return nullptr;
		}

		const char* TypeName(int a_type)
		{
			switch (a_type) {
			case 0:
				return "normale";
			case 1:
				return "potente";
			case 2:
				return "magia";
			case 3:
				return "arco";
			default:
				return "?";
			}
		}

		// a_hand: 1 destra, 0 sinistra (solo per la magia, serve a scegliere l'animazione); a_key: 0 = tasto dell'attacco normale, 3 = tasto dell'arco
		bool BeginAirAttack(RE::PlayerCharacter* a_p, double a_now, int a_type, int a_hand, int a_key)
		{
			if (const char* why = AirAttackBlock(a_p)) {
				if (R.jaRejLogs < 10) {
					++R.jaRejLogs;
					AtkLog(std::format("{} rifiutato: {}", TypeName(a_type), why).c_str());
				}
				return false;
			}
			if (a_type == 2) {
				if (R.jaAnimOn && R.jaType != 2) {
					AtkLog("magia rifiutata: un altro attacco e' in corso");
					return false;
				}
				if ((a_now - R.jaCastAt) < kAtkCastLock) {
					return false;  // niente log: e' la pausa di 2 s tra due lanci
				}
			} else if (R.jaFlightUsed || R.jaAnimOn) {
				AtkLog(std::format("{} rifiutato: hai gia' attaccato in questo volo", TypeName(a_type)).c_str());
				return false;
			}

			AtkClearFlags();
			switch (a_type) {
			case 0:
				SetG(F.gAtkNormal, 1.0f);
				a_p->SetGraphVariableBool("bMJGPMA", true);  // lo spegne l'animazione (PIE.$MJGPMAOFF)
				break;
			case 1:
				SetG(F.gAtkPower, 1.0f);
				break;
			case 2:
				SetG(F.gAtkCast, 1.0f);
				SetG(F.gAtkHand, static_cast<float>(a_hand));
				break;
			case 3:
				SetG(F.gAtkRanged, 1.0f);
				break;
			}
			SetG(F.gAtkP1, 1.0f);
			R.jaType = a_type;
			R.jaBurstDone = false;
			R.jaAnimOn = true;
			R.jaFlightUsed = true;
			R.jaStartAt = a_now;
			R.jaEventAt = a_now + kAtkStartDelay;
			R.jaCleanAt = -1.0;
			R.jaCleanMax = -1.0;
			R.jaAnimEndAt = -1.0;
			R.jaCastFireAt = -1.0;
			R.jaCastDone = false;
			R.jaBowKey = a_type == 3 ? a_key : -1;
			R.jaBowRelease = false;
			R.jaBowFired = false;
			++R.jaCount;
			R.jaLast = TypeName(a_type);
			AtkLog(std::format("{} avviato (mano {}, equipaggiato: destra {} sinistra {})", TypeName(a_type), a_hand, EquipType(a_p, false), EquipType(a_p, true)).c_str());
			SetAtkText(std::format("attacco in corso: {}", TypeName(a_type)));
			return true;
		}

		// 0.1 s dopo l'inizio: l'evento che fa partire l'animazione (come lo script)
		void SendAttackAnimEvent(RE::PlayerCharacter* a_p, double a_now)
		{
			const char* evName = "OffsetGPMA";
			if (R.jaType == 1 || R.jaType == 3) {
				const bool fast = GraphBool(a_p, "bMJFastJump", false);
				evName = fast ? "JumpStandingStart" : "JumpDirectionalStart";
			}
			const bool accepted = a_p->NotifyAnimationGraph(evName);
			{
				// diagnostica: se il grafo rifiuta l'evento (false) l'animazione non puo' partire; se lo accetta ma non si vede nulla
				// il problema e' nelle condizioni OAR (i globali qui sotto devono valere 1 per l'attacco scelto)
				auto gv = [](RE::TESGlobal* g) { return g ? g->value : -9.0f; };
				AtkLog(std::format("evento '{}' mandato al grafo: {} | globali P1={:.0f} P2={:.0f} normale={:.0f} potente={:.0f} magia={:.0f} arco={:.0f} uno={:.0f} | bMJGPMA={} bMJFastJump={} bAnimationDriven={}",
					evName, accepted ? "ACCETTATO" : "RIFIUTATO", gv(F.gAtkP1), gv(F.gAtkP2), gv(F.gAtkNormal), gv(F.gAtkPower), gv(F.gAtkCast), gv(F.gAtkRanged), gv(F.gOne),
					GraphBool(a_p, "bMJGPMA", false) ? 1 : 0, GraphBool(a_p, "bMJFastJump", false) ? 1 : 0, GraphBool(a_p, "bAnimationDriven", false) ? 1 : 0).c_str());
			}
			if (R.jaType == 1) {
				// PowerAttackFinish dello script: 1 s di MJ_AirPowerAttacking e gravita' forte, animazione guidata
				SetG(F.gAtkAirPower, 1.0f);
				R.jaPowerOn = true;
				R.jaPowerOffAt = a_now + kAtkPowerSeconds;
				SetDriven(a_p, true);
				R.jaDriven = true;
				R.gravityOverride = true;
				R.gravityValue = kAtkPowerGravity;
				R.jaPowerGrav = true;
			} else if (R.jaType == 0) {
				R.jaAnimEndAt = a_now + kAtkNormalFailsafe;  // se Collision_AttackEnd non arriva
			}
		}

		// script di base: prima la mano destra, poi la sinistra; la prima magia che ti puoi permettere
		bool DoAirCast(RE::PlayerCharacter* a_p, double a_now)
		{
			R.jaCastDone = true;
			R.jaCastFireAt = -1.0;
			R.jaCastAt = a_now;
			R.jaAnimEndAt = a_now + kAtkCastAnimEnd;
			auto* avo = a_p->AsActorValueOwner();
			if (!avo) {
				return false;
			}
			bool anySpell = false;
			for (int hand = 1; hand >= 0; --hand) {
				auto* o = a_p->GetEquippedObject(hand == 0);
				auto* spell = o ? o->As<RE::SpellItem>() : nullptr;
				if (!spell) {
					continue;
				}
				anySpell = true;
				const float cost = spell->CalculateMagickaCost(a_p);
				if (!std::isfinite(cost) || avo->GetActorValue(RE::ActorValue::kMagicka) < cost) {
					continue;
				}
				auto* caster = a_p->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
				if (!caster) {
					continue;
				}
				caster->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, a_p);
				avo->DamageActorValue(RE::ActorValue::kMagicka, cost);
				AtkLog(std::format("magia lanciata (mano {}, costo {:.0f})", hand == 1 ? "destra" : "sinistra", cost).c_str());
				return true;
			}
			if (anySpell) {
				Msg("Magicka insufficiente per la magia in salto");
				AtkLog("magia non lanciata: magicka insufficiente");
			} else {
				AtkLog("magia non lanciata: nessun incantesimo in mano");
			}
			return false;
		}

		// l'arco: freccia e balestra partono dal punto di tiro dell'arma, come quando tiri a terra; costa una freccia
		bool FireAirArrow(RE::PlayerCharacter* a_p)
		{
			auto* o = a_p->GetEquippedObject(false);
			auto* weap = o ? o->As<RE::TESObjectWEAP>() : nullptr;
			if (!weap || !(weap->IsBow() || weap->IsCrossbow())) {
				AtkLog("tiro: niente arco in mano");
				return false;
			}
			auto* ammo = a_p->GetCurrentAmmo();
			if (!ammo) {
				Msg("Nessuna freccia equipaggiata");
				AtkLog("tiro: nessuna freccia equipaggiata");
				return false;
			}
			RE::ProjectileHandle handle;
			RE::Projectile::LaunchArrow(&handle, a_p, ammo, weap);
			a_p->RemoveItem(ammo, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			AtkLog("freccia scoccata");
			return true;
		}

		// tentativi di attacco: restituisce quale tipo si vuole (-1 nessuno) in base all'equipaggiamento
		void TryNormalAttack(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			const int   tr = EquipType(a_p, false);
			const int   tl = EquipType(a_p, true);
			const bool  bow = (tr == 7 || tr == 12);
			const bool  melee = (tr >= 0 && tr <= 6) || (tl >= 1 && tl <= 4);
			const bool  hasSpell = (tr == 9 || tl == 9);
			const bool  casterOnly = hasSpell && !(tr >= 1 && tr <= 6) && !(tl >= 1 && tl <= 4);
			if (bow) {
				if (s.airAttackAutoPick) {
					BeginAirAttack(a_p, a_now, 3, 1, 0);
				}
				return;
			}
			if (casterOnly) {
				if (s.airAttackAutoPick) {
					WitchEnter(a_p, a_now);  // 2.0: la magia in salto e' la strega volante
				}
				return;
			}
			if (melee) {
				BeginAirAttack(a_p, a_now, 0, 1, 0);
			} else {
				AtkLog("normale: niente di utilizzabile in mano");
			}
		}

		void TryPowerAttack(RE::PlayerCharacter* a_p, double a_now)
		{
			const int  tr = EquipType(a_p, false);
			const int  tl = EquipType(a_p, true);
			const bool bow = (tr == 7 || tr == 12);
			const bool melee = (tr >= 0 && tr <= 6) || (tl >= 1 && tl <= 4);
			if (!bow && melee) {
				// 2.0: l'attacco pesante in salto parte solo da una certa altezza (poco sopra un salto normale)
				const float minH = Settings::Get().airPowerMinHeight;
				if (minH > 0.0f) {
					const float h = AirHeight(a_p);
					if (h < minH) {
						if (R.jaRejLogs < 10) {
							++R.jaRejLogs;
							AtkLog(std::format("potente rifiutato: altezza {:.0f} sotto il minimo {:.0f}", h, minH).c_str());
						}
						return;
					}
				}
				BeginAirAttack(a_p, a_now, 1, 1, 0);
			}
		}

		void TryCastAttack(RE::PlayerCharacter* a_p, double a_now)
		{
			const int tr = EquipType(a_p, false);
			const int tl = EquipType(a_p, true);
			if (tr == 9 || tl == 9) {
				WitchEnter(a_p, a_now);  // 2.0: la magia in salto e' la strega volante
			}
		}

		void TryRangedAttack(RE::PlayerCharacter* a_p, double a_now)
		{
			const int tr = EquipType(a_p, false);
			if (tr == 7 || tr == 12) {
				BeginAirAttack(a_p, a_now, 3, 1, 3);
			}
		}

		// atterraggio: le variabili restano accese ancora un attimo perche' l'animazione di atterraggio (quella dell'attacco potente
		// in particolare) le legge; poi UpdateAirAttacks spegne tutto
		void AirAttackLanded(double a_now)
		{
			if (!F.atkOk || (R.jaType < 0 && !R.jaFlightUsed && !R.jaAnimOn)) {
				return;
			}
			const bool pow = R.jaType == 1;
			R.jaCleanAt = a_now + (pow ? kAtkPowerLandKeep : kAtkLandKeep);
			R.jaCleanMax = a_now + (pow ? kAtkPowerLandMax : kAtkLandKeep + 0.5);
			R.jaEventAt = -1.0;
			R.jaCastFireAt = -1.0;
			R.jaBowFired = true;  // atterrato col tiro ancora da fare: niente freccia
			AtkReleaseGravity();
		}

		void AirAttackJumpUp(RE::PlayerCharacter* a_p)
		{
			if (R.jaCleanAt >= 0.0) {
				EndAirAttack(a_p, "nuovo salto");
			}
		}

		void UpdateAirAttacks(RE::PlayerCharacter* a_p, double a_now, const KeyEvt& a_n, const KeyEvt& a_pw, const KeyEvt& a_c, const KeyEvt& a_r)
		{
			if (!F.atkOk) {
				return;
			}
			const auto& s = Settings::Get();
			const bool  active = R.jaType >= 0 || R.jaFlightUsed || R.jaAnimOn || R.jaCleanAt >= 0.0;

			// spento / situazioni in cui un attacco in salto non ha senso: pulizia completa
			if (!s.airAttackEnabled || !R.applied || R.dashActive || R.slamActive || R.wmState != 0 || (R.glide && s.paraCompat) || PlayerBusy(a_p)) {
				if (active) {
					EndAirAttack(a_p, !s.airAttackEnabled ? "attacchi spenti" : "dash, slam, planata, strega volante o personaggio occupato");
				}
				return;
			}

			// dopo l'atterraggio: pulizia ritardata
			if (R.jaCleanAt >= 0.0) {
				double due = R.jaCleanAt;
				if (R.jaType == 1 && R.jaPowerLandAt > 0.0) {
					due = std::max(due, R.jaPowerLandAt + 1.0);  // come lo script: 1 s dopo MJPowerAttackLanding
				}
				if (a_now >= std::min(due, R.jaCleanMax)) {
					EndAirAttack(a_p, "atterrato");
				}
				return;
			}

			// attacco in corso
			if (R.jaType >= 0) {
				if (!R.airborne) {
					return;  // l'atterraggio lo gestisce OnLanded
				}
				if ((a_now - R.jaStartAt) > kAtkMaxFlight) {
					EndAirAttack(a_p, "troppo lungo");
					return;
				}
				if (R.jaEventAt >= 0.0 && a_now >= R.jaEventAt) {
					R.jaEventAt = -1.0;
					SendAttackAnimEvent(a_p, a_now);
				}
				if (R.jaPowerOn && a_now >= R.jaPowerOffAt) {
					R.jaPowerOn = false;
					SetG(F.gAtkAirPower, 0.0f);
					AtkReleaseGravity();
				}
				if (R.jaType == 2 && !R.jaCastDone) {
					if (R.jaCastFireAt < 0.0 && R.jaEventAt < 0.0 && (a_now - R.jaStartAt) >= (kAtkStartDelay + kAtkCastFallback)) {
						R.jaCastFireAt = a_now;  // l'animazione non ha mandato l'annotazione (animazioni assenti?): lancio lo stesso
					}
					if (R.jaCastFireAt >= 0.0 && a_now >= R.jaCastFireAt) {
						DoAirCast(a_p, a_now);
					}
				}
				if (R.jaType == 3 && R.jaAnimOn && !R.jaBowFired) {
					const KeyEvt& k = (R.jaBowKey == 0) ? a_n : a_r;
					if (!k.down && !R.jaBowRelease) {
						R.jaBowRelease = true;
						R.jaBowReleaseAt = a_now;
					}
					const bool tooLong = (a_now - R.jaStartAt) >= kAtkBowMaxDraw;
					if ((R.jaBowRelease && a_now >= std::max(R.jaStartAt + kAtkBowMinDraw, R.jaBowReleaseAt)) || tooLong) {
						R.jaBowFired = true;
						SetG(F.gAtkP1, 0.0f);
						SetG(F.gAtkP2, 1.0f);  // animazione di rilascio dell'arco
						FireAirArrow(a_p);
						R.jaAnimEndAt = a_now + kAtkBowAnimEnd;
					}
				}
				if (R.jaAnimEndAt >= 0.0 && a_now >= R.jaAnimEndAt) {
					R.jaAnimEndAt = -1.0;
					if (R.jaType == 2 || R.jaType == 3) {
						// magia e arco non hanno Collision_AttackEnd: spengo tutto, cosi' non resta l'animazione appesa
						AtkClearFlags();
						R.jaAnimOn = false;
						AtkLog("animazione finita");
					} else {
						AirAttackAnimEnd("animazione finita (nessun Collision_AttackEnd)");
					}
				}
			}

			// nuovi attacchi
			if (!R.airborne) {
				return;
			}
			if (a_r.pressed) {
				TryRangedAttack(a_p, a_now);
			} else if (a_c.pressed) {
				TryCastAttack(a_p, a_now);
			} else if (a_pw.pressed) {
				TryPowerAttack(a_p, a_now);
			} else if (a_n.pressed) {
				TryNormalAttack(a_p, a_now);
			}
		}

		// ----------------------------------------------------------------
		// eventi, atterraggio
		// ----------------------------------------------------------------
		void OnLanded(RE::PlayerCharacter* a_p, double a_now)
		{
			SetG(F.gInAir, 0.0f);
			AirAttackLanded(a_now);
			R.jumpCount = 0;
			R.acValid = false;
			if (R.dashUsed) {
				ArmSpeedGuard(a_now, kGuardAfterLand);  // atterraggio dopo un dash: il controller puo' portarsi dietro la velocita' del dash
				ArmLandingFix(a_now);
			}
			R.dashUsed = false;  // nuovo volo, nuovo dash
			R.dashBufferUntil = 0.0;
			if (R.dashActive) {
				EndDash(a_p, a_now, "atterrato");
			}
			if (!GraphBool(a_p, "bMJRollLand", false)) {
				a_p->SetGraphVariableBool("MagicalJumpingDashing", false);
			}
			if (!R.slamActive) {
				R.gravityOverride = false;
				ApplyBaselines();
			}
		}

		void LogParaEvent(const char* a_what)
		{
			if (R.paraEvLogs < 40) {
				++R.paraEvLogs;
				logger::info("planata: evento di animazione: {}", a_what);
			}
		}

		void ProcessEvents(RE::PlayerCharacter* a_p, double a_now)
		{
			std::vector<Ev> evs;
			{
				std::lock_guard lk(g_evMutex);
				evs.swap(g_events);
			}
			for (const auto e : evs) {
				switch (e) {
				case Ev::JumpUp:
					AirAttackJumpUp(a_p);
					// conteggio dei salti di questo volo (il dash e lo slam non contano)
					if (R.wmState == 0 && !R.dashActive && !R.slamActive && !(R.glide && Settings::Get().paraCompat) && (a_now - R.dashStart) > 1.2 && (a_now - R.lastJumpUpAt) > 0.25) {
						++R.jumpCount;
						R.lastJumpUpAt = a_now;
						logger::info("salto numero {} di questo volo", R.jumpCount);
						if (R.jumpCount >= 2) {
							StartRedirect(a_p, a_now);
						}
					}
					R.prevPending = true;
					R.prevAt = a_now + 0.12;
					if (R.sjArmed && !GraphBool(a_p, "bMJVirtualJump", false)) {
						R.sjResetAt = a_now + 0.2;
					}
					break;
				case Ev::SoftLanding:
					R.delayed.push_back({ a_now + 1.1, 0 });
					break;
				case Ev::FastLanding:
					R.delayed.push_back({ a_now + 0.0, 0 });
					break;
				case Ev::PowerAttackLanding:
					R.delayed.push_back({ a_now + 1.0, 0 });
					if (R.jaType == 1 && R.jaPowerLandAt < 0.0) {
						R.jaPowerLandAt = a_now;  // niente esplosione del gioco: l'attacco potente in salto colpisce con l'arma
						HeavyBurst(a_p);          // 2.0: con un'arma a due mani parte il piccolo burst di fuoco (nessuna attesa)
					}
					break;
				case Ev::AttackEnd:
					// bastano gli attacchi in corso: Collision_AttackEnd arriva anche da altre mod. L'arco si spegne col rilascio
					if (R.jaAnimOn && R.jaType != 3 && R.airborne && (a_now - R.jaStartAt) > 0.15 && (a_now - R.jaLastEvAt[0]) > kAtkDup) {
						R.jaLastEvAt[0] = a_now;
						AirAttackAnimEnd("Collision_AttackEnd");
					}
					break;
				case Ev::AirCast:
					if (R.jaType == 2 && !R.jaCastDone && R.jaCastFireAt < 0.0 && (a_now - R.jaLastEvAt[1]) > kAtkDup) {
						R.jaLastEvAt[1] = a_now;
						R.jaCastFireAt = a_now + kAtkCastDelay;
					}
					break;
				case Ev::Parachute:
					R.delayed.push_back({ a_now + 0.0, 0 });
					R.glideEvOn = true;
					R.glideWhy = "evento Parachute";
					LogParaEvent("Parachute (inizio planata)");
					break;
				case Ev::ParaStart:
					R.glideEvOn = true;
					R.glideWhy = "evento SoundPlay.ParaglideEquip";
					LogParaEvent("SoundPlay.ParaglideEquip (inizio planata)");
					break;
				case Ev::ParaEnd:
					R.glideEvOn = false;
					LogParaEvent("la mod di planata si ripiega (fine planata)");
					break;
				}
			}
		}

		void RunDelayed(RE::PlayerCharacter* a_p, double a_now)
		{
			if (R.delayed.empty()) {
				return;
			}
			for (auto it = R.delayed.begin(); it != R.delayed.end();) {
				if (a_now >= it->at) {
					if (it->kind == 0 && !R.dashActive && !(R.slamActive && R.slamPhase < 3)) {
						SetDriven(a_p, false);
					}
					it = R.delayed.erase(it);
				} else {
					++it;
				}
			}
		}

		void UpdateAir(RE::PlayerCharacter* a_p, double a_now)
		{
			const bool inAir = a_p->IsInMidair();
			if (inAir) {
				R.lastAirAt = a_now;
				++R.airTicks;
				R.groundTicks = 0;
				if (!R.airborne && R.airTicks >= 2) {
					R.airborne = true;
					R.takeoffZ = R.groundZValid ? R.groundZ : a_p->GetPositionZ();  // quota da cui e' partito il volo
					R.takeoffValid = true;
				}
			} else {
				R.groundZ = a_p->GetPositionZ();
				R.groundZValid = true;
				++R.groundTicks;
				R.airTicks = 0;
				if (R.airborne && R.groundTicks >= 2) {
					R.airborne = false;
					OnLanded(a_p, a_now);
				}
			}
		}

		// ----------------------------------------------------------------
		// salti massimi (doppio / triplo disattivabili)
		// Il gate dei salti permette i salti in aria solo se il giocatore ha il perk 83A: quando il
		// limite e' raggiunto lo tolgo, a terra lo rimetto.
		// ----------------------------------------------------------------
		void UpdateJumpGate(RE::PlayerCharacter* a_p)
		{
			const auto& s = Settings::Get();
			if (R.glideSuspended || R.witchSuspended) {
				return;  // i perk li ha tolti la modalita' planata / strega volante e li rimette lei
			}
			const int   maxJumps = !s.doubleJumpEnabled ? 1 : (!s.tripleJumpEnabled ? 2 : 3);
			int         used = R.jumpCount;
			if (R.airborne && used == 0) {
				used = 1;  // caduto da un bordo: il salto "a terra" e' comunque consumato
			}
			const bool close = R.applied && maxJumps < 3 && used >= maxJumps;
			if (close == R.jumpGateClosed) {
				return;
			}
			R.jumpGateClosed = close;
			if (F.perk) {
				if (close) {
					a_p->RemovePerk(F.perk);
				} else if (!a_p->HasPerk(F.perk)) {
					a_p->AddPerk(F.perk);
				}
			}
			if (close) {
				logger::info("salti: raggiunto il massimo ({}), blocco altri salti in aria fino all'atterraggio", maxJumps);
			} else {
				logger::info("salti: di nuovo permessi");
			}
		}

		// ----------------------------------------------------------------
		// movimento orizzontale in aria (slider). 100 % = non tocco nulla.
		// Altrimenti scalo la velocita' orizzontale del controller del personaggio: una volta sola
		// per ogni "nuova" velocita' calcolata dal gioco, cosi' non si accumula frame dopo frame.
		// ----------------------------------------------------------------
		void UpdateAirControl(RE::PlayerCharacter* a_p)
		{
			const float f = Settings::Get().airControl / 100.0f;
			if (std::fabs(f - 1.0f) < 0.01f || !R.airborne || R.dashActive || R.slamActive || PlayerBusy(a_p)) {
				R.acValid = false;
				return;
			}
			auto* ctrl = a_p->GetCharController();
			if (!ctrl) {
				R.acValid = false;
				return;
			}
			RE::hkVector4 v;
			ctrl->GetLinearVelocityImpl(v);
			auto*       comp = reinterpret_cast<float*>(&v.quad);
			const float x = comp[0];
			const float y = comp[1];
			if (!std::isfinite(x) || !std::isfinite(y)) {
				R.acValid = false;
				return;
			}
			if (R.acValid) {
				const float mag = std::max(std::hypot(R.acX, R.acY), 0.05f);
				if (std::hypot(x - R.acX, y - R.acY) <= 0.04f * mag) {
					return;  // e' ancora la velocita' che ho scritto io
				}
			}
			comp[0] = x * f;
			comp[1] = y * f;
			ctrl->SetLinearVelocityImpl(v);
			R.acX = x * f;
			R.acY = y * f;
			R.acValid = true;
			if (!R.acLogged) {
				R.acLogged = true;
				logger::info("controllo in aria attivo: {:.0f}%", f * 100.0f);
			}
		}

		// ----------------------------------------------------------------
		// Mod di planata
		// Le animazioni di planata della mod di planata sono pose senza movimento proprio: ti sposti perche' il
		// gioco muove il personaggio con la sua fisica. Con la mod accesa le animazioni di caduta di Magical
		// Jumping (annotazione MJFallHeight1) accendono bAnimationDriven: in planata il movimento e' guidato
		// da un'animazione che non si muove e il personaggio resta quasi fermo. Qui riconosco la planata
		// (eventi del behavior della mod di planata + variabile bParaGliding) e per tutta la durata sistemo le
		// cose; alla fine rimetto tutto come prima.
		// ----------------------------------------------------------------
		std::int32_t GraphInt(RE::PlayerCharacter* a_p, const char* a_name, std::int32_t a_default)
		{
			std::int32_t v = a_default;
			if (a_p->GetGraphVariableInt(a_name, v)) {
				return v;
			}
			return a_default;
		}

		constexpr const char* kGlideClearVars[] = {
			"bAnimationDriven", "bMJFallHeight1", "bMJFallHeight2", "bMJFastJump", "bMJStandingJump", "bMJVirtualJump", "bMJDirectionChange1", "bMJDirectionChange2"
		};

		void SetParaText(const std::string& a_text)
		{
			std::lock_guard lk(g_paraMutex);
			g_paraText = a_text;
		}

		// riga di log con tutto quello che serve per capire cosa succede in planata
		std::string GlideSnapshot(RE::PlayerCharacter* a_p)
		{
			float hSpeed = 0.0f;
			float vz = 0.0f;
			if (auto* ctrl = a_p->GetCharController()) {
				RE::hkVector4 v;
				ctrl->GetLinearVelocityImpl(v);
				const auto* c = reinterpret_cast<const float*>(&v.quad);
				hSpeed = std::hypot(c[0], c[1]);
				vz = c[2];
			}
			const bool hasPerk = F.perk && a_p->HasPerk(F.perk);
			const bool hasAnim = F.perkAnim && a_p->HasPerk(F.perkAnim);
			return std::format(
				"velocita' orizzontale {:.2f} m/s, verticale {:.2f} m/s, gravita' {:.2f}, salti {}, variabili MJ accese: {}, "
				"planata: bParaGliding={} bWantPara={} iState_ParaGlide={} iParaGlideGen={} bInJumpState={}, "
				"perk gate dei salti {} perk animazioni {} interruttore animazioni {:.0f}",
				hSpeed, vz, R.appliedGravity, R.jumpCount, DescribeJumpVars(a_p), GraphBool(a_p, "bParaGliding", false) ? 1 : 0,
				GraphBool(a_p, "bWantPara", false) ? 1 : 0, GraphInt(a_p, "iState_ParaGlide", -1), GraphInt(a_p, "iParaGlideGen", -1),
				GraphBool(a_p, "bInJumpState", false) ? 1 : 0, hasPerk ? "si" : "no", hasAnim ? "si" : "no", F.gEnabled ? F.gEnabled->value : -1.0f);
		}

		// "come a mod spenta" per la durata della planata: via i perk (gate dei salti e animazioni) e
		// l'interruttore delle animazioni, cosi' OAR e gate dei salti non toccano la planata
		void GlideSuspend(RE::PlayerCharacter* a_p)
		{
			if (R.glideSuspended) {
				return;
			}
			R.glideSuspended = true;
			R.glideRemovedPerk = false;
			R.glideRemovedAnim = false;
			if (Settings::Get().paraRemovePerks) {
				if (F.perk && a_p->HasPerk(F.perk)) {
					a_p->RemovePerk(F.perk);
					R.glideRemovedPerk = true;
				}
				if (F.perkAnim && a_p->HasPerk(F.perkAnim)) {
					a_p->RemovePerk(F.perkAnim);
					R.glideRemovedAnim = true;
				}
			}
			SetG(F.gEnabled, 0.0f);
			logger::info("planata: animazioni della mod sospese per la planata{}", (R.glideRemovedPerk || R.glideRemovedAnim) ? " (perk tolti)" : "");
		}

		void GlideResume(RE::PlayerCharacter* a_p)
		{
			if (!R.glideSuspended) {
				return;
			}
			R.glideSuspended = false;
			if (!R.witchSuspended) {
				if (R.glideRemovedPerk && F.perk && !R.jumpGateClosed && !a_p->HasPerk(F.perk)) {
					a_p->AddPerk(F.perk);
				}
				if (R.glideRemovedAnim && F.perkAnim && !a_p->HasPerk(F.perkAnim)) {
					a_p->AddPerk(F.perkAnim);
				}
				SetG(F.gEnabled, 1.0f);
			}
			R.glideRemovedPerk = false;
			R.glideRemovedAnim = false;
			R.lastMaintain = 0.0;  // controllo subito che sia tutto a posto
			logger::info("planata: animazioni della mod rimesse");
		}

		void GlideStart(RE::PlayerCharacter* a_p, double a_now, const char* a_why)
		{
			const auto& s = Settings::Get();
			R.glide = true;
			R.glideSince = a_now;
			R.glideFixes = 0;
			R.glideFixLogs = 0;
			R.glideLogs = 0;
			R.glideLogAt = a_now + 1.0;
			++R.glideCount;
			R.glideOffSince = -1.0;
			if (R.glideCount <= 40) {
				logger::info("planata: planata {} iniziata ({}). Compatibilita' {}. Stato: {}", R.glideCount, a_why, s.paraCompat ? "attiva" : "spenta", GlideSnapshot(a_p));
			} else if (!R.glideCapLogged) {
				R.glideCapLogged = true;
				logger::info("planata: oltre 40 planate in questa sessione, non scrivo piu' una riga per ogni inizio e fine");
			}
			if (s.paraCompat) {
				R.dirUntil = 0.0;  // un cambio direzione in corso si ferma
				R.dirReport = false;
				R.acValid = false;
				R.gdValid = false;
				if (s.paraSuspend) {
					GlideSuspend(a_p);
				}
			}
		}

		void GlideEnd(RE::PlayerCharacter* a_p, double a_now, const char* a_why)
		{
			if (R.glideCount <= 40) {
				logger::info("planata: planata {} finita dopo {:.1f} s ({}). Correzioni (bAnimationDriven e flag di caduta): {}. Stato: {}", R.glideCount, a_now - R.glideSince, a_why,
					R.glideFixes, GlideSnapshot(a_p));
			}
			R.glide = false;
			R.glideOffSince = -1.0;
			R.glideSuspendOffAt = a_now;  // le animazioni tornano da UpdateGlide: all'atterraggio o dopo 1,5 s senza una nuova planata
			R.acValid = false;
			R.gdValid = false;
		}

		void UpdateGlide(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();

			// a terra la mod di planata non serve: leggo la variabile al massimo una volta al secondo, in aria a ogni frame
			bool hasPara = R.paraHas;
			if (R.airborne || R.glide || a_now >= R.paraPollAt) {
				R.paraPollAt = a_now + 1.0;
				hasPara = GraphInt(a_p, "hasParaGlider", 0) != 0;
			}
			if (hasPara != R.paraHas) {
				R.paraHas = hasPara;
				if (hasPara) {
					R.paraSeen = true;
				}
				logger::info("planata: {}", hasPara ? "rilevato sul personaggio (hasParaGlider=1)" : "hasParaGlider=0, la mod di planata non e' piu' attiva sul personaggio");
			}
			const bool varOn = hasPara && (R.airborne || R.glide) && GraphBool(a_p, "bParaGliding", false);
			if (varOn != R.paraVar) {
				R.paraVar = varOn;
				if (R.paraVarLogs < 20) {
					++R.paraVarLogs;
					logger::info("planata: variabile bParaGliding {}", varOn ? "accesa" : "spenta");
				}
			}
			if (!R.airborne) {
				R.glideEvOn = false;
			}

			const bool want = hasPara && R.airborne && (R.glideEvOn || varOn);
			if (want) {
				R.glideOffSince = -1.0;
				if (!R.glide) {
					GlideStart(a_p, a_now, R.glideEvOn ? R.glideWhy : "variabile bParaGliding");
				}
			} else if (R.glide) {
				// se e' atterrato o la mod di planata e' sparito chiudo subito, altrimenti aspetto 0,3 s: una variabile che oscilla non deve far
				// spegnere e riaccendere tutto a ogni frame
				if (!hasPara || !R.airborne) {
					GlideEnd(a_p, a_now, !hasPara ? "mod di planata tolta" : "atterrato");
				} else {
					if (R.glideOffSince < 0.0) {
						R.glideOffSince = a_now;
					}
					if ((a_now - R.glideOffSince) >= 0.3) {
						GlideEnd(a_p, a_now, "la mod di planata si ripiega");
					}
				}
			}
			// le animazioni tornano all'atterraggio oppure dopo 1,5 s senza una nuova planata (piu' planate di fila = un solo spegni/riaccendi)
			if (R.glideSuspended && !R.glide && (!R.airborne || (a_now - R.glideSuspendOffAt) >= 1.5)) {
				GlideResume(a_p);
			}

			// diagnostica: in aria con la mod di planata ma nessuna planata riconosciuta (serve per capire se gli eventi arrivano)
			if (hasPara && R.airborne && !R.glide) {
				if (R.paraAirSince < 0.0) {
					R.paraAirSince = a_now;
					R.paraAirAt = a_now + 2.0;
				} else if (a_now >= R.paraAirAt && R.paraAirLogs < 20) {
					R.paraAirAt = a_now + 2.0;
					++R.paraAirLogs;
					logger::info("planata: in aria da {:.1f} s con la mod di planata, nessuna planata riconosciuta (nessun evento di inizio e bParaGliding spento). Stato: {}",
						a_now - R.paraAirSince, GlideSnapshot(a_p));
				}
			} else {
				R.paraAirSince = -1.0;
			}

			if (R.glide && s.paraCompat && s.paraClearDriven) {
				std::string names;
				int         n = 0;
				for (const auto* name : kGlideClearVars) {
					bool v = false;
					if (a_p->GetGraphVariableBool(name, v) && v) {
						a_p->SetGraphVariableBool(name, false);
						++n;
						if (R.glideFixLogs < 6) {
							names += name;
							names += ' ';
						}
					}
				}
				if (n > 0) {
					R.glideFixes += n;
					if (R.glideFixLogs < 6) {
						++R.glideFixLogs;
						logger::info("planata: spenti durante la planata (a {:.2f} s dall'inizio): {}", a_now - R.glideSince, names);
					}
				}
			}

			if (R.glide && a_now >= R.glideLogAt && R.glideLogs < 25) {
				R.glideLogAt = a_now + 1.0;
				++R.glideLogs;
				logger::info("planata: planata {} a {:.1f} s: {}", R.glideCount, a_now - R.glideSince, GlideSnapshot(a_p));
			}

			if (a_now >= R.paraTextAt) {
				R.paraTextAt = a_now + 0.5;
				if (!R.paraHas && !R.paraSeen) {
					SetParaText("Mod di planata non rilevata sul personaggio (hasParaGlider = 0).");
				} else if (R.glide) {
					SetParaText(std::format("IN PLANATA da {:.1f} s (planata n. {}). Correzioni fatte finora: {}.", a_now - R.glideSince, R.glideCount, R.glideFixes));
				} else {
					SetParaText(std::format("Mod di planata rilevata{}. Planate in questa sessione: {}.", R.paraHas ? "" : " (ora non attiva)", R.glideCount));
				}
			}
		}

		// ----------------------------------------------------------------
		// acqua dopo un volo: se il personaggio finisce in acqua dopo salti / dash / slam
		// ripulisco lo stato di animazione e, se non riesce a muoversi, ripristino l'idle.
		// ----------------------------------------------------------------
		void UpdateWater(RE::PlayerCharacter* a_p, double a_now)
		{
			const bool swimming = IsSwimmingNow(a_p);
			if (!swimming) {
				R.wasSwimming = false;
				R.waterIdlePending = false;
				return;
			}
			if (!R.wasSwimming) {
				R.wasSwimming = true;
				const bool afterFlight = R.airborne || R.dashActive || R.slamActive || (a_now - R.lastAirAt) < 3.0;
				logger::info("acqua: il personaggio e' entrato in acqua (ultimo volo {:.1f} s fa, dash={}, slam={}, variabili accese: {})", a_now - R.lastAirAt,
					R.dashActive, R.slamActive, DescribeJumpVars(a_p));
				if (afterFlight) {
					R.waterWatchUntil = a_now + kWaterWatch;
					R.waterSampleAt = a_now + 0.5;
					R.waterLastX = a_p->GetPositionX();
					R.waterLastY = a_p->GetPositionY();
					R.waterStuck = 0;
					R.waterRescues = 0;
					const int fixed = CleanJumpGraphState(a_p);
					DispelSelf(a_p, F.spJump1);
					DispelSelf(a_p, F.spJump2);
					DispelSelf(a_p, F.spJump3);
					DispelSelf(a_p, F.spDash);
					SetG(F.gInAir, 0.0f);
					SetG(F.gSlam, 0.0f);
					logger::info("acqua dopo un volo (dash={}, slam={}): ripulite {} variabili di animazione", R.dashActive, R.slamActive, fixed);
					// se c'era davvero qualcosa di acceso (o c'e' stato un dash / slam da poco) poco dopo rimetto anche
					// l'animazione di base e riattivo i controlli; un semplice tuffo in acqua non viene toccato
					R.waterIdlePending = fixed > 0 || R.dashActive || R.slamActive || (a_now - R.dashStart) < 3.0 || (a_now - R.slamStart) < 3.0;
					R.waterIdleAt = a_now + 0.3;
				}
			}

			if (R.waterIdlePending && a_now >= R.waterIdleAt) {
				R.waterIdlePending = false;
				const char* ev = nullptr;
				const int   fixed = RecoverAnimState(a_p, &ev);
				logger::info("acqua: animazione di base ripristinata ('{}', {} variabili ancora accese)", ev ? ev : "?", fixed);
			}

			if (a_now < R.waterWatchUntil) {
				// le animazioni possono riaccendere le variabili anche dopo: continuo a ripulire
				const int fixed = CleanJumpGraphState(a_p);
				if (fixed > 0) {
					logger::info("acqua: riaccese {} variabili, rispente", fixed);
				}
				if (a_now >= R.waterSampleAt) {
					R.waterSampleAt = a_now + 0.5;
					const float x = a_p->GetPositionX();
					const float y = a_p->GetPositionY();
					const float moved = std::hypot(x - R.waterLastX, y - R.waterLastY);
					R.waterLastX = x;
					R.waterLastY = y;
					float input = std::hypot(R.stickX, R.stickY);  // stick letto dal controller (o dal gioco)
					if (const auto* pc = RE::PlayerControls::GetSingleton()) {
						input = std::max(input, std::hypot(pc->data.moveInputVec.x, pc->data.moveInputVec.y));
					}
					R.waterStuck = (input > 0.5f && moved < 2.0f) ? R.waterStuck + 1 : 0;
					if (R.waterStuck >= 3 && R.waterRescues < 4) {
						++R.waterRescues;
						R.waterStuck = 0;
						logger::warn("acqua: il personaggio non si muove nonostante lo stick (variabili accese: {}), ripristino l'animazione di base", DescribeJumpVars(a_p));
						RecoverAnimState(a_p, nullptr);
					}
				}
			}
		}

		void ResetActions(RE::PlayerCharacter* a_p)
		{
			const double now = Now();
			WitchFinish(a_p, "reset");
			if (R.dashActive) {
				EndDash(a_p, now, "mod disattivata o salvataggio caricato");
			}
			if (R.slamActive) {
				EndSlam(a_p, now);
			}
			SetTimeScale(1.0f);
			R.gravityOverride = false;
			R.sjArmed = false;
			R.sjResetAt = -1.0;
			R.delayed.clear();
			EndAirAttack(a_p, "reset");
		}

		// ----------------------------------------------------------------
		// reset delle animazioni del personaggio (reset classico:
		// bAnimationDriven spento, controlli riattivati, idle di reset, gravita' normale).
		// Va chiamato solo dal thread di gioco (Tick).
		// ----------------------------------------------------------------
		void ResetPlayerState(RE::PlayerCharacter* a_p, const char* a_why)
		{
			const double now = Now();
			logger::info("reset animazioni del personaggio ({})", a_why);

			// 1. tutto quello che la mod ha in corso
			ResetActions(a_p);
			R.dirUntil = 0.0;
			R.acValid = false;
			R.slamActive = false;
			R.slamPhase = 0;
			R.dashActive = false;
			R.dashEventSent = false;
			R.dashUsed = false;
			R.dashBufferUntil = 0.0;
			R.dashCooldownUntil = now + 0.3;
			R.slamCooldownUntil = now + 0.3;
			R.jumpCount = 0;
			R.lastJumpUpAt = -100.0;
			R.prevValid = false;
			R.prevPending = false;
			R.waterWatchUntil = 0.0;
			R.waterIdlePending = false;
			R.guardUntil = 0.0;
			R.guardAfterDash = false;
			{
				std::lock_guard lk(g_evMutex);
				g_events.clear();
			}
			DispelSelf(a_p, F.spJump1);
			DispelSelf(a_p, F.spJump2);
			DispelSelf(a_p, F.spJump3);
			DispelSelf(a_p, F.spDash);
			DispelSelf(a_p, F.spDashEnd);
			SetG(F.gInAir, 0.0f);
			SetG(F.gSlam, 0.0f);
			SetG(F.gDirChanged, 0.0f);

			// 2-4. variabili del grafo di animazione, controlli del giocatore (come EnablePlayerControls()
			// dei vecchi script) e idle di reset (idle standard: Idle 0xE6538 del gioco base)
			const char* ev = nullptr;
			const int   fixed = RecoverAnimState(a_p, &ev);

			// 5. gravita', salto e permessi dei salti come all'inizio
			R.jumpGateClosed = false;
			if (R.applied) {
				R.appliedGravity = -1.0f;
				R.appliedJump = -1.0f;
				Maintain(a_p);
				ApplyBaselines();
			}
			logger::info("reset: idle '{}', ripulite {} variabili di animazione", ev ? ev : "?", fixed);
			RE::SendHUDMessage::ShowHUDMessage("MJN: animazioni del personaggio ripristinate");
		}

		// tasto / pulsanti di reset: tenuti premuti per resetHold secondi
		void UpdateResetInput(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			const bool  allowed = InputAllowed();
			bool        raw = KeyIsDown(s.keyReset);
			if (!raw && s.padEnabled && s.padReset != 0 && allowed && Pad::GameFocused()) {
				const auto mask = static_cast<std::uint32_t>(s.padReset);
				raw = (Pad::Poll() & mask) == mask;
			}
			const auto e = Poll(R.kReset, raw, allowed, a_now);
			if (!e.down) {
				R.resetFired = false;
			} else if (!R.resetFired && e.held >= s.resetHold) {
				R.resetFired = true;
				ResetPlayerState(a_p, "tasto");
			}
		}

		// ----------------------------------------------------------------
		// strascico del dash dopo un atterraggio a meta' dash ("bug supersonico")
		// Cosa dicono i log: il bug compare SOLO quando si atterra mentre l'animazione del dash e' ancora nella fase veloce
		// (atterraggi a 0.4-0.5 s dall'inizio, con 140-175 m/s orizzontali). Da quel momento la variabile "Speed" del grafo
		// vale un'annotazione di movimento del dash (2500 o 3500) x 4.93, a terra il movimento e' velocityMod = speedPct x 5.29 m/s
		// con speedPct 30-48, e il valore cala di circa 3 al secondo (10-15 s per tornare normale). Gli atterraggi a dash
		// finito o tardi (3-4 m/s) non lasciano niente. Scrivere sul controller DOPO l'update non serve: il motore rifa'
		// velocityMod a ogni frame prima di muovere il personaggio. Quindi:
		//  1. l'intervento e' agganciato anche a PlayerCharacter::UpdateAnimation, cioe' dopo che l'animazione ha calcolato il
		//     movimento e prima che la fisica lo usi: li' limito velocityMod, speedPct e la velocita' del controller;
		//  2. se nonostante questo il personaggio si sposta troppo in fretta, blocco lo spostamento per frame (posizione);
		//  3. (spento di default) ripristino delle animazioni.
		// Parte SOLO nei 25 s dopo la fine di un dash e solo se Speed supera la soglia (default 3000 = 44 m/s, il doppio del
		// massimo visto in schivate e mosse speciali, ~1500). Mosse di combattimento e schivate non vengono mai toccate.
		// Non fa niente in dash, slam, planata, a cavallo, in acqua, in ragdoll.
		// ----------------------------------------------------------------
		void ProbeReset();  // definita piu' avanti (velocita' alla radice)

		void ResetAnomaly()
		{
			ProbeReset();
			R.gvBuilt = false;
			R.lfClamp = false;
			R.lfPosValid = false;
			R.lfArmUntil = 0.0;
			R.lfSince = -1.0;
			R.lfCalmSince = -1.0;
			R.lfFrames = 0;
			R.lfLogs = 0;
			R.lfPeak = 0.0f;
			R.lfHookCalls = 0;
			R.lfHookMax = 0.0f;
			R.lfEntryCuts = 0;
			R.lfPin = false;
			R.lfPinValid = false;
			R.lfPinFrames = 0;
			R.lfStep = 0.0f;
			R.lfStepMax = 0.0f;
			R.lfBack = 0.0f;
			R.lfProbes = 0;
			R.lfResets = 0;
		}

		// da chiamare quando finisce un dash (o si atterra dopo un dash): apre la finestra in cui una Speed altissima e' strascico del dash
		void ArmLandingFix(double a_now)
		{
			if (Settings::Get().speedFix) {
				R.lfArmUntil = std::max(R.lfArmUntil, a_now + kLfWindow);
			}
		}

		void EndAnomaly(double a_now, const char* a_why)
		{
			if (R.lfSince >= 0.0 && R.lfEpisodes <= 12) {
				logger::info("strascico del dash: episodio {} finito dopo {:.1f} s ({}). Picco di Speed {:.0f}; l'aggancio a UpdateAnimation ha visto velocityMod sopra il limite {} volte (massimo {:.1f} m/s), "
							 "all'ingresso ne ha tolto qualcosa {} volte; blocco dello spostamento: ha riportato indietro il personaggio in {} frame, passo massimo del controller {:.1f} m/s; ripristini fatti {}",
					R.lfEpisodes, a_now - R.lfSince, a_why, R.lfPeak, R.lfHookCalls, R.lfHookMax, R.lfEntryCuts, R.lfPinFrames, R.lfStepMax, R.lfResets);
			}
			if (R.lfSince >= 0.0 && R.lfEpisodes <= 12) {
				logger::info("  radice: campo agganciato {} (scritture {}, valore piu' alto visto {:.1f} m/s); variabili del grafo riportate al limite {} volte",
					R.pbLocked ? "SI" : "no", R.pbWrites, R.pbWorst, R.gvCuts);
			}
			R.lfSince = -1.0;
			R.lfCalmSince = -1.0;
			R.lfClamp = false;
			R.lfFrames = 0;
			R.lfPin = false;
			R.lfPinValid = false;
			ProbeReset();
		}

		float StickInput()
		{
			float input = std::hypot(R.stickX, R.stickY);
			if (const auto* pc = RE::PlayerControls::GetSingleton()) {
				input = std::max(input, std::hypot(pc->data.moveInputVec.x, pc->data.moveInputVec.y));
			}
			return input;
		}

		// altre variabili float del grafo che riguardano il movimento (solo quelle che esistono davvero)
		std::string DescribeGraphFloats(RE::PlayerCharacter* a_p)
		{
			static constexpr const char* kNames[] = { "Speed", "SpeedSampled", "SpeedAcc", "SpeedDamped", "Direction", "TurnDelta", "VelocityZ" };
			std::string                  out;
			for (const auto* n : kNames) {
				float v = 0.0f;
				if (a_p->GetGraphVariableFloat(n, v)) {
					out += std::format("{}={:.1f} ", n, v);
				}
			}
			return out.empty() ? std::string("nessuna") : out;
		}

		// variabili booleane del grafo che decidono se il motore applica il movimento dell'animazione
		std::string DescribeGraphBools(RE::PlayerCharacter* a_p)
		{
			static constexpr const char* kNames[] = { "bAllowRotation", "bAnimationDriven", "MagicalJumpingDashing", "bMJRollLand", "bMJFastJump" };
			std::string                  out;
			for (const auto* n : kNames) {
				bool v = false;
				if (a_p->GetGraphVariableBool(n, v)) {
					out += std::format("{}={} ", n, v ? 1 : 0);
				}
			}
			return out.empty() ? std::string("nessuna") : out;
		}

		// da dove potrebbe venire una velocita' alta: tempo di gioco ed effetti attivi che toccano SpeedMult.
		// ATTENZIONE: qui NON si leggono i valori degli attributi (GetActorValue & co.): nel crash del 18/10 la lettura di WeaponSpeedMult
		// sul personaggio ha chiuso il gioco (Crash Logger: SkyrimSE.exe+06330C4).
		std::string DescribeSpeedSources(RE::PlayerCharacter* a_p)
		{
			std::string out = std::format("tempo di gioco x{:.2f} (obiettivo x{:.2f})", RE::BSTimer::QGlobalTimeMultiplier(), RE::BSTimer::QGlobalTimeMultiplierTarget());
			int n = 0;
			if (auto* target = a_p->AsMagicTarget()) {
				if (auto* list = target->GetActiveEffectList()) {
					for (auto* ae : *list) {
						if (!ae) {
							continue;
						}
						const auto* base = ae->GetBaseObject();
						if (!base || (base->data.primaryAV != RE::ActorValue::kSpeedMult && base->data.secondaryAV != RE::ActorValue::kSpeedMult)) {
							continue;
						}
						if (++n > 6) {
							break;
						}
						const auto* file = base->GetFile(0);
						const char* nm = base->GetName();
						out += std::format("; effetto su SpeedMult: '{}' {:08X} ({}) magnitudine {:.1f}", (nm && *nm) ? nm : "senza nome", base->GetFormID(),
							file ? file->GetFilename() : std::string_view("?"), ae->magnitude);
					}
				}
			}
			if (n == 0) {
				out += "; nessun effetto attivo su SpeedMult";
			}
			return out;
		}

		// Cerca nel controller (solo i campi che il codice gia' legge: da 0x70 a 0x268) i float che valgono come la
		// velocita' anomala: serve a scoprire dove il gioco tiene il valore. Solo lettura, solo per il log.
		std::string ScanControllerFloats(RE::bhkCharacterController* a_ctrl, float a_pct, float a_mps, float a_var)
		{
			std::string out;
			if (!a_ctrl) {
				return "nessun controller";
			}
			const auto* base = reinterpret_cast<const std::uint8_t*>(a_ctrl);
			int         n = 0;
			for (std::size_t off = 0x70; off < 0x268 && n < 14; off += 4) {
				float f = 0.0f;
				std::memcpy(&f, base + off, sizeof(f));
				if (!std::isfinite(f)) {
					continue;
				}
				const float af = std::fabs(f);
				const auto  approx = [af](float a_target) { return a_target > 1.0f && std::fabs(af - a_target) <= a_target * 0.02f; };
				const char* what = approx(a_pct) ? "come speedPct" : (approx(a_mps) ? "come m/s" : (approx(a_var) ? "come Speed" : nullptr));
				if (what) {
					out += std::format("+{:X}={:.2f} ({}) ", off, f, what);
					++n;
				}
			}
			return out.empty() ? std::string("nessuno") : out;
		}

		// Limita il movimento orizzontale del controller a a_capMps (velocityMod, speedPct e, se richiesto, la velocita').
		// Restituisce il valore che velocityMod aveva prima (m/s).
		float ClampMotion(RE::bhkCharacterController* a_ctrl, float a_capMps, bool a_withVelocity)
		{
			auto* mod = reinterpret_cast<float*>(&a_ctrl->velocityMod.quad);
			float before = std::hypot(mod[0], mod[1]);
			if (!std::isfinite(before)) {
				mod[0] = 0.0f;
				mod[1] = 0.0f;
				before = 0.0f;
			} else if (before > a_capMps) {
				const float k = a_capMps / before;
				mod[0] *= k;
				mod[1] *= k;
			}
			const float pctCap = std::max(a_capMps / kRunMps, 1.0f);
			if (!std::isfinite(a_ctrl->speedPct) || a_ctrl->speedPct > pctCap) {
				a_ctrl->speedPct = pctCap;
			}
			if (a_withVelocity) {
				RE::hkVector4 v;
				a_ctrl->GetLinearVelocityImpl(v);
				auto*       c = reinterpret_cast<float*>(&v.quad);
				const float h = std::hypot(c[0], c[1]);
				const float velCap = a_capMps * 1.05f;
				if (!std::isfinite(h)) {
					c[0] = 0.0f;
					c[1] = 0.0f;
					a_ctrl->SetLinearVelocityImpl(v);
				} else if (h > velCap) {
					const float k = velCap / h;
					c[0] *= k;
					c[1] *= k;
					a_ctrl->SetLinearVelocityImpl(v);
				}
			}
			return before;
		}

		// Blocco dello spostamento: il controller non puo' allontanarsi, tra una chiamata e la successiva, piu' di
		// cap x tempo trascorso. Se lo ha fatto lo riporto indietro (solo x/y, metri Havok) e sposto della stessa quantita'
		// anche la posizione del personaggio, cosi' controller e riferimento restano coerenti.
		// Si chiama sia all'ingresso di UpdateAnimation sia a fine Tick: la somma dei due consentiti resta cap x tempo.
		// Dai log: nei primi frame dopo l'atterraggio il personaggio si sposta di 50-200 m/s anche con velocityMod
		// e velocita' del controller a zero, quindi il limite sulla velocita' non basta. Restituisce true se ha spostato qualcosa.
		bool PinDisplacement(RE::PlayerCharacter* a_p, RE::bhkCharacterController* a_ctrl, double a_now, float a_capMps, const void* a_cell)
		{
			RE::hkVector4 pos;
			a_ctrl->GetPositionImpl(pos, false);
			auto* c = reinterpret_cast<float*>(&pos.quad);
			if (!std::isfinite(c[0]) || !std::isfinite(c[1])) {
				R.lfPinValid = false;
				return false;
			}
			bool moved = false;
			if (R.lfPinValid && a_cell == R.lfPinCell) {
				const double dt = std::clamp(a_now - R.lfPinAt, 0.0, 0.1);
				const float  dx = c[0] - R.lfPinX;
				const float  dy = c[1] - R.lfPinY;
				const float  d = std::hypot(dx, dy);
				const float  allowed = a_capMps * static_cast<float>(dt) * kLfPinMargin + 0.003f;
				if (dt >= 0.004) {
					R.lfStep = d / static_cast<float>(dt);
					R.lfStepMax = std::max(R.lfStepMax, R.lfStep);
				}
				if (d > allowed && d < 150.0f) {
					const float k = allowed / d;
					const float nx = R.lfPinX + dx * k;
					const float ny = R.lfPinY + dy * k;
					R.lfBack = d - allowed;
					a_p->data.location.x += (nx - c[0]) * kHavokScale;
					a_p->data.location.y += (ny - c[1]) * kHavokScale;
					c[0] = nx;
					c[1] = ny;
					a_ctrl->SetPositionImpl(pos, false, false);
					moved = true;
				}
			}
			R.lfPinX = c[0];
			R.lfPinY = c[1];
			R.lfPinAt = a_now;
			R.lfPinCell = a_cell;
			R.lfPinValid = true;
			return moved;
		}

		// ----------------------------------------------------------------
		// la velocita' "nascosta" da cui il motore ricava ogni frame velocityMod e la variabile Speed
		// Dai log: all'ingresso di UpdateAnimation velocityMod vale ~250 m/s e cala di ~0.45 a frame (circa 19 m/s^2)
		// anche se alla fine del frame precedente l'avevo portato a 12; la Speed del grafo scritta DENTRO UpdateAnimation e'
		// 250 x 68. Quindi c'e' un valore Q che il motore tiene da qualche parte e da cui ricava le due cose.
		// Non so dove stia: durante l'episodio cerco, solo in lettura, nei campi float del processo del personaggio
		// (MiddleHighProcessData e HighProcessData) quello che vale come velocityMod (o x69.99, o /5.287, o x68) e che cala
		// insieme a lui. Se un campo corrisponde per 3 frame di fila lo "aggancio" e da quel momento lo riporto al limite
		// a ogni ingresso e uscita di UpdateAnimation. Se non ne trova nessuno non scrive niente e il log elenca i candidati.
		// ----------------------------------------------------------------
		constexpr float       kProbeScale[4] = { 1.0f, kHavokScale, 1.0f / kRunMps, kSpeedVarPerMps };
		constexpr const char* kProbeScaleName[4] = { "m/s come velocityMod", "unita' di gioco/s", "speedPct", "Speed del grafo" };
		constexpr const char* kProbeRegionName[2] = { "MiddleHighProcessData", "HighProcessData" };

		struct ProbeRegions
		{
			std::uint8_t* base[2] = { nullptr, nullptr };
			std::size_t   size[2] = { 0, 0 };
		};

		bool ProbeRegionsOf(RE::PlayerCharacter* a_p, ProbeRegions& a_out)
		{
			auto* proc = a_p->GetActorRuntimeData().currentProcess;
			if (!proc) {
				return false;
			}
			a_out.base[0] = reinterpret_cast<std::uint8_t*>(proc->middleHigh);
			a_out.size[0] = sizeof(RE::MiddleHighProcessData);
			a_out.base[1] = reinterpret_cast<std::uint8_t*>(proc->high);
			a_out.size[1] = sizeof(RE::HighProcessData);
			return a_out.base[0] || a_out.base[1];
		}

		// limita il campo agganciato; true se ha scritto
		bool ProbeApply(const ProbeRegions& a_rg, float a_capMps)
		{
			const auto& l = R.pbLock;
			if (!a_rg.base[l.region]) {
				return false;
			}
			auto* p = reinterpret_cast<float*>(a_rg.base[l.region] + l.off);
			float f = *p;
			if (!std::isfinite(f)) {
				*p = 0.0f;
				++R.pbWrites;
				return true;
			}
			const float lim = a_capMps * kProbeScale[l.scale];
			if (std::fabs(f) > lim) {
				R.pbWorst = std::max(R.pbWorst, std::fabs(f) / kProbeScale[l.scale]);
				*p = std::copysign(lim, f);
				++R.pbWrites;
				return true;
			}
			return false;
		}

		// da chiamare all'ingresso di UpdateAnimation, PRIMA di limitare velocityMod (a_rawMod = velocityMod come l'ha lasciato il motore)
		void ProbeEntry(RE::PlayerCharacter* a_p, float a_rawMod, float a_capMps)
		{
			ProbeRegions rg;
			if (!ProbeRegionsOf(a_p, rg)) {
				return;
			}
			++R.pbFrames;
			if (R.pbLocked) {
				ProbeApply(rg, a_capMps);
				return;
			}
			if (!std::isfinite(a_rawMod) || a_rawMod < 40.0f) {
				return;  // calibro solo nei frame veloci: con valori piccoli troverei mille coincidenze
			}
			std::array<bool, 24> seen{};
			for (std::uint8_t r = 0; r < 2; ++r) {
				if (!rg.base[r]) {
					continue;
				}
				for (std::size_t off = 0; off + sizeof(float) <= rg.size[r]; off += sizeof(float)) {
					float f = 0.0f;
					std::memcpy(&f, rg.base[r] + off, sizeof(f));
					if (!std::isfinite(f)) {
						continue;
					}
					const float af = std::fabs(f);
					if (af < 1.0f) {
						continue;
					}
					for (std::uint8_t k = 0; k < 4; ++k) {
						const float target = a_rawMod * kProbeScale[k];
						if (std::fabs(af - target) > 0.025f * target) {
							continue;
						}
						int idx = -1;
						for (int c = 0; c < R.pbCandCount; ++c) {
							const auto& cd = R.pbCands[static_cast<std::size_t>(c)];
							if (cd.region == r && cd.off == off && cd.scale == k) {
								idx = c;
								break;
							}
						}
						if (idx < 0) {
							if (R.pbCandCount >= static_cast<int>(R.pbCands.size())) {
								continue;
							}
							idx = R.pbCandCount++;
							auto& nc = R.pbCands[static_cast<std::size_t>(idx)];
							nc.region = r;
							nc.off = static_cast<std::uint16_t>(off);
							nc.scale = k;
							nc.hits = 0;
							nc.changed = false;
							nc.last = f;
							if (R.pbLogs < 40) {
								++R.pbLogs;
								logger::warn("  radice: candidato +0x{:X} di {} = {:.2f} ({} di velocityMod {:.2f})", off, kProbeRegionName[r], f, kProbeScaleName[k], a_rawMod);
							}
						}
						auto& cd = R.pbCands[static_cast<std::size_t>(idx)];
						if (cd.hits > 0 && f != cd.last) {
							cd.changed = true;
						}
						++cd.hits;
						cd.last = f;
						seen[static_cast<std::size_t>(idx)] = true;
					}
				}
			}
			// tolgo i candidati che questo frame non corrispondevano piu'
			int kept = 0;
			for (int c = 0; c < R.pbCandCount; ++c) {
				if (seen[static_cast<std::size_t>(c)]) {
					R.pbCands[static_cast<std::size_t>(kept)] = R.pbCands[static_cast<std::size_t>(c)];
					++kept;
				}
			}
			R.pbCandCount = kept;
			// aggancio: 3 frame di fila e il valore cambia insieme a velocityMod (un campo fisso non e' lui)
			int best = -1;
			for (int c = 0; c < R.pbCandCount; ++c) {
				const auto& cd = R.pbCands[static_cast<std::size_t>(c)];
				if (cd.hits >= 3 && cd.changed) {
					if (best < 0 || cd.scale < R.pbCands[static_cast<std::size_t>(best)].scale) {
						best = c;
					}
				}
			}
			if (best >= 0) {
				R.pbLock = R.pbCands[static_cast<std::size_t>(best)];
				R.pbLocked = true;
				logger::warn("  radice: AGGANCIATO il campo +0x{:X} di {} (vale come {}). Da ora lo riporto a {:.1f} m/s a ogni ingresso e uscita di UpdateAnimation",
					R.pbLock.off, kProbeRegionName[R.pbLock.region], kProbeScaleName[R.pbLock.scale], a_capMps);
				ProbeApply(rg, a_capMps);
			}
		}

		void ProbeExit(RE::PlayerCharacter* a_p, float a_capMps)
		{
			if (!R.pbLocked) {
				return;
			}
			ProbeRegions rg;
			if (ProbeRegionsOf(a_p, rg)) {
				ProbeApply(rg, a_capMps);
			}
		}

		void ProbeReset()
		{
			R.pbCandCount = 0;
			R.pbLocked = false;
			R.pbLock = Runtime::ProbeCand{};
			R.pbWrites = 0;
			R.pbWorst = 0.0f;
			R.pbFrames = 0;
			R.gvCuts = 0;
			R.gvLogs = 0;
		}

		// Variabili float del grafo di animazione legate alla velocita'. NON si legge piu' l'elenco dei nomi dai dati interni del behavior
		// (BSAnimationGraphManager -> hkbBehaviorGraphStringData): nel crash del 19/10 (Crash Logger, Core.cpp BuildSpeedVarNames, lettura
		// all'indirizzo 0x4EC84EF8) un nome non era un puntatore valido. Uso una lista fissa e chiedo il valore al gioco con
		// GetGraphVariableFloat, che risponde false se la variabile non esiste.
		void BuildSpeedVarNames(RE::PlayerCharacter* a_p)
		{
			R.gvNames.clear();
			R.gvBuilt = true;
			static constexpr const char* kNames[] = { "Speed", "SpeedSampled", "SpeedDamped" };
			for (const auto* n : kNames) {
				float v = 0.0f;
				if (a_p->GetGraphVariableFloat(n, v)) {
					R.gvNames.emplace_back(n);
				}
			}
			std::string list;
			for (const auto& n : R.gvNames) {
				list += n + " ";
			}
			logger::warn("  variabili float di velocita' trovate nel grafo ({}): {}", R.gvNames.size(), list.empty() ? std::string("nessuna") : list);
		}

		// riporta al limite (cap x 68) le variabili di velocita' del grafo che lo superano
		void ClampSpeedVars(RE::PlayerCharacter* a_p, float a_capMps, bool a_entry)
		{
			if (!R.gvBuilt) {
				const double now = Now();
				if (now < R.gvTryAt) {
					return;
				}
				R.gvTryAt = now + 2.0;
				BuildSpeedVarNames(a_p);
				if (!R.gvBuilt) {
					return;
				}
			}
			const float lim = a_capMps * kSpeedVarPerMps;
			for (const auto& n : R.gvNames) {
				float v = 0.0f;
				if (!a_p->GetGraphVariableFloat(n.c_str(), v)) {
					continue;
				}
				if (!std::isfinite(v) || v > lim) {
					a_p->SetGraphVariableFloat(n.c_str(), lim);
					++R.gvCuts;
					if (R.gvLogs < 16) {
						++R.gvLogs;
						logger::info("  variabile del grafo '{}' valeva {:.0f} (limite {:.0f}) [{}]", n, v, lim, a_entry ? "ingresso" : "uscita");
					}
				}
			}
		}

		void UpdateLandingFix(RE::PlayerCharacter* a_p, double a_now)
		{
			const auto& s = Settings::Get();
			const bool  episode = R.lfSince >= 0.0;
			if (!s.speedFix) {
				if (episode) {
					EndAnomaly(a_now, "funzione spenta");
				}
				return;
			}
			// fuori dalla finestra dopo un dash non faccio niente (nemmeno leggere): schivate e mosse speciali non c'entrano
			if (!episode && a_now > R.lfArmUntil) {
				R.lfPosValid = false;
				return;
			}
			// in questi casi una velocita' alta e' normale, o non e' affare della mod
			if (R.dashActive || R.slamActive || R.glide || PlayerBusy(a_p)) {
				if (episode) {
					EndAnomaly(a_now, "dash, slam, planata o personaggio occupato");
				}
				R.lfPosValid = false;
				return;
			}
			float speedVar = 0.0f;
			if (!a_p->GetGraphVariableFloat("Speed", speedVar)) {
				return;
			}

			// spostamento reale dall'ultimo frame
			const float px = a_p->GetPositionX();
			const float py = a_p->GetPositionY();
			const void* cell = a_p->GetParentCell();
			float       disp = 0.0f;
			double      dt = 0.0;
			if (R.lfPosValid && cell == R.lfCell) {
				dt = a_now - R.lfAt;
				const float d = std::hypot(px - R.lfX, py - R.lfY);
				if (d <= kTeleportUnits && dt >= 0.004 && dt <= 0.5) {
					disp = d / static_cast<float>(dt) / kHavokScale;
				}
			}
			R.lfPosValid = true;
			R.lfX = px;
			R.lfY = py;
			R.lfAt = a_now;
			R.lfCell = cell;

			auto*       ctrl = a_p->GetCharController();
			const float cap = s.speedFixMax;
			const bool  bad = !std::isfinite(speedVar);
			const bool  high = bad || speedVar > (episode ? s.speedFixTrigger * 0.5f : s.speedFixTrigger);

			if (!episode) {
				if (!high) {
					return;
				}
				// inizio di un episodio
				R.lfSince = a_now;
				++R.lfEpisodes;
				R.lfFrames = 0;
				R.lfLogs = 0;
				R.lfPeak = bad ? 0.0f : speedVar;
				R.lfHookCalls = 0;
				R.lfHookMax = 0.0f;
				R.lfEntryCuts = 0;
				R.lfPin = s.speedFixPin;  // il blocco dello spostamento parte subito, non dopo 4 frame di fuga
				R.lfPinValid = false;
				R.lfPinFrames = 0;
				R.lfStep = 0.0f;
				R.lfStepMax = 0.0f;
				R.lfBack = 0.0f;
				R.lfProbeAt = a_now + kLfProbeEvery;
				R.lfProbes = 0;
				R.lfResets = 0;
				R.lfCalmSince = -1.0;
				ProbeReset();
				R.gvBuilt = false;  // i nomi si rileggono a ogni episodio
				R.gvTryAt = 0.0;
				R.lfClamp = true;
				R.hkPreMod = R.hkPrePct = R.hkPreSpeed = R.hkPostMod = R.hkPostPct = R.hkPostSpeed = -1.0f;
				if (R.lfEpisodes <= 12) {
					float vm[3] = { 0.0f, 0.0f, 0.0f };
					float pd[2] = { 0.0f, 0.0f };
					float ov[2] = { 0.0f, 0.0f };
					float iv[2] = { 0.0f, 0.0f };
					float dp[2] = { 0.0f, 0.0f };
					float pct = 0.0f;
					float velTime = 0.0f;
					float acro = 0.0f;
					float h = 0.0f;
					float vz = 0.0f;
					if (ctrl) {
						const auto* m = reinterpret_cast<const float*>(&ctrl->velocityMod.quad);
						const auto* p = reinterpret_cast<const float*>(&ctrl->pushDelta.quad);
						const auto* o = reinterpret_cast<const float*>(&ctrl->outVelocity.quad);
						const auto* i = reinterpret_cast<const float*>(&ctrl->initialVelocity.quad);
						const auto* d = reinterpret_cast<const float*>(&ctrl->deltaPos.quad);
						vm[0] = m[0];
						vm[1] = m[1];
						vm[2] = m[2];
						pd[0] = p[0];
						pd[1] = p[1];
						ov[0] = o[0];
						ov[1] = o[1];
						iv[0] = i[0];
						iv[1] = i[1];
						dp[0] = d[0];
						dp[1] = d[1];
						pct = ctrl->speedPct;
						velTime = ctrl->velocityTime;
						acro = ctrl->acrobatics;
						RE::hkVector4 v;
						ctrl->GetLinearVelocityImpl(v);
						const auto* c = reinterpret_cast<const float*>(&v.quad);
						h = std::hypot(c[0], c[1]);
						vz = c[2];
					}
					logger::warn("strascico del dash (episodio {}): Speed {:.0f} (soglia {:.0f}), controller {:.1f} m/s (verticale {:.1f}), spostamento reale {:.1f} m/s, stick {:.2f}, "
								 "velocityMod {:.2f}/{:.2f}/{:.2f}, speedPct {:.2f}, pushDelta {:.2f}/{:.2f}, outVelocity {:.2f}/{:.2f}, initialVelocity {:.2f}/{:.2f}, deltaPos {:.3f}/{:.3f}, velocityTime {:.3f}, acrobatics {:.3f}",
						R.lfEpisodes, speedVar, s.speedFixTrigger, h, vz, disp, StickInput(), vm[0], vm[1], vm[2], pct, pd[0], pd[1], ov[0], ov[1], iv[0], iv[1], dp[0], dp[1], velTime, acro);
					logger::warn("  contesto: dash finito {:.2f} s fa ({}), ultimo volo {:.1f} s fa, in aria: {}, variabili MJ: {}, nel limitatore post-dash: {}",
						a_now - R.dashEndAt, R.dashEndWhy ? R.dashEndWhy : "?", a_now - R.lastAirAt, R.airborne ? "si" : "no", DescribeJumpVars(a_p), R.guardUntil > a_now ? "si" : "no");
					if (s.speedFixDeepLog) {
						Trace::Step("strascico del dash: effetti attivi");
						logger::warn("  velocita' e tempo: {}", DescribeSpeedSources(a_p));
						Trace::Step("strascico del dash: variabili del grafo");
						logger::warn("  variabili float del grafo: {}", DescribeGraphFloats(a_p));
						logger::warn("  variabili bool del grafo: {}", DescribeGraphBools(a_p));
						Trace::Step("strascico del dash: campi del controller");
						logger::warn("  campi del controller che valgono come la velocita' anomala: {}", ScanControllerFloats(ctrl, pct, std::hypot(vm[0], vm[1]), speedVar));
						Trace::Step("strascico del dash");
					}
				}
			}

			// durante l'episodio
			++R.lfFrames;
			if (!bad) {
				R.lfPeak = std::max(R.lfPeak, speedVar);
			}
			if (!high) {
				if (R.lfCalmSince < 0.0) {
					R.lfCalmSince = a_now;
				} else if ((a_now - R.lfCalmSince) >= kAnomCalm) {
					EndAnomaly(a_now, "velocita' tornata normale");
					return;
				}
			} else {
				R.lfCalmSince = -1.0;
			}
			if ((a_now - R.lfSince) > kLfMax) {
				EndAnomaly(a_now, "tempo massimo");
				return;
			}

			Trace::Step("strascico del dash: correzione");
			float velBefore = 0.0f;
			float velAfter = 0.0f;
			float modSeen = 0.0f;
			float pctSeen = 0.0f;
			bool  pinned = false;
			if (ctrl) {
				const auto* m = reinterpret_cast<const float*>(&ctrl->velocityMod.quad);
				modSeen = std::hypot(m[0], m[1]);
				pctSeen = ctrl->speedPct;
				RE::hkVector4 v;
				ctrl->GetLinearVelocityImpl(v);
				const auto* c = reinterpret_cast<const float*>(&v.quad);
				velBefore = std::hypot(c[0], c[1]);
				ClampMotion(ctrl, cap, true);
				velAfter = std::min(velBefore, cap * 1.05f);

				// livello 2: il personaggio non puo' spostarsi piu' di cap x tempo (vedi PinDisplacement); attivo dal primo frame
				if (R.lfPin && PinDisplacement(a_p, ctrl, a_now, cap, cell)) {
					++R.lfPinFrames;
					pinned = true;
				}
			}
			// la posizione di riferimento per il frame dopo e' quella DOPO il blocco (il personaggio e' stato riportato indietro)
			R.lfX = a_p->GetPositionX();
			R.lfY = a_p->GetPositionY();
			// "Speed" del grafo: il gioco l'ha sempre rimessa in alto, ma provo lo stesso (il log mostra se regge)
			if (!bad) {
				a_p->SetGraphVariableFloat("Speed", std::min(speedVar, cap * kSpeedVarPerMps * 0.6f));
			}

			if (R.lfEpisodes <= 12) {
				if (R.lfLogs < kLfLogFrames) {
					++R.lfLogs;
					logger::info("  strascico, frame {} (dt {:.0f} ms): Speed {:.0f}; [Tick] velocityMod {:.1f} m/s, speedPct {:.2f}, controller {:.1f} -> {:.1f} m/s, spostamento del riferimento {:.1f} m/s; "
								 "[ingresso UpdateAnimation] velocityMod {:.1f}, speedPct {:.2f}, Speed {:.0f}; [uscita] velocityMod {:.1f}, speedPct {:.2f}, Speed {:.0f}; "
								 "[blocco] passo del controller {:.1f} m/s, {}",
						R.lfFrames, dt * 1000.0, speedVar, modSeen, pctSeen, velBefore, velAfter, disp, R.hkPreMod, R.hkPrePct, R.hkPreSpeed, R.hkPostMod, R.hkPostPct, R.hkPostSpeed,
						R.lfStep, pinned ? std::format("riportato indietro di {:.2f} m", R.lfBack) : std::string(R.lfPin ? "nessuna correzione" : "spento"));
				} else if (a_now >= R.lfProbeAt && R.lfProbes < 8) {
					++R.lfProbes;
					R.lfProbeAt = a_now + kLfProbeEvery;
					logger::info("  strascico, dopo {:.1f} s: Speed {:.0f}; [Tick] velocityMod {:.1f} m/s, speedPct {:.2f}, spostamento del riferimento {:.1f} m/s; [ingresso UpdateAnimation] velocityMod {:.1f}, speedPct {:.2f}, Speed {:.0f}; "
								 "passo massimo del controller finora {:.1f} m/s; frame con riporto indietro {}",
						a_now - R.lfSince, speedVar, modSeen, pctSeen, disp, R.hkPreMod, R.hkPrePct, R.hkPreSpeed, R.lfStepMax, R.lfPinFrames);
				}
			}

			// livello 3 (spento di default): ripristino delle animazioni
			if (s.speedFixReset && high && R.lfResets < 1 && (a_now - R.lfSince) >= 2.0) {
				++R.lfResets;
				logger::warn("strascico del dash: dopo {:.1f} s e' ancora attivo (Speed {:.0f}): ripristino le animazioni come col tasto di reset", a_now - R.lfSince, speedVar);
				ResetPlayerState(a_p, "strascico del dash");
			}
		}

		// ----------------------------------------------------------------
		// Disattivazione dei vecchi script e del vecchio plugin delle impostazioni.
		// Gli script Papyrus restano "appesi" nel salvataggio anche dopo aver tolto i file .pex:
		// quest e alias che li ospitano, effetti magici attivi sul personaggio e soprattutto i
		// riferimenti che il vecchio plugin delle impostazioni crea a ogni caricamento (PlaceAtMe dei suoi controller
		// e degli stack). Se quel plugin sparisce con quei riferimenti ancora nel
		// salvataggio, il gioco va in crash al caricamento. Qui si fermano le quest, si tolgono
		// gli effetti e gli incantesimi dei vecchi script dal personaggio e si eliminano i riferimenti.
		// Va eseguito dal thread di gioco (Tick).
		// ----------------------------------------------------------------
		bool SameNoCase(std::string_view a_a, std::string_view a_b)
		{
			if (a_a.size() != a_b.size()) {
				return false;
			}
			for (std::size_t i = 0; i < a_a.size(); ++i) {
				if (std::tolower(static_cast<unsigned char>(a_a[i])) != std::tolower(static_cast<unsigned char>(a_b[i]))) {
					return false;
				}
			}
			return true;
		}

		// il form e' stato definito (la prima volta) da questo plugin?
		bool OriginIs(const RE::TESForm* a_form, std::string_view a_plugin)
		{
			if (!a_form) {
				return false;
			}
			const auto* file = a_form->GetFile(0);
			return file && SameNoCase(file->GetFilename(), a_plugin);
		}

		std::string FormLabel(const RE::TESForm* a_form)
		{
			if (!a_form) {
				return "?";
			}
			const char* n = a_form->GetName();
			return std::format("'{}' {:08X}", (n && *n) ? n : "senza nome", a_form->GetFormID());
		}

		void SetLegacyText(const std::string& a_text)
		{
			std::lock_guard lk(g_legacyMutex);
			g_legacyText = a_text;
		}

		struct LegacyScan
		{
			std::vector<RE::TESQuest*>      quests;
			std::vector<RE::TESObjectREFR*> refs;
			std::size_t                     scanned = 0;
		};

		// quest dei due plugin, e riferimenti creati dagli script (la base e' del vecchio plugin delle impostazioni,
		// oppure di doublejump.esp ma il riferimento e' stato creato in gioco)
		LegacyScan ScanLegacyForms()
		{
			LegacyScan out;
			const auto& [map, lock] = RE::TESForm::GetAllForms();
			[[maybe_unused]] const RE::BSReadLockGuard l{ lock };
			if (!map) {
				return out;
			}
			for (auto& kv : *map) {
				auto* form = kv.second;
				if (!form) {
					continue;
				}
				++out.scanned;
				switch (form->GetFormType()) {
				case RE::FormType::Quest:
					if (OriginIs(form, kSettingsPlugin) || OriginIs(form, kPlugin)) {
						out.quests.push_back(static_cast<RE::TESQuest*>(form));
					}
					break;
				case RE::FormType::Reference:
					{
						auto* ref = static_cast<RE::TESObjectREFR*>(form);
						auto* base = ref->GetBaseObject();
						if (!base) {
							break;
						}
						if (OriginIs(base, kSettingsPlugin) || (OriginIs(base, kPlugin) && ref->IsDynamicForm())) {
							out.refs.push_back(ref);
						}
					}
					break;
				default:
					break;
				}
			}
			return out;
		}

		struct LegacyTally
		{
			int  effects = 0;
			int  spells = 0;
			int  quests = 0;
			int  refs = 0;
			int  questsLeft = 0;  // quest ancora attive dopo lo stop (solo verifica)
			int  refsLeft = 0;    // riferimenti non ancora eliminati (solo verifica)
			bool ability = false;
		};

		// a_pass: 0 = prima passata (tutto), 1 = ripetizione, 2 = solo verifica
		LegacyTally LegacyPass(RE::PlayerCharacter* a_p, int a_pass, double a_now)
		{
			LegacyTally t;
			const bool  verifyOnly = (a_pass >= 2);
			const bool  quiet = (a_pass >= 1);  // nel log scrivo i dettagli solo la prima volta

			// 1. effetti attivi del personaggio che vengono da doublejump.esp (portano gli script vecchi)
			//    Non tocco nulla se sto facendo un salto, un dash o uno slam (le passate successive).
			const bool busy = R.airborne || R.dashActive || R.slamActive;
			if (auto* target = a_p->AsMagicTarget()) {
				if (auto* list = target->GetActiveEffectList()) {
					for (auto* ae : *list) {
						if (!ae) {
							continue;
						}
						auto* base = ae->GetBaseObject();
						if (!OriginIs(base, kPlugin)) {
							continue;
						}
						if (verifyOnly) {
							++t.effects;
							continue;
						}
						if (a_pass > 0 && busy) {
							continue;
						}
						if (!quiet) {
							logger::info("  effetto attivo tolto: {}", FormLabel(base));
						}
						ae->Dispel(true);
						++t.effects;
					}
				}
			}

			// 2. incantesimi di doublejump.esp nella lista del personaggio (tranne l'abilita' base, vedi sotto)
			if (!verifyOnly) {
				std::vector<RE::SpellItem*> toRemove;
				for (auto* sp : a_p->GetActorRuntimeData().addedSpells) {
					if (sp && sp != F.ability && OriginIs(sp, kPlugin)) {
						toRemove.push_back(sp);
					}
				}
				for (auto* sp : toRemove) {
					if (!quiet) {
						logger::info("  incantesimo tolto: {}", FormLabel(sp));
					}
					a_p->RemoveSpell(sp);
					++t.spells;
				}
				// l'abilita' base ha un effetto sempre attivo che puo' portare uno script vecchio:
				// la tolgo, aspetto qualche secondo e Maintain() la rimette (nuova, senza script)
				if (a_pass == 0 && F.ability && a_p->HasSpell(F.ability)) {
					a_p->RemoveSpell(F.ability);
					R.abilityHoldUntil = a_now + 3.5;
					t.ability = true;
					logger::info("  abilita' base tolta, la rimetto tra qualche secondo: {}", FormLabel(F.ability));
				}
			}

			// 3. quest e riferimenti
			const auto scan = ScanLegacyForms();
			if (a_pass == 0) {
				logger::info("  controllati {} form: {} quest, {} riferimenti da vecchi script", scan.scanned, scan.quests.size(), scan.refs.size());
			}
			for (auto* q : scan.quests) {
				if (!q->IsEnabled()) {
					continue;
				}
				if (verifyOnly) {
					++t.questsLeft;
					logger::warn("  quest ancora attiva: {}", FormLabel(q));
					continue;
				}
				if (!quiet) {
					logger::info("  quest fermata: {} ({})", FormLabel(q), OriginIs(q, kSettingsPlugin) ? "SettingsManager.esp" : "doublejump.esp");
				}
				q->Stop();
				++t.quests;
			}
			std::size_t seen = 0;
			for (auto* r : scan.refs) {
				if (r->IsDeleted()) {
					continue;
				}
				if (verifyOnly) {
					++t.refsLeft;
					logger::warn("  riferimento non ancora eliminato: {} (base {})", FormLabel(r), FormLabel(r->GetBaseObject()));
					continue;
				}
				if (++seen > kLegacyMaxRefs) {
					logger::warn("  troppi riferimenti ({}), mi fermo a {}", scan.refs.size(), kLegacyMaxRefs);
					break;
				}
				if (!quiet) {
					logger::info("  riferimento eliminato: {} (base {}, cella {})", FormLabel(r), FormLabel(r->GetBaseObject()), r->GetParentCell() ? "caricata" : "-");
				}
				if (r->Is3DLoaded() && !r->IsDisabled()) {
					r->Disable();
				}
				r->SetDelete(true);
				++t.refs;
			}
			return t;
		}

		void UpdateLegacy(RE::PlayerCharacter* a_p, double a_now)
		{
			if (g_legacyRequest.exchange(false)) {
				if (R.legacyPass < 0) {
					auto*       dh = RE::TESDataHandler::GetSingleton();
					const bool  smLoaded = dh && dh->LookupModByName(kSettingsPlugin);
					logger::info("disattivazione dei vecchi script: inizio (SettingsManager.esp {})", smLoaded ? "caricato" : "non caricato");
					ResetActions(a_p);  // niente dash, slam o salti a meta'
					R.legacyPass = 0;
					R.legacyNextAt = a_now;
					R.legacyEffects = R.legacySpells = R.legacyQuests = R.legacyRefs = 0;
					R.legacyAbility = false;
					g_legacyBusy.store(true);
					SetLegacyText("In corso...");
				}
			}
			if (R.legacyPass < 0 || a_now < R.legacyNextAt) {
				return;
			}
			const int  pass = R.legacyPass;
			const auto t = LegacyPass(a_p, pass, a_now);
			if (pass < 2) {  // l'ultima passata e' solo una verifica: i suoi numeri sono quello che resta, non quello che ho tolto
				R.legacyEffects += t.effects;
				R.legacySpells += t.spells;
				R.legacyQuests += t.quests;
				R.legacyRefs += t.refs;
				R.legacyAbility = R.legacyAbility || t.ability;
			}
			if (pass < 2) {
				logger::info("disattivazione vecchi script, passata {}/{}: effetti {}, incantesimi {}, quest {}, riferimenti {}", pass + 1, kLegacyPasses, t.effects, t.spells, t.quests, t.refs);
			} else {
				logger::info("disattivazione vecchi script, verifica: restano {} effetti di doublejump.esp attivi, {} quest, {} riferimenti", t.effects, t.questsLeft, t.refsLeft);
			}
			if (pass + 1 < kLegacyPasses) {
				R.legacyPass = pass + 1;
				R.legacyNextAt = a_now + (pass == 0 ? 2.0 : 3.0);
				return;
			}

			// fine: riassunto
			R.legacyPass = -1;
			g_legacyBusy.store(false);
			std::string text;
			const int   total = R.legacyEffects + R.legacySpells + R.legacyQuests + R.legacyRefs;
			if (total == 0 && !R.legacyAbility) {
				text = "Non ho trovato niente da disattivare: i vecchi script sembrano gia' puliti.";
			} else {
				text = std::format("Fatto: fermate {} quest, tolti {} effetti attivi e {} incantesimi, eliminati {} riferimenti creati dagli script.", R.legacyQuests, R.legacyEffects, R.legacySpells, R.legacyRefs);
			}
			if (t.questsLeft > 0 || t.refsLeft > 0) {
				text += std::format(" Attenzione: restano {} quest attive e {} riferimenti non eliminati (vedi il log), riprova tra qualche secondo.", t.questsLeft, t.refsLeft);
			} else {
				text += " Ora SALVA il gioco, esci, togli il plugin delle impostazioni e i vecchi script, poi rientra e ripulisci con ResaverSSE.";
			}
			logger::info("disattivazione dei vecchi script: {}", text);
			SetLegacyText(text);
			RE::SendHUDMessage::ShowHUDMessage("MJN: vecchi script disattivati. Salva il gioco prima di toglierli.");
		}

		// ----------------------------------------------------------------
		// cambio di zona / teletrasporto / caricamento
		// Quando il personaggio cambia zona il gioco ricostruisce molte cose (cella, 3D, grafo di animazione): per un attimo il plugin
		// non legge e non scrive niente sul personaggio (variabili del grafo, perk, incantesimi), cosi' non puo' trovarsi in mezzo.
		// In esterno tutte le celle dello stesso mondo contano come una zona sola: attraversare il confine tra due celle non e' un cambio.
		// ----------------------------------------------------------------
		const void* AreaKey(RE::PlayerCharacter* a_p)
		{
			const auto* cell = a_p->GetParentCell();
			if (!cell) {
				return nullptr;
			}
			if (cell->IsInteriorCell()) {
				return cell;
			}
			if (const void* ws = a_p->GetWorldspace()) {
				return ws;
			}
			static const char kExteriorUnknown = 0;  // esterno senza mondo noto: una zona sola, mai "nessuna zona"
			return &kExteriorUnknown;
		}

		// true = il personaggio e' in un cambio di zona (o appena arrivato): non lo tocco
		bool InTransition(RE::PlayerCharacter* a_p, double a_now)
		{
			const void* area = AreaKey(a_p);
			const bool  fresh = !R.had3D;
			R.had3D = true;

			if (!area) {
				if (R.lastArea) {
					logger::info("zona: il personaggio non e' in nessuna cella (caricamento in corso?), aspetto");
					Trace::Note("zona non disponibile");
				}
				R.lastArea = nullptr;
				R.lastPosValid = false;
				R.quietUntil = a_now + kTransitionQuiet;
				return true;
			}

			const float x = a_p->GetPositionX();
			const float y = a_p->GetPositionY();
			const char* why = nullptr;
			if (fresh) {
				why = "personaggio appena caricato";
			} else if (area != R.lastArea) {
				why = R.lastArea ? "cambio di zona" : "zona di nuovo disponibile";
			} else if (R.lastPosValid && std::hypot(x - R.lastPosX, y - R.lastPosY) > kTeleportUnits) {
				why = "spostamento improvviso (teletrasporto)";
			}
			R.lastArea = area;
			R.lastPosValid = true;
			R.lastPosX = x;
			R.lastPosY = y;

			if (why) {
				WitchAbortLight();
				R.quietUntil = a_now + kTransitionQuiet;
				R.lastSink = 0.0;  // l'ascolto degli eventi di animazione si riattacca a pausa finita
				R.airTicks = 0;
				R.groundTicks = 0;
				R.acValid = false;
				R.gdValid = false;
				R.prevValid = false;
				R.prevPending = false;
				R.camValid = false;
				R.posValid = false;
				ResetAnomaly();
				{
					std::lock_guard lk(g_evMutex);
					g_events.clear();
				}
				Trace::Note(why);
				if (++R.transitions <= 40) {
					logger::info("zona: {}. Pausa di {:.1f} s senza toccare il personaggio (ultimo passo prima: {})", why, kTransitionQuiet, Trace::CurrentStep());
				}
			}
			return a_now < R.quietUntil;
		}

		// ----------------------------------------------------------------
		// aggiornamento a ogni frame del giocatore
		// ----------------------------------------------------------------
		void Tick(RE::PlayerCharacter* a_p)
		{
			if (!R.initialized || !a_p) {
				return;
			}
			Trace::Tick();
			Trace::Step("controllo 3D");
			if (!a_p->Is3DLoaded()) {
				R.had3D = false;
				return;
			}
			const double now = Now();
			Trace::Step("controllo zona");
			if (InTransition(a_p, now)) {
				S.state = "in pausa (cambio di zona)";
				return;
			}
			auto& s = Settings::Get();

			if (s.dirty && (now - s.dirtyAt) > 0.75) {
				s.Save();
			}

			// il reset funziona sempre, anche con la mod spenta (serve proprio quando qualcosa si e' inceppato)
			Trace::Step("reset e pulizia");
			if (g_resetRequest.exchange(false)) {
				ResetPlayerState(a_p, "menu");
			}
			UpdateResetInput(a_p, now);
			UpdateLegacy(a_p, now);  // pure questo funziona anche con la mod spenta
			if (!F.ok) {
				return;
			}

			if (now - R.lastSink > 4.0) {
				R.lastSink = now;
				Trace::Step("ascolto eventi di animazione");
				RegisterSink(a_p);
			}

			S.applied = R.applied;
			S.airborne = R.airborne;

			if (!s.enabled) {
				if (R.applied) {
					Unapply(a_p);
				}
				S.state = "disattivata";
				return;
			}
			if (!R.applied) {
				Apply(a_p);
			}
			Trace::Step("effetti visivi");
			ApplyFx(true);
			if (now - R.lastMaintain > 1.5) {
				R.lastMaintain = now;
				Trace::Step("perk e incantesimo base");
				Maintain(a_p);
			}

			// avviso una volta sola se il file del gate dei salti non punta al nostro perk
			if (!R.bjWarned && R.loadedAt > 0.0 && (now - R.loadedAt) > 6.0) {
				R.bjWarned = true;
				if (S.bjFileFound && !S.bjMatch) {
					RE::SendHUDMessage::ShowHUDMessage("MJN: apri il menu F1 > MJN > Generale e premi 'Correggi gate dei salti', poi riavvia il gioco.");
				}
			}

			bool allowed = InputAllowed();
			Trace::Step("direzione e eventi");
			UpdateHeadingCal(a_p, now);
			ProcessEvents(a_p, now);
			Trace::Step("aria");
			UpdateAir(a_p, now);
			Trace::Step("planata");
			UpdateGlide(a_p, now);
			const bool glidePause = R.glide && s.paraCompat && s.paraPause;
			if (glidePause) {
				allowed = false;  // in planata i comandi della mod (dash, slam, super salto) sono fermi
			}
			const bool inputOk = InputAllowed();  // per il tasto di uscita dalla strega volante (non lo ferma la planata)
			if (R.wmState != 0) {
				allowed = false;  // strega volante: niente salto, dash, slam o attacchi; solo magie e tasto di uscita
			}
			UpdateDirection(now);

			bool padOn[kPadActions] = {};
			{
				const std::uint32_t pad = (s.padEnabled && allowed && Pad::GameFocused()) ? Pad::Poll() : 0u;
				EvalPad(pad, now, padOn);
			}

			const auto kSuper = Poll(R.kSuper, KeyIsDown(s.keySuperJump) || padOn[0], allowed, now);
			const auto kDash = Poll(R.kDash, KeyIsDown(s.keyDash) || padOn[1], allowed, now);
			const auto kSlam = Poll(R.kSlam, KeyIsDown(s.keySlam) || padOn[2], allowed, now);
			const auto kAirN = Poll(R.kAirN, KeyIsDown(s.keyAirNormal) || padOn[3], allowed, now);
			const auto kAirP = Poll(R.kAirP, KeyIsDown(s.keyAirPower) || padOn[4], allowed, now);
			const auto kAirC = Poll(R.kAirC, KeyIsDown(s.keyAirCast) || padOn[5], allowed, now);
			const auto kAirR = Poll(R.kAirR, KeyIsDown(s.keyAirRanged) || padOn[6], allowed, now);
			bool       exitKeys = false;
			if (s.keyWitchExit > 0) {
				exitKeys = KeyIsDown(s.keyWitchExit) && (s.keyWitchExit2 <= 0 || KeyIsDown(s.keyWitchExit2));
			} else if (s.keyWitchExit2 > 0) {
				exitKeys = KeyIsDown(s.keyWitchExit2);
			}
			const auto kWitchX = Poll(R.kWitchExit, (exitKeys || padOn[7]) && R.wmState != 0, inputOk, now);

			Trace::Step("super salto, dash, slam");
			UpdateSuperJump(now, kSuper, allowed);
			UpdateDash(a_p, now, kDash, allowed);
			UpdateSlam(a_p, now, kSlam, allowed);
			Trace::Step("attacchi in salto");
			UpdateAirAttacks(a_p, now, kAirN, kAirP, kAirC, kAirR);
			Trace::Step("strega volante");
			UpdateWitch(a_p, now, kWitchX);
			WitchPostDiag(a_p, now);
			Trace::Step("permessi dei salti");
			UpdateJumpGate(a_p);
			Trace::Step("movimento in aria");
			if (glidePause || R.wmState != 0) {
				R.acValid = false;
				R.gdValid = false;
			} else if (!ApplyRedirect(a_p, now)) {
				UpdateSpeedGuard(a_p, now);
				UpdateAirControl(a_p);
			}
			Trace::Step("strascico del dash");
			UpdateLandingFix(a_p, now);
			Trace::Step("acqua");
			UpdateWater(a_p, now);
			Trace::Step("azioni ritardate");
			RunDelayed(a_p, now);
			ApplyBaselines();
			Trace::Step("fine frame");

			if (R.wmState != 0) {
				S.state = "strega volante";
			} else if (R.slamActive) {
				S.state = "slam";
			} else if (R.dashActive) {
				S.state = "dash";
			} else if (R.sjArmed) {
				S.state = "super salto pronto";
			} else if (R.jaAnimOn) {
				S.state = "attacco in salto";
			} else if (R.airborne) {
				S.state = "in aria";
			} else {
				S.state = "a terra";
			}
		}

		// ----------------------------------------------------------------
		// aggancio all'update del giocatore
		// ----------------------------------------------------------------
		struct PlayerUpdateHook
		{
			static void thunk(RE::PlayerCharacter* a_this, float a_delta)
			{
				func(a_this, a_delta);
				try {
					Tick(a_this);
				} catch (...) {
					static bool once = false;
					if (!once) {
						once = true;
						logger::error("eccezione in Tick (ignorata)");
					}
				}
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};

		// azzera le variabili di velocita' del grafo che esistono davvero (solo strega volante)
		void ZeroSpeedVars(RE::PlayerCharacter* a_p)
		{
			static constexpr const char* kNames[] = { "Speed", "SpeedSampled", "SpeedDamped" };
			for (const auto* n : kNames) {
				float v = 0.0f;
				if (a_p->GetGraphVariableFloat(n, v) && v != 0.0f) {
					a_p->SetGraphVariableFloat(n, 0.0f);
				}
			}
		}

		// Aggancio a PlayerCharacter::UpdateAnimation (vfunc 0x7D, lo stesso punto usato da altri sistemi di movimento).
		// Serve SOLO durante un episodio di strascico del dash (R.lfClamp): negli altri casi chiama la funzione del gioco e basta.
		// Dai log la variabile "Speed" del grafo (17000) e' sempre uguale a velocityMod ALL'INGRESSO
		// di questa funzione x 68, e il motore rimette velocityMod a ~250 m/s prima di ogni chiamata. Il limite messo DOPO
		// la funzione non poteva quindi cambiare la Speed che l'animazione usa. Ora limito anche all'ingresso, cosi'
		// l'animazione parte da un movimento normale; poi limito di nuovo all'uscita (prima che la fisica usi il valore) e
		// applico il blocco dello spostamento.
		struct PlayerAnimHook
		{
			static void thunk(RE::PlayerCharacter* a_this, float a_delta)
			{
				// strega volante: mentre sei sospeso il grafo di animazione non deve vedere nessun movimento (niente camminata: il personaggio resta
				// nella posa delle magie e si sposta fluttuando). Per la durata della chiamata azzero tutto cio' da cui il gioco ricava la camminata:
				// input di movimento, velocityMod / speedPct del controller e le variabili di velocita' del grafo (anche dopo, per chi le legge piu' tardi).
				// Input e controller tornano com'erano appena finita la chiamata: la fisica e il mio movimento non cambiano.
				if (R.wmState != 0 && Settings::Get().witchLegsLock) {
					RE::bhkCharacterController* ctrl = nullptr;
					RE::PlayerControls*         pc = nullptr;
					RE::NiPoint2                savedIn(0.0f, 0.0f);
					bool                        ctrlSaved = false;
					bool                        inSaved = false;
					float                       preSpeed = -1.0f;
					float                       preMod = 0.0f;
					float                       postSpeed = -1.0f;
					try {
						ctrl = a_this->GetCharController();
						if (ctrl) {
							auto* m = reinterpret_cast<float*>(&ctrl->velocityMod.quad);
							std::memcpy(R.wmLegMod, m, sizeof(R.wmLegMod));
							R.wmLegPct = ctrl->speedPct;
							preMod = std::hypot(m[0], m[1]);
							m[0] = 0.0f;
							m[1] = 0.0f;
							m[2] = 0.0f;
							ctrl->speedPct = 0.0f;
							ctrlSaved = true;
						}
						pc = RE::PlayerControls::GetSingleton();
						if (pc) {
							savedIn = pc->data.moveInputVec;
							pc->data.moveInputVec.x = 0.0f;
							pc->data.moveInputVec.y = 0.0f;
							inSaved = true;
						}
						a_this->GetGraphVariableFloat("Speed", preSpeed);
						ZeroSpeedVars(a_this);
					} catch (...) {
					}
					func(a_this, a_delta);
					try {
						a_this->GetGraphVariableFloat("Speed", postSpeed);
						ZeroSpeedVars(a_this);
					} catch (...) {
					}
					try {
						if (inSaved && pc) {
							pc->data.moveInputVec = savedIn;
						}
						if (ctrlSaved && a_this->GetCharController() == ctrl) {
							auto* m = reinterpret_cast<float*>(&ctrl->velocityMod.quad);
							if (m[0] == 0.0f && m[1] == 0.0f && m[2] == 0.0f) {
								std::memcpy(m, R.wmLegMod, sizeof(R.wmLegMod));
							}
							if (ctrl->speedPct == 0.0f) {
								ctrl->speedPct = R.wmLegPct;
							}
						}
					} catch (...) {
					}
					++R.wmLegCalls;
					const double nowL = Now();
					if (R.wmCount <= 3 && R.wmLegLogs < 24 && nowL >= R.wmLegLogAt) {
						R.wmLegLogAt = nowL + 0.5;
						++R.wmLegLogs;
						WitchLog(std::format("gambe ferme: all'ingresso di UpdateAnimation Speed del grafo {:.0f}, velocityMod {:.2f} m/s, input di movimento ({:.2f}, {:.2f}) -> azzerati; "
											 "dopo l'aggiornamento Speed del grafo {:.0f} ({} chiamate)",
							preSpeed, preMod, savedIn.x, savedIn.y, postSpeed, R.wmLegCalls));
					}
					return;
				}
				if (!R.lfClamp) {
					func(a_this, a_delta);
					return;
				}
				const float cap = Settings::Get().speedFixMax;
				try {
					if (auto* c = a_this->GetCharController()) {
						float preSpeed = -1.0f;
						a_this->GetGraphVariableFloat("Speed", preSpeed);
						const float pctBefore = c->speedPct;
						if (Settings::Get().speedFixSource) {
							const auto* m0 = reinterpret_cast<const float*>(&c->velocityMod.quad);
							ProbeEntry(a_this, std::hypot(m0[0], m0[1]), cap);
						}
						const float before = ClampMotion(c, cap, false);
						R.hkPreMod = before;
						R.hkPrePct = pctBefore;
						R.hkPreSpeed = preSpeed;
						R.lfHookMax = std::max(R.lfHookMax, before);
						if (before > cap || !(pctBefore <= std::max(cap / kRunMps, 1.0f))) {
							++R.lfEntryCuts;
						}
						if (Settings::Get().speedFixVars) {
							ClampSpeedVars(a_this, cap, true);
						}
						if (R.lfPin) {
							PinDisplacement(a_this, c, Now(), cap, a_this->GetParentCell());
						}
					}
				} catch (...) {
					static bool once = false;
					if (!once) {
						once = true;
						logger::error("eccezione nell'aggancio di UpdateAnimation, ingresso (ignorata)");
					}
				}
				func(a_this, a_delta);
				try {
					if (auto* c = a_this->GetCharController()) {
						const auto* m = reinterpret_cast<const float*>(&c->velocityMod.quad);
						float       postSpeed = -1.0f;
						a_this->GetGraphVariableFloat("Speed", postSpeed);
						R.hkPostMod = std::hypot(m[0], m[1]);
						R.hkPostPct = c->speedPct;
						R.hkPostSpeed = postSpeed;
						if (Settings::Get().speedFixSource) {
							ProbeExit(a_this, cap);
						}
						const float before = ClampMotion(c, cap, true);
						if (before > cap) {
							++R.lfHookCalls;
						}
						if (Settings::Get().speedFixVars) {
							ClampSpeedVars(a_this, cap, false);
						}
					}
				} catch (...) {
					static bool once = false;
					if (!once) {
						once = true;
						logger::error("eccezione nell'aggancio di UpdateAnimation, uscita (ignorata)");
					}
				}
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	// --------------------------------------------------------------------
	// interfaccia pubblica
	// --------------------------------------------------------------------
	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
		PlayerUpdateHook::func = vtbl.write_vfunc(0xAD, PlayerUpdateHook::thunk);
		logger::info("hook PlayerCharacter::Update installato");
		PlayerAnimHook::func = vtbl.write_vfunc(0x7D, PlayerAnimHook::thunk);
		logger::info("hook PlayerCharacter::UpdateAnimation installato");
	}

	void Init()
	{
		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh) {
			logger::error("TESDataHandler non disponibile");
			return;
		}
		F.perk = dh->LookupForm<RE::BGSPerk>(kPerkMain, kPlugin);
		F.perkSoft = dh->LookupForm<RE::BGSPerk>(kPerkSoftLanding, kPlugin);
		F.perkAnim = dh->LookupForm<RE::BGSPerk>(kPerkAnim, kPlugin);
		F.ability = dh->LookupForm<RE::SpellItem>(kAbilityMaster, kPlugin);
		F.spJump1 = dh->LookupForm<RE::SpellItem>(kSpellJump1, kPlugin);
		F.spJump2 = dh->LookupForm<RE::SpellItem>(kSpellJump2, kPlugin);
		F.spJump3 = dh->LookupForm<RE::SpellItem>(kSpellJump3, kPlugin);
		F.spDash = dh->LookupForm<RE::SpellItem>(kSpellDash, kPlugin);
		F.spDashEnd = dh->LookupForm<RE::SpellItem>(kSpellDashEnd, kPlugin);
		F.mgJump1 = dh->LookupForm<RE::EffectSetting>(kEffJump1, kPlugin);
		F.mgJump2 = dh->LookupForm<RE::EffectSetting>(kEffJump2, kPlugin);
		F.mgJump3 = dh->LookupForm<RE::EffectSetting>(kEffJump3, kPlugin);
		F.gEnabled = dh->LookupForm<RE::TESGlobal>(kGlobEnabled, kPlugin);
		F.gDirChanged = dh->LookupForm<RE::TESGlobal>(kGlobDirChanged, kPlugin);
		F.gInAir = dh->LookupForm<RE::TESGlobal>(kGlobInAir, kPlugin);
		F.gSlam = dh->LookupForm<RE::TESGlobal>(kGlobSlam, kPlugin);
		F.gDashLen = dh->LookupForm<RE::TESGlobal>(kGlobDashLen, kPlugin);
		F.gAnimLanding = dh->LookupForm<RE::TESGlobal>(kGlobAnimLanding, kPlugin);
		F.gDirOption = dh->LookupForm<RE::TESGlobal>(kGlobDirOption, kPlugin);
		F.gOne = dh->LookupForm<RE::TESGlobal>(kGlobOne, kPlugin);
		F.gAtkP1 = dh->LookupForm<RE::TESGlobal>(kGlobAtkP1, kPlugin);
		F.gAtkP2 = dh->LookupForm<RE::TESGlobal>(kGlobAtkP2, kPlugin);
		F.gAtkAirPower = dh->LookupForm<RE::TESGlobal>(kGlobAtkAirPower, kPlugin);
		F.gAtkHand = dh->LookupForm<RE::TESGlobal>(kGlobAtkHand, kPlugin);
		F.gAtkNormal = dh->LookupForm<RE::TESGlobal>(kGlobAtkNormal, kPlugin);
		F.gAtkCast = dh->LookupForm<RE::TESGlobal>(kGlobAtkCast, kPlugin);
		F.gAtkPower = dh->LookupForm<RE::TESGlobal>(kGlobAtkPower, kPlugin);
		F.gAtkRanged = dh->LookupForm<RE::TESGlobal>(kGlobAtkRanged, kPlugin);
		F.gWitch = dh->LookupForm<RE::TESGlobal>(kGlobWitch, kPlugin);
		F.spBurst = dh->LookupForm<RE::SpellItem>(kSpellBurst, kPlugin);
		F.mgBurstDmg = dh->LookupForm<RE::EffectSetting>(kEffBurstDamage, kPlugin);
		F.spWitchFx = dh->LookupForm<RE::SpellItem>(kSpellWitchFx, kPlugin);
		F.shRed = dh->LookupForm<RE::TESEffectShader>(kShaderRed, kPlugin);
		logger::info("2.0: strega volante (variabile {}, effetti {}), burst di fuoco {}, shader rosso {}", F.gWitch ? "ok" : "MANCA", F.spWitchFx ? "ok" : "MANCANO", F.spBurst && F.mgBurstDmg ? "ok" : "MANCA",
			F.shRed ? "ok" : "MANCA");
		F.atkOk = F.gAtkP1 && F.gAtkP2 && F.gAtkAirPower && F.gAtkHand && F.gAtkNormal && F.gAtkCast && F.gAtkPower && F.gAtkRanged;
		logger::info("attacchi in salto: variabili in doublejump.esp {}", F.atkOk ? "trovate (8/8)" : "MANCANO (serve il doublejump.esp nuovo)");
		F.explSlam = dh->LookupForm<RE::BGSExplosion>(kExplSlam, kPlugin);
		F.explPower = dh->LookupForm<RE::BGSExplosion>(kExplPower, kPlugin);
		F.artFire = dh->LookupForm<RE::BGSArtObject>(kArtDashFire, kPlugin);
		F.artOrig = dh->LookupForm<RE::BGSArtObject>(kArtDashOrig, kPlugin);
		int soundsFound = 0;
		for (int i = 0; i < kSoundDashCount; ++i) {
			F.sndDash[static_cast<std::size_t>(i)] = dh->LookupForm<RE::BGSSoundDescriptorForm>(kSoundDashFirst + static_cast<std::uint32_t>(i), kPlugin);
			if (F.sndDash[static_cast<std::size_t>(i)]) {
				++soundsFound;
			}
		}
		logger::info("dash: effetto fuoco {}, effetto originale {}, suoni {}/{}", F.artFire ? "ok" : "MANCA", F.artOrig ? "ok" : "MANCA", soundsFound, kSoundDashCount);
		InitFx(dh);
		F.spFireStorm = ResolveFireStorm(dh);

		F.ok = F.perk && F.perkAnim && F.ability && F.spJump1 && F.spDash && F.spDashEnd && F.gEnabled && F.gInAir && F.gSlam && F.gDashLen;
		if (F.atkOk) {
			AtkClearFlags();
			SetG(F.gAtkHand, -1.0f);
		}
		if (!F.ok) {
			logger::error("doublejump.esp mancante o incompleto: la mod resta spenta (controlla che sia attivo il doublejump.esp nuovo)");
		} else {
			logger::info("form trovati (perk {:08X})", F.perk->GetFormID());
		}

		if (auto* s = JumpHeightSetting()) {
			R.vanillaJump = s->data.f;
		}
		if (auto* s = GravitySetting()) {
			R.vanillaGravity = s->data.f;
		}
		R.vanillaCaptured = true;
		logger::info("valori di Skyrim: salto {:.2f}, gravita' {:.2f}", R.vanillaJump, R.vanillaGravity);

		S.formsOk = F.ok;
		CheckBJ();
		if (S.bjFileFound) {
			logger::info("gate dei salti: perk nel file {:08X}, atteso {:08X} ({})", S.bjPerkInFile, S.bjPerkExpected, S.bjMatch ? "ok" : "DA CORREGGERE");
		} else {
			logger::warn("gate dei salti: BetterJumpingSE.txt non trovato");
		}
		R.initialized = true;
	}

	void OnNewOrLoad()
	{
		if (!R.initialized) {
			return;
		}
		if (auto* p = RE::PlayerCharacter::GetSingleton()) {
			// niente stati rimasti a meta'
			R.dashActive = false;
			R.dashUsed = false;
			R.dashBufferUntil = 0.0;
			R.slamActive = false;
			R.slamPhase = 0;
			R.sjArmed = false;
			R.sjResetAt = -1.0;
			R.gravityOverride = false;
			R.delayed.clear();
			EndAirAttack(nullptr, "caricamento");
			if (F.atkOk) {
				AtkClearFlags();  // i valori delle variabili globali restano nel salvataggio
				SetG(F.gAtkHand, -1.0f);
			}
			R.airborne = false;
			R.airTicks = 0;
			R.groundTicks = 0;
			R.prevValid = false;
			R.prevPending = false;
			R.dirChanged = false;
			R.dirUntil = 0.0;
			R.camValid = false;
			R.posValid = false;
			R.resetFired = false;
			R.dashEventSent = false;
			R.jumpCount = 0;
			R.jumpGateClosed = false;
			R.acValid = false;
			R.wasSwimming = false;
			R.waterWatchUntil = 0.0;
			R.waterIdlePending = false;
			R.guardUntil = 0.0;
			R.guardAfterDash = false;
			R.gdValid = false;
			ResetAnomaly();
			R.glide = false;
			R.glideEvOn = false;
			R.glideSuspended = false;
			R.glideRemovedPerk = false;
			R.glideRemovedAnim = false;
			WitchAbortLight();
			R.groundZValid = false;
			R.takeoffValid = false;
			R.paraHas = false;
			R.paraVar = false;
			R.legacyPass = -1;
			R.abilityHoldUntil = 0.0;
			g_legacyBusy.store(false);
			g_legacyRequest.store(false);
			SetLegacyText("");
			if (R.timeScaled) {
				if (auto* t = RE::BSTimer::GetSingleton()) {
					t->SetGlobalTimeMultiplier(R.prevTimeScale, true);
				}
				R.timeScaled = false;
			}
			R.applied = false;  // ri-applica con i valori del salvataggio caricato
			R.lastSink = 0.0;
			R.lastMaintain = 0.0;
			R.had3D = false;
			R.lastArea = nullptr;
			R.lastPosValid = false;
			R.quietUntil = 0.0;
			R.loadedAt = Now();
			{
				std::lock_guard lk(g_evMutex);
				g_events.clear();
			}
			// l'ascolto degli eventi di animazione lo riattacca Tick, a pausa di zona finita (qui il grafo del personaggio puo' non essere ancora pronto)
		}
	}

	const Status& GetStatus()
	{
		return S;
	}

	void MenuTouched()
	{
		g_menuUntil.store(Now() + 0.3);
	}

	void RequestReset()
	{
		g_resetRequest.store(true);
	}

	void RequestLegacyCleanup()
	{
		if (!g_legacyBusy.load()) {
			g_legacyRequest.store(true);
		}
	}

	bool LegacyBusy()
	{
		return g_legacyBusy.load() || g_legacyRequest.load();
	}

	std::string LegacyStatus()
	{
		std::lock_guard lk(g_legacyMutex);
		return g_legacyText;
	}

	std::string AirAttackStatus()
	{
		std::string out;
		if (!F.ok) {
			return "doublejump.esp non trovato: la mod e' spenta.";
		}
		out += F.atkOk ? "Variabili degli attacchi in doublejump.esp: trovate (8/8).\n" : "Variabili degli attacchi in doublejump.esp: MANCANO. Serve il doublejump.esp nuovo (quello di questa versione).\n";
		std::lock_guard lk(g_atkMutex);
		out += g_atkText.empty() ? std::string("Nessun attacco in salto ancora eseguito in questa sessione.") : g_atkText;
		return out;
	}

	std::string WitchStatus()
	{
		if (!F.ok) {
			return "doublejump.esp non trovato: la mod e' spenta.";
		}
		std::string out = F.gWitch ? "Variabile MJN_Witch in doublejump.esp: trovata.\n" : "Variabile MJN_Witch in doublejump.esp: MANCA. Serve il doublejump.esp nuovo (quello di questa versione).\n";
		out += F.spWitchFx ? "Effetti visivi: trovati.\n" : "Effetti visivi: MANCANO nel doublejump.esp.\n";
		out += (F.spBurst && F.mgBurstDmg) ? "Burst di fuoco: trovato.\n" : "Burst di fuoco: MANCA nel doublejump.esp.\n";
		out += F.shRed ? "Shader rosso della protezione dalla caduta: trovato.\n" : "Shader rosso della protezione dalla caduta: MANCA nel doublejump.esp.\n";
		std::lock_guard lk(g_atkMutex);
		out += g_witchText.empty() ? std::string("Strega volante non ancora usata in questa sessione.") : g_witchText;
		return out;
	}

	std::string GlideStatus()
	{
		std::lock_guard lk(g_paraMutex);
		return g_paraText.empty() ? std::string("Mod di planata non rilevata sul personaggio (hasParaGlider = 0).") : g_paraText;
	}

	float VanillaGravity()
	{
		return R.vanillaCaptured ? R.vanillaGravity : 0.0f;
	}

	bool FireStormFound()
	{
		return F.spFireStorm != nullptr;
	}

	double FireStormRemaining()
	{
		const double left = g_fireReadyAt.load() - Now();
		return left > 0.0 ? left : 0.0;
	}

	void FireStormResetCooldown()
	{
		g_fireReadyAt.store(0.0);
	}

	bool FixJumpGate()
	{
		if (!F.perk) {
			return false;
		}
		auto bj = ReadBJ();
		if (bj.perkLine < 0) {
			return false;
		}
		bj.lines[static_cast<std::size_t>(bj.perkLine)] = std::format("0x{:X}", F.perk->GetFormID());
		std::ofstream out(kBJPath, std::ios::trunc);
		if (!out.is_open()) {
			logger::error("impossibile scrivere {}", kBJPath);
			return false;
		}
		for (const auto& l : bj.lines) {
			out << l << "\n";
		}
		out.close();
		S.bjFixedNow = true;
		logger::info("BetterJumpingSE.txt aggiornato con il perk {:08X}", F.perk->GetFormID());
		return true;
	}
}
